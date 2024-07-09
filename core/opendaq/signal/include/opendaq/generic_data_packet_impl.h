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
#include <opendaq/data_packet_ptr.h>
#include <opendaq/packet_impl.h>

BEGIN_NAMESPACE_OPENDAQ

template <typename TInterface = IDataPacket, typename... TInterfaces>
class GenericDataPacketImpl : public PacketImpl<TInterface, TInterfaces ...>
{
public:
    explicit GenericDataPacketImpl(const DataPacketPtr& domainPacket);

    ErrCode INTERFACE_FUNC getDomainPacket(IDataPacket** packet) override;
    ErrCode INTERFACE_FUNC getPacketId(Int* packetId) override;

protected:
    DataPacketPtr domainPacket;
    Int packetId;
};

Int generatePacketId();

template <typename TInterface, typename... TInterfaces>
GenericDataPacketImpl<TInterface, TInterfaces...>::GenericDataPacketImpl(const DataPacketPtr& domainPacket)
    : domainPacket(domainPacket)
    , packetId(generatePacketId())
{
    this->type = PacketType::Data;
}

template <typename TInterface, typename... TInterfaces>
ErrCode GenericDataPacketImpl<TInterface, TInterfaces...>::getDomainPacket(IDataPacket** packet)
{
    OPENDAQ_PARAM_NOT_NULL(packet);

    *packet = domainPacket.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... TInterfaces>
ErrCode GenericDataPacketImpl<TInterface, TInterfaces...>::getPacketId(Int* packetId)
{
    OPENDAQ_PARAM_NOT_NULL(packetId);

    *packetId = this->packetId;
    return OPENDAQ_SUCCESS;
}

END_NAMESPACE_OPENDAQ
