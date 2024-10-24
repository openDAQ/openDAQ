#include <packet_streaming/packet_streaming.h>

namespace daq::packet_streaming
{

PacketBuffer::PacketBuffer(GenericPacketHeader* packetHeader, const void* payload, std::function<void()> onDestroy, bool enableTimeStamp)
    : packetHeader(packetHeader)
    , payload(payload)
    , onDestroy(std::move(onDestroy))
    , timeStamp(enableTimeStamp ? std::optional(std::chrono::steady_clock::now()) : std::nullopt)
{
}


PacketBuffer::PacketBuffer(PacketBuffer&& packetBuffer) noexcept
{
    packetHeader = packetBuffer.packetHeader;
    payload = packetBuffer.payload;

    onDestroy = packetBuffer.onDestroy;
    timeStamp = packetBuffer.timeStamp;

    packetBuffer.onDestroy = [](){};
    packetBuffer.packetHeader = nullptr;
    packetBuffer.payload = nullptr;
}

PacketBuffer::~PacketBuffer()
{
    onDestroy();
}

PacketStreamingException::PacketStreamingException(const std::string& msg)
    : runtime_error(msg)
{
}

}
