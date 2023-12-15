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
#include <coretypes/struct_impl.h>
#include <opendaq/dimension_rule_ptr.h>
#include <opendaq/rule_private.h>
#include <opendaq/dimension_rule.h>

BEGIN_NAMESPACE_OPENDAQ

class DimensionRuleImpl : public GenericStructImpl<IDimensionRule, IStruct, IRulePrivate>
{
public:
    explicit DimensionRuleImpl(DimensionRuleType ruleType, const DictPtr<IString, IBaseObject>& params);
    explicit DimensionRuleImpl(const ListPtr<INumber>& list);
    explicit DimensionRuleImpl(const NumberPtr& delta, const NumberPtr& start, const SizeT& size);   
    explicit DimensionRuleImpl(const NumberPtr& delta, const NumberPtr& start, const NumberPtr& base, const SizeT& size);
    explicit DimensionRuleImpl(IDimensionRuleBuilder* dimensionRuleBuilder);

    ErrCode INTERFACE_FUNC getType(DimensionRuleType* type) override;
    ErrCode INTERFACE_FUNC getParameters(IDict** parameters) override;

    ErrCode INTERFACE_FUNC verifyParameters() override;
    ErrCode INTERFACE_FUNC equals(IBaseObject* other, Bool* equals) const override;

    // ISerializable
    ErrCode INTERFACE_FUNC serialize(ISerializer* serializer) override;
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;

    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* /*context*/, IFunction* /*factoryCallback*/, IBaseObject** obj);

private:
    enum class LabelType
    {
        None = 0,
        Number,
        Range,
        String
    };

    ErrCode verifyParametersInternal() const;
    ErrCode checkLinearRuleValidity() const;
    ErrCode checkListRuleValidity() const;
    ErrCode checkLogRuleValidity() const;
    static LabelType getLabelType(const ObjectPtr<IBaseObject>& label);
    static bool listLabelsValid(const ListPtr<IBaseObject>& list);

    DimensionRuleType ruleType;
    DictPtr<IString, IBaseObject> params;
};

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(DimensionRuleImpl)

END_NAMESPACE_OPENDAQ
