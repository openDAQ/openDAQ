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
#include <coreobjects/unit_ptr.h>
#include <coretypes/intfs.h>
#include <coretypes/string_ptr.h>
#include <coretypes/struct_impl.h>
#include <coreobjects/unit_builder_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

class UnitImpl : public GenericStructImpl<IUnit, IStruct>
{
public:
    explicit UnitImpl(Int id, StringPtr symbol, StringPtr name, StringPtr quantity);
    explicit UnitImpl(IUnitBuilder* unitBuilder);

    ErrCode INTERFACE_FUNC getId(Int* id) override;
    ErrCode INTERFACE_FUNC getSymbol(IString** symbol) override;
    ErrCode INTERFACE_FUNC getName(IString** name) override;
    ErrCode INTERFACE_FUNC getQuantity(IString** quantity) override;

    // ISerializable
    ErrCode INTERFACE_FUNC serialize(ISerializer* serializer) override;
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;

    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* /*context*/, IFunction* /*factoryCallback*/, IBaseObject** obj);

private:
    static DictPtr<IString, IBaseObject> PackBuilder(IUnitBuilder* unitBuilder);
};

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(UnitImpl)

END_NAMESPACE_OPENDAQ
