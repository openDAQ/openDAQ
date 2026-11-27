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
#include <coreobjects/property_object_impl.h>
#include <coreobjects/property_object_ptr.h>
#include <coreobjects/property_factory.h>
#include <fmt/format.h>

BEGIN_NAMESPACE_OPENDAQ

class TransportOptionImpl final : public GenericPropertyObjectImpl<ITransportOption>
{
public:
    using Super = GenericPropertyObjectImpl<ITransportOption>;

    TransportOptionImpl();
    explicit TransportOptionImpl(ITransportOptionBuilder* builder);

    ErrCode INTERFACE_FUNC getId(IString** id) override;
    ErrCode INTERFACE_FUNC getPort(Int* port) override;
    ErrCode INTERFACE_FUNC getSecurityMode(SecurityMode* securityMode) override;
    ErrCode INTERFACE_FUNC getCertificateRequired(Bool* required) override;

    ErrCode INTERFACE_FUNC getInterfaceIds(SizeT* idCount, IntfID** ids) override;

    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;
    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

    ErrCode INTERFACE_FUNC clone(IPropertyObject** cloned) override;
};

END_NAMESPACE_OPENDAQ
