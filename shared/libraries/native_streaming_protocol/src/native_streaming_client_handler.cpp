#include <native_streaming_protocol/native_streaming_client_handler.h>
#include <native_streaming/client.hpp>

#include <opendaq/custom_log.h>
#include <opendaq/packet_factory.h>

#include <coreobjects/property_object_factory.h>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_PROTOCOL

using namespace daq::native_streaming;

NativeStreamingClientHandler::NativeStreamingClientHandler(const ContextPtr& context,
                                                           const PropertyObjectPtr& transportLayerProperties)
    : context(context)
    , transportLayerProperties(transportLayerProperties)
    , ioContextPtr(std::make_shared<boost::asio::io_context>())
    , loggerComponent(context.getLogger().getOrAddComponent("NativeStreamingClientHandler"))
    , reconnectionTimer(std::make_shared<boost::asio::steady_timer>(*ioContextPtr))
{
    readTransportLayerProps();
    resetStreamingHandlers();
    resetConfigHandlers();
    startTransportOperations();
}

NativeStreamingClientHandler::~NativeStreamingClientHandler()
{
    reconnectionTimer->cancel();
    stopTransportOperations();
}

void NativeStreamingClientHandler::readTransportLayerProps()
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
}

void NativeStreamingClientHandler::resetStreamingHandlers()
{
    this->signalAvailableHandler = [](const StringPtr&, const StringPtr&) {};
    this->signalUnavailableHandler = [](const StringPtr&) {};
    this->packetHandler = [](const StringPtr&, const PacketPtr&) {};
    this->signalSubscriptionAckCallback = [](const StringPtr&, bool) {};
    this->reconnectionStatusChangedStreamingCb = [](ClientReconnectionStatus) {};
    this->streamingInitDoneCb = []() {};
}

void NativeStreamingClientHandler::setStreamingHandlers(const OnSignalAvailableCallback& signalAvailableHandler,
                                                        const OnSignalUnavailableCallback& signalUnavailableHandler,
                                                        const OnPacketCallback& packetHandler,
                                                        const OnSignalSubscriptionAckCallback& signalSubscriptionAckCallback,
                                                        const OnReconnectionStatusChangedCallback& reconnectionStatusChangedCb,
                                                        const OnStreamingInitDoneCallback& streamingInitDoneCb)
{
    this->streamingInitDoneCb = streamingInitDoneCb;
    this->reconnectionStatusChangedStreamingCb = reconnectionStatusChangedCb;
    this->signalSubscriptionAckCallback = signalSubscriptionAckCallback;
    this->packetHandler = packetHandler;
    this->signalUnavailableHandler = signalUnavailableHandler;
    this->signalAvailableHandler = signalAvailableHandler;
}

void NativeStreamingClientHandler::resetConfigHandlers()
{
    this->reconnectionStatusChangedConfigCb = [](ClientReconnectionStatus) {};
    this->configPacketHandler = [](config_protocol::PacketBuffer&&) {};
}

void NativeStreamingClientHandler::setConfigHandlers(const ProcessConfigProtocolPacketCb& configPacketHandler,
                                                     const OnReconnectionStatusChangedCallback& reconnectionStatusChangedCb)
{
    this->configPacketHandler = configPacketHandler;
    this->reconnectionStatusChangedConfigCb = reconnectionStatusChangedCb;
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
            reconnectionStatusChanged(ClientReconnectionStatus::Restored);
        }
        else if (result == ConnectionResult::ServerUnsupported)
        {
            reconnectionStatusChanged(ClientReconnectionStatus::Unrecoverable);
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

    connectedPromise = std::promise<ConnectionResult>();
    connectedFuture = connectedPromise.get_future();

    reconnectionTimer->expires_from_now(reconnectionPeriod);
    reconnectionTimer->async_wait(std::bind(&NativeStreamingClientHandler::checkReconnectionStatus, this, std::placeholders::_1));

    client->connect();
}

void NativeStreamingClientHandler::reconnectionStatusChanged(ClientReconnectionStatus status)
{
    reconnectionStatusChangedStreamingCb(status);
    reconnectionStatusChangedConfigCb(status);
}

bool NativeStreamingClientHandler::connect(std::string host,
                                           std::string port,
                                           std::string path)
{
    initClient(host, port, path);
    connectedFuture = connectedPromise.get_future();
    client->connect();

    if (connectedFuture.wait_for(connectionTimeout) == std::future_status::ready &&
        connectedFuture.get() == ConnectionResult::Connected)
            return true;

    client.reset();
    return false;
}

void NativeStreamingClientHandler::subscribeSignal(const StringPtr& signalStringId)
{
    std::scoped_lock lock(sync);

    const auto it = std::find_if(std::begin(signalIds),
                                 std::end(signalIds),
                                 [signalStringId](const auto& pair)
                                 {
                                     return signalStringId == pair.second;
                                 });

    if (it != std::end(signalIds))
    {
        if (sessionHandler)
            sessionHandler->sendSignalSubscribe(it->first, signalStringId.toStdString());
    }
}

void NativeStreamingClientHandler::unsubscribeSignal(const StringPtr& signalStringId)
{
    std::scoped_lock lock(sync);

    const auto it = std::find_if(std::begin(signalIds),
                                 std::end(signalIds),
                                 [signalStringId](const auto& pair)
                                 {
                                     return signalStringId == pair.second;
                                 });

    if (it != std::end(signalIds))
    {
        if (sessionHandler)
            sessionHandler->sendSignalUnsubscribe(it->first, signalStringId.toStdString());
    }
}

EventPacketPtr NativeStreamingClientHandler::getDataDescriptorChangedEventPacket(const StringPtr& signalStringId)
{
    std::scoped_lock lock(sync);

    const auto it = std::find_if(std::begin(signalIds),
                                 std::end(signalIds),
                                 [signalStringId](const auto& pair)
                                 {
                                     return signalStringId == pair.second;
                                 });

    if (it != std::end(signalIds))
    {
        if (sessionHandler)
            return sessionHandler->getDataDescriptorChangedEventPacket(it->first);
    }
    return DataDescriptorChangedEventPacket(nullptr, nullptr);
}

void NativeStreamingClientHandler::sendConfigRequest(const config_protocol::PacketBuffer& packet)
{
    if (sessionHandler)
        sessionHandler->sendConfigurationPacket(packet);
}

void NativeStreamingClientHandler::sendStreamingRequest()
{
    if (sessionHandler)
        sessionHandler->sendStreamingRequest();
}

std::shared_ptr<boost::asio::io_context> NativeStreamingClientHandler::getIoContext()
{
    return ioContextPtr;
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
                    reconnectionStatusChanged(ClientReconnectionStatus::Reconnecting);
                    tryReconnect();
                });
        }
        else
        {
            reconnectionStatusChanged(ClientReconnectionStatus::Reconnecting);
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
               const StringPtr& serializedSignal,
               bool available)
    {
        handleSignal(signalNumericId, signalStringId, serializedSignal, available);
    };

    OnPacketReceivedCallback packetReceivedHandler =
        [this](const SignalNumericIdType& signalNumericId, const PacketPtr& packet)
    {
        handlePacket(signalNumericId, packet);
    };

    OnStreamingInitDoneCallback protocolInitDoneHandler =
        [this]()
    {
        streamingInitDoneCb();
    };

    OnSubscriptionAckCallback subscriptionAckCallback =
        [this](const SignalNumericIdType& signalNumericId, bool subscribed)
    {
        signalSubscriptionAckCallback(signalIds.at(signalNumericId), subscribed);
    };

    sessionHandler = std::make_shared<ClientSessionHandler>(context,
                                                            ioContextPtr,
                                                            session,
                                                            signalReceivedHandler,
                                                            packetReceivedHandler,
                                                            protocolInitDoneHandler,
                                                            subscriptionAckCallback,
                                                            errorHandler);

    ProcessConfigProtocolPacketCb configPacketReceivedHandler =
        [this](config_protocol::PacketBuffer&& packet)
    {
        configPacketHandler(std::move(packet));
    };
    sessionHandler->setConfigPacketReceivedHandler(configPacketReceivedHandler);

    sessionHandler->sendTransportLayerProperties(transportLayerProperties);
    if (connectionMonitoringEnabled)
        sessionHandler->startConnectionActivityMonitoring(heartbeatPeriod, connectionInactivityTimeout);

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
                                                const StringPtr& serializedSignal,
                                                bool available)
{
    {
        std::scoped_lock lock(sync);
        if (available)
            signalIds.insert({signalNumericId, signalStringId});
        else
            signalIds.erase(signalNumericId);
    }

    if (available)
        signalAvailableHandler(signalStringId, serializedSignal);
    else
        signalUnavailableHandler(signalStringId);
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
