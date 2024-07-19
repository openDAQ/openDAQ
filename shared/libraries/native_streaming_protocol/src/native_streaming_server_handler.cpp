#include <native_streaming_protocol/native_streaming_server_handler.h>

#include <opendaq/custom_log.h>
#include <opendaq/packet_factory.h>
#include <opendaq/search_filter_factory.h>
#include <config_protocol/config_protocol_server.h>

#include <opendaq/ids_parser.h>

#include <coreobjects/property_object_factory.h>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_PROTOCOL

using namespace daq::native_streaming;

NativeStreamingServerHandler::NativeStreamingServerHandler(const ContextPtr& context,
                                                           std::shared_ptr<boost::asio::io_context> ioContextPtr,
                                                           const ListPtr<ISignal>& signalsList,
                                                           OnSignalSubscribedCallback signalSubscribedHandler,
                                                           OnSignalUnsubscribedCallback signalUnsubscribedHandler,
                                                           SetUpConfigProtocolServerCb setUpConfigProtocolServerCb)
    : context(context)
    , ioContextPtr(ioContextPtr)
    , loggerComponent(context.getLogger().getOrAddComponent("NativeStreamingServerHandler"))
    , streamingManager(context)
    , signalSubscribedHandler(signalSubscribedHandler)
    , signalUnsubscribedHandler(signalUnsubscribedHandler)
    , setUpConfigProtocolServerCb(setUpConfigProtocolServerCb)
    , connectedClientIndex(0)
{
    for (const auto& signal : signalsList)
    {
        if (signal.getPublic())
            streamingManager.registerSignal(signal);
    }
}

void NativeStreamingServerHandler::startServer(uint16_t port)
{
    OnNewSessionCallback onNewSessionCallback = [this](std::shared_ptr<Session> session)
    {
        initSessionHandler(session);
    };

    OnAuthenticateCallback onAuthenticateCallback = [this](const daq::native_streaming::Authentication& authentication)
    {
        return onAuthenticate(authentication);
    };

    daq::native_streaming::LogCallback logCallback =
        [this](spdlog::source_loc location, spdlog::level::level_enum level, const char* msg)
    {
        loggerComponent.logMessage(SourceLocation{location.filename, location.line, location.funcname},
                                   msg,
                                   static_cast<LogLevel>(level));
    };

    server = std::make_shared<daq::native_streaming::Server>(onNewSessionCallback, onAuthenticateCallback, ioContextPtr, logCallback);
    server->start(port);
}

void NativeStreamingServerHandler::stopServer()
{
    if (server)
    {
        server->stop();
        server.reset();
    }
}

void NativeStreamingServerHandler::addSignal(const SignalPtr& signal)
{
    if (!signal.getPublic())
        return;

    std::scoped_lock lock(sync);

    auto signalNumericId = streamingManager.registerSignal(signal);
    auto streamingClientsIds = streamingManager.getRegisteredClientsIds();
    for (const auto& clientId : streamingClientsIds)
        sessionHandlers.at(clientId)->sendSignalAvailable(signalNumericId, signal);
}

void NativeStreamingServerHandler::removeComponentSignals(const StringPtr& componentId)
{
    auto signalsToRemove = List<ISignal>();
    auto removedComponentId = componentId.toStdString();

    std::scoped_lock lock(sync);

    for (const auto& [signalNumericId, signalPtr] : streamingManager.getRegisteredSignals())
    {
        auto signalStringId = signalPtr.getGlobalId().toStdString();
        // removed component is a signal, or signal is a descendant of removed component
        if (signalStringId == removedComponentId || IdsParser::isNestedComponentId(removedComponentId, signalStringId))
        {
            if (streamingManager.removeSignal(signalPtr))
                signalUnsubscribedHandler(signalPtr);

            auto streamingClientsIds = streamingManager.getRegisteredClientsIds();
            for (const auto& clientId : streamingClientsIds)
                sessionHandlers.at(clientId)->sendSignalUnavailable(signalNumericId, signalPtr);
        }
    }
}

bool NativeStreamingServerHandler::handleSignalSubscription(const SignalNumericIdType& signalNumericId,
                                                            const std::string& signalStringId,
                                                            bool subscribe,
                                                            const std::string& clientId)
{
    std::scoped_lock lock(sync);

    if (subscribe)
    {
        LOG_D("Server received subscribe command for signal: {}, numeric Id {}", signalStringId, signalNumericId);
        try
        {
            if (streamingManager.registerSignalSubscriber(signalStringId, clientId))
            {
                // creates reader
                // automatically generates first event packet that will initialize packet streaming
                signalSubscribedHandler(streamingManager.findRegisteredSignal(signalStringId));
            }
            else
            {
                // does not create reader
                // so send last event packet to initialize packet streaming
                // packet not assigned means initial event packet is not yet processed by streaming
                auto packet = streamingManager.getLastEventPacket(signalStringId);
                if (packet.assigned())
                {
                    streamingManager.sendPacketToClient(
                        signalStringId,
                        packet,
                        [this](const std::string& subscribedClientId, const packet_streaming::PacketBufferPtr& packetBuffer)
                        {
                            sessionHandlers.at(subscribedClientId)->sendPacketBuffer(packetBuffer);
                        },
                        clientId
                    );
                }
            }
        }
        catch (const std::exception& e)
        {
            LOG_W("Failed subscribing of signal: {}, numeric Id {}; {}",
                  signalStringId, signalNumericId, e.what());
            return false;
        }
    }
    else
    {
        LOG_D("Server received unsubscribe command for signal: {}, numeric Id {}",
              signalStringId, signalNumericId);
        try
        {
            if (streamingManager.removeSignalSubscriber(signalStringId, clientId))
            {
                signalUnsubscribedHandler(streamingManager.findRegisteredSignal(signalStringId));
            }
        }
        catch (const std::exception& e)
        {
            LOG_W("Failed unsubscribing of signal: {}, numeric Id {}; {}",
                  signalStringId, signalNumericId, e.what());
            return false;
        }
    }
    return true;
}

bool NativeStreamingServerHandler::onAuthenticate(const daq::native_streaming::Authentication& authentication)
{
    const auto authProvider = context.getAuthenticationProvider();

    switch (authentication.getType())
    {
        case AuthenticationType::Anonymous:
        {
            if (authProvider.isAnonymousAllowed())
                return true;

            LOG_W("Anonymous authentication rejected");
            break;
        }
        case AuthenticationType::Basic:
        {
            try
            {
                authProvider.authenticate(authentication.getUsername(), authentication.getPassword());
                return true;
            }
            catch (const DaqException& e)
            {
                LOG_W("Username authentication rejected: {}", e.what());
            }
            break;
        }
    }

    return false;
}

void NativeStreamingServerHandler::sendPacket(const SignalPtr& signal, const PacketPtr& packet)
{
    auto signalStringId = signal.getGlobalId().toStdString();

    if (packet.getType() == PacketType::Event &&
        packet.asPtr<IEventPacket>().getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED)
    {
        streamingManager.setLastEventPacket(signalStringId, packet.asPtr<IEventPacket>(true));
    }

    streamingManager.sendPacketToSubscribers(
        signalStringId,
        packet,
        [this](const std::string& subscribedClientId, const packet_streaming::PacketBufferPtr& packetBuffer)
        {
            sessionHandlers.at(subscribedClientId)->sendPacketBuffer(packetBuffer);
        }
    );
}

void NativeStreamingServerHandler::releaseSessionHandler(SessionPtr session)
{
    auto clientIter = std::find_if(sessionHandlers.begin(),
                                   sessionHandlers.end(),
                                   [&session](const std::pair<std::string, std::shared_ptr<ServerSessionHandler>>& item)
                                   {
                                       auto sessionHandler = item.second;
                                       return sessionHandler->getSession() == session;
                                   });
    if (clientIter != sessionHandlers.end())
    {
        auto clientId = clientIter->first;
        auto signalsToUnsubscribe = streamingManager.unregisterClient(clientId);
        for (const auto& signal : signalsToUnsubscribe)
        {
            signalUnsubscribedHandler(signal);
        }
    }
    else
    {
        LOG_E("Session handler already unregistered");
    }

    if (session->isOpen())
        session->close();
}

void NativeStreamingServerHandler::handleTransportLayerProps(const PropertyObjectPtr& propertyObject,
                                                             std::shared_ptr<ServerSessionHandler> sessionHandler)
{
    if (propertyObject.hasProperty("MonitoringEnabled") &&
        propertyObject.hasProperty("HeartbeatPeriod") &&
        propertyObject.hasProperty("InactivityTimeout") &&
        propertyObject.getProperty("MonitoringEnabled").getValueType() == ctBool &&
        propertyObject.getProperty("HeartbeatPeriod").getValueType() == ctInt &&
        propertyObject.getProperty("InactivityTimeout").getValueType() == ctInt)
    {
        Bool monitoringEnabled = propertyObject.getPropertyValue("MonitoringEnabled");
        Int heartbeatPeriod = propertyObject.getPropertyValue("HeartbeatPeriod");
        Int inactivityTimeout = propertyObject.getPropertyValue("InactivityTimeout");

        LOG_I("Connection activity monitoring {}, with heartbeat period {} ms, and inactivity timeout {} ms",
              monitoringEnabled ? "enabled" : "disabled",
              heartbeatPeriod,
              inactivityTimeout);

        if (monitoringEnabled)
            sessionHandler->startConnectionActivityMonitoring(heartbeatPeriod, inactivityTimeout);
    }
    else
    {
        LOG_W("Invalid transport layer properties - missing connection activity monitoring parameters");
    }

    if (propertyObject.hasProperty("Reconnected") &&
        propertyObject.hasProperty("ClientId") &&
        propertyObject.getProperty("Reconnected").getValueType() == ctBool &&
        propertyObject.getProperty("ClientId").getValueType() == ctString)
    {
        StringPtr clientId = propertyObject.getPropertyValue("ClientId");
        Bool reconnected = propertyObject.getPropertyValue("Reconnected");

        {
            std::scoped_lock lock(sync);

            if (auto clientIter = sessionHandlers.find(clientId); clientIter != sessionHandlers.end())
            {
                LOG_W("Client with id {} is already registered", clientId);
                return;
            }

            auto clientIdAssignedByServer = sessionHandler->getClientId();
            if (auto clientIter = sessionHandlers.find(clientIdAssignedByServer); clientIter != sessionHandlers.end())
            {
                auto item = sessionHandlers.extract(clientIter);
                item.key() = clientId.toStdString();
                sessionHandlers.insert(std::move(item));
            }
            else
            {
                throw NativeStreamingProtocolException(fmt::format("Client with id {} is not registered", clientIdAssignedByServer));
            }
        }

        sessionHandler->setClientId(clientId.toStdString());
        sessionHandler->setReconnected(reconnected);
    }
    else
    {
        LOG_W("Invalid transport layer properties - missing ClientId or Reconnected flag");
    }
}

void NativeStreamingServerHandler::setUpTransportLayerPropsCallback(std::shared_ptr<ServerSessionHandler> sessionHandler)
{
    auto sessionHandlerWeakPtr = std::weak_ptr<ServerSessionHandler>(sessionHandler);
    OnTrasportLayerPropertiesCallback trasportLayerPropertiesCb =
        [this, sessionHandlerWeakPtr](const PropertyObjectPtr& propertyObject)
    {
        if (auto sessionHandlerPtr = sessionHandlerWeakPtr.lock())
            handleTransportLayerProps(propertyObject, sessionHandlerPtr);
    };
    sessionHandler->setTransportLayerPropsHandler(trasportLayerPropertiesCb);
}

void NativeStreamingServerHandler::setUpConfigProtocolCallbacks(std::shared_ptr<ServerSessionHandler> sessionHandler)
{
    auto sessionHandlerWeakPtr = std::weak_ptr<ServerSessionHandler>(sessionHandler);
    SendConfigProtocolPacketCb sendConfigPacketCb =
        [sessionHandlerWeakPtr](const config_protocol::PacketBuffer& packetBuffer)
    {
        if (auto sessionHandlerPtr = sessionHandlerWeakPtr.lock())
            sessionHandlerPtr->sendConfigurationPacket(packetBuffer);
    };
    ConfigServerCallbacks configServerCallbacks = setUpConfigProtocolServerCb(sendConfigPacketCb);
    ProcessConfigProtocolPacketCb receiveConfigPacketCb = configServerCallbacks.first;
    OnPacketBufferReceivedCallback clientToDeviceStreamingCb = configServerCallbacks.second;
    sessionHandler->setConfigPacketReceivedHandler(receiveConfigPacketCb);
    sessionHandler->setPacketBufferReceivedHandler(clientToDeviceStreamingCb);
}

void NativeStreamingServerHandler::setUpStreamingInitCallback(std::shared_ptr<ServerSessionHandler> sessionHandler)
{
    auto sessionHandlerWeakPtr = std::weak_ptr<ServerSessionHandler>(sessionHandler);
    OnStreamingRequestCallback streamingInitCb =
        [this, sessionHandlerWeakPtr]()
    {
        if (auto sessionHandlerPtr = sessionHandlerWeakPtr.lock())
            this->handleStreamingInit(sessionHandlerPtr);
    };
    sessionHandler->setStreamingInitHandler(streamingInitCb);
}

void NativeStreamingServerHandler::handleStreamingInit(std::shared_ptr<ServerSessionHandler> sessionHandler)
{
    streamingManager.registerClient(sessionHandler->getClientId(), sessionHandler->getReconnected());

    auto registeredSignals = streamingManager.getRegisteredSignals();
    for (const auto& [signalNumericId, signalPtr] : registeredSignals)
    {
        sessionHandler->sendSignalAvailable(signalNumericId, signalPtr);
    }
    sessionHandler->sendStreamingInitDone();
}

void NativeStreamingServerHandler::initSessionHandler(SessionPtr session)
{
    LOG_I("New connection accepted by server");

    OnSessionErrorCallback errorHandler = [this](const std::string& errorMessage, SessionPtr session)
    {
        LOG_I("Closing connection caused by: {}", errorMessage);
        // call dispatch to run it in the ::io_context to omit concurrent access!
        ioContextPtr->dispatch([this, session]() { releaseSessionHandler(session); });
    };
    // read/write failure indicates that connection is closed, and it should be handled properly
    // server constantly and continuously perform read operation
    // so connection closing is handled only on read failure and not handled on write failure
    session->setErrorHandlers([](const std::string&, SessionPtr) {}, errorHandler);

    OnSignalSubscriptionCallback signalSubscriptionHandler =
        [this](const SignalNumericIdType& signalNumericId,
               const std::string& signalStringId,
               bool subscribe,
               const std::string& clientId)
    {
        return this->handleSignalSubscription(signalNumericId, signalStringId, subscribe, clientId);
    };

    std::string clientIdAssignedByServer;
    {
        std::scoped_lock lock(sync);
        clientIdAssignedByServer = fmt::format("AssignedByServer/{}", connectedClientIndex++);
    }

    auto sessionHandler = std::make_shared<ServerSessionHandler>(context,
                                                                 ioContextPtr,
                                                                 session,
                                                                 clientIdAssignedByServer,
                                                                 signalSubscriptionHandler,
                                                                 errorHandler);
    setUpTransportLayerPropsCallback(sessionHandler);
    setUpConfigProtocolCallbacks(sessionHandler);
    setUpStreamingInitCallback(sessionHandler);

    sessionHandlers.insert({clientIdAssignedByServer, sessionHandler});

    sessionHandler->startReading();
}

END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_PROTOCOL
