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
#include <opendaq/data_rule.h>
#include <opendaq/rule_private.h>
#include <coretypes/struct_impl.h>
#include <opendaq/data_rule_ptr.h>
#include <opendaq/data_rule_builder.h>

BEGIN_NAMESPACE_OPENDAQ

class DataRuleImpl final : public GenericStructImpl<IDataRule, IStruct, IRulePrivate>
{
public:
    explicit DataRuleImpl(DataRuleType ruleType, const DictPtr<IString, IBaseObject>& params);
    explicit DataRuleImpl(DataRuleType ruleType, const NumberPtr& param1, const NumberPtr& param2);
    explicit DataRuleImpl(const NumberPtr& constant);
    explicit DataRuleImpl();
    explicit DataRuleImpl(IDataRuleBuilder * dataRuleBuilder);
    
    ErrCode INTERFACE_FUNC getType(DataRuleType* type) override;
    ErrCode INTERFACE_FUNC getParameters(IDict** parameters) override;

    ErrCode INTERFACE_FUNC verifyParameters() override;

    ErrCode INTERFACE_FUNC equals(IBaseObject* other, Bool* equals) const override;

    // ISerializable
    ErrCode INTERFACE_FUNC serialize(ISerializer* serializer) override;
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;

    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* /*context*/, IFunction* /*factoryCallback*/, IBaseObject** obj);

private:
    ErrCode verifyParametersInternal();

    DataRuleType ruleType;
    DictPtr<IString, IBaseObject> params;
};

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(DataRuleImpl)

END_NAMESPACE_OPENDAQ
