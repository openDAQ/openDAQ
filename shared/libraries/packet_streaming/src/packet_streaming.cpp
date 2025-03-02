#include <packet_streaming/packet_streaming.h>

namespace daq::packet_streaming
{

PacketBuffer::PacketBuffer(GenericPacketHeader* packetHeader,
                           const void* payload,
                           std::function<void()> onDestroy,
                           bool enableTimeStamp,
                           size_t cacheableGroupId)
    : packetHeader(packetHeader)
    , payload(payload)
    , onDestroy(std::move(onDestroy))
    , timeStamp(enableTimeStamp ? std::optional(std::chrono::steady_clock::now()) : std::nullopt)
    , cacheableGroupId(cacheableGroupId)
{
}


PacketBuffer::PacketBuffer(PacketBuffer&& packetBuffer) noexcept
{
    packetHeader = packetBuffer.packetHeader;
    payload = packetBuffer.payload;

    onDestroy = packetBuffer.onDestroy;
    timeStamp = packetBuffer.timeStamp;
    cacheableGroupId = packetBuffer.cacheableGroupId;

    packetBuffer.onDestroy = [](){};
    packetBuffer.packetHeader = nullptr;
    packetBuffer.payload = nullptr;
    packetBuffer.cacheableGroupId = NON_CACHEABLE_GROUP_ID;
}

PacketBuffer::~PacketBuffer()
{
    onDestroy();
}

bool PacketBuffer::isCacheable()
{
    return cacheableGroupId != NON_CACHEABLE_GROUP_ID;
}

PacketStreamingException::PacketStreamingException(const std::string& msg)
    : runtime_error(msg)
{
}

}
