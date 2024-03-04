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
    , signalNumericIdCounter(0)
    , subscribersRegistry(context)
    , signalSubscribedHandler(signalSubscribedHandler)
    , signalUnsubscribedHandler(signalUnsubscribedHandler)
    , setUpConfigProtocolServerCb(setUpConfigProtocolServerCb)
{
    for (const auto& signal : signalsList)
    {
        if (signal.getPublic())
        {
            registerSignal(signal);
            subscribersRegistry.registerSignal(signal);
        }
    }
}

void NativeStreamingServerHandler::startServer(uint16_t port)
{
    OnNewSessionCallback onNewSessionCallback = [this](std::shared_ptr<Session> session)
    {
        initSessionHandler(session);
    };
    daq::native_streaming::LogCallback logCallback =
        [this](spdlog::source_loc location, spdlog::level::level_enum level, const char* msg)
    {
        loggerComponent.logMessage(SourceLocation{location.filename, location.line, location.funcname},
                                   msg,
                                   static_cast<LogLevel>(level));
    };
    server = std::make_shared<daq::native_streaming::Server>(onNewSessionCallback, ioContextPtr, logCallback);
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

    auto signalNumericId = registerSignal(signal);

    subscribersRegistry.registerSignal(signal);
    subscribersRegistry.sendToClients([signalNumericId, signal](std::shared_ptr<ServerSessionHandler>& sessionHandler)
                                      {
                                          sessionHandler->sendSignalAvailable(signalNumericId, signal);

                                          // create and send event packet to initialize packet streaming
                                          sessionHandler->sendPacket(signalNumericId,
                                                                     createDataDescriptorChangedEventPacket(signal));
                                      });
}

void NativeStreamingServerHandler::removeComponentSignals(const StringPtr& componentId)
{
    std::scoped_lock lock(sync);

    auto signalsToRemove = List<ISignal>();
    auto removedComponentId = componentId.toStdString();

    for (const auto& [signalIdKey, value] : signalRegistry)
    {
        // removed component is a signal, or signal is a descendant of removed component
        if (signalIdKey == removedComponentId || IdsParser::isNestedComponentId(removedComponentId, signalIdKey))
        {
            signalsToRemove.pushBack(std::get<0>(value));
        }
    }

    for (const auto& signal : signalsToRemove)
    {
        removeSignalInternal(signal);
    }
}

void NativeStreamingServerHandler::removeSignalInternal(const SignalPtr& signal)
{
    if (subscribersRegistry.removeSignal(signal))
        signalUnsubscribedHandler(signal);
    auto signalNumericId = findSignalNumericId(signal);
    subscribersRegistry.sendToClients([signalNumericId, signal](std::shared_ptr<ServerSessionHandler>& sessionHandler)
                                      {
                                          sessionHandler->sendSignalUnavailable(signalNumericId, signal);
                                      });
    unregisterSignal(signal);
}

bool NativeStreamingServerHandler::handleSignalSubscription(const SignalNumericIdType& signalNumericId,
                                                            const std::string& signalStringId,
                                                            bool subscribe,
                                                            SessionPtr session)
{
    std::scoped_lock lock(sync);

    if (subscribe)
    {
        LOG_D("Server received subscribe command for signal: {}, numeric Id {}",
              signalStringId, signalNumericId);
        try
        {
            if (subscribersRegistry.registerSignalSubscriber(signalStringId, session))
            {
                signalSubscribedHandler(findRegisteredSignal(signalStringId));
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
            if (subscribersRegistry.removeSignalSubscriber(signalStringId, session))
            {
                signalUnsubscribedHandler(findRegisteredSignal(signalStringId));
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

void NativeStreamingServerHandler::sendPacket(const SignalPtr& signal, const PacketPtr& packet)
{
    auto signalNumericId = findSignalNumericId(signal);
    subscribersRegistry.sendToSubscribers(
        signal,
        [signalNumericId, &packet](std::shared_ptr<ServerSessionHandler>& sessionHandler)
        {
            sessionHandler->sendPacket(signalNumericId, packet);
        });
}

void NativeStreamingServerHandler::releaseSessionHandler(SessionPtr session)
{
    auto toUnsubscribe = subscribersRegistry.unregisterClient(session);
    for (const auto& item : toUnsubscribe)
    {
        signalUnsubscribedHandler(findRegisteredSignal(item));
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
        LOG_W("Invalid transport layer properties");
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
    ProcessConfigProtocolPacketCb receiveConfigPacketCb = setUpConfigProtocolServerCb(sendConfigPacketCb);
    sessionHandler->setConfigPacketReceivedHandler(receiveConfigPacketCb);
}

void NativeStreamingServerHandler::handleStreamingInit(SessionPtr session)
{
    subscribersRegistry.sendToClient(
        session,
        [this](std::shared_ptr<ServerSessionHandler>& sessionHandler)
        {
            // send sorted signals
            std::map<SignalNumericIdType, SignalPtr> sortedSignals;
            for (const auto& signalRegistryItem : signalRegistry)
            {
                sortedSignals.insert({std::get<1>(signalRegistryItem.second),
                                      std::get<0>(signalRegistryItem.second)});
            }
            for (const auto& sortedSignalsItem : sortedSignals)
            {
                sessionHandler->sendSignalAvailable(sortedSignalsItem.first, sortedSignalsItem.second);

                // create and send event packet to initialize packet streaming
                sessionHandler->sendPacket(sortedSignalsItem.first,
                                           createDataDescriptorChangedEventPacket(sortedSignalsItem.second));
            }
            sessionHandler->sendStreamingInitDone();
        }
    );
}

void NativeStreamingServerHandler::initSessionHandler(SessionPtr session)
{
    LOG_I("New connection accepted by server");

    OnSessionErrorCallback errorHandler = [this](const std::string& errorMessage, SessionPtr session)
    {
        LOG_W("Closing connection caused by: {}", errorMessage);
        // call dispatch to run it in the ::io_context to omit concurrent access!
        ioContextPtr->dispatch([this, session]() { releaseSessionHandler(session); });
    };
    // read/write failure indicates that connection is closed, and it should be handled properly
    // server constantly and continuously perform read operation
    // so connection closing is handled only on read failure and not handled on write failure
    session->setErrorHandlers([](const std::string&, SessionPtr) {}, errorHandler);

    OnStreamingRequestCallback streamingInitHandler =
        [this](SessionPtr session)
    {
        this->handleStreamingInit(session);
    };

    OnSignalSubscriptionCallback signalSubscriptionHandler =
        [this](const SignalNumericIdType& signalNumericId,
               const std::string& signalStringId,
               bool subscribe,
               SessionPtr session)
    {
        return this->handleSignalSubscription(signalNumericId, signalStringId, subscribe, session);
    };

    auto sessionHandler = std::make_shared<ServerSessionHandler>(context,
                                                                 *ioContextPtr.get(),
                                                                 session,
                                                                 streamingInitHandler,
                                                                 signalSubscriptionHandler,
                                                                 errorHandler);
    setUpTransportLayerPropsCallback(sessionHandler);
    setUpConfigProtocolCallbacks(sessionHandler);

    subscribersRegistry.registerClient(sessionHandler);
    sessionHandler->startReading();
}

SignalNumericIdType NativeStreamingServerHandler::findSignalNumericId(const SignalPtr& signal)
{
    auto signalKey = signal.getGlobalId().toStdString();

    if (auto iter = signalRegistry.find(signalKey); iter != signalRegistry.end())
        return std::get<1>(iter->second);
    else
        throw NativeStreamingProtocolException("Signal is not registered");
}

SignalNumericIdType NativeStreamingServerHandler::registerSignal(const SignalPtr& signal)
{
    auto signalKey = signal.getGlobalId().toStdString();

    if (auto iter = signalRegistry.find(signalKey); iter == signalRegistry.end())
    {
        auto signalNumericId = ++signalNumericIdCounter;
        signalRegistry.insert({signalKey, {signal, signalNumericId}});
        return signalNumericId;
    }
    else
    {
        throw NativeStreamingProtocolException("Signal is already registered");
    }
}

void NativeStreamingServerHandler::unregisterSignal(const SignalPtr& signal)
{
    auto signalKey = signal.getGlobalId().toStdString();

    if (auto iter = signalRegistry.find(signalKey); iter != signalRegistry.end())
        signalRegistry.erase(iter);
    else
        throw NativeStreamingProtocolException("Signal is not registered");
}

SignalPtr NativeStreamingServerHandler::findRegisteredSignal(const std::string& signalKey)
{
    if (auto iter = signalRegistry.find(signalKey); iter != signalRegistry.end())
        return std::get<0>(iter->second);
    else
        throw NativeStreamingProtocolException("Signal is not registered");
}

EventPacketPtr NativeStreamingServerHandler::createDataDescriptorChangedEventPacket(const SignalPtr& signal)
{
    DataDescriptorPtr domainDescriptor;
    if (signal.getDomainSignal().assigned())
        domainDescriptor = signal.getDomainSignal().getDescriptor();
    return DataDescriptorChangedEventPacket(signal.getDescriptor(), domainDescriptor);
}

END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_PROTOCOL
