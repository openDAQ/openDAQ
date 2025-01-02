/*
 * Copyright 2022-2025 openDAQ d.o.o.
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
#include <opendaq/device_domain_ptr.h>
#include <coretypes/intfs.h>
#include <coretypes/string_ptr.h>
#include <coretypes/struct_impl.h>

BEGIN_NAMESPACE_OPENDAQ

class DeviceDomainImpl : public GenericStructImpl<IDeviceDomain, IStruct>
{
public:
    explicit DeviceDomainImpl(RatioPtr tickResolution,
                              StringPtr origin,
                              UnitPtr unit,
                              ReferenceDomainInfoPtr referenceDomainInfo = nullptr);

    ErrCode INTERFACE_FUNC getTickResolution(IRatio** tickResolution) override;
    ErrCode INTERFACE_FUNC getOrigin(IString** origin) override;
    ErrCode INTERFACE_FUNC getUnit(IUnit** unit) override;
    ErrCode INTERFACE_FUNC getReferenceDomainInfo(IReferenceDomainInfo** referenceDomainInfo) override;

    // ISerializable
    ErrCode INTERFACE_FUNC serialize(ISerializer* serializer) override;
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;

    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* /*context*/, IFunction* /*factoryCallback*/, IBaseObject** obj);
};

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(DeviceDomainImpl)

END_NAMESPACE_OPENDAQ
