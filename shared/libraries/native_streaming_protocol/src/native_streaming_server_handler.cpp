#include <native_streaming_protocol/native_streaming_server_handler.h>

#include <opendaq/custom_log.h>
#include <opendaq/packet_factory.h>
#include <opendaq/search_filter_factory.h>
#include <config_protocol/config_protocol_server.h>

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

NativeStreamingServerHandler::~NativeStreamingServerHandler()
{
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

void NativeStreamingServerHandler::removeSignal(const SignalPtr& signal)
{
    auto signalNumericId = findSignalNumericId(signal);
    subscribersRegistry.sendToClients([signalNumericId, signal](std::shared_ptr<ServerSessionHandler>& sessionHandler)
                                      {
                                          sessionHandler->sendSignalUnavailable(signalNumericId, signal);
                                      });
    unregisterSignal(signal);
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

void NativeStreamingServerHandler::setUpConfigProtocolCallbacks(std::shared_ptr<ServerSessionHandler> sessionHandler)
{
    auto sessionWeakPtr = std::weak_ptr<ServerSessionHandler>(sessionHandler);
    ConfigProtocolPacketCb sendConfigPacketCb =
        [sessionWeakPtr](const config_protocol::PacketBuffer& packetBuffer)
    {
        if (auto sessionPtr = sessionWeakPtr.lock())
            sessionPtr->sendConfigurationPacket(packetBuffer);
    };
    ConfigProtocolPacketCb receiveConfigPacketCb = setUpConfigProtocolServerCb(sendConfigPacketCb);
    sessionHandler->setConfigPacketReceivedHandler(receiveConfigPacketCb);
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

    OnSignalSubscriptionCallback signalSubscriptionHandler =
        [this](const SignalNumericIdType& signalNumericId,
               const std::string& signalStringId,
               bool subscribe,
               SessionPtr session)
    {
        if (subscribe)
        {
            LOG_I("Server received subscribe command for signal: {}, numeric Id {}",
                  signalStringId, signalNumericId);
            if (subscribersRegistry.registerSignalSubscriber(signalStringId, session))
            {
                signalSubscribedHandler(findRegisteredSignal(signalStringId));
            }
        }
        else
        {
            LOG_I("Server received unsubscribe command for signal: {}, numeric Id {}",
                  signalStringId, signalNumericId);
            if (subscribersRegistry.removeSignalSubscriber(signalStringId, session))
            {
                signalUnsubscribedHandler(findRegisteredSignal(signalStringId));
            }
        }
    };

    auto sessionHandler = std::make_shared<ServerSessionHandler>(context,
                                                                 *ioContextPtr.get(),
                                                                 session,
                                                                 signalSubscriptionHandler,
                                                                 errorHandler);
    setUpConfigProtocolCallbacks(sessionHandler);

    // send sorted signals to newly connected client
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
    sessionHandler->sendInitializationDone();
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
