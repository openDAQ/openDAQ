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
#include <coretypes/dictobject_factory.h>
#include <coretypes/intfs.h>
#include <opendaq/data_rule_builder.h>
#include <opendaq/data_rule_ptr.h>
#include <opendaq/rule_private.h>

BEGIN_NAMESPACE_OPENDAQ

class DataRuleBuilderImpl final : public ImplementationOf<IDataRuleBuilder>
{
public:
    explicit DataRuleBuilderImpl();
    explicit DataRuleBuilderImpl(const DataRulePtr& ruleToCopy);

    ErrCode INTERFACE_FUNC build(IDataRule** scaling) override;

    ErrCode INTERFACE_FUNC setType(DataRuleType type) override;
    ErrCode INTERFACE_FUNC getType(DataRuleType* type) override;

    ErrCode INTERFACE_FUNC setParameters(IDict* parameters) override;
    ErrCode INTERFACE_FUNC getParameters(IDict** parameters) override;
    ErrCode INTERFACE_FUNC addParameter(IString* name, IBaseObject* parameter) override;
    ErrCode INTERFACE_FUNC removeParameter(IString* name) override;

private:
    DataRuleType ruleType;
    DictPtr<IString, IBaseObject> params;
};

END_NAMESPACE_OPENDAQ
