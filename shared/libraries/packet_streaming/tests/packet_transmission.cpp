#include "packet_transmission.h"

namespace daq::packet_streaming
{

void PacketTransmission::sendPacketBuffer(const PacketBufferPtr& packetBuffer)
{
    GenericPacketHeader* recvPacketHeader {};
    recvPacketHeader = static_cast<GenericPacketHeader*>(std::malloc(packetBuffer->packetHeader->size));
    std::memcpy(recvPacketHeader, packetBuffer->packetHeader, packetBuffer->packetHeader->size);

    void* recvPayload;

    if (packetBuffer->packetHeader->payloadSize > 0)
    {
        recvPayload = std::malloc(packetBuffer->packetHeader->payloadSize);
        std::memcpy(recvPayload, packetBuffer->payload, packetBuffer->packetHeader->payloadSize);
    }
    else
        recvPayload = nullptr;

    auto recvPacketBuffer = std::make_shared<PacketBuffer>(recvPacketHeader, recvPayload, [recvPacketHeader, recvPayload]() {
        std::free(recvPacketHeader);
        if (recvPayload != nullptr)
          std::free(recvPayload);
    });

    queue.push(recvPacketBuffer);
}

PacketBufferPtr PacketTransmission::recvPacketBuffer()
{
    if (queue.empty())
        return nullptr;

    auto pb = queue.front();
    queue.pop();
    return pb;
}

}
