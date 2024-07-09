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

#include <functional>
#include <stdexcept>
#include <string>
#include <coretypes/string_ptr.h>
#include <coretypes/dictobject_factory.h>
#include <coretypes/baseobject_factory.h>

namespace daq::config_protocol
{

using ParamsDictPtr = DictPtr<IString, IBaseObject>;

inline auto ParamsDict() -> decltype(Dict<IString, IBaseObject>())
{
    return Dict<IString, IBaseObject>();
}

inline auto ParamsDict(std::initializer_list<std::pair<const StringPtr, ObjectPtr<IBaseObject>>> init) -> decltype(Dict<IString, IBaseObject>(init))
{
    return Dict<IString, IBaseObject>(init);
}


enum PacketType: uint8_t
{
    GetProtocolInfo = 0x80,
    UpgradeProtocol = 0x81,
    Rpc = 0x82,
    ServerNotification = 0x83,
    InvalidRequest = 0x84
};

#pragma pack(push, 1)
struct PacketHeader
{
    uint8_t headerSize;
    PacketType type;
    char unused[2];
    uint32_t payloadSize;
    uint64_t id;
};
#pragma pack(pop)
static_assert(sizeof(PacketHeader) == 16);

using DeleterCallback = std::function<void(void*)>;

class PacketBuffer
{
public:
    PacketBuffer();
    PacketBuffer(const PacketBuffer&) = delete;
    PacketBuffer(PacketBuffer&& packetBuffer) noexcept;

    // create a packet that copies payload
    PacketBuffer(PacketType packetType, uint64_t id, const void* payload, size_t payloadSize);
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

    void setId(uint64_t id);
    uint64_t getId() const;

    void setPayloadSize(size_t payloadSize);
    size_t getPayloadSize() const;

    void* getPayload() const;

    static PacketBuffer createGetProtocolInfoRequest(uint64_t id);
    void parseProtocolInfoRequest() const;

    static PacketBuffer createGetProtocolInfoReply(uint64_t id, uint16_t currentVersion, const std::vector<uint16_t>& supportedVersions);
    void parseProtocolInfoReply(uint16_t& currentVersion, std::vector<uint16_t>& supportedVersions) const;

    static PacketBuffer createUpgradeProtocolRequest(uint64_t id, uint16_t version);
    void parseProtocolUpgradeRequest(uint16_t& version) const;

    static PacketBuffer createUpgradeProtocolReply(uint64_t id, bool success);
    void parseProtocolUpgradeReply(bool& success) const;

    static PacketBuffer createRpcRequestOrReply(uint64_t id, const char* json, size_t jsonSize);
    StringPtr parseRpcRequestOrReply() const;

    static PacketBuffer createServerNotification(const char* json, size_t jsonSize);
    StringPtr parseServerNotification() const;

    static PacketBuffer createInvalidRequestReply(uint64_t id);
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

inline auto format_as(PacketType type)
{
    switch (type)
    {
        case GetProtocolInfo:
            return "GetProtocolInfo";
        case UpgradeProtocol:
            return "UpgradeProtocol";
        case Rpc:
            return "Rpc";
        case ServerNotification:
            return "ServerNotification";
        case InvalidRequest:
            return "InvalidRequest";
    }
    return "Unknown type";
}

}
