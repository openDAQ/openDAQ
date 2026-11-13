/*
 * Copyright 2022-2026 openDAQ d.o.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// Non-blocking SFTP over an SshConnection, driven by an I/O thread (see ssh_connection.h for the connection itself).
//
//   SftpFileTransferAsyncImpl
//    +- SftpAsyncEngine    one per async file transfer object; shared with its I/O thread, which ends once the
//        |                 object is released.
//        +- SshConnection  socket and session, in non-blocking mode once connected; replaced when the connection is
//        |                 restored.
//        +- control SFTP channel, for keepalive requests.
//        +- Transfer       one per started, unfinished operation, holding a reference to it.
//            +- SFTP channel and file handle while the transfer is running or paused.
//            +- local file, across connection losses.
//
// Only the I/O thread uses libssh2 and the transfers. Other threads post requests to it (start, pause, resume,
// cancel, shutdown) and wake it up through a loopback socket.
//
// libssh2 rules followed in non-blocking mode (from its sources; see the .cpp):
// - A call that returned LIBSSH2_ERROR_EAGAIN is repeated with the same arguments before the transfer does anything
//   else. It may have left a partly sent packet in the session, which no other call can send past.
// - An SFTP channel serves one unfinished call at a time, hence one channel per transfer. libssh2_sftp_init keeps
//   its state in the session, so only one channel is opened at a time.
// - Once the connection is given up, the session's send and receive callbacks stop using the socket: sends are
//   discarded and receives fail, so every pending call ends at once and all resources are released without waiting
//   for the server.

#pragma once
#include <sftp_file_transfer/libssh2/ssh_connection.h>

#include <opendaq/logger_component_ptr.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <functional>
#include <list>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace daq::sftp::libssh2
{

class SftpFileTransferOperationImpl;

// Properties from the backend config (see SftpFileTransferAsyncLibssh2DefaultConfig), already validated.
struct AsyncConnectionProperties
{
    std::size_t chunkSize = 0;                      // bytes per libssh2 read or write call
    StringPtr ciphers;                              // comma separated; empty for libssh2's default
    std::size_t maxActiveTransfers = 0;             // SFTP channels used by transfers at once
    std::chrono::milliseconds connectionTimeout{};  // blocking connect, and silence of the server while it owes a reply
    std::chrono::milliseconds keepaliveInterval{};  // between keepalive requests while no transfer is running
    std::size_t reconnectAttempts = 0;              // 0: a lost connection is not restored
    std::chrono::milliseconds reconnectInterval{};  // between attempts to restore the connection
};

// The values of FileTransferStatusType, in its order.
enum class TransferStatus
{
    Created,
    Queued,
    Running,
    Paused,
    Completed,
    Failed,
    Canceled
};

// The values of ConnectionStatusType used for file transfers, in its order.
enum class ConnectionStatus
{
    Connected,
    Reconnecting,
    Unrecoverable
};

// What the session's send and receive callbacks use (see the .cpp); the session's abstract pointer points to it.
struct SessionIo
{
    bool abandoned = false;  // the connection was given up: sends are discarded, receives fail
    std::chrono::steady_clock::time_point lastReceived;
};

class SftpAsyncEngine
{
public:
    // Called on the I/O thread with every change of the connection status.
    using ConnectionStatusCallback = std::function<void(ConnectionStatus status, const StringPtr& message)>;

    // Connects in the calling thread, failing as SftpFileTransferLibssh2 does, then starts the I/O thread. The
    // authentication config is kept to restore the connection.
    static std::shared_ptr<SftpAsyncEngine> create(const Endpoint& endpoint,
                                                   const AuthenticationConfigPtr& authenticationConfig,
                                                   const AsyncConnectionProperties& properties,
                                                   const LoggerComponentPtr& loggerComponent,
                                                   ConnectionStatusCallback connectionStatusCallback);

    ~SftpAsyncEngine();
    SftpAsyncEngine(const SftpAsyncEngine&) = delete;
    SftpAsyncEngine& operator=(const SftpAsyncEngine&) = delete;

    // False if no fingerprint was supplied and the connection was made without host key verification.
    bool hostKeyVerified() const noexcept;

    // Requests, carried out on the I/O thread; any thread. start and cancel return false if the engine is shut down
    // and will not carry them out. The engine keeps a reference to a started operation until it has finished.
    bool start(SftpFileTransferOperationImpl* operation);
    void pause(SftpFileTransferOperationImpl* operation);
    void resume(SftpFileTransferOperationImpl* operation);
    bool cancel(SftpFileTransferOperationImpl* operation);

    bool onIoThread() const noexcept;

    // Stops reporting the connection status, then has the I/O thread cancel every unfinished transfer, close the
    // connection and end; waits for that unless called on the I/O thread. Called by the owner's destructor.
    void shutdown();

private:
    struct Transfer;
    class Wakeup;

    SftpAsyncEngine(const Endpoint& endpoint,
                    const AuthenticationConfigPtr& authenticationConfig,
                    const AsyncConnectionProperties& properties,
                    const LoggerComponentPtr& loggerComponent,
                    ConnectionStatusCallback connectionStatusCallback);

    bool post(std::function<void()> request);
    void run();

    // I/O thread only (connect also in create, before the thread starts).
    void connect();
    void reportConnectionStatus(ConnectionStatus status, const StringPtr& message);
    void connectionLost(const StringPtr& message);
    void releaseConnection();
    void giveUp(const StringPtr& message);
    void tryReconnect();
    void stopTransfers();
    bool drive();
    void startQueued();
    bool outstanding() const;
    void removeFinished();
    int pollTimeout(std::chrono::steady_clock::time_point now) const;
    Transfer* find(SftpFileTransferOperationImpl* operation);

    // Transfer steps, see the .cpp.
    bool step(Transfer& transfer);
    bool activate(Transfer& transfer);
    bool openChannel(Transfer& transfer);
    bool openRemoteFile(Transfer& transfer);
    bool readRemoteAttributes(Transfer& transfer);
    bool uploadData(Transfer& transfer);
    bool downloadData(Transfer& transfer);
    bool closeRemoteFile(Transfer& transfer);
    bool closeChannel(Transfer& transfer);
    bool ping();
    bool handleResult(Transfer& transfer, int rc, const char* operation);
    void fail(Transfer& transfer, ErrCode errorCode, const StringPtr& message);
    void beginClose(Transfer& transfer, TransferStatus outcome);
    void closed(Transfer& transfer);
    void finish(Transfer& transfer);
    void finish(Transfer& transfer, TransferStatus outcome, ErrCode errorCode, const StringPtr& message);

    const Endpoint endpoint;
    const AuthenticationConfigPtr authenticationConfig;
    const AsyncConnectionProperties properties;
    const LoggerComponentPtr loggerComponent;
    bool verified = false;

    // Keeps the socket API usable for the wakeup socket while there is no connection.
    Library library;
    std::unique_ptr<Wakeup> wakeup;

    std::recursive_mutex connectionStatusSync;  // recursive: the callback may lead to shutdown on the same thread
    ConnectionStatusCallback connectionStatusCallback;

    std::mutex requestSync;
    std::vector<std::function<void()>> requests;
    bool shutdownRequested = false;

    std::thread thread;
    std::thread::id ioThreadId;  // set before any other thread can use the engine

    // I/O thread only (and the creating thread before the I/O thread starts).
    SessionIo socketIo;
    std::unique_ptr<SshConnection> connection;
    LIBSSH2_SFTP* control = nullptr;
    bool pingPending = false;
    std::chrono::steady_clock::time_point lastPing;
    std::chrono::steady_clock::time_point quietSince;  // the server owes nothing before this time (see outstanding)
    ConnectionStatus connectionStatus = ConnectionStatus::Connected;
    StringPtr unrecoverableMessage;
    std::size_t reconnectAttemptsMade = 0;
    std::chrono::steady_clock::time_point nextReconnect;
    bool releasing = false;  // releaseConnection is running: transfers only release their resources
    Transfer* channelOpener = nullptr;
    std::list<std::unique_ptr<Transfer>> transfers;
};

}
