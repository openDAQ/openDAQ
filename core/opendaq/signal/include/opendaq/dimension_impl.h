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
#include <opendaq/dimension_ptr.h>
#include <coreobjects/unit_ptr.h>
#include <opendaq/dimension_rule_ptr.h>
#include <coretypes/struct_impl.h>
#include <opendaq/dimension_builder.h>

BEGIN_NAMESPACE_OPENDAQ

class DimensionImpl : public GenericStructImpl<IDimension, IStruct>
{
public:
    explicit DimensionImpl(const DimensionRulePtr& rule,
                           const UnitPtr& unit,
                           const StringPtr& name);

    explicit DimensionImpl(IDimensionBuilder* dimensionBuilder);

    ErrCode INTERFACE_FUNC getName(IString** name) override;
    ErrCode INTERFACE_FUNC getSize(SizeT* size) override;
    ErrCode INTERFACE_FUNC getUnit(IUnit** unit) override;
    ErrCode INTERFACE_FUNC getRule(IDimensionRule** rule) override;
    ErrCode INTERFACE_FUNC getLabels(IList** labels) override;
    
    ErrCode INTERFACE_FUNC equals(IBaseObject* other, Bool* equals) const override;

    // ISerializable
    ErrCode INTERFACE_FUNC serialize(ISerializer* serializer) override;
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;

    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* /*context*/, IFunction* /*factoryCallback*/, IBaseObject** obj);

private:
    ListPtr<IBaseObject> getLinearLabels() const;
    ListPtr<IBaseObject> getLogLabels() const;
    ListPtr<IBaseObject> getListLabels() const;
    static DictPtr<IString, IBaseObject> PackBuilder(IDimensionBuilder* dimensionBuilder);

    StringPtr name;
    UnitPtr unit;
    DimensionRulePtr rule;
};

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(DimensionImpl)

END_NAMESPACE_OPENDAQ
