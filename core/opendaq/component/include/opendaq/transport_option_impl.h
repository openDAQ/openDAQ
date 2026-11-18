/*
 * Copyright 2022-2026 openDAQ d.o.o.
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
#include <opendaq/transport_option.h>
#include <opendaq/transport_option_factory.h>
#include <opendaq/component_type.h>
#include <coretypes/impl.h>
#include <coretypes/serializable.h>
#include <coretypes/serialized_object.h>
#include <coretypes/dictobject_factory.h>
#include <coreobjects/property_object_ptr.h>
#include <coreobjects/property_factory.h>
#include <fmt/format.h>

BEGIN_NAMESPACE_OPENDAQ

// An immutable transport option, serialized as a tagged object with its security mode as a string. A server capability
// does not hold it as it is: it keeps each option as a plain Property object, which any peer reads.
class TransportOptionImpl final : public ImplementationOf<ITransportOption, ISerializable>
{
public:
    TransportOptionImpl(const StringPtr& id, Int port, SecurityMode securityMode, Bool certificateRequired);

    ErrCode INTERFACE_FUNC getId(IString** id) override;
    ErrCode INTERFACE_FUNC getPort(Int* port) override;
    ErrCode INTERFACE_FUNC getSecurityMode(SecurityMode* securityMode) override;
    ErrCode INTERFACE_FUNC getCertificateRequired(Bool* required) override;

    ErrCode INTERFACE_FUNC serialize(ISerializer* serializer) override;
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;
    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

private:
    StringPtr id;
    Int port;
    SecurityMode securityMode;
    Bool certificateRequired;
};

END_NAMESPACE_OPENDAQ
