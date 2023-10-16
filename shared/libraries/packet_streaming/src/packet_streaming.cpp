#include <packet_streaming/packet_streaming.h>

namespace daq::packet_streaming
{

PacketBuffer::PacketBuffer(GenericPacketHeader* packetHeader, const void* payload, std::function<void()> onDestroy)
    : packetHeader(packetHeader)
    , payload(payload)
    , onDestroy(std::move(onDestroy))
{
}


PacketBuffer::PacketBuffer(PacketBuffer&& packetBuffer) noexcept
{
    packetHeader = packetBuffer.packetHeader;
    payload = packetBuffer.payload;

    onDestroy = packetBuffer.onDestroy;

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
