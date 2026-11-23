#include <sftp_file_transfer/libssh2/sftp_async_engine.h>
#include <sftp_file_transfer/libssh2/sftp_file_transfer_operation_impl.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#endif

#include <coretypes/exceptions.h>
#include <coreobjects/exceptions.h>
#include <opendaq/custom_log.h>
#include <fmt/format.h>

#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/post.hpp>

#include <algorithm>
#include <cerrno>

namespace daq::sftp::libssh2
{

namespace detail
{

using Clock = std::chrono::steady_clock;

// Limits the transfer steps per pass of the I/O thread (see process), so requests are carried out while transfers
// keep making progress.
constexpr int MaxRoundsPerPass = 64;

// Limits the rounds spent releasing the resources of a given-up connection (see releaseConnection).
constexpr int MaxReleaseRounds = 100;

// Thrown while driving the transfers when the connection itself has failed.
struct ConnectionLost
{
    StringPtr message;
};

// Results of libssh2 calls that mean the connection has failed, as opposed to one transfer.
bool isConnectionError(int rc)
{
    switch (rc)
    {
        case LIBSSH2_ERROR_SOCKET_NONE:
        case LIBSSH2_ERROR_BANNER_RECV:
        case LIBSSH2_ERROR_BANNER_SEND:
        case LIBSSH2_ERROR_INVALID_MAC:
        case LIBSSH2_ERROR_KEX_FAILURE:
        case LIBSSH2_ERROR_SOCKET_SEND:
        case LIBSSH2_ERROR_KEY_EXCHANGE_FAILURE:
        case LIBSSH2_ERROR_TIMEOUT:
        case LIBSSH2_ERROR_DECRYPT:
        case LIBSSH2_ERROR_SOCKET_DISCONNECT:
        case LIBSSH2_ERROR_PROTO:
        case LIBSSH2_ERROR_SOCKET_TIMEOUT:
        case LIBSSH2_ERROR_SOCKET_RECV:
        case LIBSSH2_ERROR_ENCRYPT:
        case LIBSSH2_ERROR_BAD_SOCKET:
            return true;
        default:
            return false;
    }
}

// The protocol of a connected TCP socket, to hand it to asio.
boost::asio::ip::tcp socketProtocol(libssh2_socket_t socket)
{
    sockaddr_storage address{};
    socklen_t length = sizeof(address);
    if (::getsockname(socket, reinterpret_cast<sockaddr*>(&address), &length) == 0 && address.ss_family == AF_INET6)
        return boost::asio::ip::tcp::v6();
    return boost::asio::ip::tcp::v4();
}

// Whether the last failed socket call would have blocked (or was interrupted) rather than failed.
bool wouldBlock()
{
#ifdef _WIN32
    const int error = ::WSAGetLastError();
    return error == WSAEWOULDBLOCK || error == WSAEINTR;
#else
    return errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR;
#endif
}

// The session's send and receive callbacks (see libssh2_session_callback_set2). They do what libssh2's own ones do:
// return the byte count, -EAGAIN if the socket would block, or a negative value on failure. In addition, receiving
// records the time of the last data from the server, and once the connection is abandoned, sends are discarded and
// receives fail without using the socket.

LIBSSH2_SEND_FUNC(sendData)
{
    auto* io = static_cast<SessionIo*>(*abstract);
    if (io->abandoned)
        return static_cast<ssize_t>(length);

#ifdef _WIN32
    const int sent = ::send(socket, static_cast<const char*>(buffer), static_cast<int>(length), flags);
#else
    const ssize_t sent = ::send(socket, buffer, length, flags);
#endif
    if (sent < 0)
        return wouldBlock() ? -EAGAIN : -1;
    return static_cast<ssize_t>(sent);
}

LIBSSH2_RECV_FUNC(receiveData)
{
    auto* io = static_cast<SessionIo*>(*abstract);
    if (io->abandoned)
        return -1;

#ifdef _WIN32
    const int received = ::recv(socket, static_cast<char*>(buffer), static_cast<int>(length), flags);
#else
    const ssize_t received = ::recv(socket, buffer, length, flags);
#endif
    if (received < 0)
        return wouldBlock() ? -EAGAIN : -1;
    if (received > 0)
        io->lastReceived = Clock::now();
    return static_cast<ssize_t>(received);
}

}

// ---------------------------------------------------------------------------
// Transfer
// ---------------------------------------------------------------------------

struct SftpAsyncEngine::Transfer
{
    // The order in which a transfer runs; CloseRemoteFile and CloseChannel are also reached from the others on
    // failure, cancellation or a lost connection.
    enum class Step
    {
        Activate,              // open the local file for an upload, or rewind it after a lost connection
        OpenChannel,           // libssh2_sftp_init
        OpenRemoteFile,        // libssh2_sftp_open_ex
        ReadRemoteAttributes,  // download only: libssh2_sftp_fstat_ex, and open the local file
        Data,                  // libssh2_sftp_write / libssh2_sftp_read
        CloseRemoteFile,       // libssh2_sftp_close_handle
        CloseChannel           // libssh2_sftp_shutdown
    };

    Transfer(SftpFileTransferOperationImpl* operation, std::size_t chunkSize)
        : reference(operation)
        , operation(operation)
        , upload(operation->isUpload())
        , localPath(operation->localPath())
        , remotePath(operation->remotePath())
        , buffer(chunkSize)
    {
    }

    const ObjectPtr<IFileTransferOperation> reference;  // keeps the operation alive while the transfer is tracked
    SftpFileTransferOperationImpl* const operation;
    const bool upload;
    const StringPtr localPath;
    const StringPtr remotePath;

    // As reported to the operation, except that a Running transfer reports Running also while closing.
    TransferStatus status = TransferStatus::Queued;
    Step step = Step::Activate;
    bool pauseRequested = false;
    bool cancelRequested = false;
    bool callPending = false;  // the last libssh2 call returned EAGAIN and is repeated with the same arguments
    bool closing = false;      // the outcome is decided; it is reported once the remote file and channel are closed
    bool finished = false;     // the outcome is reported; the transfer is removed

    LIBSSH2_SFTP* sftp = nullptr;
    LIBSSH2_SFTP_HANDLE* handle = nullptr;

    // Kept across a lost connection, to continue where the transfer stopped.
    bool remoteOpened = false;  // the remote file was opened (an upload's truncated) before: reopen at offset
    std::ifstream source;
    std::ofstream destination;
    std::vector<char> buffer;
    std::size_t bufferFill = 0;  // upload: bytes read into the buffer
    std::size_t bufferDone = 0;  // upload: bytes of the buffer the server confirmed
    bool endOfSource = false;
    std::uint64_t offset = 0;  // bytes the server confirmed (upload) or written to the local file (download)
    LIBSSH2_SFTP_ATTRIBUTES remoteAttributes{};  // download: read when the remote file was first opened

    TransferStatus outcome = TransferStatus::Completed;
    ErrCode errorCode = OPENDAQ_SUCCESS;
    StringPtr errorMessage = String("");
};

// ---------------------------------------------------------------------------
// Construction and requests (any thread)
// ---------------------------------------------------------------------------

SftpAsyncEngine::SftpAsyncEngine(const Endpoint& endpoint,
                                 const AuthenticationConfigPtr& authenticationConfig,
                                 const AsyncConnectionProperties& properties,
                                 const LoggerComponentPtr& loggerComponent,
                                 ConnectionStatusCallback connectionStatusCallback)
    : endpoint(endpoint)
    , authenticationConfig(authenticationConfig)
    , properties(properties)
    , loggerComponent(loggerComponent)
    , connectionStatusCallback(std::move(connectionStatusCallback))
{
}

std::shared_ptr<SftpAsyncEngine> SftpAsyncEngine::create(const Endpoint& endpoint,
                                                         const AuthenticationConfigPtr& authenticationConfig,
                                                         const AsyncConnectionProperties& properties,
                                                         const LoggerComponentPtr& loggerComponent,
                                                         ConnectionStatusCallback connectionStatusCallback)
{
    std::shared_ptr<SftpAsyncEngine> engine(
        new SftpAsyncEngine(endpoint, authenticationConfig, properties, loggerComponent, std::move(connectionStatusCallback)));
    engine->connect();
    engine->verified = engine->connection->hostKeyVerified();

    // The thread keeps the engine alive until it ends, which may be after the owner has released it (see shutdown).
    // The work guard keeps it running while there is nothing to do; stop ends it.
    engine->schedule();
    engine->thread = std::thread([engine]
    {
        const auto work = boost::asio::make_work_guard(engine->ioContext);
        engine->ioContext.run();
    });
    engine->ioThreadId = engine->thread.get_id();
    return engine;
}

// Defined here, where Transfer is complete.
SftpAsyncEngine::~SftpAsyncEngine() = default;

bool SftpAsyncEngine::hostKeyVerified() const noexcept
{
    return verified;
}

bool SftpAsyncEngine::onIoThread() const noexcept
{
    return std::this_thread::get_id() == ioThreadId;
}

bool SftpAsyncEngine::post(std::function<void()> request)
{
    // Posted under the lock, so no request is queued behind stop (see shutdown) and silently dropped.
    std::scoped_lock lock(requestSync);
    if (shutdownRequested)
        return false;
    boost::asio::post(ioContext, [this, request = std::move(request)]
    {
        request();
        schedule();
    });
    return true;
}

bool SftpAsyncEngine::start(SftpFileTransferOperationImpl* operation)
{
    // The reference keeps the operation alive until the request has run.
    return post([this, reference = ObjectPtr<IFileTransferOperation>(operation), operation]
    {
        // Canceled before this request ran.
        if (operation->status() != TransferStatus::Created)
            return;
        if (connectionStatus == ConnectionStatus::Unrecoverable)
        {
            operation->setStatus(TransferStatus::Failed,
                                 OPENDAQ_ERR_GENERALERROR,
                                 fmt::format("Connection to the server lost: {}", unrecoverableMessage));
            return;
        }
        transfers.push_back(std::make_unique<Transfer>(operation, properties.chunkSize));
        operation->setStatus(TransferStatus::Queued);
    });
}

void SftpAsyncEngine::pause(SftpFileTransferOperationImpl* operation)
{
    post([this, reference = ObjectPtr<IFileTransferOperation>(operation), operation]
    {
        Transfer* transfer = find(operation);
        if (!transfer)
            return;
        if (transfer->status == TransferStatus::Queued)
        {
            transfer->status = TransferStatus::Paused;
            operation->setStatus(TransferStatus::Paused);
        }
        else if (transfer->status == TransferStatus::Running)
        {
            transfer->pauseRequested = true;  // carried out by step, once the transfer is moving data
        }
    });
}

void SftpAsyncEngine::resume(SftpFileTransferOperationImpl* operation)
{
    post([this, reference = ObjectPtr<IFileTransferOperation>(operation), operation]
    {
        Transfer* transfer = find(operation);
        if (!transfer)
            return;
        transfer->pauseRequested = false;
        if (transfer->status != TransferStatus::Paused)
            return;

        // Paused while moving data, the transfer still holds its channel and remote file; otherwise it waits for
        // a slot again.
        transfer->status = transfer->handle ? TransferStatus::Running : TransferStatus::Queued;
        operation->setStatus(transfer->status);
    });
}

bool SftpAsyncEngine::cancel(SftpFileTransferOperationImpl* operation)
{
    return post([this, reference = ObjectPtr<IFileTransferOperation>(operation), operation]
    {
        Transfer* transfer = find(operation);
        if (!transfer)
        {
            // Not started, or finished already.
            if (operation->status() == TransferStatus::Created)
                operation->setStatus(TransferStatus::Canceled);
            return;
        }
        if (!transfer->sftp && !transfer->handle && transfer->step == Transfer::Step::Activate)
            finish(*transfer, TransferStatus::Canceled, OPENDAQ_SUCCESS, "");
        else
            transfer->cancelRequested = true;  // carried out by step, once no libssh2 call is pending
    });
}

void SftpAsyncEngine::shutdown()
{
    {
        std::scoped_lock lock(connectionStatusSync);
        connectionStatusCallback = nullptr;
    }
    {
        std::scoped_lock lock(requestSync);
        if (!shutdownRequested)
            boost::asio::post(ioContext, [this] { stop(); });
        shutdownRequested = true;
    }

    // From the I/O thread (a status change handler released the owner), the thread cannot wait for itself: it ends
    // on its own once the current handler has returned, and frees the engine then.
    if (onIoThread())
        thread.detach();
    else if (thread.joinable())
        thread.join();
}

// ---------------------------------------------------------------------------
// I/O thread
// ---------------------------------------------------------------------------

void SftpAsyncEngine::stop()
{
    stopTransfers();
    timer.cancel();
    ioContext.stop();
}

void SftpAsyncEngine::schedule()
{
    if (processScheduled)
        return;
    processScheduled = true;
    boost::asio::post(ioContext, [this]
    {
        processScheduled = false;
        process();
    });
}

void SftpAsyncEngine::process()
{
    bool again = false;
    if (connectionStatus == ConnectionStatus::Reconnecting && detail::Clock::now() >= nextReconnect)
        tryReconnect();

    if (connectionStatus == ConnectionStatus::Connected)
    {
        const bool anyRunning = std::any_of(transfers.begin(), transfers.end(), [](const auto& transfer)
        {
            return !transfer->finished && transfer->status == TransferStatus::Running;
        });
        if (!pingPending && !anyRunning && detail::Clock::now() - lastPing >= properties.keepaliveInterval)
            pingPending = true;

        again = drive();
    }

    if (connectionStatus == ConnectionStatus::Connected)
    {
        const auto now = detail::Clock::now();
        if (!outstanding())
            quietSince = now;
        else if (now - std::max(quietSince, socketIo.lastReceived) > properties.connectionTimeout)
            connectionLost(fmt::format("No response from the server for {} ms", properties.connectionTimeout.count()));
    }
    removeFinished();

    // The transfers can go on right away: continue after the requests queued meanwhile.
    if (again)
    {
        schedule();
        return;
    }

    // Wait for the socket only while the server owes something: otherwise nothing reads what it sends, and a
    // readable socket would end every wait at once.
    if (connectionStatus == ConnectionStatus::Connected && outstanding())
        waitForSocket();

    // Setting the expiry cancels the previous wait, whose handler then gets operation_aborted.
    timer.expires_at(nextDeadline(detail::Clock::now()));
    timer.async_wait([this](const boost::system::error_code& error)
    {
        if (!error)
            schedule();
    });
}

void SftpAsyncEngine::waitForSocket()
{
    // In the directions libssh2 is blocked on, at most one wait per direction. A wait started while data is already
    // there completes at once: asio re-arms the socket for each wait (on Linux with EPOLL_CTL_MOD, which reports a
    // socket that is ready already).
    const int directions = libssh2_session_block_directions(connection->session());
    const bool read = (directions & LIBSSH2_SESSION_BLOCK_INBOUND) || !(directions & LIBSSH2_SESSION_BLOCK_OUTBOUND);
    const bool write = (directions & LIBSSH2_SESSION_BLOCK_OUTBOUND) != 0;

    // Waits on a released socket end with operation_aborted, after the flags were reset (see releaseConnection).
    if (read && !waitingToRead)
    {
        waitingToRead = true;
        socketWaiter->async_wait(boost::asio::socket_base::wait_read, [this](const boost::system::error_code& error)
        {
            if (error == boost::asio::error::operation_aborted)
                return;
            waitingToRead = false;
            schedule();
        });
    }
    if (write && !waitingToWrite)
    {
        waitingToWrite = true;
        socketWaiter->async_wait(boost::asio::socket_base::wait_write, [this](const boost::system::error_code& error)
        {
            if (error == boost::asio::error::operation_aborted)
                return;
            waitingToWrite = false;
            schedule();
        });
    }
}

std::chrono::steady_clock::time_point SftpAsyncEngine::nextDeadline(std::chrono::steady_clock::time_point now) const
{
    // At least once a second; earlier for the watchdog, a keepalive or a reconnect attempt.
    auto deadline = now + std::chrono::seconds(1);
    if (connectionStatus == ConnectionStatus::Connected)
    {
        if (outstanding())
            deadline = std::min(deadline, std::max(quietSince, socketIo.lastReceived) + properties.connectionTimeout);
        else
            deadline = std::min(deadline, lastPing + properties.keepaliveInterval);
    }
    else if (connectionStatus == ConnectionStatus::Reconnecting)
    {
        deadline = std::min(deadline, nextReconnect);
    }
    return deadline;
}

void SftpAsyncEngine::connect()
{
    auto newConnection =
        std::make_unique<SshConnection>(endpoint, authenticationConfig, properties.ciphers, properties.connectionTimeout);
    LIBSSH2_SESSION* session = newConnection->session();

    // Opened while the session is still blocking. A failure here closes the session, which frees the channel.
    LIBSSH2_SFTP* newControl = libssh2_sftp_init(session);
    if (!newControl)
        DAQ_THROW_EXCEPTION(NotFoundException, "Failed to open the SFTP channel: {}", lastErrorMessage(session));

    libssh2_session_set_blocking(session, 0);

    const auto now = detail::Clock::now();
    socketIo = SessionIo{};
    socketIo.lastReceived = now;
    *libssh2_session_abstract(session) = &socketIo;
    libssh2_session_callback_set2(session, LIBSSH2_CALLBACK_SEND, reinterpret_cast<libssh2_cb_generic*>(&detail::sendData));
    libssh2_session_callback_set2(session, LIBSSH2_CALLBACK_RECV, reinterpret_cast<libssh2_cb_generic*>(&detail::receiveData));

    // asio only waits on the socket; libssh2 reads and writes it, and the connection closes it (see releaseConnection).
    socketWaiter.emplace(ioContext);
    socketWaiter->assign(detail::socketProtocol(newConnection->socket()), newConnection->socket());

    connection = std::move(newConnection);
    control = newControl;
    pingPending = false;
    lastPing = now;
    quietSince = now;
}

void SftpAsyncEngine::reportConnectionStatus(ConnectionStatus status, const StringPtr& message)
{
    // Held during the call, so the owner's destructor (shutdown) waits for it. A copy is called: the callback may
    // lead to shutdown on this thread, which resets the original.
    std::scoped_lock lock(connectionStatusSync);
    const ConnectionStatusCallback callback = connectionStatusCallback;
    if (!callback)
        return;
    try
    {
        callback(status, message);
    }
    catch (const std::exception& e)
    {
        DAQLOGF_W(loggerComponent, "Reporting the connection status failed: {}", e.what());
    }
}

void SftpAsyncEngine::connectionLost(const StringPtr& message)
{
    DAQLOGF_W(loggerComponent, "Connection to {}:{} lost: {}", endpoint.host, endpoint.port, message);
    releaseConnection();

    if (properties.reconnectAttempts == 0)
    {
        giveUp(message);
        return;
    }

    connectionStatus = ConnectionStatus::Reconnecting;
    reconnectAttemptsMade = 0;
    nextReconnect = detail::Clock::now();
    reportConnectionStatus(ConnectionStatus::Reconnecting, message);

    for (auto& transfer : transfers)
    {
        if (transfer->finished)
            continue;

        // A decided outcome stands, except that an upload has to be closed on the server to complete: it is
        // resumed, finds nothing left to send and closes the remote file again.
        if (transfer->closing && (transfer->outcome != TransferStatus::Completed || !transfer->upload))
        {
            finish(*transfer);
            continue;
        }
        if (transfer->cancelRequested)
        {
            finish(*transfer, TransferStatus::Canceled, OPENDAQ_SUCCESS, "");
            continue;
        }

        transfer->closing = false;
        transfer->outcome = TransferStatus::Completed;
        if (transfer->status == TransferStatus::Running)
        {
            transfer->status = transfer->pauseRequested ? TransferStatus::Paused : TransferStatus::Queued;
            transfer->pauseRequested = false;
            transfer->operation->setStatus(transfer->status);
        }
    }
}

void SftpAsyncEngine::releaseConnection()
{
    // From here on, sends are discarded and receives fail: every pending libssh2 call completes or fails at once,
    // and closing a file or channel does not wait for the server. A call that left a partly sent packet in the
    // session must still complete it before any other call can send, so the transfers are stepped in rounds rather
    // than one after the other.
    socketIo.abandoned = true;
    releasing = true;

    for (int round = 0; round < detail::MaxReleaseRounds; ++round)
    {
        bool busy = false;
        for (auto& transfer : transfers)
        {
            if (transfer->finished)
                continue;
            if (!transfer->callPending && transfer->step != Transfer::Step::CloseRemoteFile &&
                transfer->step != Transfer::Step::CloseChannel)
            {
                if (transfer->handle)
                    transfer->step = Transfer::Step::CloseRemoteFile;
                else if (transfer->sftp)
                    transfer->step = Transfer::Step::CloseChannel;
                else
                {
                    transfer->step = Transfer::Step::Activate;
                    continue;
                }
            }
            busy = true;
            step(*transfer);
        }

        if (control)
        {
            busy = true;
            if (pingPending)
                ping();
            else if (libssh2_sftp_shutdown(control) != LIBSSH2_ERROR_EAGAIN)
                control = nullptr;
        }

        if (!busy)
            break;
    }

    // Whatever is still open is freed with the session (channels) or not at all (file handles); not expected.
    for (auto& transfer : transfers)
    {
        if (transfer->handle || transfer->sftp)
            DAQLOGF_W(loggerComponent, "Resources of the transfer of \"{}\" could not be released", transfer->remotePath);
        transfer->handle = nullptr;
        transfer->sftp = nullptr;
        transfer->callPending = false;
        transfer->step = Transfer::Step::Activate;
    }
    control = nullptr;
    pingPending = false;
    channelOpener = nullptr;

    // Handed back before the connection closes the socket; pending waits end with operation_aborted.
    if (socketWaiter)
    {
        boost::system::error_code error;
        socketWaiter->release(error);
        socketWaiter.reset();
    }
    waitingToRead = false;
    waitingToWrite = false;

    connection.reset();
    releasing = false;
}

void SftpAsyncEngine::giveUp(const StringPtr& message)
{
    DAQLOGF_E(loggerComponent, "Connection to {}:{} could not be restored: {}", endpoint.host, endpoint.port, message);
    connectionStatus = ConnectionStatus::Unrecoverable;
    unrecoverableMessage = message;
    reportConnectionStatus(ConnectionStatus::Unrecoverable, message);

    for (auto& transfer : transfers)
        if (!transfer->finished)
            finish(*transfer, TransferStatus::Failed, OPENDAQ_ERR_GENERALERROR, fmt::format("Connection to the server lost: {}", message));
}

void SftpAsyncEngine::tryReconnect()
{
    try
    {
        connect();
    }
    catch (const std::exception& e)
    {
        ++reconnectAttemptsMade;

        // A rejected host key or credential is not going to change by retrying.
        const auto* daqError = dynamic_cast<const DaqException*>(&e);
        const bool permanent = daqError && (daqError->getErrCode() == OPENDAQ_ERR_AUTHENTICATION_FAILED ||
                                            daqError->getErrCode() == OPENDAQ_ERR_INVALIDPARAMETER);
        if (permanent || reconnectAttemptsMade >= properties.reconnectAttempts)
        {
            giveUp(e.what());
            return;
        }

        DAQLOGF_W(loggerComponent,
                  "Attempt {} of {} to restore the connection to {}:{} failed: {}",
                  reconnectAttemptsMade,
                  properties.reconnectAttempts,
                  endpoint.host,
                  endpoint.port,
                  e.what());
        nextReconnect = detail::Clock::now() + properties.reconnectInterval;
        return;
    }

    DAQLOGF_I(loggerComponent, "Connection to {}:{} restored", endpoint.host, endpoint.port);
    connectionStatus = ConnectionStatus::Connected;
    reportConnectionStatus(ConnectionStatus::Connected, "");
}

void SftpAsyncEngine::stopTransfers()
{
    if (connection)
        releaseConnection();

    for (auto& transfer : transfers)
    {
        if (transfer->finished)
            continue;
        if (transfer->closing && (transfer->outcome != TransferStatus::Completed || !transfer->upload))
            finish(*transfer);
        else
            finish(*transfer, TransferStatus::Canceled, OPENDAQ_SUCCESS, "");
    }
    transfers.clear();
}

bool SftpAsyncEngine::drive()
{
    try
    {
        for (int round = 0; round < detail::MaxRoundsPerPass; ++round)
        {
            startQueued();

            bool progressed = false;
            for (auto& transfer : transfers)
            {
                const bool eligible = transfer->status == TransferStatus::Running ||
                                      (transfer->status == TransferStatus::Paused && transfer->cancelRequested);
                if (!transfer->finished && eligible)
                    progressed = step(*transfer) || progressed;
            }
            if (pingPending)
                progressed = ping() || progressed;

            if (!progressed)
                return false;
        }
        return true;
    }
    catch (const detail::ConnectionLost& e)
    {
        connectionLost(e.message);
        return false;
    }
}

void SftpAsyncEngine::startQueued()
{
    // A paused transfer that holds a channel keeps its slot.
    std::size_t active = 0;
    for (const auto& transfer : transfers)
    {
        if (!transfer->finished && (transfer->status == TransferStatus::Running ||
                                    (transfer->status == TransferStatus::Paused && (transfer->sftp || transfer->handle))))
            ++active;
    }

    for (auto& transfer : transfers)
    {
        if (active >= properties.maxActiveTransfers)
            break;
        if (transfer->finished || transfer->status != TransferStatus::Queued)
            continue;
        transfer->status = TransferStatus::Running;
        transfer->step = Transfer::Step::Activate;
        transfer->operation->setStatus(TransferStatus::Running);
        ++active;
    }
}

bool SftpAsyncEngine::outstanding() const
{
    // Whether the server owes a reply: some transfer is in progress, or a keepalive request.
    return pingPending || std::any_of(transfers.begin(), transfers.end(), [](const auto& transfer)
    {
        return !transfer->finished && (transfer->status == TransferStatus::Running ||
                                       (transfer->status == TransferStatus::Paused && transfer->cancelRequested));
    });
}

void SftpAsyncEngine::removeFinished()
{
    transfers.remove_if([](const auto& transfer) { return transfer->finished; });
}

SftpAsyncEngine::Transfer* SftpAsyncEngine::find(SftpFileTransferOperationImpl* operation)
{
    for (auto& transfer : transfers)
        if (transfer->operation == operation && !transfer->finished)
            return transfer.get();
    return nullptr;
}

// ---------------------------------------------------------------------------
// Transfer steps (I/O thread). Each returns whether the transfer progressed; false means it waits for the server
// or for another transfer. Connection-level failures are thrown as ConnectionLost, except while releasing.
// ---------------------------------------------------------------------------

bool SftpAsyncEngine::step(Transfer& transfer)
{
    // Requests take effect between libssh2 calls only (see the header).
    if (!releasing && !transfer.callPending && !transfer.closing)
    {
        if (transfer.cancelRequested)
        {
            beginClose(transfer, TransferStatus::Canceled);
            return true;
        }
        if (transfer.pauseRequested && transfer.step == Transfer::Step::Data)
        {
            transfer.pauseRequested = false;
            transfer.status = TransferStatus::Paused;
            transfer.operation->setStatus(TransferStatus::Paused);
            return true;
        }
    }

    switch (transfer.step)
    {
        case Transfer::Step::Activate:
            return activate(transfer);
        case Transfer::Step::OpenChannel:
            return openChannel(transfer);
        case Transfer::Step::OpenRemoteFile:
            return openRemoteFile(transfer);
        case Transfer::Step::ReadRemoteAttributes:
            return readRemoteAttributes(transfer);
        case Transfer::Step::Data:
            return transfer.upload ? uploadData(transfer) : downloadData(transfer);
        case Transfer::Step::CloseRemoteFile:
            return closeRemoteFile(transfer);
        case Transfer::Step::CloseChannel:
            return closeChannel(transfer);
    }
    return false;
}

bool SftpAsyncEngine::activate(Transfer& transfer)
{
    if (transfer.upload)
    {
        if (!transfer.source.is_open())
        {
            // Local paths are passed to the standard library as narrow strings (on Windows: in the ANSI code page).
            transfer.source.open(transfer.localPath.toStdString(), std::ios::binary);
            if (!transfer.source)
            {
                finish(transfer,
                       TransferStatus::Failed,
                       OPENDAQ_ERR_NOTFOUND,
                       fmt::format("Failed to open local file \"{}\" for reading", transfer.localPath));
                return true;
            }
            transfer.source.seekg(0, std::ios::end);
            const auto size = transfer.source.tellg();
            transfer.source.seekg(0);
            transfer.operation->setTotalBytes(size > 0 ? static_cast<std::uint64_t>(size) : 0);
        }
        else
        {
            // After a lost connection: continue after the bytes the server confirmed.
            transfer.source.clear();
            transfer.source.seekg(static_cast<std::streamoff>(transfer.offset));
        }
        transfer.bufferFill = 0;
        transfer.bufferDone = 0;
        transfer.endOfSource = false;
    }

    transfer.step = Transfer::Step::OpenChannel;
    return true;
}

bool SftpAsyncEngine::openChannel(Transfer& transfer)
{
    // libssh2_sftp_init keeps the state of a pending call in the session, not in the channel.
    if (channelOpener && channelOpener != &transfer)
        return false;

    LIBSSH2_SESSION* session = connection->session();
    channelOpener = &transfer;
    transfer.sftp = libssh2_sftp_init(session);
    if (!transfer.sftp)
    {
        const int rc = libssh2_session_last_errno(session);
        if (rc != LIBSSH2_ERROR_EAGAIN)
            channelOpener = nullptr;
        return handleResult(transfer, rc, "open an SFTP channel for");
    }

    channelOpener = nullptr;
    transfer.callPending = false;
    transfer.step = Transfer::Step::OpenRemoteFile;
    return true;
}

bool SftpAsyncEngine::openRemoteFile(Transfer& transfer)
{
    // Reopened after a lost connection, an upload continues in the file it truncated before.
    const bool reopening = transfer.remoteOpened;
    constexpr long Mode = LIBSSH2_SFTP_S_IRUSR | LIBSSH2_SFTP_S_IWUSR | LIBSSH2_SFTP_S_IRGRP | LIBSSH2_SFTP_S_IROTH;  // 0644
    const unsigned long flags = transfer.upload
                                    ? LIBSSH2_FXF_WRITE | LIBSSH2_FXF_CREAT | (reopening ? 0 : LIBSSH2_FXF_TRUNC)
                                    : LIBSSH2_FXF_READ;

    transfer.handle = libssh2_sftp_open_ex(transfer.sftp,
                                           transfer.remotePath.getCharPtr(),
                                           static_cast<unsigned int>(transfer.remotePath.getLength()),
                                           flags,
                                           transfer.upload ? Mode : 0,
                                           LIBSSH2_SFTP_OPENFILE);
    if (!transfer.handle)
        return handleResult(transfer,
                            libssh2_session_last_errno(connection->session()),
                            transfer.upload ? "create remote file" : "open remote file");

    transfer.callPending = false;
    transfer.remoteOpened = true;
    if (transfer.upload)
    {
        if (reopening)
            libssh2_sftp_seek64(transfer.handle, transfer.offset);
        transfer.step = Transfer::Step::Data;
    }
    else
    {
        transfer.step = Transfer::Step::ReadRemoteAttributes;
    }
    return true;
}

bool SftpAsyncEngine::readRemoteAttributes(Transfer& transfer)
{
    LIBSSH2_SFTP_ATTRIBUTES attributes{};
    const int rc = libssh2_sftp_fstat_ex(transfer.handle, &attributes, 0);
    if (rc < 0)
        return handleResult(transfer, rc, "read the attributes of remote file");
    transfer.callPending = false;

    if (!transfer.destination.is_open())
    {
        transfer.remoteAttributes = attributes;
        transfer.operation->setTotalBytes((attributes.flags & LIBSSH2_SFTP_ATTR_SIZE) ? attributes.filesize : 0);

        // Opened only once the remote file is known to be readable, so a missing remote file leaves no local file.
        transfer.destination.open(transfer.localPath.toStdString(), std::ios::binary | std::ios::trunc);
        if (!transfer.destination)
        {
            fail(transfer, OPENDAQ_ERR_GENERALERROR, fmt::format("Failed to open local file \"{}\" for writing", transfer.localPath));
            return true;
        }
    }
    else
    {
        // After a lost connection: the local file holds the start of the remote file, as long as it is unchanged.
        const auto& before = transfer.remoteAttributes;
        const unsigned long compared = LIBSSH2_SFTP_ATTR_SIZE | LIBSSH2_SFTP_ATTR_ACMODTIME;
        if ((attributes.flags & compared) != (before.flags & compared) ||
            ((before.flags & LIBSSH2_SFTP_ATTR_SIZE) && attributes.filesize != before.filesize) ||
            ((before.flags & LIBSSH2_SFTP_ATTR_ACMODTIME) && attributes.mtime != before.mtime))
        {
            fail(transfer,
                 OPENDAQ_ERR_GENERALERROR,
                 fmt::format("Remote file \"{}\" changed while the connection was being restored", transfer.remotePath));
            return true;
        }
        libssh2_sftp_seek64(transfer.handle, transfer.offset);
    }

    transfer.step = Transfer::Step::Data;
    return true;
}

bool SftpAsyncEngine::uploadData(Transfer& transfer)
{
    if (!transfer.callPending && transfer.bufferDone == transfer.bufferFill)
    {
        if (!transfer.endOfSource)
        {
            transfer.source.read(transfer.buffer.data(), static_cast<std::streamsize>(transfer.buffer.size()));
            if (transfer.source.bad())
            {
                fail(transfer, OPENDAQ_ERR_GENERALERROR, fmt::format("Failed to read local file \"{}\"", transfer.localPath));
                return true;
            }
            transfer.bufferFill = static_cast<std::size_t>(transfer.source.gcount());
            transfer.bufferDone = 0;
            transfer.endOfSource = transfer.bufferFill < transfer.buffer.size();
        }
        if (transfer.bufferFill == 0 || transfer.bufferDone == transfer.bufferFill)
        {
            beginClose(transfer, TransferStatus::Completed);
            return true;
        }
    }

    // Pass what the server has not confirmed yet: libssh2 skips the part it has already sent (see sftp_write).
    const ssize_t rc = libssh2_sftp_write(transfer.handle,
                                          transfer.buffer.data() + transfer.bufferDone,
                                          transfer.bufferFill - transfer.bufferDone);
    if (rc < 0)
        return handleResult(transfer, static_cast<int>(rc), "write remote file");
    transfer.callPending = false;
    if (rc == 0)
        return false;  // all sent, waiting for the server to confirm

    transfer.bufferDone += static_cast<std::size_t>(rc);
    transfer.offset += static_cast<std::uint64_t>(rc);
    transfer.operation->setBytesTransferred(transfer.offset);
    return true;
}

bool SftpAsyncEngine::downloadData(Transfer& transfer)
{
    const ssize_t rc = libssh2_sftp_read(transfer.handle, transfer.buffer.data(), transfer.buffer.size());
    if (rc < 0)
        return handleResult(transfer, static_cast<int>(rc), "read remote file");
    transfer.callPending = false;

    if (rc == 0)
    {
        transfer.destination.close();
        if (!transfer.destination)
            fail(transfer, OPENDAQ_ERR_GENERALERROR, fmt::format("Failed to write local file \"{}\"", transfer.localPath));
        else
            beginClose(transfer, TransferStatus::Completed);
        return true;
    }

    if (!transfer.destination.write(transfer.buffer.data(), static_cast<std::streamsize>(rc)))
    {
        fail(transfer, OPENDAQ_ERR_GENERALERROR, fmt::format("Failed to write local file \"{}\"", transfer.localPath));
        return true;
    }
    transfer.offset += static_cast<std::uint64_t>(rc);
    transfer.operation->setBytesTransferred(transfer.offset);
    return true;
}

bool SftpAsyncEngine::closeRemoteFile(Transfer& transfer)
{
    const int rc = libssh2_sftp_close_handle(transfer.handle);
    if (rc == LIBSSH2_ERROR_EAGAIN)
    {
        transfer.callPending = true;
        return false;
    }
    transfer.callPending = false;
    transfer.handle = nullptr;  // freed by libssh2 whatever the result
    transfer.step = Transfer::Step::CloseChannel;

    if (rc < 0 && !releasing)
    {
        if (detail::isConnectionError(rc))
            throw detail::ConnectionLost{lastErrorMessage(connection->session())};

        // Closing completes the write on the server, so its result decides whether an upload completed.
        if (transfer.upload && transfer.outcome == TransferStatus::Completed)
        {
            try
            {
                throwFileError(connection->session(), transfer.sftp, "close remote file", transfer.remotePath);
            }
            catch (const DaqException& e)
            {
                transfer.outcome = TransferStatus::Failed;
                transfer.errorCode = e.getErrCode();
                transfer.errorMessage = e.what();
            }
        }
    }
    return true;
}

bool SftpAsyncEngine::closeChannel(Transfer& transfer)
{
    const int rc = libssh2_sftp_shutdown(transfer.sftp);
    if (rc == LIBSSH2_ERROR_EAGAIN)
    {
        transfer.callPending = true;
        return false;
    }
    transfer.callPending = false;
    transfer.sftp = nullptr;  // freed by libssh2 whatever the result

    closed(transfer);
    if (rc < 0 && !releasing && detail::isConnectionError(rc))
        throw detail::ConnectionLost{lastErrorMessage(connection->session())};
    return true;
}

bool SftpAsyncEngine::ping()
{
    LIBSSH2_SFTP_ATTRIBUTES attributes{};
    const int rc = libssh2_sftp_stat_ex(control, ".", 1, LIBSSH2_SFTP_STAT, &attributes);
    if (rc == LIBSSH2_ERROR_EAGAIN)
        return false;

    // Any reply proves the connection alive, including an SFTP error.
    pingPending = false;
    lastPing = detail::Clock::now();
    if (rc < 0 && !releasing && detail::isConnectionError(rc))
        throw detail::ConnectionLost{lastErrorMessage(connection->session())};
    return true;
}

bool SftpAsyncEngine::handleResult(Transfer& transfer, int rc, const char* operation)
{
    if (rc == LIBSSH2_ERROR_EAGAIN)
    {
        transfer.callPending = true;
        return false;
    }
    transfer.callPending = false;

    LIBSSH2_SESSION* session = connection->session();
    if (detail::isConnectionError(rc) && !releasing)
        throw detail::ConnectionLost{lastErrorMessage(session)};

    if (!transfer.sftp)
    {
        fail(transfer,
             OPENDAQ_ERR_GENERALERROR,
             fmt::format("Failed to {} \"{}\": {}", operation, transfer.remotePath, lastErrorMessage(session)));
        return true;
    }
    try
    {
        throwFileError(session, transfer.sftp, operation, transfer.remotePath);
    }
    catch (const DaqException& e)
    {
        fail(transfer, e.getErrCode(), e.what());
    }
    return true;
}

void SftpAsyncEngine::fail(Transfer& transfer, ErrCode errorCode, const StringPtr& message)
{
    if (!releasing && !transfer.closing)
    {
        transfer.errorCode = errorCode;
        transfer.errorMessage = message;
    }
    beginClose(transfer, TransferStatus::Failed);
}

void SftpAsyncEngine::beginClose(Transfer& transfer, TransferStatus outcome)
{
    // While releasing a connection, the remote file and channel are only closed; the outcome is decided afterwards.
    if (!releasing && !transfer.closing)
    {
        transfer.closing = true;
        transfer.outcome = outcome;
    }

    if (transfer.handle)
        transfer.step = Transfer::Step::CloseRemoteFile;
    else if (transfer.sftp)
        transfer.step = Transfer::Step::CloseChannel;
    else
        closed(transfer);
}

void SftpAsyncEngine::closed(Transfer& transfer)
{
    if (releasing)
        transfer.step = Transfer::Step::Activate;
    else
        finish(transfer);
}

void SftpAsyncEngine::finish(Transfer& transfer)
{
    finish(transfer, transfer.outcome, transfer.errorCode, transfer.errorMessage);
}

void SftpAsyncEngine::finish(Transfer& transfer, TransferStatus outcome, ErrCode errorCode, const StringPtr& message)
{
    transfer.finished = true;
    transfer.status = outcome;
    transfer.source.close();
    transfer.destination.close();
    transfer.operation->setStatus(outcome, errorCode, message);
}

}
