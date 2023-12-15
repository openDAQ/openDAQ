#include "config_packet_transmission.h"

namespace daq::config_protocol
{

void ConfigPacketTransmission::sendFromClient(PacketBuffer&& packetBuffer)
{
    clientToServerQueue.push_back(std::move(packetBuffer));
}

bool ConfigPacketTransmission::recvOnServer(PacketBuffer& packetBuffer)
{
    if (clientToServerQueue.empty())
        return false;

    packetBuffer = std::move(clientToServerQueue.front());
    clientToServerQueue.pop_front();
    return true;
}

void ConfigPacketTransmission::sendFromServer(PacketBuffer&& packetBuffer)
{
    serverToClientQueue.push_back(std::move(packetBuffer));
}

}
