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
#include <coretypes/freezable.h>
#include <coretypes/impl.h>
#include <coretypes/intfs.h>
#include <opendaq/dimension_builder.h>
#include <opendaq/dimension_ptr.h>
#include <opendaq/dimension_rule_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

class DimensionBuilderImpl : public ImplementationOf<IDimensionBuilder>
{
public:
    explicit DimensionBuilderImpl();
    explicit DimensionBuilderImpl(const DimensionPtr& copy);

    ErrCode INTERFACE_FUNC build(IDimension** dimension) override;

    ErrCode INTERFACE_FUNC setName(IString* name) override;
    ErrCode INTERFACE_FUNC getName(IString** name) override;

    ErrCode INTERFACE_FUNC setUnit(IUnit* unit) override;
    ErrCode INTERFACE_FUNC getUnit(IUnit** unit) override;

    ErrCode INTERFACE_FUNC setRule(IDimensionRule* rule) override;
    ErrCode INTERFACE_FUNC getRule(IDimensionRule** rule) override;

private:
    StringPtr name;
    UnitPtr unit;
    DimensionRulePtr rule;
};

END_NAMESPACE_OPENDAQ
