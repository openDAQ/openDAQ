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
#include <coretypes/coretypes.h>
#include <opendaq/dimension_rule_builder.h>
#include <opendaq/dimension_rule_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

class DimensionRuleBuilderImpl : public ImplementationOf<IDimensionRuleBuilder>
{
public:
    explicit DimensionRuleBuilderImpl();
    explicit DimensionRuleBuilderImpl(const DimensionRulePtr& rule);  // Copy
    
    ErrCode INTERFACE_FUNC build(IDimensionRule** dimensionRule) override;

    ErrCode INTERFACE_FUNC setType(DimensionRuleType type) override;
    ErrCode INTERFACE_FUNC getType(DimensionRuleType* type) override;

    ErrCode INTERFACE_FUNC setParameters(IDict* parameters) override;
    ErrCode INTERFACE_FUNC getParameters(IDict** parameters) override;
    ErrCode INTERFACE_FUNC addParameter(IString* name, IBaseObject* parameter) override;
    ErrCode INTERFACE_FUNC removeParameter(IString* name) override;

private:
    DimensionRuleType ruleType;
    DictPtr<IString, IBaseObject> params;
};

END_NAMESPACE_OPENDAQ
