#include <native_streaming_protocol/streaming_manager.h>

#include <opendaq/custom_log.h>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_PROTOCOL

using namespace daq::native_streaming;

StreamingManager::StreamingManager(const ContextPtr& context)
    : context(context)
    , signalNumericIdCounter(0)
{
    auto logger = this->context.getLogger();
    if (!logger.assigned())
        throw ArgumentNullException("Logger must not be null");
    loggerComponent = logger.getOrAddComponent("NativeStreamingSubscribers");
}

void StreamingManager::sendPacketToSubscribers(const std::string& signalStringId,
                                               const PacketPtr& packet,
                                               const SendPacketBufferCallback& sendPacketBufferCb)
{
    std::scoped_lock lock(sync);

    if (auto iter = registeredSignals.find(signalStringId); iter != registeredSignals.end())
    {
        const auto& registeredSignal = iter->second;
        for (const auto& subscribedClientId : registeredSignal.subscribedClientsIds)
        {
            auto packetStreamingServerPtr = packetStreamingServers.at(subscribedClientId);

            packetStreamingServerPtr->addDaqPacket(registeredSignal.numericId, packet);
            while (const auto packetBuffer = packetStreamingServerPtr->getNextPacketBuffer())
            {
                sendPacketBufferCb(subscribedClientId, packetBuffer);
            }
        }
    }
    else
    {
        throw NativeStreamingProtocolException(fmt::format("Signal {} is not registered in streaming", signalStringId));
    }
}

void StreamingManager::sendPacketToClient(const std::string& signalStringId,
                                          const PacketPtr& packet,
                                          const SendPacketBufferCallback& sendPacketBufferCb,
                                          const std::string& clientId)
{
    std::scoped_lock lock(sync);

    if (auto iter = registeredSignals.find(signalStringId); iter != registeredSignals.end())
    {
        const auto& registeredSignal = iter->second;
        if (auto it = packetStreamingServers.find(clientId); it != packetStreamingServers.end())
        {
            auto packetStreamingServerPtr = it->second;

            packetStreamingServerPtr->addDaqPacket(registeredSignal.numericId, packet);
            while (const auto packetBuffer = packetStreamingServerPtr->getNextPacketBuffer())
            {
                sendPacketBufferCb(clientId, packetBuffer);
            }
        }
        else
        {
            throw NativeStreamingProtocolException(fmt::format("Client {} is not registered in streaming", signalStringId));
        }
    }
    else
    {
        throw NativeStreamingProtocolException(fmt::format("Signal {} is not registered in streaming", signalStringId));
    }
}

SignalNumericIdType StreamingManager::registerSignal(const SignalPtr& signal)
{
    auto signalStringId = signal.getGlobalId().toStdString();

    std::scoped_lock lock(sync);

    if (auto iter = registeredSignals.find(signalStringId); iter == registeredSignals.end())
    {
        auto signalNumericId = ++signalNumericIdCounter;
        registeredSignals.insert({signalStringId, RegisteredSignal(signal, signalNumericId)});
        return signalNumericId;
    }
    else
    {
        throw NativeStreamingProtocolException("Signal is already registered");
    }
}

bool StreamingManager::removeSignal(const SignalPtr& signal)
{
    bool doSignalUnsubscribe = false;
    auto signalStringId = signal.getGlobalId().toStdString();

    std::scoped_lock lock(sync);

    if (auto signalIter = registeredSignals.find(signalStringId); signalIter != registeredSignals.end())
    {
        const auto& subscribers = signalIter->second.subscribedClientsIds;
        if (!subscribers.empty())
            doSignalUnsubscribe = true;
        registeredSignals.erase(signalIter);
    }
    else
    {
        throw NativeStreamingProtocolException(fmt::format("Signal {} is not registered in streaming", signalStringId));
    }
    return doSignalUnsubscribe;
}

void StreamingManager::registerClient(const std::string& clientId, bool reconnected)
{
    std::scoped_lock lock(sync);

    if (auto clientIter = streamingClientsIds.find(clientId); clientIter != streamingClientsIds.end())
    {
        auto message = fmt::format("Client with id {} is already registered", clientId);
        LOG_C("{}", message);
        throw NativeStreamingProtocolException(message);
    }

    LOG_I("Client with ID \"{}\" (reconnected - {}) requested streaming", clientId, reconnected);

    streamingClientsIds.insert(clientId);

    // remove cached packet server if client is not auto-reconnected
    if (auto it = packetStreamingServers.find(clientId); it != packetStreamingServers.end() && !reconnected)
        packetStreamingServers.erase(it);

    // create new associated packet server if required
    if (auto it = packetStreamingServers.find(clientId); it == packetStreamingServers.end())
        packetStreamingServers.insert({clientId, std::make_shared<packet_streaming::PacketStreamingServer>(10)});
}

ListPtr<ISignal> StreamingManager::unregisterClient(const std::string& clientId)
{
    auto signalsToUnsubscribe = List<ISignal>();

    {
        std::scoped_lock lock(sync);

        // find and remove registered client Id
        if (auto clientIter = streamingClientsIds.find(clientId); clientIter != streamingClientsIds.end())
        {
            streamingClientsIds.erase(clientId);
        }
        else
        {
            LOG_I("Client was not registered");
            return List<ISignal>();
        }

        LOG_I("Streaming client with ID \"{}\" disconnected", clientId);

        // FIXME keep and reuse packet server when packet retransmission feature will be enabled
        if (auto it = packetStreamingServers.find(clientId); it != packetStreamingServers.end())
            packetStreamingServers.erase(it);
    }

    // find and remove client Id from subscribers
    for (auto& [signalStringId, registeredSignal] : registeredSignals)
    {
        if (removeSignalSubscriber(signalStringId, clientId))
        {
            LOG_D("Signal: {} - is unsubscribed", signalStringId);
            signalsToUnsubscribe.pushBack(registeredSignal.daqSignal);
        }
    }

    return signalsToUnsubscribe;
}

bool StreamingManager::registerSignalSubscriber(const std::string& signalStringId, const std::string& subscribedClientId)
{
    bool doSignalSubscribe = false;

    std::scoped_lock lock(sync);

    if (auto iter = registeredSignals.find(signalStringId); iter != registeredSignals.end())
    {
        auto& subscribers = iter->second.subscribedClientsIds;

        if (auto subscribersIter = subscribers.find(subscribedClientId); subscribersIter == subscribers.end())
        {
            if (subscribers.empty())
            {
                LOG_D("Signal: {} has first subscriber", signalStringId);
                doSignalSubscribe = true;
            }
            subscribers.insert(subscribedClientId);
        }
    }
    else
    {
        throw NativeStreamingProtocolException(fmt::format("Signal {} is not registered in streaming", signalStringId));
    }

    return doSignalSubscribe;
}

bool StreamingManager::removeSignalSubscriber(const std::string& signalStringId, const std::string& subscribedClientId)
{
    bool doSignalUnsubscribe = false;

    std::scoped_lock lock(sync);

    if (auto iter = registeredSignals.find(signalStringId); iter != registeredSignals.end())
    {
        auto& subscribers = iter->second.subscribedClientsIds;

        if (auto subscribersIter = subscribers.find(subscribedClientId); subscribersIter != subscribers.end())
        {
            subscribers.erase(subscribersIter);
            if (subscribers.empty())
            {
                LOG_D("Signal: {} has not subscribers", signalStringId);
                doSignalUnsubscribe = true;
            }
        }
    }
    else
    {
        throw NativeStreamingProtocolException(fmt::format("Signal {} is not registered in streaming", signalStringId));
    }

    return doSignalUnsubscribe;
}

void StreamingManager::setLastEventPacket(const std::string& signalStringId, const EventPacketPtr& packet)
{
    std::scoped_lock lock(sync);

    if (auto iter = registeredSignals.find(signalStringId); iter != registeredSignals.end())
        iter->second.lastEventPacket = packet;
    else
        throw NativeStreamingProtocolException(fmt::format("Signal {} is not registered in streaming", signalStringId));
}

EventPacketPtr StreamingManager::getLastEventPacket(const std::string& signalStringId)
{
    std::scoped_lock lock(sync);

    if (auto iter = registeredSignals.find(signalStringId); iter != registeredSignals.end())
        return iter->second.lastEventPacket;
    else
        throw NativeStreamingProtocolException(fmt::format("Signal {} is not registered in streaming", signalStringId));
}

SignalNumericIdType StreamingManager::findSignalNumericId(const SignalPtr& signal)
{
    auto signalStringId = signal.getGlobalId().toStdString();

    std::scoped_lock lock(sync);

    if (auto iter = registeredSignals.find(signalStringId); iter != registeredSignals.end())
        return iter->second.numericId;
    else
        throw NativeStreamingProtocolException(fmt::format("Signal {} is not registered in streaming", signalStringId));
}

SignalPtr StreamingManager::findRegisteredSignal(const std::string& signalStringId)
{
    std::scoped_lock lock(sync);

    if (auto iter = registeredSignals.find(signalStringId); iter != registeredSignals.end())
        return iter->second.daqSignal;
    else
        throw NativeStreamingProtocolException(fmt::format("Signal {} is not registered in streaming", signalStringId));
}

std::map<SignalNumericIdType, SignalPtr> StreamingManager::getRegisteredSignals()
{
    std::map<SignalNumericIdType, SignalPtr> sortedSignals;

    std::scoped_lock lock(sync);

    for (const auto& [_, registeredSignal] : registeredSignals)
    {
        sortedSignals.insert({registeredSignal.numericId, registeredSignal.daqSignal});
    }

    return sortedSignals;
}

std::vector<std::string> StreamingManager::getRegisteredClientsIds()
{
    std::scoped_lock lock(sync);

    return std::vector<std::string>(streamingClientsIds.begin(), streamingClientsIds.end());
}

StreamingManager::RegisteredSignal::RegisteredSignal(SignalPtr daqSignal, SignalNumericIdType numericId)
    : daqSignal(daqSignal)
    , numericId(numericId)
{}

END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_PROTOCOL
