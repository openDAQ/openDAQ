#include <packet_streaming/packet_streaming_client.h>
#include <opendaq/event_packet_ids.h>
#include <opendaq/event_packet_utils.h>
#include <opendaq/packet_factory.h>
#include <opendaq/deleter_factory.h>
#include <opendaq/data_descriptor_factory.h>

namespace daq::packet_streaming
{

PacketStreamingClient::PacketStreamingClient()
    : jsonDeserializer(JsonDeserializer())
{
}

void PacketStreamingClient::addPacketBuffer(const PacketBufferPtr& packetBuffer)
{
    switch (packetBuffer->packetHeader->type)
    {
        case PacketType::event:
            addEventPacketBuffer(packetBuffer);
            break;
        case PacketType::data:
            addDataPacketBuffer(packetBuffer, nullptr);
            break;
        case PacketType::release:
            addReleasePacketBuffer(packetBuffer);
            break;
        case PacketType::alreadySent:
            addAlreadySentPacketBuffer(packetBuffer);
            break;
    }
}

std::tuple<uint32_t, PacketPtr> PacketStreamingClient::getNextDaqPacket()
{
    if (queue.empty())
        return {std::numeric_limits<uint32_t>::max(), nullptr};

    auto result = queue.front();
    queue.pop();
    return result;
}

bool PacketStreamingClient::areReferencesCleared() const
{
    return (referencedPacketBuffers.empty() && referencedPackets.empty() && packetBuffersWaitingForDomainPackets.empty());
}

void PacketStreamingClient::addEventPacketBuffer(const PacketBufferPtr& packetBuffer)
{
    bool forwardPacket = false;
    auto signalId = packetBuffer->packetHeader->signalId;

    const auto eventPayloadString = String((ConstCharPtr) packetBuffer->payload);

    EventPacketPtr packet = jsonDeserializer.deserialize(eventPayloadString);

    if (packet.getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED)
    {
        const auto [valueDescriptorChanged, domainDescriptorChanged, newValueDescriptor, newDomainDescriptors] =
            parseDataDescriptorEventPacket(packet);

        std::scoped_lock lock(descriptorsSync);

        const auto dataDescIt = dataDescriptors.find(signalId);
        const auto domainDescIt = domainDescriptors.find(signalId);
        if (valueDescriptorChanged && dataDescIt != dataDescriptors.end() && dataDescIt->second != newValueDescriptor ||
            domainDescriptorChanged && domainDescIt != domainDescriptors.end() && domainDescIt->second != newDomainDescriptors ||
            dataDescIt == dataDescriptors.end() ||
            domainDescIt == domainDescriptors.end())
        {
            forwardPacket = true;
        }

        if (valueDescriptorChanged)
            dataDescriptors.insert_or_assign(signalId, newValueDescriptor);
        if (domainDescriptorChanged)
            domainDescriptors.insert_or_assign(signalId, newDomainDescriptors);
    }

    if (forwardPacket)
    {
        queue.push({signalId, packet});
    }
}

DataPacketPtr PacketStreamingClient::addDataPacketBuffer(const PacketBufferPtr& packetBuffer, const DataPacketPtr& domainPacket)
{
    const auto dataPacketHeader = reinterpret_cast<DataPacketHeader*>(packetBuffer->packetHeader);
    auto domainPacketId = dataPacketHeader->domainPacketId;
    auto domPacket = domainPacket;

    if (!domPacket.assigned() && domainPacketId >= 0)
    {
        // check if domain packet already arrived
        const auto domainPacketIt = referencedPackets.find(domainPacketId);
        if (domainPacketIt != referencedPackets.end())
        {
            // domain packet arrived before this packet, should be in the referenced packets
            domPacket = domainPacketIt->second;
        }
        else
        {
            // domain packet did not arrive yet, do not process this packet now, will do it later when the domain packet arrives
            const auto it = packetBuffersWaitingForDomainPackets.find(domainPacketId);
            if (it == packetBuffersWaitingForDomainPackets.end())
                packetBuffersWaitingForDomainPackets.insert({domainPacketId, {packetBuffer}});
            else
                it->second.push_back(packetBuffer);
            referencedPacketBuffers.insert({dataPacketHeader->packetId, packetBuffer});
            return nullptr;
        }
    }

    auto signalId = dataPacketHeader->genericHeader.signalId;

    const auto sigIt = dataDescriptors.find(signalId);
    if (sigIt == dataDescriptors.end())
        throw PacketStreamingException("Descriptor not registered");

    const auto valueDescriptor = sigIt->second;

    NumberPtr offset;
    const auto packetOffsetType = (dataPacketHeader->genericHeader.flags & PACKET_FLAG_OFFSET_TYPE_MASK) >> PACKET_FLAG_OFFSET_TYPE_SHIFT;
    switch (packetOffsetType)
    {
        case PACKET_OFFSET_TYPE_INT:
            offset = dataPacketHeader->packetOffsetInt64;
            break;
        case PACKET_OFFSET_TYPE_FLOAT:
            offset = dataPacketHeader->packetOffsetFloat64;
            break;
    }

    DataPacketPtr packet;

    if (packetBuffer->payload == nullptr)
    {
        packet = DataPacketWithDomain(domPacket, valueDescriptor, dataPacketHeader->sampleCount, offset);
        assert(packet.getRawData() == nullptr);
    }
    else
    {
        packet = DataPacketWithExternalMemory(domPacket,
                                              valueDescriptor,
                                              dataPacketHeader->sampleCount,
                                              const_cast<void*>(packetBuffer->payload),
                                              Deleter([packetBuffer = packetBuffer](void*) mutable { packetBuffer.reset(); }),
                                              offset,
                                              dataPacketHeader->genericHeader.payloadSize);
    }

    queue.push({signalId, packet});

    // check if this is domain packet that other value packets have been waiting for
    const auto packetsWaitingIt = packetBuffersWaitingForDomainPackets.find(dataPacketHeader->packetId);
    if (packetsWaitingIt != packetBuffersWaitingForDomainPackets.end())
    {
        // iterate through all the packet buffers that are waiting for this domain packet and process those value packets
        for (const auto& pktBuffer : packetsWaitingIt->second)
        {
            const auto pktHeader = reinterpret_cast<DataPacketHeader*>(pktBuffer->packetHeader);
            referencedPacketBuffers.erase(pktHeader->packetId);

            auto dataPacket = addDataPacketBuffer(pktBuffer, packet);
            for (const auto sigId: pktBuffer->additionalSignalIds)
                queue.push({sigId, dataPacket});
        }
        packetBuffersWaitingForDomainPackets.erase(packetsWaitingIt);
    }

    // if the domain packet arrives first or this is shared domain packet, it should have the CAN_RELEASE flag OFF,
    // so keep it in the referenced packets list
    if (!(dataPacketHeader->genericHeader.flags & PACKET_FLAG_CAN_RELEASE))
        referencedPackets.insert({dataPacketHeader->packetId, packet});

    return packet;
}

void PacketStreamingClient::addReleasePacketBuffer(const PacketBufferPtr& packetBuffer)
{
    auto packetIds = static_cast<const Int*>(packetBuffer->payload);
    const auto packetCount = static_cast<size_t>(packetBuffer->packetHeader->payloadSize) / sizeof(Int);

    for (size_t i = 0; i < packetCount; i++)
    {
        const auto packetId = *packetIds++;

        const auto packetIt = referencedPackets.find(packetId);
        if (packetIt != referencedPackets.end())
            referencedPackets.erase(packetIt);
        else
        {
            const auto packetBufferIt = referencedPacketBuffers.find(packetId);
            if (packetBufferIt != referencedPacketBuffers.end())
            {
                const auto dataPacketHeader = reinterpret_cast<DataPacketHeader*>(packetBufferIt->second->packetHeader);
                dataPacketHeader->genericHeader.flags += PACKET_FLAG_CAN_RELEASE;
            }
            else
            {
                throw PacketStreamingException("Packet not found");
            }
        }
    }
}

void PacketStreamingClient::addAlreadySentPacketBuffer(const PacketBufferPtr& packetBuffer)
{
    const auto alreadySentPacketHeader = reinterpret_cast<AlreadySentPacketHeader*>(packetBuffer->packetHeader);
    auto signalId = alreadySentPacketHeader->genericHeader.signalId;
    auto packetId = alreadySentPacketHeader->packetId;
    const auto domainPacketId = alreadySentPacketHeader->domainPacketId;

    const auto sigIt = dataDescriptors.find(signalId);
    if (sigIt == dataDescriptors.end())
        throw PacketStreamingException("Descriptor not registered");

    const auto packetIt = referencedPackets.find(packetId);
    if (packetIt == referencedPackets.end())
    {
        const auto packetBuffersIt = packetBuffersWaitingForDomainPackets.find(domainPacketId);
        if (packetBuffersIt == packetBuffersWaitingForDomainPackets.end())
            throw PacketStreamingException("Packet not found");

        const auto packetBufferIt = std::find_if(packetBuffersIt->second.begin(),
                                        packetBuffersIt->second.end(),
                                        [packetId](const PacketBufferPtr& packetBuffer) {
                                            const auto otherPacketId = reinterpret_cast<DataPacketHeader*>(packetBuffer->packetHeader)->packetId;
                                            return (packetId == otherPacketId);
                                        });
        if (packetBufferIt == packetBuffersIt->second.end())
            throw PacketStreamingException("Packet not found");

        const auto& packetPtr = *packetBufferIt;
        packetPtr->additionalSignalIds.push_back(signalId);
        return;
    }

    queue.push({signalId, packetIt->second});

    if (alreadySentPacketHeader->genericHeader.flags & PACKET_FLAG_CAN_RELEASE)
        referencedPackets.erase(packetIt);
}

}
