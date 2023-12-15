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

#include <config_protocol/config_protocol.h>
#include <deque>

namespace daq::config_protocol
{

class ConfigPacketTransmission
{
public:
    void sendFromClient(PacketBuffer&& packetBuffer);
    bool recvOnServer(PacketBuffer& packetBuffer);

    void sendFromServer(PacketBuffer&& packetBuffer);

private:
    std::deque<PacketBuffer> clientToServerQueue;
    std::deque<PacketBuffer> serverToClientQueue;
};

}
