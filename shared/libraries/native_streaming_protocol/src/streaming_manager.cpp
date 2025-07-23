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
        DAQ_THROW_EXCEPTION(ArgumentNullException, "Logger must not be null");
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

void StreamingManager::processPackets(const tsl::ordered_map<std::string, PacketBufferData>& packetIndices,
                                      const std::vector<IPacket*>& packets)
{
    std::scoped_lock lock(sync);

    for (auto& [signalStringId, packetData] : packetIndices)
    {
        if (auto it1 = registeredSignals.find(signalStringId); it1 != registeredSignals.end())
        {
            auto& registeredSignal = it1->second;

            for (int i = packetData.index; i < packetData.index + packetData.count; ++i)
            {
                auto packet = PacketPtr::Adopt(packets[i]);
                
                if (packet.getType() == PacketType::Event)
                {
                    const auto eventPacket = packet.asPtr<IEventPacket>(true);
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

                if (auto it2 = registeredSignal.subscribedClientsIds.begin(); it2 != registeredSignal.subscribedClientsIds.end())
                {
                    while (std::next(it2) != registeredSignal.subscribedClientsIds.end())
                    {
                        packetStreamingServers.at(*it2)->addDaqPacket(registeredSignal.numericId, packet);
                        ++it2;
                    }
        
                    pushToPacketStreamingServer(packetStreamingServers.at(*it2), std::move(packet), registeredSignal.numericId);
                }
            }
        }
        else
        {
            throw NativeStreamingProtocolException(fmt::format("Signal {} is not registered in streaming", signalStringId));
        }
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

void StreamingManager::linearCachingAssertion(const std::string& condition,
                                              const PacketStreamingServerPtr& packetStreamingServerPtr,
                                              const packet_streaming::PacketBufferPtr& packetBuffer)
{
    std::string debugInfoString =
        fmt::format("\nPacketStreamingServer: "
                    "available buffers count {}, "
                    "cacheable groups count {}, "
                    "non-cacheable buffers count {};\n",
                    packetStreamingServerPtr->getAvailableBuffersCount(),
                    packetStreamingServerPtr->getCountOfCacheableGroups(),
                    packetStreamingServerPtr->getNonCacheableBuffersCount());

    if (packetBuffer)
    {
        debugInfoString +=
            fmt::format("cacheable group: "
                        "ID {}, "
                        "count of buffers {}, "
                        "size of buffers {};\n"
                        "packetBuffer: "
                        "type {}, "
                        "header size {}, "
                        "payload size {}",
                        packetBuffer->cacheableGroupId,
                        packetStreamingServerPtr->getCountOfCacheableBuffers(packetBuffer->cacheableGroupId),
                        packetStreamingServerPtr->getSizeOfCacheableBuffers(packetBuffer->cacheableGroupId),
                        packetBuffer->getTypeString(),
                        packetBuffer->packetHeader->size,
                        packetBuffer->packetHeader->payloadSize);
    }

    throw NativeStreamingProtocolException(fmt::format(R"(Linear caching failure: {};{})", condition, debugInfoString));
}

WriteTask StreamingManager::cachePacketsToLinearBuffer(const PacketStreamingServerPtr& packetStreamingServerPtr,
                                                       size_t cacheableGroupId,
                                                       std::optional<std::chrono::steady_clock::time_point>& timeStamp)
{
    size_t countOfCacheableBuffer = packetStreamingServerPtr->getCountOfCacheableBuffers(cacheableGroupId);
    size_t sizeOfCacheableBuffers = packetStreamingServerPtr->getSizeOfCacheableBuffers(cacheableGroupId);
    size_t linearBufferCurPos = 0;

    if (packetStreamingServerPtr->getAvailableBuffersCount() < countOfCacheableBuffer)
        linearCachingAssertion("buffersAvailable < countOfCacheableBuffer", packetStreamingServerPtr, nullptr);

    const size_t linearCacheBufferSize =
        TransportHeader::PACKED_HEADER_SIZE * countOfCacheableBuffer + sizeOfCacheableBuffers;
    auto linearCacheBuffer = std::make_shared<std::vector<char>>(linearCacheBufferSize);

    for(size_t i = 0; i < countOfCacheableBuffer; ++i)
    {
        auto packetBufferPtr = packetStreamingServerPtr->getNextPacketBuffer();

        if (packetBufferPtr == nullptr)
            linearCachingAssertion("packetBufferPtr == nullptr", packetStreamingServerPtr, nullptr);
        if (cacheableGroupId != packetBufferPtr->cacheableGroupId)
            linearCachingAssertion("cacheableGroupId != packetBufferPtr->cacheableGroupId", packetStreamingServerPtr, packetBufferPtr);
        if (linearCacheBufferSize < (linearBufferCurPos +
                                     TransportHeader::PACKED_HEADER_SIZE +
                                     packetBufferPtr->packetHeader->size +
                                     packetBufferPtr->packetHeader->payloadSize))
        {
            linearCachingAssertion("linearCacheBufferSize < linearBufferCurPos + packetBufferSize", packetStreamingServerPtr, packetBufferPtr);
        }

        BaseSessionHandler::copyHeadersToBuffer(packetBufferPtr, linearCacheBuffer->data() + linearBufferCurPos);
        linearBufferCurPos += TransportHeader::PACKED_HEADER_SIZE + packetBufferPtr->packetHeader->size;
        if (packetBufferPtr->packetHeader->payloadSize > 0)
        {
            std::memcpy(linearCacheBuffer->data() + linearBufferCurPos,
                        packetBufferPtr->payload,
                        packetBufferPtr->packetHeader->payloadSize);
        }
        linearBufferCurPos += packetBufferPtr->packetHeader->payloadSize;

        if (!timeStamp.has_value() && packetBufferPtr->timeStamp.has_value())
            timeStamp = packetBufferPtr->timeStamp.value();
    }

    if (linearBufferCurPos != linearCacheBufferSize)
        linearCachingAssertion("linearBufferCurPos != linearCacheBufferSize", packetStreamingServerPtr, nullptr);

    WriteHandler linearBufferHandler = [linearCacheBuffer = linearCacheBuffer]() {};
    return WriteTask(boost::asio::const_buffer(linearCacheBuffer->data(), linearCacheBuffer->size()), linearBufferHandler);
}

SignalNumericIdType StreamingManager::registerSignal(const SignalPtr& signal)
{
    auto signalStringId = signal.getGlobalId().toStdString();

    std::scoped_lock lock(sync);

    if (auto iter = registeredSignals.find(signalStringId); iter == registeredSignals.end())
    {
        auto signalNumericId = ++signalNumericIdCounter;
        registeredSignals.insert({signalStringId, RegisteredServerSignal(signal, signalNumericId)});
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

    // remove cached packet server & client if client is not auto-reconnected
    if (auto it = packetStreamingServers.find(clientId); it != packetStreamingServers.end() && !reconnected)
        packetStreamingServers.erase(it);
    if (auto it = packetStreamingClients.find(clientId); it != packetStreamingClients.end() && !reconnected)
        packetStreamingClients.erase(it);

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

    // create new associated packet client if required
    if (auto it = packetStreamingClients.find(clientId); it == packetStreamingClients.end())
    {
        packetStreamingClients.insert(
            {
                clientId,
                std::make_shared<packet_streaming::PacketStreamingClient>()
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
        LOG_I("Streaming client with ID \"{}\" disconnected", clientId);
        streamingClientsIds.erase(clientId);
    }
    else
    {
        LOG_I("Client {} was not registered as streaming client", clientId);
        return List<ISignal>();
    }

    // FIXME keep and reuse packet server when packet retransmission feature will be enabled
    if (auto it = packetStreamingServers.find(clientId); it != packetStreamingServers.end())
        packetStreamingServers.erase(it);

    if (auto it = packetStreamingClients.find(clientId); it != packetStreamingClients.end())
        packetStreamingClients.erase(it);

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
    std::vector<daq::native_streaming::WriteTask> tasks;
    std::optional<std::chrono::steady_clock::time_point> timeStamp(std::nullopt);

    const auto nonCacheableBuffersCount = packetStreamingServerPtr->getNonCacheableBuffersCount();
    const auto cacheableGroupsCount = packetStreamingServerPtr->getCountOfCacheableGroups();
    // header and payload separate write tasks for each non-cacheable buffer
    // plus one task for each group of cacheable buffers
    tasks.reserve(2 * nonCacheableBuffersCount + cacheableGroupsCount);

    while (auto packetBufferPtr = packetStreamingServerPtr->peekNextPacketBuffer())
    {
        if (packetBufferPtr->isCacheable())
        {
            tasks.push_back(cachePacketsToLinearBuffer(packetStreamingServerPtr, packetBufferPtr->cacheableGroupId, timeStamp));
        }
        else
        {
            if (!timeStamp.has_value() && packetBufferPtr->timeStamp.has_value())
                timeStamp = packetBufferPtr->timeStamp.value();
            BaseSessionHandler::createAndPushPacketBufferTasks(packetStreamingServerPtr->getNextPacketBuffer(), tasks);
        }
    }

    return {tasks, timeStamp};
}

void StreamingManager::registerClientSignal(const SignalNumericIdType& signalNumericId,
                                            const StringPtr& signalStringId,
                                            const std::string& clientId)
{
    std::scoped_lock lock(sync);

    if (const auto it = registeredClientSignals.find(signalStringId); it == registeredClientSignals.end())
    {
        registeredClientSignals.insert({signalStringId, RegisteredClientSignal(signalStringId, signalNumericId, clientId)});
    }
    else
    {
        LOG_E("Client signal with string Id {} is already registered by server; client ids: registered {}, new {}",
              signalStringId,
              it->second.clientId,
              clientId);
        return;
    }
}

void StreamingManager::unregisterClientSignal(const SignalNumericIdType& signalNumericId,
                                              const StringPtr& signalStringId,
                                              const std::string& clientId)
{
    std::scoped_lock lock(sync);

    registeredClientSignals.erase(signalStringId);
}

void StreamingManager::unregisterClientSignals(const std::string& clientId, SignalAvailableCallback cb)
{
    std::scoped_lock lock(sync);

    auto clientIdMatch = [&clientId](const std::pair<std::string, RegisteredClientSignal>& pair) {
        return pair.second.clientId == clientId;
    };
    for (auto clientSignalIter = std::find_if(registeredClientSignals.begin(), registeredClientSignals.end(), clientIdMatch);
         clientSignalIter != registeredClientSignals.end();
         clientSignalIter = std::find_if(clientSignalIter, registeredClientSignals.end(), clientIdMatch))
    {
        cb(clientSignalIter->second.signalStringId);
        clientSignalIter = registeredClientSignals.erase(clientSignalIter);
    }
}

void StreamingManager::handleClientSignalSubscribeAck(const SignalNumericIdType& signalNumericId,
                                                      bool subscribed,
                                                      const std::string& clientId,
                                                      SubscribeAckCallback cb)
{
    std::scoped_lock lock(sync);

    auto clientAndNumericIdMatch = [&clientId, &signalNumericId](const std::pair<std::string, RegisteredClientSignal>& pair) {
        return pair.second.clientId == clientId && pair.second.signalNumericId == signalNumericId;
    };

    if (auto clientSignalIter = std::find_if(registeredClientSignals.begin(), registeredClientSignals.end(), clientAndNumericIdMatch);
         clientSignalIter != registeredClientSignals.end())
    {
        cb(clientSignalIter->second.signalStringId, subscribed);
    }
}

void StreamingManager::doClientSignalSubscription(const std::string& signalStringId, DoSubscribeCallback cb)
{
    std::scoped_lock lock(sync);

    if (const auto clientSignalIter = registeredClientSignals.find(signalStringId); clientSignalIter != registeredClientSignals.end())
    {
        cb(clientSignalIter->second.signalNumericId, clientSignalIter->second.clientId);
    }
}

void StreamingManager::handleReceivedPacketBuffer(const packet_streaming::PacketBufferPtr& packetBuffer,
                                                  const std::string& clientId,
                                                  OnPacketCallback cb)
{
    std::scoped_lock lock(sync);

    PacketStreamingClientPtr packetStreamingClientPtr;
    if (const auto it = packetStreamingClients.find(clientId); it != packetStreamingClients.end())
        packetStreamingClientPtr = it->second;
    else
        return;

    packetStreamingClientPtr->addPacketBuffer(packetBuffer);
    auto [signalNumericId, packet] = packetStreamingClientPtr->getNextDaqPacket();
    while (packet.assigned())
    {
        auto clientAndNumericIdMatch = [&clientId, signalNumericId = signalNumericId](const std::pair<std::string, RegisteredClientSignal>& pair) {
            return pair.second.clientId == clientId && pair.second.signalNumericId == signalNumericId;
        };

        if (auto clientSignalIter = std::find_if(registeredClientSignals.begin(), registeredClientSignals.end(), clientAndNumericIdMatch);
            clientSignalIter != registeredClientSignals.end())
        {
            cb(clientSignalIter->second.signalStringId, packet);
        }
        std::tie(signalNumericId, packet) = packetStreamingClientPtr->getNextDaqPacket();
    }
}

StreamingManager::RegisteredServerSignal::RegisteredServerSignal(SignalPtr daqSignal, SignalNumericIdType numericId)
    : daqSignal(daqSignal)
    , numericId(numericId)
{}

StreamingManager::RegisteredClientSignal::RegisteredClientSignal(const std::string& signalStringId,
                                                                 SignalNumericIdType signalNumericId,
                                                                 const std::string& clientId)
    : signalStringId(signalStringId)
    , signalNumericId(signalNumericId)
    , clientId(clientId)
{}

END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_PROTOCOL
