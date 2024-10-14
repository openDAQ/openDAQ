#include <native_streaming_protocol/native_streaming_server_handler.h>

#include <opendaq/custom_log.h>
#include <opendaq/search_filter_factory.h>
#include <config_protocol/config_protocol_server.h>

#include <opendaq/ids_parser.h>

#include <coreobjects/property_object_factory.h>
#include <memory>
#include <coreobjects/user_factory.h>
#include <opendaq/errors.h>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_PROTOCOL

using namespace daq::native_streaming;


struct UserContextDeleter
{
    void operator()(void* obj)
    {
        ((IUser*) (obj))->releaseRef();
    }
};


NativeStreamingServerHandler::NativeStreamingServerHandler(const ContextPtr& context,
                                                           std::shared_ptr<boost::asio::io_context> ioContextPtr,
                                                           const ListPtr<ISignal>& signalsList,
                                                           OnSignalSubscribedCallback signalSubscribedHandler,
                                                           OnSignalUnsubscribedCallback signalUnsubscribedHandler,
                                                           SetUpConfigProtocolServerCb setUpConfigProtocolServerCb,
                                                           SizeT maxAllowedConfigConnections)
    : context(context)
    , ioContextPtr(ioContextPtr)
    , loggerComponent(context.getLogger().getOrAddComponent("NativeStreamingServerHandler"))
    , streamingManager(context)
    , signalSubscribedHandler(signalSubscribedHandler)
    , signalUnsubscribedHandler(signalUnsubscribedHandler)
    , setUpConfigProtocolServerCb(setUpConfigProtocolServerCb)
    , connectedClientIndex(0)
    , maxAllowedConfigConnections(maxAllowedConfigConnections)
    , configConnectionsCount(0)
{
    for (const auto& signal : signalsList)
    {
        if (signal.getPublic())
            streamingManager.registerSignal(signal);
    }
}

void NativeStreamingServerHandler::startServer(uint16_t port)
{
    OnNewSessionCallback onNewSessionCallback =
        [thisWeakPtr = this->weak_from_this()](std::shared_ptr<Session> session)
    {
        if (const auto thisPtr = thisWeakPtr.lock())
            thisPtr->initSessionHandler(session);
    };

    OnAuthenticateCallback onAuthenticateCallback =
        [thisWeakPtr = this->weak_from_this()](const daq::native_streaming::Authentication& authentication, std::shared_ptr<void>& userContextOut)
    {
        if (const auto thisPtr = thisWeakPtr.lock())
            return thisPtr->onAuthenticate(authentication, userContextOut);
        return false;
    };

    daq::native_streaming::LogCallback logCallback =
        [thisWeakPtr = this->weak_from_this()](spdlog::source_loc location, spdlog::level::level_enum level, const char* msg)
    {
        if (const auto thisPtr = thisWeakPtr.lock())
            thisPtr->loggerComponent.logMessage(SourceLocation{location.filename, location.line, location.funcname},
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
                                                            const SignalPtr& signal,
                                                            bool subscribe,
                                                            const std::string& clientId)
{
    std::scoped_lock lock(sync);
    const auto signalStringId = signal.getGlobalId();

    if (subscribe)
    {
        LOG_D("Server received subscribe command for signal: {}, numeric Id {}", signalStringId, signalNumericId);
        try
        {
            // The lambda passed as a parameter will be invoked immediately, making it safe to directly capture this
            bool doSignalSubscribe = streamingManager.registerSignalSubscriber(
                signalStringId,
                clientId,
                [this](const std::string& subscribedClientId, packet_streaming::PacketBufferPtr&& packetBuffer)
                {
                    sessionHandlers.at(subscribedClientId)->sendPacketBuffer(std::move(packetBuffer));
                });

            if (doSignalSubscribe)
            {
                signalSubscribedHandler(signal);
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
                signalUnsubscribedHandler(signal);
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

bool NativeStreamingServerHandler::onAuthenticate(const daq::native_streaming::Authentication& authentication,
                                                  std::shared_ptr<void>& userContextOut)
{
    const auto authProvider = context.getAuthenticationProvider();

    switch (authentication.getType())
    {
        case AuthenticationType::Anonymous:
        {
            try
            {
                UserPtr user = authProvider.authenticateAnonymous();
                userContextOut = std::shared_ptr<daq::IUser>(user.detach(), UserContextDeleter());
                return true;
            }
            catch (const DaqException& e)
            {
                LOG_W("Anonymous authentication rejected: ", e.what());
            }

            break;
        }
        case AuthenticationType::Basic:
        {
            try
            {
                UserPtr user = authProvider.authenticate(authentication.getUsername(), authentication.getPassword());
                userContextOut = std::shared_ptr<daq::IUser>(user.detach(), UserContextDeleter());
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

void NativeStreamingServerHandler::sendPacket(const SignalPtr& signal, PacketPtr&& packet)
{
    const auto signalStringId = signal.getGlobalId().toStdString();

    // The lambda passed as a parameter will be invoked immediately, making it safe to directly capture this
    streamingManager.sendPacketToSubscribers(
        signalStringId,
        std::move(packet),
        [this](const std::string& subscribedClientId, packet_streaming::PacketBufferPtr&& packetBuffer)
        {
            sessionHandlers.at(subscribedClientId)->sendPacketBuffer(std::move(packetBuffer));
        }
    );
}

void NativeStreamingServerHandler::releaseSessionHandler(SessionPtr session)
{
    std::shared_ptr<ServerSessionHandler> removedSessionHandler; // keep object outside the scoped lock
    {
        std::scoped_lock lock(sync);

        auto clientIter = std::find_if(sessionHandlers.begin(),
                                       sessionHandlers.end(),
                                       [&session](const std::pair<std::string, std::shared_ptr<ServerSessionHandler>>& item)
                                       {
                                           auto sessionHandler = item.second;
                                           return sessionHandler->getSession() == session;
                                       });
        if (clientIter != sessionHandlers.end())
        {
            removedSessionHandler = clientIter->second;
            auto clientId = clientIter->first;
            auto signalsToUnsubscribe = streamingManager.unregisterClient(clientId);
            for (const auto& signal : signalsToUnsubscribe)
                signalUnsubscribedHandler(signal);

            if (removedSessionHandler->isConfigProtocolUsed())
                configConnectionsCount--;

            sessionHandlers.erase(clientIter);
        }
        else
        {
            LOG_E("Session handler already unregistered");
        }
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
        [thisWeakPtr = this->weak_from_this(), sessionHandlerWeakPtr](const PropertyObjectPtr& propertyObject)
    {
        if (const auto sessionHandlerPtr = sessionHandlerWeakPtr.lock())
            if (const auto thisPtr = thisWeakPtr.lock())
                thisPtr->handleTransportLayerProps(propertyObject, sessionHandlerPtr);
    };
    sessionHandler->setTransportLayerPropsHandler(trasportLayerPropertiesCb);
}

void NativeStreamingServerHandler::setUpConfigProtocolCallbacks(std::shared_ptr<ServerSessionHandler> sessionHandler,
                                                                config_protocol::PacketBuffer&& firstPacketBuffer)
{
    auto sessionHandlerWeakPtr = std::weak_ptr<ServerSessionHandler>(sessionHandler);
    SendConfigProtocolPacketCb sendConfigPacketCb =
        [sessionHandlerWeakPtr](const config_protocol::PacketBuffer& packetBuffer)
    {
        if (const auto sessionHandlerPtr = sessionHandlerWeakPtr.lock())
            sessionHandlerPtr->sendConfigurationPacket(packetBuffer);
    };

    UserPtr user = sessionHandler->getUser();
    ConfigServerCallbacks configServerCallbacks = setUpConfigProtocolServerCb(sendConfigPacketCb, user);
    ProcessConfigProtocolPacketCb receiveConfigPacketCb = configServerCallbacks.first;
    OnPacketBufferReceivedCallback clientToDeviceStreamingCb = configServerCallbacks.second;
    sessionHandler->setConfigPacketReceivedHandler(receiveConfigPacketCb);
    sessionHandler->setPacketBufferReceivedHandler(clientToDeviceStreamingCb);

    // handle first received config packet with instantiated callback
    if (receiveConfigPacketCb)
        receiveConfigPacketCb(std::move(firstPacketBuffer));
}

void NativeStreamingServerHandler::connectConfigProtocol(std::shared_ptr<ServerSessionHandler> sessionHandler,
                                                         config_protocol::PacketBuffer&& firstPacketBuffer)
{
    {
        std::scoped_lock lock(sync);

        // Rejects new conenctions in case if count of concurrent config connections exceed the limit
        // defined with "MaxAllowedConfigConnections" property (where 0 value stands for unlimited count)
        if (maxAllowedConfigConnections != UNLIMITED_CONFIGURATION_CONNECTIONS && configConnectionsCount == maxAllowedConfigConnections)
        {
            std::string errorMessage =
                "Native configuration connection rejected - connections limit reached on server; addresses of connected clients:";
            for (const auto& [_, handler] : sessionHandlers)
            {
                if (handler->isConfigProtocolUsed())
                    errorMessage += " " + handler->getSession()->getEndpointAddress() + ";";
            }
            auto reply = config_protocol::ConfigProtocolServer::generateConnectionRejectedReply(
                firstPacketBuffer.getId(),
                OPENDAQ_ERR_CONNECTION_LIMIT_REACHED,
                errorMessage,
                JsonSerializer()
            );
            sessionHandler->sendConfigurationPacket(reply);
            LOG_W("{}", errorMessage)
            return;
        }
        else
        {
            sessionHandler->triggerUseConfigProtocol();
            configConnectionsCount++;
        }
    }
    this->setUpConfigProtocolCallbacks(sessionHandler, std::move(firstPacketBuffer));
}

void NativeStreamingServerHandler::setUpStreamingInitCallback(std::shared_ptr<ServerSessionHandler> sessionHandler)
{
    auto sessionHandlerWeakPtr = std::weak_ptr<ServerSessionHandler>(sessionHandler);
    OnStreamingRequestCallback streamingInitCb =
        [thisWeakPtr = this->weak_from_this(), sessionHandlerWeakPtr]()
    {
        if (auto sessionHandlerPtr = sessionHandlerWeakPtr.lock())
            if (const auto thisPtr = thisWeakPtr.lock())
                thisPtr->handleStreamingInit(sessionHandlerPtr);
    };
    sessionHandler->setStreamingInitHandler(streamingInitCb);
}

void NativeStreamingServerHandler::handleStreamingInit(std::shared_ptr<ServerSessionHandler> sessionHandler)
{
    std::scoped_lock lock(sync);

    streamingManager.registerClient(sessionHandler->getClientId(), sessionHandler->getReconnected());

    auto registeredSignals = streamingManager.getRegisteredSignals();
    for (const auto& [signalNumericId, signalPtr] : registeredSignals)
    {
        sessionHandler->sendSignalAvailable(signalNumericId, signalPtr);
    }
    sessionHandler->sendStreamingInitDone();
}

void NativeStreamingServerHandler::onSessionError(const std::string& errorMessage, SessionPtr session)
{
    LOG_I("Closing connection caused by: {}", errorMessage);
    // call dispatch to run it in the ::io_context to omit concurrent access!
    ioContextPtr->dispatch(
        [thisWeakPtr = this->weak_from_this(), session]()
        {
            if (const auto thisPtr = thisWeakPtr.lock())
                return thisPtr->releaseSessionHandler(session);
        });
}

void NativeStreamingServerHandler::initSessionHandler(SessionPtr session)
{
    LOG_I("New connection accepted by server, client endpoint: {}", session->getEndpointAddress());

    auto findSignalHandler = [thisWeakPtr = this->weak_from_this()](const std::string& signalId)
    {
        if (const auto thisPtr = thisWeakPtr.lock())
            return thisPtr->streamingManager.findRegisteredSignal(signalId);
        throw NativeStreamingProtocolException("Server handler object destroyed");
    };

    OnSessionErrorCallback errorHandler =
        [thisWeakPtr = this->weak_from_this()](const std::string& errorMessage, SessionPtr session)
    {
        if (const auto thisPtr = thisWeakPtr.lock())
            thisPtr->onSessionError(errorMessage, session);
    };

    // read/write failure indicates that connection is closed, and it should be handled properly
    // server constantly and continuously perform read operation
    // so connection closing is handled only on read failure and not handled on write failure
    session->setErrorHandlers([](const std::string&, SessionPtr) {}, errorHandler);

    OnSignalSubscriptionCallback signalSubscriptionHandler =
        [thisWeakPtr = this->weak_from_this()](const SignalNumericIdType& signalNumericId,
                                               const SignalPtr& signal,
                                               bool subscribe,
                                               const std::string& clientId)
    {
        if (const auto thisPtr = thisWeakPtr.lock())
            return thisPtr->handleSignalSubscription(signalNumericId, signal, subscribe, clientId);
        return false;
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
                                                                 findSignalHandler,
                                                                 signalSubscriptionHandler,
                                                                 errorHandler);

    setUpTransportLayerPropsCallback(sessionHandler);

    ProcessConfigProtocolPacketCb onFirstConfigPacketReceived =
        [thisWeakPtr = this->weak_from_this(), sessionHandlerWeakPtr = std::weak_ptr<ServerSessionHandler>(sessionHandler)]
        (config_protocol::PacketBuffer&& firstPacketBuffer)
    {
        if (const auto thisPtr = thisWeakPtr.lock())
        {
            if (const auto sessionHandler = sessionHandlerWeakPtr.lock())
                thisPtr->connectConfigProtocol(sessionHandler, std::move(firstPacketBuffer));
        }
    };
    sessionHandler->setConfigPacketReceivedHandler(onFirstConfigPacketReceived);

    setUpStreamingInitCallback(sessionHandler);

    sessionHandlers.insert({clientIdAssignedByServer, sessionHandler});

    sessionHandler->startReading();
}

END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_PROTOCOL
