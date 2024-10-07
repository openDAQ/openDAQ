#include <config_protocol/config_protocol.h>
#include <cassert>
#include <coretypes/stringobject_factory.h>

namespace daq::config_protocol
{

PacketBuffer::PacketBuffer(PacketType packetType, uint64_t id, const void* payload, size_t payloadSize)
    : captured(false)
{
    buffer = allocateHeaderAndPayload(payloadSize);
    buffer->headerSize = sizeof(PacketHeader);
    buffer->type = packetType;
    buffer->payloadSize = static_cast<uint32_t>(payloadSize);
    buffer->id = id;
    if (payload != nullptr)
    {
        void* bufferPayload = static_cast<void*>(buffer + 1);
        std::memcpy(bufferPayload, payload, payloadSize);
    }
    else
        assert(payloadSize == 0);
}

PacketBuffer::PacketBuffer(void* mem, bool copy)
    : captured(!copy)
{
    if (copy)
    {
        const auto memPacketHeader = static_cast<PacketHeader*>(mem);
        const auto memSize = memPacketHeader->headerSize + memPacketHeader->payloadSize;
        buffer = static_cast<PacketHeader*>(std::malloc(memSize));
        std::memcpy(buffer, memPacketHeader, memSize);
    }
    else
    {
        const auto memPacketHeader = static_cast<PacketHeader*>(mem);
        buffer = static_cast<PacketHeader*>(memPacketHeader);
    }
}

PacketBuffer::PacketBuffer(void* mem, DeleterCallback deleterCallback)
    : captured(true)
    , deleterCallback(std::move(deleterCallback))
{
    const auto memPacketHeader = static_cast<PacketHeader*>(mem);
    buffer = static_cast<PacketHeader*>(memPacketHeader);
}

void PacketBuffer::operator=(PacketBuffer&& other)
{
    reset();
    
    buffer = other.buffer;
    captured = other.captured;
    deleterCallback = std::move(other.deleterCallback);

    other.buffer = nullptr;
    other.captured = false;
}

PacketBuffer::PacketBuffer()
    : buffer(nullptr)
    , captured(false)
{
}

PacketBuffer::PacketBuffer(PacketBuffer&& packetBuffer) noexcept
{
    buffer = packetBuffer.buffer;
    captured = packetBuffer.captured;
    deleterCallback = std::move(packetBuffer.deleterCallback);

    packetBuffer.buffer = nullptr;
    packetBuffer.captured = false;
}

void PacketBuffer::reset()
{
    if (buffer != nullptr)
    {
        if (captured)
        {
            if (deleterCallback)
                deleterCallback(static_cast<void*>(buffer));
        }
        else
        {
            deallocateMem(buffer);
        }

        buffer = nullptr;
        captured = false;
        deleterCallback = nullptr;
    }
}

PacketBuffer::~PacketBuffer()
{
    reset();
}

PacketHeader* PacketBuffer::allocateHeaderAndPayload(size_t payloadSize)
{
    const auto ptr = static_cast<PacketHeader*>(std::malloc(sizeof(PacketHeader) + payloadSize));
    if (ptr == nullptr)
        throw ConfigProtocolException("Out of memory");
    ptr->headerSize = sizeof(PacketHeader);
    ptr->payloadSize = static_cast<uint32_t>(payloadSize);
    return ptr;
}

void PacketBuffer::deallocateMem(void* mem)
{
    std::free(mem);
}


void* PacketBuffer::getBuffer() const
{
    return static_cast<void*>(buffer);
}

size_t PacketBuffer::getLength() const
{
    if (!buffer)
        return 0;

    return buffer->headerSize + buffer->payloadSize;
}

void* PacketBuffer::detach()
{
    const auto buf = getBuffer();
    buffer = nullptr;
    captured = false;
    deleterCallback = nullptr;
    return buf;
}

void PacketBuffer::setPacketType(PacketType packetType)
{
    buffer->type = packetType;
}

PacketType PacketBuffer::getPacketType() const
{
    return buffer->type;
}

void PacketBuffer::setId(uint64_t id)
{
    buffer->id = id;
}

uint64_t PacketBuffer::getId() const
{
    return buffer->id;
}

void PacketBuffer::setPayloadSize(size_t payloadSize)
{
    buffer->payloadSize = static_cast<uint32_t>(payloadSize);
}

size_t PacketBuffer::getPayloadSize() const
{
    return buffer->payloadSize;
}

void* PacketBuffer::getPayload() const
{
    return static_cast<void*>(buffer + 1);
}

PacketBuffer PacketBuffer::createGetProtocolInfoRequest(uint64_t id)
{
    auto packetBuffer = PacketBuffer(PacketType::GetProtocolInfo, id, nullptr, 0);
    return packetBuffer;
}

PacketBuffer PacketBuffer::createGetProtocolInfoReply(uint64_t id, uint16_t currentVersion, const std::set<uint16_t>& supportedVersions)
{
    const auto payloadSize = sizeof(uint16_t) + sizeof(uint16_t) +
                             supportedVersions.size() * sizeof(uint16_t);

    const auto mem = PacketBuffer::allocateHeaderAndPayload(payloadSize);
    auto packetBuffer = PacketBuffer(mem, std::bind(&PacketBuffer::deallocateMem, std::placeholders::_1));
    packetBuffer.setPacketType(PacketType::GetProtocolInfo);
    packetBuffer.setId(id);
    auto payload = static_cast<uint16_t*>(packetBuffer.getPayload());
    *payload++ = currentVersion;
    *payload++ = static_cast<uint16_t>(supportedVersions.size());
    for (const auto item : supportedVersions)
        *payload++ = item;
    return packetBuffer;
}

void PacketBuffer::parseProtocolInfoRequest() const
{
    if (getPacketType() != PacketType::GetProtocolInfo)
        throw ConfigProtocolException("Invalid packet type");

    if (getPayloadSize() != 0)
        throw ConfigProtocolException("Invalid payload");
}

void PacketBuffer::parseProtocolInfoReply(uint16_t& currentVersion, std::set<uint16_t>& supportedVersions) const
{
    if (getPacketType() != PacketType::GetProtocolInfo)
        throw ConfigProtocolException("Invalid packet type");

    if (getPayloadSize() < sizeof(uint16_t) + sizeof(uint16_t))
        throw ConfigProtocolException("Invalid payload");

    auto payload = static_cast<uint16_t*>(getPayload());
    currentVersion = *payload++;
    const size_t verCount = *payload++;
    for (size_t i = 0; i < verCount; i++)
        supportedVersions.insert(*payload++);
}

PacketBuffer PacketBuffer::createUpgradeProtocolRequest(uint64_t id, uint16_t version)
{
    auto packetBuffer = PacketBuffer(PacketType::UpgradeProtocol, id, &version, sizeof(uint16_t));
    return packetBuffer;
}

void PacketBuffer::parseProtocolUpgradeRequest(uint16_t& version) const
{
    if (getPacketType() != PacketType::UpgradeProtocol)
        throw ConfigProtocolException("Invalid packet type");

    if (getPayloadSize() != sizeof(uint16_t))
        throw ConfigProtocolException("Invalid payload");

    version = *(static_cast<uint16_t*>(getPayload()));
}

PacketBuffer PacketBuffer::createUpgradeProtocolReply(uint64_t id, bool success)
{
    uint8_t success_ = success ? 1 : 0;
    auto packetBuffer = PacketBuffer(PacketType::UpgradeProtocol, id, &success_, sizeof(uint8_t));
    return packetBuffer;
}

void PacketBuffer::parseProtocolUpgradeReply(bool& success) const
{
    if (getPacketType() != PacketType::UpgradeProtocol)
        throw ConfigProtocolException("Invalid packet type");

    if (getPayloadSize() != sizeof(uint8_t))
        throw ConfigProtocolException("Invalid payload");

    success = *(static_cast<uint8_t*>(getPayload())) != 0 ? true : false;
}

PacketBuffer PacketBuffer::createRpcRequestOrReply(uint64_t id, const char* json, size_t jsonSize)
{
    auto packetBuffer = PacketBuffer(PacketType::Rpc, id, json, jsonSize);
    return packetBuffer;
}

PacketBuffer PacketBuffer::createInvalidRequestReply(uint64_t id)
{
    auto packetBuffer = PacketBuffer(PacketType::InvalidRequest, id, nullptr, 0);
    return packetBuffer;
}

void PacketBuffer::parseInvalidRequestReply() const
{
    if (getPacketType() != PacketType::InvalidRequest)
        throw ConfigProtocolException("Invalid packet type");

    if (getPayloadSize() != 0)
        throw ConfigProtocolException("Invalid payload");
}


StringPtr PacketBuffer::parseRpcRequestOrReply() const
{
    if (getPacketType() != PacketType::Rpc)
        throw ConfigProtocolException("Invalid packet type");

    const auto payloadSize = getPayloadSize();

    if (payloadSize == 0)
        throw ConfigProtocolException("Invalid payload");

    auto jsonStr = String(static_cast<char*>(getPayload()), payloadSize);
    return jsonStr;
}

PacketBuffer PacketBuffer::createServerNotification(const char* json, size_t jsonSize)
{
    auto packetBuffer = PacketBuffer(PacketType::ServerNotification, std::numeric_limits<uint64_t>::max(), json, jsonSize);
    return packetBuffer;
}

StringPtr PacketBuffer::parseServerNotification() const
{
    if (getPacketType() != PacketType::ServerNotification)
        throw ConfigProtocolException("Invalid packet type");

    const auto payloadSize = getPayloadSize();

    if (payloadSize == 0)
        throw ConfigProtocolException("Invalid payload");

    auto jsonStr = String(static_cast<char*>(getPayload()), payloadSize);
    return jsonStr;
}

PacketBuffer PacketBuffer::createNoReplyRpcRequest(const char* json, size_t jsonSize)
{
    auto packetBuffer = PacketBuffer(PacketType::NoReplyRpc, std::numeric_limits<uint64_t>::max(), json, jsonSize);
    return packetBuffer;
}

StringPtr PacketBuffer::parseNoReplyRpcRequest() const
{
    if (getPacketType() != PacketType::NoReplyRpc)
        throw ConfigProtocolException("Invalid packet type");

    const auto payloadSize = getPayloadSize();

    if (payloadSize == 0)
        throw ConfigProtocolException("Invalid payload");

    auto jsonStr = String(static_cast<char*>(getPayload()), payloadSize);
    return jsonStr;
}

PacketBuffer PacketBuffer::createConnectionRejectedReply(uint64_t id, const char* json, size_t jsonSize)
{
    auto packetBuffer = PacketBuffer(PacketType::ConnectionRejected, id, json, jsonSize);
    return packetBuffer;
}

StringPtr PacketBuffer::parseConnectionRejectedReply() const
{
    if (getPacketType() != PacketType::ConnectionRejected)
        throw ConfigProtocolException("Invalid packet type");

    const auto payloadSize = getPayloadSize();

    if (payloadSize == 0)
        throw ConfigProtocolException("Invalid payload");

    auto jsonStr = String(static_cast<char*>(getPayload()), payloadSize);
    return jsonStr;
}

ConfigProtocolException::ConfigProtocolException(const std::string& msg)
    : runtime_error(msg)
{
}

}
