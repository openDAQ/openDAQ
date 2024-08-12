/*
 * Copyright 2022-2024 openDAQ d.o.o.
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
#include <coretypes/struct_impl.h>
#include <opendaq/reference_domain_info_builder_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

class ReferenceDomainInfoImpl : public GenericStructImpl<IReferenceDomainInfo, IStruct>
{
public:
    using Super = GenericStructImpl<IReferenceDomainInfo, IStruct>;

    explicit ReferenceDomainInfoImpl(IReferenceDomainInfoBuilder* referenceDomainInfoBuilder);

    ErrCode INTERFACE_FUNC getReferenceDomainId(IString** referenceDomainId) override;
    ErrCode INTERFACE_FUNC getReferenceDomainOffset(IInteger** referenceDomainOffset) override;
    ErrCode INTERFACE_FUNC getReferenceDomainIsAbsolute(IBoolean** referenceDomainIsAbsolute) override;

    ErrCode INTERFACE_FUNC equals(IBaseObject* other, Bool* equal) const override;

    // ISerializable
    ErrCode INTERFACE_FUNC serialize(ISerializer* serializer) override;
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;

    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* /*context*/, IFunction* /*factoryCallback*/, IBaseObject** obj);

    // IBaseObject
    ErrCode INTERFACE_FUNC queryInterface(const IntfID& id, void** intf) override;
    ErrCode INTERFACE_FUNC borrowInterface(const IntfID& id, void** intf) const override;

protected:
    StringPtr referenceDomainId;
    IntegerPtr referenceDomainOffset;
    BoolPtr referenceDomainIsAbsolute;

private:
    static DictPtr<IString, IBaseObject> PackBuilder(IReferenceDomainInfoBuilder* referenceDomainInfoBuilder);
};

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(ReferenceDomainInfoImpl)

END_NAMESPACE_OPENDAQ
