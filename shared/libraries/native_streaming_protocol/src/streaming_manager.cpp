#include <native_streaming_protocol/streaming_manager.h>

#include <opendaq/custom_log.h>
#include <opendaq/packet_factory.h>
#include <opendaq/event_packet_params.h>
#include <opendaq/data_descriptor_factory.h>

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
                                               PacketPtr&& packet,
                                               const SendPacketBufferCallback& sendPacketBufferCb)
{
    std::scoped_lock lock(sync);

    if (auto iter = registeredSignals.find(signalStringId); iter != registeredSignals.end())
    {
        auto& registeredSignal = iter->second;

        if (packet.getType() == PacketType::Event)
        {
            auto eventPacket = packet.asPtr<IEventPacket>();
            if (eventPacket.getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED)
            {
                const DataDescriptorPtr dataDescriptorParam = eventPacket.getParameters().get(event_packet_param::DATA_DESCRIPTOR);
                const DataDescriptorPtr domainDescriptorParam = eventPacket.getParameters().get(event_packet_param::DOMAIN_DATA_DESCRIPTOR);
                if (dataDescriptorParam.assigned())
                    registeredSignal.lastDataDescriptorParam = dataDescriptorParam;
                if (domainDescriptorParam.assigned())
                    registeredSignal.lastDomainDescriptorParam = domainDescriptorParam;
            }
        }

        if (auto it = registeredSignal.subscribedClientsIds.begin(); it != registeredSignal.subscribedClientsIds.end())
        {
            while (std::next(it) != registeredSignal.subscribedClientsIds.end())
            {
                sendDaqPacket(sendPacketBufferCb, packetStreamingServers.at(*it), PacketPtr(packet), *it, registeredSignal.numericId);  // copy packet ptr
                ++it;
            }

            sendDaqPacket(sendPacketBufferCb, packetStreamingServers.at(*it), std::move(packet), *it, registeredSignal.numericId); // move packet ptr
        }
    }
    else
    {
        throw NativeStreamingProtocolException(fmt::format("Signal {} is not registered in streaming", signalStringId));
    }
}

void StreamingManager::processPacket(const std::string& signalStringId, PacketPtr&& packet)
{
    std::scoped_lock lock(sync);

    if (auto iter = registeredSignals.find(signalStringId); iter != registeredSignals.end())
    {
        auto& registeredSignal = iter->second;

        if (packet.getType() == PacketType::Event)
        {
            auto eventPacket = packet.asPtr<IEventPacket>();
            if (eventPacket.getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED)
            {
                const DataDescriptorPtr dataDescriptorParam = eventPacket.getParameters().get(event_packet_param::DATA_DESCRIPTOR);
                const DataDescriptorPtr domainDescriptorParam = eventPacket.getParameters().get(event_packet_param::DOMAIN_DATA_DESCRIPTOR);
                if (dataDescriptorParam.assigned())
                    registeredSignal.lastDataDescriptorParam = dataDescriptorParam;
                if (domainDescriptorParam.assigned())
                    registeredSignal.lastDomainDescriptorParam = domainDescriptorParam;
            }
        }

        if (auto it = registeredSignal.subscribedClientsIds.begin(); it != registeredSignal.subscribedClientsIds.end())
        {
            while (std::next(it) != registeredSignal.subscribedClientsIds.end())
            {
                packetStreamingServers.at(*it)->addDaqPacket(registeredSignal.numericId, PacketPtr(packet));
                ++it;
            }
            pushToPacketStreamingServer(packetStreamingServers.at(*it), std::move(packet), registeredSignal.numericId);
        }
    }
    else
    {
        throw NativeStreamingProtocolException(fmt::format("Signal {} is not registered in streaming", signalStringId));
    }
}

PacketStreamingServerPtr StreamingManager::getPacketServerIfRegistered(const std::string& clientId)
{
    std::scoped_lock lock(sync);

    if (const auto it = streamingClientsIds.find(clientId); it != streamingClientsIds.end())
        return packetStreamingServers.at(clientId);

    return nullptr;
}

void StreamingManager::sendDaqPacket(const SendPacketBufferCallback& sendPacketBufferCb,
                                     const PacketStreamingServerPtr& packetStreamingServerPtr,
                                     PacketPtr&& packet,
                                     const std::string& clientId,
                                     SignalNumericIdType singalNumericId)
{
    pushToPacketStreamingServer(packetStreamingServerPtr,  std::move(packet), singalNumericId);
    while (auto packetBuffer = packetStreamingServerPtr->getNextPacketBuffer())
    {
        sendPacketBufferCb(clientId, std::move(packetBuffer));
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

void StreamingManager::registerClient(const std::string& clientId,
                                      bool reconnected,
                                      bool enablePacketBufferTimestamps,
                                      size_t packetStreamingReleaseThreshold,
                                      size_t cacheablePacketPayloadSizeMax)
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
    {
        packetStreamingServers.insert(
            {
                clientId,
                std::make_shared<packet_streaming::PacketStreamingServer>(
                    cacheablePacketPayloadSizeMax,
                    packetStreamingReleaseThreshold,
                    enablePacketBufferTimestamps)
            }
        );
    }
}

ListPtr<ISignal> StreamingManager::unregisterClient(const std::string& clientId)
{
    auto signalsToUnsubscribe = List<ISignal>();

    std::scoped_lock lock(sync);

    // find and remove registered client Id
    if (auto clientIter = streamingClientsIds.find(clientId); clientIter != streamingClientsIds.end())
    {
        streamingClientsIds.erase(clientId);
    }
    else
    {
        LOG_I("Client {} was not registered as streaming client", clientId);
        return List<ISignal>();
    }

    LOG_I("Streaming client with ID \"{}\" disconnected", clientId);

    // FIXME keep and reuse packet server when packet retransmission feature will be enabled
    if (auto it = packetStreamingServers.find(clientId); it != packetStreamingServers.end())
        packetStreamingServers.erase(it);

    // find and remove client Id from subscribers
    for (auto& [signalStringId, registeredSignal] : registeredSignals)
    {
        if (removeSignalSubscriberNoLock(signalStringId, clientId))
        {
            LOG_D("Signal: {} - is unsubscribed", signalStringId);
            signalsToUnsubscribe.pushBack(registeredSignal.daqSignal);
        }
    }

    return signalsToUnsubscribe;
}

bool StreamingManager::registerSignalSubscriber(const std::string& signalStringId,
                                                const std::string& subscribedClientId,
                                                const SendPacketBufferCallback& sendPacketBufferCb)
{
    bool doSignalSubscribe = false;

    std::scoped_lock lock(sync);

    if (auto iter = registeredSignals.find(signalStringId); iter != registeredSignals.end())
    {
        auto& registeredSignal = iter->second;
        auto& subscribers = registeredSignal.subscribedClientsIds;

        if (auto subscribersIter = subscribers.find(subscribedClientId); subscribersIter == subscribers.end())
        {
            if (subscribers.empty())
            {
                LOG_D("Signal: {} has first subscriber", signalStringId);
                doSignalSubscribe = true;
                // should trigger creating a reader,
                // which automatically generates first event packet that will initialize packet streaming
            }
            else
            {
                // does not trigger creating a reader
                // so create and send event packet to initialize packet streaming
                // descriptor params not assigned means initial event packet is not yet processed by streaming
                if (registeredSignal.lastDataDescriptorParam.assigned())
                {
                    sendDaqPacket(sendPacketBufferCb,
                                  packetStreamingServers.at(subscribedClientId),
                                  DataDescriptorChangedEventPacket(registeredSignal.lastDataDescriptorParam,
                                                                   registeredSignal.lastDomainDescriptorParam),
                                  subscribedClientId,
                                  registeredSignal.numericId);
                }
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
    std::scoped_lock lock(sync);

    return removeSignalSubscriberNoLock(signalStringId, subscribedClientId);
}

bool StreamingManager::removeSignalSubscriberNoLock(const std::string& signalStringId, const std::string& subscribedClientId)
{
    bool doSignalUnsubscribe = false;

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

void StreamingManager::pushToPacketStreamingServer(const PacketStreamingServerPtr& packetStreamingServer,
                                                   PacketPtr&& packet,
                                                   SignalNumericIdType singalNumericId)
{
    packetStreamingServer->addDaqPacket(singalNumericId, std::move(packet));
}

StreamingWriteTasks StreamingManager::getStreamingWriteTasks(const PacketStreamingServerPtr& packetStreamingServerPtr)
{
    std::optional<std::chrono::steady_clock::time_point> timeStamp(std::nullopt);
    auto nonCacheableBuffersCount = packetStreamingServerPtr->getNonCacheableBuffersCount();
    auto cacheableBuffersCount = packetStreamingServerPtr->getAvailableBuffersCount() - nonCacheableBuffersCount;

    size_t linearBufferCurPos = 0;
    std::shared_ptr<std::vector<char>> linearCacheBuffer;
    if (cacheableBuffersCount > 0)
    {
        linearCacheBuffer =
            std::make_shared<std::vector<char>>(TransportHeader::PACKED_HEADER_SIZE * cacheableBuffersCount +
                                                packetStreamingServerPtr->getSizeOfCacheableBuffers());
    }

    // header and payload separate write tasks for each non-mergeable buffer
    // plus one task for all mergeable buffers
    std::vector<daq::native_streaming::WriteTask> tasks;
    tasks.reserve(2 * nonCacheableBuffersCount + 1);

    while (auto packetBufferPtr = packetStreamingServerPtr->getNextPacketBuffer())
    {
        if (packetStreamingServerPtr->isCacheablePacketBuffer(packetBufferPtr) && linearCacheBuffer)
        {
            BaseSessionHandler::copyHeadersToBuffer(packetBufferPtr, linearCacheBuffer->data() + linearBufferCurPos);
            linearBufferCurPos += TransportHeader::PACKED_HEADER_SIZE + packetBufferPtr->packetHeader->size;

            if (packetBufferPtr->packetHeader->payloadSize > 0)
            {
                std::memcpy(linearCacheBuffer->data() + linearBufferCurPos,
                            packetBufferPtr->payload,
                            packetBufferPtr->packetHeader->payloadSize);
                linearBufferCurPos += packetBufferPtr->packetHeader->payloadSize;
            }
        }
        else
        {
            BaseSessionHandler::createAndPushPacketBufferTasks(std::move(packetBufferPtr), tasks);
        }

        if (!timeStamp.has_value() && packetBufferPtr->timeStamp.has_value())
            timeStamp = packetBufferPtr->timeStamp.value();
    }

    if (linearCacheBuffer)
    {
        WriteHandler linearBufferHandler = [linearCacheBuffer]() {};
        tasks.push_back(
            WriteTask(boost::asio::const_buffer(linearCacheBuffer->data(), linearCacheBuffer->size()),
                      linearBufferHandler)
            );
    }

    return {tasks, timeStamp};
}

StreamingManager::RegisteredSignal::RegisteredSignal(SignalPtr daqSignal, SignalNumericIdType numericId)
    : daqSignal(daqSignal)
    , numericId(numericId)
{}

END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_PROTOCOL
