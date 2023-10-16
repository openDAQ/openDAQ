#include <native_streaming_protocol/native_streaming_server_handler.h>

#include <opendaq/custom_log.h>
#include <opendaq/packet_factory.h>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_PROTOCOL

using namespace daq::native_streaming;

NativeStreamingServerHandler::NativeStreamingServerHandler(const ContextPtr& context,
                                                           std::shared_ptr<boost::asio::io_context> ioContextPtr,
                                                           const ListPtr<ISignal>& signalsList,
                                                           OnSignalSubscribedCallback signalSubscribedHandler,
                                                           OnSignalUnsubscribedCallback signalUnsubscribedHandler)
    : context(context)
    , ioContextPtr(ioContextPtr)
    , logger(context.getLogger())
    , signalNumericIdCounter(0)
    , subscribersRegistry(context)
    , signalSubscribedHandler(signalSubscribedHandler)
    , signalUnsubscribedHandler(signalUnsubscribedHandler)
{
    if (!this->logger.assigned())
        throw ArgumentNullException("Logger must not be null");
    loggerComponent = this->logger.getOrAddComponent("NativeStreamingServerHandler");

    for (const auto& signal : signalsList)
    {
        registerSignal(signal);
        subscribersRegistry.registerSignal(signal);
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
    auto signalNumericId = registerSignal(signal);

    subscribersRegistry.registerSignal(signal);
    subscribersRegistry.sendToClients([signalNumericId, signal](std::shared_ptr<ServerSessionHandler>& sessionHandler)
                                      {
                                          sessionHandler->sendSignalAvailable(signalNumericId, signal);
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
    subscribersRegistry.sendToClients(
        [signalNumericId, &packet](std::shared_ptr<ServerSessionHandler>& sessionHandler)
        {
            sessionHandler->sendPacket(signalNumericId, packet);
        });
    return;

    // TODO send data packets only to subscribers
    if (packet.getType() == PacketType::Event)
    {
        subscribersRegistry.sendToClients(
            [signalNumericId, &packet](std::shared_ptr<ServerSessionHandler>& sessionHandler)
            {
                sessionHandler->sendPacket(signalNumericId, packet);
            });
    }
    else
    {
        subscribersRegistry.sendToSubscribers(
            signal,
            [signalNumericId, &packet](std::shared_ptr<ServerSessionHandler>& sessionHandler)
            {
                sessionHandler->sendPacket(signalNumericId, packet);
            });
    }
}

void NativeStreamingServerHandler::releaseSessionHandler(SessionPtr session)
{
    auto toUnsubscribe = subscribersRegistry.unregisterClient(session);
    for (const auto& item : toUnsubscribe)
    {
        signalUnsubscribedHandler(findRegisteredSignal(item));
    }
    session->close();
}

void NativeStreamingServerHandler::initSessionHandler(SessionPtr session)
{
    LOG_I("New connection accepted by server");

    OnErrorCallback errorHandler = [this](const std::string& errorMessage, SessionPtr session)
    {
        LOG_D("Server lost connection with client: {}", errorMessage);
        // call dispatch to run it in the ::io_context to omit concurrent access!
        ioContextPtr->dispatch([this, session]() { releaseSessionHandler(session); });
    };

    OnSignalSubscriptionCallback signalSubscriptionHandler =
        [this](const SignalNumericIdType& signalNumericId,
               const std::string& signalStringId,
               bool subscribe,
               SessionPtr session)
    {
        if (subscribe)
        {
            LOG_D("Server received subscribe command for signal: {}, numeric Id {}",
                  signalStringId, signalNumericId);
            if (subscribersRegistry.registerSignalSubscriber(signalStringId, session))
            {
                signalSubscribedHandler(findRegisteredSignal(signalStringId));
            }
        }
        else
        {
            LOG_D("Server received unsubscribe command for signal: {}, numeric Id {}",
                  signalStringId, signalNumericId);
            if (subscribersRegistry.removeSignalSubscriber(signalStringId, session))
            {
                signalUnsubscribedHandler(findRegisteredSignal(signalStringId));
            }
        }
    };

    auto sessionHandler = std::make_shared<ServerSessionHandler>(context, session, signalSubscriptionHandler, errorHandler);
    sessionHandler->initErrorHandlers();

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
