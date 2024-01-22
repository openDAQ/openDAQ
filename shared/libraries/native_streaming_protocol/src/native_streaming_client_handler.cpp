#include <native_streaming_protocol/native_streaming_client_handler.h>
#include <native_streaming/client.hpp>

#include <opendaq/custom_log.h>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_PROTOCOL

using namespace daq::native_streaming;

static std::chrono::seconds connectionTimeout = std::chrono::seconds(1);
static std::chrono::seconds protocolInitTimeout = std::chrono::seconds(1);
static std::chrono::seconds reconnectionPeriod = std::chrono::seconds(1);

NativeStreamingClientHandler::NativeStreamingClientHandler(
    const ContextPtr& context,
    std::shared_ptr<boost::asio::io_context> ioContextPtr,
    OnSignalAvailableCallback signalAvailableHandler,
    OnSignalUnavailableCallback signalUnavailableHandler,
    OnPacketCallback packetHandler,
    OnSignalSubscriptionAckCallback signalSubscriptionAckCallback,
    OnReconnectionStatusChangedCallback reconnectionStatusChangedCb)
    : context(context)
    , ioContextPtr(ioContextPtr)
    , logger(context.getLogger())
    , signalAvailableHandler(signalAvailableHandler)
    , signalUnavailableHandler(signalUnavailableHandler)
    , packetHandler(packetHandler)
    , signalSubscriptionAckCallback(signalSubscriptionAckCallback)
    , reconnectionStatusChangedCb(reconnectionStatusChangedCb)
    , reconnectionTimer(std::make_shared<boost::asio::steady_timer>(*ioContextPtr.get()))
    , protocolInitTimer(std::make_shared<boost::asio::steady_timer>(*ioContextPtr.get()))
{
    if (!this->logger.assigned())
        throw ArgumentNullException("Logger must not be null");
    loggerComponent = this->logger.getOrAddComponent("NativeStreamingClientHandler");
}

NativeStreamingClientHandler::~NativeStreamingClientHandler()
{
    reconnectionTimer->cancel();
    protocolInitTimer->cancel();
}

void NativeStreamingClientHandler::checkProtocolInitializationStatus(const boost::system::error_code& ec)
{
    if (ec)
        return;

    if (isProtocolInitialized())
        reconnectionStatusChangedCb(ClientReconnectionStatus::Restored);
    else
        reconnectionStatusChangedCb(ClientReconnectionStatus::Unrecoverable);
}

bool NativeStreamingClientHandler::isProtocolInitialized(std::chrono::seconds timeout)
{
    return (protocolInitFuture.wait_for(timeout) == std::future_status::ready);
}

void NativeStreamingClientHandler::checkReconnectionStatus(const boost::system::error_code& ec)
{
    if (ec)
        return;

    auto status = connectedFuture.wait_for(std::chrono::seconds(0));
    if (status == std::future_status::ready)
    {
        ConnectionResult result = connectedFuture.get();
        if (result == ConnectionResult::Connected)
        {
            if (isProtocolInitialized())
            {
                reconnectionStatusChangedCb(ClientReconnectionStatus::Restored);
            }
            else
            {
                protocolInitTimer->expires_from_now(protocolInitTimeout);
                protocolInitTimer->async_wait(
                    std::bind(&NativeStreamingClientHandler::checkProtocolInitializationStatus, this, std::placeholders::_1));
            }
        }
        else if (result == ConnectionResult::ServerUnsupported)
        {
            reconnectionStatusChangedCb(ClientReconnectionStatus::Unrecoverable);
        }
        else
        {
            tryReconnect();
        }
    }
    else
    {
        // connection is still pending
        reconnectionTimer->expires_from_now(reconnectionPeriod);
        reconnectionTimer->async_wait(std::bind(&NativeStreamingClientHandler::checkReconnectionStatus, this, std::placeholders::_1));
    }
}

void NativeStreamingClientHandler::tryReconnect()
{
    LOG_I("Try reconnect ...");

    reconnectionTimer->cancel();
    protocolInitTimer->cancel();

    connectedPromise = std::promise<ConnectionResult>();
    connectedFuture = connectedPromise.get_future();
    protocolInitPromise = std::promise<void>();
    protocolInitFuture = protocolInitPromise.get_future();

    reconnectionTimer->expires_from_now(reconnectionPeriod);
    reconnectionTimer->async_wait(std::bind(&NativeStreamingClientHandler::checkReconnectionStatus, this, std::placeholders::_1));

    client->connect();
}

bool NativeStreamingClientHandler::connect(std::string host,
                                           std::string port,
                                           std::string path)
{
    initClient(host, port, path);
    connectedFuture = connectedPromise.get_future();
    protocolInitFuture = protocolInitPromise.get_future();
    client->connect();

    auto status = connectedFuture.wait_for(connectionTimeout);
    if (status == std::future_status::ready)
    {
        if (connectedFuture.get() == ConnectionResult::Connected &&
            isProtocolInitialized(protocolInitTimeout))
            return true;
    }

    client.reset();
    return false;
}

void NativeStreamingClientHandler::subscribeSignal(const StringPtr& signalStringId)
{
    const auto it = std::find_if(std::begin(signalIds),
                                 std::end(signalIds),
                                 [signalStringId](const auto& pair)
                                 {
                                     return signalStringId == pair.second;
                                 });

    if (it != std::end(signalIds))
    {
        sessionHandler->sendSignalSubscribe(it->first, signalStringId.toStdString());
    }
}

void NativeStreamingClientHandler::unsubscribeSignal(const StringPtr& signalStringId)
{
    const auto it = std::find_if(std::begin(signalIds),
                                 std::end(signalIds),
                                 [signalStringId](const auto& pair)
                                 {
                                     return signalStringId == pair.second;
                                 });

    if (it != std::end(signalIds))
    {
        sessionHandler->sendSignalUnsubscribe(it->first, signalStringId.toStdString());
    }
}

EventPacketPtr NativeStreamingClientHandler::getDataDescriptorChangedEventPacket(const StringPtr& signalStringId)
{
    const auto it = std::find_if(std::begin(signalIds),
                                 std::end(signalIds),
                                 [signalStringId](const auto& pair)
                                 {
                                     return signalStringId == pair.second;
                                 });

    if (it != std::end(signalIds))
    {
        return sessionHandler->getDataDescriptorChangedEventPacket(it->first);
    }
    else
    {
        throw NativeStreamingProtocolException("Signal Id not found");
    }
}

void NativeStreamingClientHandler::initClientSessionHandler(SessionPtr session)
{
    LOG_D("Client connected");

    OnSessionErrorCallback errorHandler = [this](const std::string& errorMessage, SessionPtr session)
    {
        LOG_W("Closing connection caused by: {}", errorMessage);
        sessionHandler.reset();
        if (session->isOpen())
        {
            session->close(
                [this](const boost::system::error_code&)
                {
                    reconnectionStatusChangedCb(ClientReconnectionStatus::Reconnecting);
                    tryReconnect();
                });
        }
        else
        {
            reconnectionStatusChangedCb(ClientReconnectionStatus::Reconnecting);
            tryReconnect();
        }
    };
    // read/write failure indicates that connection is closed, and it should be handled properly
    // client constantly and continuously perform read operation
    // so connection closing is handled only on read failure and not handled on write failure
    session->setErrorHandlers([](const std::string&, SessionPtr) {}, errorHandler);

    OnSignalCallback signalReceivedHandler =
        [this](const SignalNumericIdType& signalNumericId,
               const StringPtr& signalStringId,
               const StringPtr& domainSignalStringId,
               const DataDescriptorPtr& signalDescriptor,
               const StringPtr& name,
               const StringPtr& description,
               bool available)
    {
        handleSignal(signalNumericId, signalStringId, domainSignalStringId, signalDescriptor, name, description, available);
    };

    OnPacketReceivedCallback packetReceivedHandler =
        [this](const SignalNumericIdType& signalNumericId, const PacketPtr& packet)
    {
        handlePacket(signalNumericId, packet);
    };

    OnProtocolInitDoneCallback protocolInitDoneHandler =
        [this]()
    {
        protocolInitPromise.set_value();
    };

    OnSubscriptionAckCallback subscriptionAckCallback =
        [this](const SignalNumericIdType& signalNumericId, bool subscribed)
    {
        signalSubscriptionAckCallback(signalIds.at(signalNumericId), subscribed);
    };

    sessionHandler = std::make_shared<ClientSessionHandler>(context,
                                                            *ioContextPtr.get(),
                                                            session,
                                                            signalReceivedHandler,
                                                            packetReceivedHandler,
                                                            protocolInitDoneHandler,
                                                            subscriptionAckCallback,
                                                            errorHandler);
    sessionHandler->startReading();

    connectedPromise.set_value(ConnectionResult::Connected);
}

void NativeStreamingClientHandler::initClient(std::string host,
                                              std::string port,
                                              std::string path)
{
    OnNewSessionCallback onNewSessionCallback =
        [this](SessionPtr session)
    {
        initClientSessionHandler(session);
    };
    OnCompleteCallback onResolveFailCallback =
        [this](const boost::system::error_code& ec)
    {
        LOG_E("Address resolving failed: {}", ec.message());
        connectedPromise.set_value(ConnectionResult::ServerUnreachable);
    };
    OnCompleteCallback onConnectFailCallback =
        [this](const boost::system::error_code& ec)
    {
        LOG_E("Connection failed: {}", ec.message());
        connectedPromise.set_value(ConnectionResult::ServerUnreachable);
    };
    OnCompleteCallback onHandshakeFailCallback =
        [this](const boost::system::error_code& ec)
    {
        LOG_E("Handshake failed: {}", ec.message());
        connectedPromise.set_value(ConnectionResult::ServerUnsupported);
    };
    LogCallback logCallback =
        [this](spdlog::source_loc location, spdlog::level::level_enum level, const char* msg)
    {
        loggerComponent.logMessage(SourceLocation{location.filename, location.line, location.funcname},
                                   msg,
                                   static_cast<LogLevel>(level));
    };

    client = std::make_shared<Client>(host,
                                      port,
                                      path,
                                      onNewSessionCallback,
                                      onResolveFailCallback,
                                      onConnectFailCallback,
                                      onHandshakeFailCallback,
                                      ioContextPtr,
                                      logCallback);
}

void NativeStreamingClientHandler::handlePacket(const SignalNumericIdType& signalNumericId, const PacketPtr& packet)
{
    packetHandler(signalIds.at(signalNumericId), packet);
}

void NativeStreamingClientHandler::handleSignal(const SignalNumericIdType& signalNumericId,
                                                const StringPtr& signalStringId,
                                                const StringPtr& domainSignalStringId,
                                                const DataDescriptorPtr& signalDescriptor,
                                                const StringPtr& name,
                                                const StringPtr& description,
                                                bool available)
{
    if (available)
    {
        signalIds.insert({signalNumericId, signalStringId});
        signalAvailableHandler(signalStringId, domainSignalStringId, signalDescriptor, name, description);
    }
    else
    {
        signalIds.erase(signalNumericId);
        signalUnavailableHandler(signalStringId);
    }
}

END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_PROTOCOL
