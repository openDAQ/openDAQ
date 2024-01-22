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
#include <coretypes/dictobject_factory.h>
#include <opendaq/event_packet.h>
#include <opendaq/packet_impl.h>
#include <coretypes/string_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

class EventPacketImpl : public PacketImpl<IEventPacket, ISerializable>
{
public:
    using Super = PacketImpl<IEventPacket, ISerializable>;

    explicit EventPacketImpl(StringPtr eventId, DictPtr<IString, IBaseObject> parameters);

    ErrCode INTERFACE_FUNC getEventId(IString** id) override;
    ErrCode INTERFACE_FUNC getParameters(IDict** parameters) override;

    ErrCode INTERFACE_FUNC equals(IBaseObject* other, Bool* equals) const override;

    // ISerializable
    ErrCode INTERFACE_FUNC serialize(ISerializer* serializer) override;
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;

    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* /*context*/, IFunction* /*factoryCallback*/, IBaseObject** obj);

private:
    StringPtr eventId;
    DictPtr<IString, IBaseObject> parameters;
};

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(EventPacketImpl)

END_NAMESPACE_OPENDAQ
