/*
 * Copyright 2022-2024 openDAQ d. o. o.
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

#include <coretypes/baseobject_factory.h>

namespace daq::packet_streaming
{

enum PacketType: uint8_t { event = 0, data, release, alreadySent };

#define PACKET_OFFSET_TYPE_NONE  0x0
#define PACKET_OFFSET_TYPE_INT   0x1
#define PACKET_OFFSET_TYPE_FLOAT 0x2

#define PACKET_FLAG_CAN_RELEASE            0x1
#define PACKET_FLAG_OFFSET_TYPE_MASK       (0x2 | 0x4)

#define PACKET_FLAG_OFFSET_TYPE_SHIFT      1

struct GenericPacketHeader
{
    uint8_t size;
    PacketType type;
    uint8_t version;
    uint8_t flags;
    uint32_t signalId;
    uint32_t payloadSize;
};

// TODO: this could be further optimized by create data with domain and/or offset packets
struct DataPacketHeader
{
    GenericPacketHeader genericHeader;
    Int packetId;
    Int domainPacketId;
    Int sampleCount;
    union
    {
        double packetOffsetFloat64;
        int64_t packetOffsetInt64;
    };
};

struct AlreadySentPacketHeader
{
    GenericPacketHeader genericHeader;
    Int packetId;
    Int domainPacketId;
};

struct PacketBuffer
{
    PacketBuffer(const PacketBuffer&) = delete;
    PacketBuffer(PacketBuffer&& packetBuffer) noexcept;
    PacketBuffer(GenericPacketHeader* packetHeader, const void* payload, std::function<void()> onDestroy);

    GenericPacketHeader* packetHeader;
    const void* payload;

    std::function<void()> onDestroy;
    std::vector<uint32_t> additionalSignalIds;

    ~PacketBuffer();
};

using PacketBufferPtr = std::shared_ptr<PacketBuffer>;

class PacketStreamingException : public std::runtime_error
{
public:
    PacketStreamingException(const std::string& msg);
};

}
