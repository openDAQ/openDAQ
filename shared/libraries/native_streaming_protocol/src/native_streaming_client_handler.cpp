#include <native_streaming_protocol/native_streaming_client_handler.h>
#include <native_streaming/client.hpp>

#include <opendaq/custom_log.h>
#include <opendaq/packet_factory.h>

#include <coreobjects/property_factory.h>
#include <coreobjects/property_object_factory.h>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_PROTOCOL

using namespace daq::native_streaming;

NativeStreamingClientImpl::NativeStreamingClientImpl(const ContextPtr& context,
                                                     const PropertyObjectPtr& transportLayerProperties,
                                                     const PropertyObjectPtr& authenticationObject,
                                                     const std::shared_ptr<boost::asio::io_context>& ioContextPtr)
    : context(context)
    , transportLayerProperties(transportLayerProperties)
    , authenticationObject(authenticationObject)
    , ioContextPtr(ioContextPtr)
    , loggerComponent(context.getLogger().getOrAddComponent("NativeStreamingClientImpl"))
    , reconnectionTimer(std::make_shared<boost::asio::steady_timer>(*ioContextPtr))
{
    manageTransportLayerProps();
    resetStreamingHandlers();
    resetConfigHandlers();
}

NativeStreamingClientImpl::~NativeStreamingClientImpl()
{
    reconnectionTimer->cancel();
}

void NativeStreamingClientImpl::manageTransportLayerProps()
{
    if (!transportLayerProperties.assigned())
        throw ArgumentNullException("Transport layer properties cannot be null");

    if (!transportLayerProperties.hasProperty("MonitoringEnabled"))
        throw NotFoundException("Transport layer MonitoringEnabled property not found");
    if (!transportLayerProperties.hasProperty("HeartbeatPeriod"))
        throw NotFoundException("Transport layer HeartbeatPeriod property not found");
    if (!transportLayerProperties.hasProperty("InactivityTimeout"))
        throw NotFoundException("Transport layer InactivityTimeout property not found");
    if (!transportLayerProperties.hasProperty("ConnectionTimeout"))
        throw NotFoundException("Transport layer ConnectionTimeout property not found");
    if (!transportLayerProperties.hasProperty("StreamingInitTimeout"))
        throw NotFoundException("Transport layer StreamingInitTimeout property not found");
    if (!transportLayerProperties.hasProperty("ReconnectionPeriod"))
        throw NotFoundException("Transport layer ReconnectionPeriod property not found");

    if (transportLayerProperties.hasProperty("ClientId") &&
        transportLayerProperties.getProperty("ClientId").getValueType() != ctString)
        throw NotFoundException("Transport layer ClientId property should be of String type");
    if (transportLayerProperties.getProperty("MonitoringEnabled").getValueType() != ctBool)
        throw NotFoundException("Transport layer MonitoringEnabled property should be of Bool type");
    if (transportLayerProperties.getProperty("HeartbeatPeriod").getValueType() != ctInt)
        throw NotFoundException("Transport layer HeartbeatPeriod property should be of Int type");
    if (transportLayerProperties.getProperty("InactivityTimeout").getValueType() != ctInt)
        throw NotFoundException("Transport layer InactivityTimeout property should be of Int type");
    if (transportLayerProperties.getProperty("ConnectionTimeout").getValueType() != ctInt)
        throw NotFoundException("Transport layer ConnectionTimeout property should be of Int type");
    if (transportLayerProperties.getProperty("ReconnectionPeriod").getValueType() != ctInt)
        throw NotFoundException("Transport layer ReconnectionPeriod property should be of Int type");

    connectionMonitoringEnabled = transportLayerProperties.getPropertyValue("MonitoringEnabled");
    heartbeatPeriod = transportLayerProperties.getPropertyValue("HeartbeatPeriod");
    connectionInactivityTimeout = transportLayerProperties.getPropertyValue("InactivityTimeout");
    connectionTimeout = std::chrono::milliseconds(transportLayerProperties.getPropertyValue("ConnectionTimeout"));
    reconnectionPeriod = std::chrono::milliseconds(transportLayerProperties.getPropertyValue("ReconnectionPeriod"));

    if (!transportLayerProperties.hasProperty("Reconnected"))
        transportLayerProperties.addProperty(BoolProperty("Reconnected", False));
}

void NativeStreamingClientImpl::resetStreamingHandlers()
{
    this->signalAvailableHandler = [](const StringPtr&, const StringPtr&) {};
    this->signalUnavailableHandler = [](const StringPtr&) {};
    this->packetHandler = [](const StringPtr&, const PacketPtr&) {};
    this->signalSubscriptionAckCallback = [](const StringPtr&, bool) {};
    this->connectionStatusChangedStreamingCb = [](const EnumerationPtr&) {};
    this->streamingInitDoneCb = []() {};
}

void NativeStreamingClientImpl::setStreamingHandlers(const OnSignalAvailableCallback& signalAvailableHandler,
                                                     const OnSignalUnavailableCallback& signalUnavailableHandler,
                                                     const OnPacketCallback& packetHandler,
                                                     const OnSignalSubscriptionAckCallback& signalSubscriptionAckCallback,
                                                     const OnConnectionStatusChangedCallback& connectionStatusChangedCb,
                                                     const OnStreamingInitDoneCallback& streamingInitDoneCb)
{
    this->streamingInitDoneCb = streamingInitDoneCb;
    this->connectionStatusChangedStreamingCb = connectionStatusChangedCb;
    this->signalSubscriptionAckCallback = signalSubscriptionAckCallback;
    this->packetHandler = packetHandler;
    this->signalUnavailableHandler = signalUnavailableHandler;
    this->signalAvailableHandler = signalAvailableHandler;
}

void NativeStreamingClientImpl::resetConfigHandlers()
{
    this->connectionStatusChangedConfigCb = [](const EnumerationPtr&) {};
    this->configPacketHandler = [](config_protocol::PacketBuffer&&) {};
}

void NativeStreamingClientImpl::setConfigHandlers(const ProcessConfigProtocolPacketCb& configPacketHandler,
                                                  const OnConnectionStatusChangedCallback& connectionStatusChangedCb)
{
    this->configPacketHandler = configPacketHandler;
    this->connectionStatusChangedConfigCb = connectionStatusChangedCb;
}

void NativeStreamingClientImpl::checkReconnectionResult(const boost::system::error_code& ec)
{
    if (ec)
        return;

    auto status = connectedFuture.wait_for(std::chrono::seconds(0));
    if (status == std::future_status::ready)
    {
        ConnectionResult result = connectedFuture.get();
        if (result == ConnectionResult::Connected)
        {
            connectionStatusChanged(Enumeration("ConnectionStatusType", "Connected", this->context.getTypeManager()));
        }
        else if (result == ConnectionResult::ServerUnsupported)
        {
            connectionStatusChanged(Enumeration("ConnectionStatusType", "Unrecoverable", this->context.getTypeManager()));
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
        reconnectionTimer->async_wait(std::bind(&NativeStreamingClientImpl::checkReconnectionResult, this, std::placeholders::_1));
    }
}

void NativeStreamingClientImpl::tryReconnect()
{
    LOG_I("Try reconnect ...");

    reconnectionTimer->cancel();

    connectedPromise = std::promise<ConnectionResult>();
    connectedFuture = connectedPromise.get_future();

    reconnectionTimer->expires_from_now(reconnectionPeriod);
    reconnectionTimer->async_wait(std::bind(&NativeStreamingClientImpl::checkReconnectionResult, this, std::placeholders::_1));

    client->connect(connectionTimeout);
}

void NativeStreamingClientImpl::connectionStatusChanged(const EnumerationPtr& status)
{
    connectionStatusChangedStreamingCb(status);
    connectionStatusChangedConfigCb(status);
}

bool NativeStreamingClientImpl::connect(std::string host,
                                           std::string port,
                                           std::string path)
{
    initClient(host, port, path);
    connectedFuture = connectedPromise.get_future();
    client->connect(connectionTimeout);

    if (connectedFuture.wait_for(connectionTimeout) == std::future_status::ready &&
        connectedFuture.get() == ConnectionResult::Connected)
            return true;

    client.reset();
    return false;
}

void NativeStreamingClientImpl::subscribeSignal(const StringPtr& signalStringId)
{
    std::scoped_lock lock(registeredSignalsSync);

    const auto it = std::find_if(std::begin(signalIds),
                                 std::end(signalIds),
                                 [signalStringId](const auto& pair)
                                 {
                                     return signalStringId == pair.second;
                                 });

    if (it != std::end(signalIds))
    {
        if (auto sessionHandlerTemp = this->sessionHandler; sessionHandlerTemp)
            sessionHandlerTemp->sendSignalSubscribe(it->first, signalStringId.toStdString());
    }
}

void NativeStreamingClientImpl::unsubscribeSignal(const StringPtr& signalStringId)
{
    std::scoped_lock lock(registeredSignalsSync);

    const auto it = std::find_if(std::begin(signalIds),
                                 std::end(signalIds),
                                 [signalStringId](const auto& pair)
                                 {
                                     return signalStringId == pair.second;
                                 });

    if (it != std::end(signalIds))
    {
        if (auto sessionHandlerTemp = this->sessionHandler; sessionHandlerTemp)
            sessionHandlerTemp->sendSignalUnsubscribe(it->first, signalStringId.toStdString());
    }
}

void NativeStreamingClientImpl::sendConfigRequest(const config_protocol::PacketBuffer& packet)
{
    if (auto sessionHandlerTemp = this->sessionHandler; sessionHandlerTemp)
        sessionHandlerTemp->sendConfigurationPacket(packet);
}

void NativeStreamingClientImpl::sendStreamingRequest()
{
    // FIXME keep and reuse packet client when packet retransmission feature will be enabled
    packetStreamingClientPtr = std::make_shared<packet_streaming::PacketStreamingClient>();

    {
        std::scoped_lock lock(registeredSignalsSync);
        signalIds.clear();
    }

    if (auto sessionHandlerTemp = this->sessionHandler; sessionHandlerTemp)
        sessionHandlerTemp->sendStreamingRequest();
}

void NativeStreamingClientImpl::sendStreamingPacket(SignalNumericIdType signalNumericId, const PacketPtr& packet)
{
    if (auto sessionHandlerTemp = this->sessionHandler; sessionHandlerTemp)
    {
        if (auto packetStreamingServerTemp = this->packetStreamingServerPtr; packetStreamingServerTemp)
        {
            packetStreamingServerTemp->addDaqPacket(signalNumericId, packet);
            while (auto packetBuffer = packetStreamingServerTemp->getNextPacketBuffer())
            {
                sessionHandlerTemp->sendPacketBuffer(std::move(packetBuffer));
            }
        }
    }
}

void NativeStreamingClientImpl::onSessionError(const std::string& errorMessage, SessionPtr session)
{
    LOG_W("Closing connection caused by: {}", errorMessage);
    sessionHandler.reset();
    packetStreamingServerPtr.reset();

    connectionStatusChanged(Enumeration("ConnectionStatusType", "Reconnecting", this->context.getTypeManager()));
    transportLayerProperties.setPropertyValue("Reconnected", True);
    tryReconnect();
}

void NativeStreamingClientImpl::onPacketBufferReceived(const packet_streaming::PacketBufferPtr& packetBuffer)
{
    if (packetStreamingClientPtr)
    {
        packetStreamingClientPtr->addPacketBuffer(packetBuffer);

        auto [signalNumericId, packet] = packetStreamingClientPtr->getNextDaqPacket();
        while (packet.assigned())
        {
            packetHandler(signalIds.at(signalNumericId), packet);
            std::tie(signalNumericId, packet) = packetStreamingClientPtr->getNextDaqPacket();
        }
    }
}

void NativeStreamingClientImpl::initClientSessionHandler(SessionPtr session)
{
    LOG_I("Client connected to server endpoint: {}", session->getEndpointAddress());

    OnSessionErrorCallback errorHandler =
        [thisWeakPtr = this->weak_from_this()](const std::string& errorMessage, SessionPtr session)
    {
        if (const auto thisPtr = thisWeakPtr.lock())
            thisPtr->onSessionError(errorMessage, session);
    };
    // read/write failure indicates that connection is closed, and it should be handled properly
    // client constantly and continuously perform read operation
    // so connection closing is handled only on read failure and not handled on write failure
    session->setErrorHandlers([](const std::string&, SessionPtr) {}, errorHandler);

    OnSignalCallback signalReceivedHandler =
        [thisWeakPtr = this->weak_from_this()](const SignalNumericIdType& signalNumericId,
                                               const StringPtr& signalStringId,
                                               const StringPtr& serializedSignal,
                                               bool available)
    {
        if (const auto thisPtr = thisWeakPtr.lock())
            thisPtr->handleSignal(signalNumericId, signalStringId, serializedSignal, available);
    };

    OnStreamingInitDoneCallback protocolInitDoneHandler =
        [thisWeakPtr = this->weak_from_this()]()
    {
        if (const auto thisPtr = thisWeakPtr.lock())
            thisPtr->streamingInitDoneCb();
    };

    OnSubscriptionAckCallback subscriptionAckCallback =
        [thisWeakPtr = this->weak_from_this()](const SignalNumericIdType& signalNumericId, bool subscribed)
    {
        if (const auto thisPtr = thisWeakPtr.lock())
            thisPtr->signalSubscriptionAckCallback(thisPtr->signalIds.at(signalNumericId), subscribed);
    };

    sessionHandler = std::make_shared<ClientSessionHandler>(context,
                                                            ioContextPtr,
                                                            session,
                                                            signalReceivedHandler,
                                                            protocolInitDoneHandler,
                                                            subscriptionAckCallback,
                                                            errorHandler);

    ProcessConfigProtocolPacketCb configPacketReceivedHandler =
        [thisWeakPtr = this->weak_from_this()](config_protocol::PacketBuffer&& packet)
    {
        if (const auto thisPtr = thisWeakPtr.lock())
            thisPtr->configPacketHandler(std::move(packet));
    };
    sessionHandler->setConfigPacketReceivedHandler(configPacketReceivedHandler);

    OnPacketBufferReceivedCallback packetBufferReceivedHandler =
        [thisWeakPtr = this->weak_from_this()](const packet_streaming::PacketBufferPtr& packetBuffer)
    {
        if (const auto thisPtr = thisWeakPtr.lock())
            thisPtr->onPacketBufferReceived(packetBuffer);
    };
    sessionHandler->setPacketBufferReceivedHandler(packetBufferReceivedHandler);

    // FIXME keep and reuse packet server when packet retransmission feature will be enabled
    packetStreamingServerPtr = std::make_shared<packet_streaming::PacketStreamingServer>();

    sessionHandler->sendTransportLayerProperties(transportLayerProperties);
    if (connectionMonitoringEnabled)
        sessionHandler->startConnectionActivityMonitoring(heartbeatPeriod, connectionInactivityTimeout);

    sessionHandler->startReading();

    connectedPromise.set_value(ConnectionResult::Connected);
}

Authentication NativeStreamingClientImpl::initClientAuthenticationObject(const PropertyObjectPtr& authenticationObject)
{
    const StringPtr username = authenticationObject.getPropertyValue("Username");
    const StringPtr password = authenticationObject.getPropertyValue("Password");

    if (username.getLength() == 0)
        return Authentication();

    return Authentication(username, password);
}

void NativeStreamingClientImpl::onConnectionFailed(const std::string& errorMessage, const ConnectionResult result)
{
    LOG_E("{}", errorMessage);
    connectedPromise.set_value(result);
}

void NativeStreamingClientImpl::initClient(std::string host,
                                              std::string port,
                                              std::string path)
{
    const auto clientAuth = initClientAuthenticationObject(authenticationObject);

    OnNewSessionCallback onNewSessionCallback =
        [thisWeakPtr = this->weak_from_this()](SessionPtr session)
    {
        if (const auto thisPtr = thisWeakPtr.lock())
            thisPtr->initClientSessionHandler(session);
    };
    OnCompleteCallback onResolveFailCallback =
        [thisWeakPtr = this->weak_from_this()](const boost::system::error_code& ec)
    {
        if (const auto thisPtr = thisWeakPtr.lock())
            thisPtr->onConnectionFailed(fmt::format("Address resolving failed: {}", ec.message()),
                                        ConnectionResult::ServerUnreachable);
    };
    OnCompleteCallback onConnectFailCallback =
        [thisWeakPtr = this->weak_from_this()](const boost::system::error_code& ec)
    {
        if (const auto thisPtr = thisWeakPtr.lock())
            thisPtr->onConnectionFailed(fmt::format("Connection failed: {}", ec.message()),
                                        ConnectionResult::ServerUnreachable);
    };
    OnCompleteCallback onHandshakeFailCallback =
        [thisWeakPtr = this->weak_from_this()](const boost::system::error_code& ec)
    {
        if (const auto thisPtr = thisWeakPtr.lock())
            thisPtr->onConnectionFailed(fmt::format("Handshake failed: {}", ec.message()),
                                        ConnectionResult::ServerUnsupported);
    };
    LogCallback logCallback =
        [thisWeakPtr = this->weak_from_this()](spdlog::source_loc location, spdlog::level::level_enum level, const char* msg)
    {
        if (const auto thisPtr = thisWeakPtr.lock())
            thisPtr->loggerComponent.logMessage(SourceLocation{location.filename, location.line, location.funcname},
                                                msg,
                                                static_cast<LogLevel>(level));
    };

    client = std::make_shared<Client>(host,
                                      port,
                                      path,
                                      clientAuth,
                                      onNewSessionCallback,
                                      onResolveFailCallback,
                                      onConnectFailCallback,
                                      onHandshakeFailCallback,
                                      ioContextPtr,
                                      logCallback);
}

void NativeStreamingClientImpl::handleSignal(const SignalNumericIdType& signalNumericId,
                                                const StringPtr& signalStringId,
                                                const StringPtr& serializedSignal,
                                                bool available)
{
    {
        std::scoped_lock lock(registeredSignalsSync);
        if (available)
        {
            if (const auto it = signalIds.find(signalNumericId); it == signalIds.end())
            {
                signalIds.insert({signalNumericId, signalStringId});
            }
            else
            {
                LOG_E("Signal with numeric Id {} is already registered by client; string ids: registered {}, new {}",
                      signalNumericId,
                      it->second,
                      signalStringId);
                throw DuplicateItemException();
            }
        }
        else
        {
            signalIds.erase(signalNumericId);
        }
    }

    if (available)
        signalAvailableHandler(signalStringId, serializedSignal);
    else
        signalUnavailableHandler(signalStringId);
}

NativeStreamingClientHandler::NativeStreamingClientHandler(const ContextPtr& context,
                                                           const PropertyObjectPtr& transportLayerProperties,
                                                           const PropertyObjectPtr& authenticationObject)
    : ioContextPtr(std::make_shared<boost::asio::io_context>())
    , loggerComponent(context.getLogger().getOrAddComponent("NativeStreamingClientHandler"))
    , clientHandlerPtr(std::make_shared<NativeStreamingClientImpl>(context, transportLayerProperties, authenticationObject, ioContextPtr))
{
    startTransportOperations();
}

NativeStreamingClientHandler::~NativeStreamingClientHandler()
{
    clientHandlerPtr.reset();
    stopTransportOperations();
}

bool NativeStreamingClientHandler::connect(std::string host, std::string port, std::string path)
{
    return clientHandlerPtr->connect(host, port, path);
}

void NativeStreamingClientHandler::subscribeSignal(const StringPtr& signalStringId)
{
    clientHandlerPtr->subscribeSignal(signalStringId);
}

void NativeStreamingClientHandler::unsubscribeSignal(const StringPtr& signalStringId)
{
    clientHandlerPtr->unsubscribeSignal(signalStringId);
}

void NativeStreamingClientHandler::sendConfigRequest(const config_protocol::PacketBuffer& packet)
{
    clientHandlerPtr->sendConfigRequest(packet);
}

void NativeStreamingClientHandler::sendStreamingRequest()
{
    clientHandlerPtr->sendStreamingRequest();
}

void NativeStreamingClientHandler::sendStreamingPacket(SignalNumericIdType signalNumericId, const PacketPtr& packet)
{
    clientHandlerPtr->sendStreamingPacket(signalNumericId, packet);
}

std::shared_ptr<boost::asio::io_context> NativeStreamingClientHandler::getIoContext()
{
    return ioContextPtr;
}

void NativeStreamingClientHandler::resetStreamingHandlers()
{
    clientHandlerPtr->resetConfigHandlers();
}

void NativeStreamingClientHandler::setStreamingHandlers(const OnSignalAvailableCallback& signalAvailableHandler,
                                                        const OnSignalUnavailableCallback& signalUnavailableHandler,
                                                        const OnPacketCallback& packetHandler,
                                                        const OnSignalSubscriptionAckCallback& signalSubscriptionAckCallback,
                                                        const OnConnectionStatusChangedCallback& connectionStatusChangedCb,
                                                        const OnStreamingInitDoneCallback& streamingInitDoneCb)
{
    clientHandlerPtr->setStreamingHandlers(signalAvailableHandler,
                                    signalUnavailableHandler,
                                    packetHandler,
                                    signalSubscriptionAckCallback,
                                    connectionStatusChangedCb,
                                    streamingInitDoneCb);
}

void NativeStreamingClientHandler::resetConfigHandlers()
{
    clientHandlerPtr->resetConfigHandlers();
}

void NativeStreamingClientHandler::setConfigHandlers(const ProcessConfigProtocolPacketCb& configPacketHandler,
                                                     const OnConnectionStatusChangedCallback& connectionStatusChangedCb)
{
    clientHandlerPtr->setConfigHandlers(configPacketHandler, connectionStatusChangedCb);
}

void NativeStreamingClientHandler::startTransportOperations()
{
    ioThread =
        std::thread([this]()
                    {
                        using namespace boost::asio;
                        executor_work_guard<io_context::executor_type> workGuard(ioContextPtr->get_executor());
                        ioContextPtr->run();
                        LOG_I("Native transport IO thread finished");
                    });
}

void NativeStreamingClientHandler::stopTransportOperations()
{
    ioContextPtr->stop();
    if (ioThread.get_id() != std::this_thread::get_id())
    {
        if (ioThread.joinable())
        {
            ioThread.join();
            LOG_I("Native transport IO thread joined");
        }
        else
        {
            LOG_W("Native transport IO thread is not joinable");
        }
    }
    else
    {
        LOG_C("Native transport IO thread cannot join itself");
    }
}

END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_PROTOCOL
