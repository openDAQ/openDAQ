/*
 * Copyright 2022-2023 Blueberry d.o.o.
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

#include <functional>
#include <stdexcept>
#include <string>
#include <coretypes/string_ptr.h>

namespace daq::config_protocol
{

enum PacketType: uint8_t { getProtocolInfo = 0x80, upgradeProtocol = 0x81, rpc = 0x82, serverNotification = 0x83, invalidRequest = 0x84 };

struct PacketHeader
{
    uint8_t headerSize;
    PacketType type;
    char unused[2];
    uint32_t payloadSize;
    size_t id;
};

using DeleterCallback = std::function<void(void*)>;

class PacketBuffer
{
public:
    PacketBuffer();
    PacketBuffer(const PacketBuffer&) = delete;
    PacketBuffer(PacketBuffer&& packetBuffer) noexcept;

    // create a packet that copies payload
    PacketBuffer(PacketType packetType, size_t id, const void* payload, size_t payloadSize);
    // create a packet that either copies packet or just takes a pointer
    PacketBuffer(void* mem, bool copy);
    // create a packet that takes ownership of memory, no copy
    PacketBuffer(void* mem, DeleterCallback deleterCallback);

    void operator=(PacketBuffer&& other);
    void operator=(const PacketBuffer& other) = delete;

    ~PacketBuffer();

    void reset();

    static PacketHeader* allocateHeaderAndPayload(size_t payloadSize);
    static void deallocateMem(void* mem);

    void* getBuffer() const;
    size_t getLength() const;

    void* detach();

    void setPacketType(PacketType packetType);
    PacketType getPacketType() const;

    void setId(size_t id);
    size_t getId() const;

    void setPayloadSize(size_t payloadSize);
    size_t getPayloadSize() const;

    void* getPayload() const;

    static PacketBuffer createGetProtocolInfoRequest(size_t id);
    void parseProtocolInfoRequest() const;

    static PacketBuffer createGetProtocolInfoReply(size_t id, uint16_t currentVersion, const std::vector<uint16_t>& supportedVersions);
    void parseProtocolInfoReply(uint16_t& currentVersion, std::vector<uint16_t>& supportedVersions) const;

    static PacketBuffer createUpgradeProtocolRequest(size_t id, uint16_t version);
    void parseProtocolUpgradeRequest(uint16_t& version) const;

    static PacketBuffer createUpgradeProtocolReply(size_t id, bool success);
    void parseProtocolUpgradeReply(bool& success) const;

    static PacketBuffer createRpcRequestOrReply(size_t id, const char* json, size_t jsonSize);
    StringPtr parseRpcRequestOrReply() const;

    static PacketBuffer createServerNotification(const char* json, size_t jsonSize);
    StringPtr parseServerNotification() const;

    static PacketBuffer createInvalidRequestReply(size_t id);
    void parseInvalidRequestReply() const;

private:
    PacketHeader* buffer;
    bool captured;
    DeleterCallback deleterCallback;
};

class ConfigProtocolException : public std::runtime_error
{
public:
    ConfigProtocolException(const std::string& msg);
};

}
