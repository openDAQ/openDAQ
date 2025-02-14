/*
 * Copyright 2022-2024 openDAQ d.o.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <packet_streaming/packet_streaming.h>
#include <opendaq/data_packet_ptr.h>
#include <opendaq/event_packet_ptr.h>
#include <queue>

namespace daq::packet_streaming
{

struct PacketCollection
{
    std::mutex sync;
    std::unordered_set<Int> sent;
    std::vector<Int> readyForRelease;
};

using PacketCollectionPtr = std::shared_ptr<PacketCollection>;
    
enum class ReleaseAction { markForRelease, subscribe, alreadySent };

class PacketStreamingServer
{
public:
    PacketStreamingServer(size_t releaseThreshold = 1, bool attachTimestampToPacketBuffer = false);

    void addDaqPacket(const uint32_t signalId, const PacketPtr& packet);
    void addDaqPacket(const uint32_t signalId, PacketPtr&& packet);
    PacketBufferPtr getNextPacketBuffer();
    size_t getAvailableBuffersCount();

    void checkAndSendReleasePacket(bool force);
    void addAlreadySentPacket(uint32_t signalId, Int packetId, Int domainPacketId, bool markForRelease);

private:
    SerializerPtr jsonSerializer;
    std::queue<PacketBufferPtr> queue;
    std::unordered_map<uint32_t, DataDescriptorPtr> dataDescriptors;
    PacketCollectionPtr packetCollection;
    size_t releaseThreshold;
    const bool attachTimestampToPacketBuffer;

    void addEventPacket(const uint32_t signalId, const EventPacketPtr& packet);
    template <bool CheckRefCount>
    static bool canReleasePacket(const DataPacketPtr& packet);
    bool shouldSendPacket(const DataPacketPtr& packet, Int packetId, bool markForRelease) const;
    static void setOffset(const DataPacketPtr& packet, DataPacketHeader* packetHeader);
    static Int getDomainPacketId(const DataPacketPtr& packet);

    template <class DataPacket>
    void addDataPacket(const uint32_t signalId, DataPacket&& packet);

    void queuePacketBuffer(const PacketBufferPtr& packetBuffer);
};

}
