#include <packet_streaming/packet_streaming_server.h>
#include <opendaq/event_packet_ids.h>
#include <opendaq/packet_destruct_callback_factory.h>
#include <opendaq/event_packet_utils.h>
#include <opendaq/data_descriptor_factory.h>
#include "opendaq/context_factory.h"
#include "opendaq/custom_log.h"

namespace daq::packet_streaming
{

PacketStreamingServer::PacketStreamingServer(size_t cacheablePacketPayloadSizeMax,
                                             size_t releaseThreshold,
                                             bool attachTimestampToPacketBuffer)
    : jsonSerializer(JsonSerializer())
    , countOfNonCacheableBuffers(0)
    , currentCacheablePacketGroupId(0)
    , packetCollection(std::make_shared<PacketCollection>())
    , releaseThreshold(releaseThreshold)
    , attachTimestampToPacketBuffer(attachTimestampToPacketBuffer)
    , cacheablePacketPayloadSizeMax(cacheablePacketPayloadSizeMax)
{
}

void PacketStreamingServer::addDaqPacket(const uint32_t signalId, const PacketPtr& packet)
{
    switch (packet.getType())
    {
        case daq::PacketType::Event:
            addEventPacket(signalId, packet);
            break;
        case daq::PacketType::Data:
            {
                DataPacketPtr dataPacket = packet;
                addDataPacket(signalId, dataPacket);
            }
            break;
        default:
            DAQ_THROW_EXCEPTION(NotSupportedException, "Packet type not supported");
    }

    checkAndSendReleasePacket(false);
}

void PacketStreamingServer::addDaqPacket(const uint32_t signalId, PacketPtr&& packet)
{
    switch (packet.getType())
    {
        case daq::PacketType::Event:
            addEventPacket(signalId, packet);
            break;
        case daq::PacketType::Data:
            addDataPacket(signalId, DataPacketPtr(std::move(packet)));
            break;
        default:
            DAQ_THROW_EXCEPTION(NotSupportedException, "Packet type not supported");
    }

    checkAndSendReleasePacket(false);
}

PacketBufferPtr PacketStreamingServer::getNextPacketBuffer()
{
    if (!queue.empty())
    {
        auto packetBuffer = queue.front();
        queue.pop();

        if (packetBuffer->isCacheable())
        {
            auto it = cacheableBuffersGroups.find(packetBuffer->cacheableGroupId);
            if (it == cacheableBuffersGroups.end())
                linearCachingAssertion("it == cacheableBuffersGroups.end()", packetBuffer);

            auto& cacheableBuffersGroup = it->second;

            if(cacheableBuffersGroup.countOfPacketBuffers == 0)
                linearCachingAssertion("cacheableBuffersGroup.countOfPacketBuffers == 0", packetBuffer);

            --cacheableBuffersGroup.countOfPacketBuffers;

            if (cacheableBuffersGroup.sizeOfPacketBuffers < (packetBuffer->packetHeader->size + packetBuffer->packetHeader->payloadSize))
                linearCachingAssertion("cacheableBuffersGroup.sizeOfPacketBuffers < packetBufferSize", packetBuffer);

            cacheableBuffersGroup.sizeOfPacketBuffers -= packetBuffer->packetHeader->size + packetBuffer->packetHeader->payloadSize;

            if (cacheableBuffersGroup.countOfPacketBuffers == 0 || cacheableBuffersGroup.sizeOfPacketBuffers == 0)
            {
                if (!(cacheableBuffersGroup.countOfPacketBuffers == 0 && cacheableBuffersGroup.sizeOfPacketBuffers == 0))
                    linearCachingAssertion(
                        "!(cacheableBuffersGroup.countOfPacketBuffers == 0 && cacheableBuffersGroup.sizeOfPacketBuffers == 0)",
                        packetBuffer);

                cacheableBuffersGroups.erase(it); // ! cacheableBuffersGroup is dangling reference
            }
        }
        else
        {
            if(countOfNonCacheableBuffers == 0)
                linearCachingAssertion("countOfNonCacheableBuffers == 0", packetBuffer);
            --countOfNonCacheableBuffers;
        }
        return packetBuffer;
    }

    return nullptr;
}

PacketBufferPtr PacketStreamingServer::peekNextPacketBuffer()
{
    if (!queue.empty())
        return queue.front();
    return nullptr;
}

size_t PacketStreamingServer::getAvailableBuffersCount() const
{
    return queue.size();
}

size_t PacketStreamingServer::getNonCacheableBuffersCount() const
{
    return countOfNonCacheableBuffers;
}

size_t PacketStreamingServer::getSizeOfCacheableBuffers(size_t cacheableGroupId) const
{
    if (const auto it = cacheableBuffersGroups.find(cacheableGroupId); it != cacheableBuffersGroups.end())
        return it->second.sizeOfPacketBuffers;
    return 0;
}

size_t PacketStreamingServer::getCountOfCacheableBuffers(size_t cacheableGroupId) const
{
    if (const auto it = cacheableBuffersGroups.find(cacheableGroupId); it != cacheableBuffersGroups.end())
        return it->second.countOfPacketBuffers;
    return 0;
}

size_t PacketStreamingServer::getCountOfCacheableGroups() const
{
    return cacheableBuffersGroups.size();
}

void PacketStreamingServer::addEventPacket(const uint32_t signalId, const EventPacketPtr& packet)
{
    const auto packetHeader = new GenericPacketHeader();
    packetHeader->size = sizeof(GenericPacketHeader);
    packetHeader->type = PacketType::event;
    packetHeader->version = 0;
    packetHeader->flags = 0;
    packetHeader->signalId = signalId;

    jsonSerializer.reset();
    packet.serialize(jsonSerializer);
    auto serializedPacket = jsonSerializer.getOutput();

    packetHeader->payloadSize = static_cast<uint32_t>(serializedPacket.getLength() + 1);

    const auto packetBuffer = std::make_shared<PacketBuffer>(
            packetHeader,
            reinterpret_cast<const void*>(serializedPacket.getCharPtr()),
            [packetHeader, serializedPacket]() mutable {
                delete packetHeader;
                serializedPacket.release();
            },
            attachTimestampToPacketBuffer,
            getPacketCacheableGroupId(packetHeader->size, packetHeader->payloadSize)
        );

    if (packet.getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED)
    {
        bool valueDescriptorChanged;
        DataDescriptorPtr newValueDescriptor;
        std::tie(valueDescriptorChanged, std::ignore, newValueDescriptor, std::ignore) =
            parseDataDescriptorEventPacket(packet);

        if (valueDescriptorChanged)
            dataDescriptors.insert_or_assign(signalId, newValueDescriptor);
    }

    queuePacketBuffer(packetBuffer);
}

template <bool CheckRefCount>
bool PacketStreamingServer::canReleasePacket(const DataPacketPtr& packet)
{
    if constexpr (CheckRefCount)
    {
        SizeT refCount;
        packet->getRefCount(&refCount);
        if (refCount == 1)
            return true;
    }

    return false;
}

bool PacketStreamingServer::shouldSendPacket(const DataPacketPtr& packet, Int packetId, bool markForRelease) const
{
    bool packetAlreadySent = false;
    {
        std::scoped_lock lock(packetCollection->sync);
        const auto packetIdSentIt = packetCollection->sent.find(packetId);
        if (packetIdSentIt != packetCollection->sent.end())
        {
            packetAlreadySent = true;
            if (markForRelease)
                packetCollection->sent.erase(packetIdSentIt);
        }
        else if (!markForRelease)
            packetCollection->sent.insert(packetId);
    }

    if (!markForRelease && !packetAlreadySent)
        packet.subscribeForDestructNotification(PacketDestructCallback(
            [packetCollection = packetCollection, packetId]
            {
                std::scoped_lock lock(packetCollection->sync);
                const auto erasedCount = packetCollection->sent.erase(packetId);
                if (erasedCount > 0)
                    packetCollection->readyForRelease.push_back(packetId);
            }));

    return !packetAlreadySent;
}

void PacketStreamingServer::setOffset(const DataPacketPtr& packet, DataPacketHeader* packetHeader)
{
    const auto offset = packet.getOffset();
    if (offset.assigned())
    {
        switch (offset.getCoreType())
        {
            case ctInt:
                packetHeader->packetOffsetInt64 = offset;
                packetHeader->genericHeader.flags += PACKET_OFFSET_TYPE_INT << PACKET_FLAG_OFFSET_TYPE_SHIFT;
                break;
            case ctFloat:
                packetHeader->packetOffsetFloat64 = offset;
                packetHeader->genericHeader.flags += PACKET_OFFSET_TYPE_FLOAT << PACKET_FLAG_OFFSET_TYPE_SHIFT;
                break;
            default:
                throw PacketStreamingException("Offset type not supported");
        }
    }
}

Int PacketStreamingServer::getDomainPacketId(const DataPacketPtr& packet)
{
    const auto domainPacket = packet.getDomainPacket();
    if (domainPacket.assigned())
        return domainPacket.getPacketId();
    return -1;
}

void PacketStreamingServer::queuePacketBuffer(const PacketBufferPtr& packetBuffer)
{
    if (packetBuffer->isCacheable())
    {
        const auto [it,_] = cacheableBuffersGroups.emplace(packetBuffer->cacheableGroupId, CacheableBuffersGroup{});
        it->second.countOfPacketBuffers++;
        it->second.sizeOfPacketBuffers += packetBuffer->packetHeader->size + packetBuffer->packetHeader->payloadSize;
    }
    else
    {
        ++countOfNonCacheableBuffers;
    }
    queue.push(packetBuffer);
}

size_t PacketStreamingServer::getPacketCacheableGroupId(size_t headerSize, size_t payloadSize)
{
    if (payloadSize <= cacheablePacketPayloadSizeMax)
    {
        if (queue.empty())
        {
            if(!cacheableBuffersGroups.empty())
                linearCachingAssertion("!cacheableBuffersGroups.empty()", nullptr);
            currentCacheablePacketGroupId = 1;
        }

        // begin a new group when queuing a cacheable buffer after a non-cacheable one
        if (!queue.empty() && !queue.back()->isCacheable())
            ++currentCacheablePacketGroupId;

        if (currentCacheablePacketGroupId == PacketBuffer::NON_CACHEABLE_GROUP_ID)
            ++currentCacheablePacketGroupId;

        return currentCacheablePacketGroupId;
    }
    return PacketBuffer::NON_CACHEABLE_GROUP_ID;
}

void PacketStreamingServer::linearCachingAssertion(const std::string& condition, const PacketBufferPtr& packetBuffer)
{
    std::string debugInfoString =
        fmt::format("\nnon-cacheable buffers count {}, "
                    "cacheable groups count {}, "
                    "available buffers count {}",
                    countOfNonCacheableBuffers,
                    cacheableBuffersGroups.size(),
                    queue.size());

    if (packetBuffer)
    {
        debugInfoString +=
            fmt::format("\ncacheable group: "
                        "ID {}, "
                        "count of buffers {}, "
                        "size of buffers {};\n"
                        "packetBuffer: "
                        "type {}, "
                        "header size {}, "
                        "payload size {}",
                        packetBuffer->cacheableGroupId,
                        getCountOfCacheableBuffers(packetBuffer->cacheableGroupId),
                        getSizeOfCacheableBuffers(packetBuffer->cacheableGroupId),
                        packetBuffer->getTypeString(),
                        packetBuffer->packetHeader->size,
                        packetBuffer->packetHeader->payloadSize);
    }
    throw PacketStreamingException(fmt::format(R"(Linear caching failure: {};{})", condition, debugInfoString));
}

template <class DataPacket>
void PacketStreamingServer::addDataPacket(const uint32_t signalId, DataPacket&& packet)
{
    if (dataDescriptors.find(signalId) == dataDescriptors.end())
        throw PacketStreamingException("No signal descriptor event received");

    constexpr bool isPacketRValue = std::is_rvalue_reference_v<DataPacket&&>;
    const bool markPacketForRelease = canReleasePacket<isPacketRValue>(packet);

    auto packetId = packet.getPacketId();
    auto domainPacketId = getDomainPacketId(packet);

    auto doSendPacket = shouldSendPacket(packet, packetId, markPacketForRelease);
    if (!doSendPacket)
    {
        addAlreadySentPacket(signalId, packetId, domainPacketId, markPacketForRelease);
        return;
    }

    const auto packetHeader = static_cast<DataPacketHeader*>(std::malloc(sizeof(DataPacketHeader)));
    packetHeader->genericHeader.size = sizeof(DataPacketHeader);
    packetHeader->genericHeader.type = PacketType::data;
    packetHeader->genericHeader.version = 0;
    packetHeader->genericHeader.flags = markPacketForRelease ? PACKET_FLAG_CAN_RELEASE : 0;
    packetHeader->genericHeader.signalId = signalId;
    packetHeader->packetId = packetId;
    packetHeader->domainPacketId = domainPacketId;
    packetHeader->sampleCount = static_cast<Int>(packet.getSampleCount());

    setOffset(packet, packetHeader);

    const auto packetDataPtr = packet.getRawData();
    const auto packetDataSize = packetDataPtr != nullptr ? packet.getRawDataSize() : 0;
    packetHeader->genericHeader.payloadSize = static_cast<uint32_t>(packetDataSize);

    const auto packetBuffer = std::make_shared<PacketBuffer>(
        reinterpret_cast<GenericPacketHeader*>(packetHeader),
        packetDataPtr,
        [packetHeader, packet = packet]() mutable
        {
            std::free(packetHeader);
            packet.release();
        },
        attachTimestampToPacketBuffer,
        getPacketCacheableGroupId(packetHeader->genericHeader.size, packetHeader->genericHeader.payloadSize)
    );

    if constexpr (isPacketRValue)
        packet.release();

    queuePacketBuffer(packetBuffer);
}

void PacketStreamingServer::checkAndSendReleasePacket(bool force)
{
    Int* packetIds;
    size_t packetsReadyForRelease;
    {
        std::scoped_lock lock(packetCollection->sync);
        packetsReadyForRelease = packetCollection->readyForRelease.size();
        if (!(force && packetsReadyForRelease > 0) && (packetsReadyForRelease < releaseThreshold))
            return;

        packetIds = new Int[packetsReadyForRelease];
        std::memcpy(packetIds, packetCollection->readyForRelease.data(), packetsReadyForRelease * sizeof(Int));
        packetCollection->readyForRelease.clear();
    }

    const auto packetHeader = new GenericPacketHeader();
    packetHeader->size = sizeof(GenericPacketHeader);
    packetHeader->type = PacketType::release;
    packetHeader->version = 0;
    packetHeader->flags = 0;
    packetHeader->signalId = std::numeric_limits<uint32_t>::max();
    packetHeader->payloadSize = static_cast<uint32_t>(packetsReadyForRelease * sizeof(Int));

    const auto packetBuffer =
        std::make_shared<PacketBuffer>(
            packetHeader,
            reinterpret_cast<const void*>(packetIds),
            [packetHeader, packetIds]
            {
                delete packetHeader;
                delete[] packetIds;
            },
            attachTimestampToPacketBuffer,
            getPacketCacheableGroupId(packetHeader->size, packetHeader->payloadSize)
        );

    queuePacketBuffer(packetBuffer);
}

void PacketStreamingServer::addAlreadySentPacket(uint32_t signalId, Int packetId, Int domainPacketId, bool markForRelease)
{
    const auto packetHeader = static_cast<AlreadySentPacketHeader*>(std::malloc(sizeof(AlreadySentPacketHeader)));
    packetHeader->genericHeader.size = sizeof(AlreadySentPacketHeader);
    packetHeader->genericHeader.type = PacketType::alreadySent;
    packetHeader->genericHeader.version = 0;
    packetHeader->genericHeader.flags = markForRelease ? PACKET_FLAG_CAN_RELEASE : 0;
    packetHeader->genericHeader.signalId = signalId;
    packetHeader->genericHeader.payloadSize = 0;
    packetHeader->packetId = packetId;
    packetHeader->domainPacketId = domainPacketId;

    const auto packetBuffer =
        std::make_shared<PacketBuffer>(
            reinterpret_cast<GenericPacketHeader*>(packetHeader), nullptr,
            [packetHeader]
            {
                std::free(packetHeader);
            },
            attachTimestampToPacketBuffer,
            getPacketCacheableGroupId(packetHeader->genericHeader.size, packetHeader->genericHeader.payloadSize)
        );

    queuePacketBuffer(packetBuffer);
}

}
