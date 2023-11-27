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
#include <coreobjects/unit_builder.h>
#include <coreobjects/unit_factory.h>
#include <coretypes/intfs.h>
#include <coretypes/string_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

class UnitBuilderImpl : public ImplementationOf<IUnitBuilder>
{
public:
    explicit UnitBuilderImpl();
    explicit UnitBuilderImpl(const UnitPtr& unitToCopy);
    explicit UnitBuilderImpl(Int id, StringPtr symbol, StringPtr name, StringPtr quantity);

    ErrCode INTERFACE_FUNC setId(Int id) override;
    ErrCode INTERFACE_FUNC setSymbol(IString* symbol) override;
    ErrCode INTERFACE_FUNC setName(IString* name) override;
    ErrCode INTERFACE_FUNC setQuantity(IString* quantity) override;

    ErrCode INTERFACE_FUNC getId(Int* id) override;
    ErrCode INTERFACE_FUNC getSymbol(IString** symbol) override;
    ErrCode INTERFACE_FUNC getName(IString** name) override;
    ErrCode INTERFACE_FUNC getQuantity(IString** quantity) override;

    ErrCode INTERFACE_FUNC build(IUnit** unit) override;

private:

    Int id;
    StringPtr symbol;
    StringPtr name;
    StringPtr quantity;
};

END_NAMESPACE_OPENDAQ
