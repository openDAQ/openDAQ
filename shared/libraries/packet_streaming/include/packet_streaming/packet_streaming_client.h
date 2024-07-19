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
#include "opendaq/event_packet_ptr.h"
#include <queue>

namespace daq::packet_streaming
{


struct DataDescriptorEventPacket
{
    EventPacketPtr eventPacket;
    DataDescriptorPtr valueDescriptor;
    DataDescriptorPtr domainDescriptor;
};

class PacketStreamingClient
{
public:
    PacketStreamingClient();

    void addPacketBuffer(const PacketBufferPtr& packetBuffer);

    std::tuple<uint32_t, PacketPtr> getNextDaqPacket();

    bool areReferencesCleared() const;

    EventPacketPtr getDataDescriptorChangedEventPacket(uint32_t signalId) const;
private:
    DeserializerPtr jsonDeserializer;
    std::queue<std::tuple<uint32_t, PacketPtr>> queue;
    std::unordered_map<uint32_t, DataDescriptorPtr> dataDescriptors;
    std::unordered_map<uint32_t, DataDescriptorPtr> domainDescriptors;

    std::unordered_map<Int, DataPacketPtr> referencedPackets;
    std::unordered_map<Int, PacketBufferPtr> referencedPacketBuffers;
    std::unordered_map<Int, std::vector<PacketBufferPtr>> packetBuffersWaitingForDomainPackets;

    mutable std::mutex descriptorsSync;

    void addEventPacketBuffer(const PacketBufferPtr& packetBuffer);
    DataPacketPtr addDataPacketBuffer(const PacketBufferPtr& packetBuffer, const DataPacketPtr& domainPacket);
    void addReleasePacketBuffer(const PacketBufferPtr& packetBuffer);
    void addAlreadySentPacketBuffer(const PacketBufferPtr& packetBuffer);
};

}
