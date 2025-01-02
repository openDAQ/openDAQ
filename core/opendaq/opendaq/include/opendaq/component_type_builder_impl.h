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
#include <opendaq/component_type_builder.h>
#include <coreobjects/property_object_ptr.h>
#include <coretypes/intfs.h>
#include <coretypes/string_ptr.h>
#include <opendaq/module_info_ptr.h>

BEGIN_NAMESPACE_OPENDAQ
class ComponentTypeBuilderImpl : public ImplementationOf<IComponentTypeBuilder>
{
public:
    explicit ComponentTypeBuilderImpl(ComponentTypeSort sort);

    ErrCode INTERFACE_FUNC build(IComponentType** componentType) override;
    
    ErrCode INTERFACE_FUNC setId(IString* id) override;
    ErrCode INTERFACE_FUNC getId(IString** id) override;

    ErrCode INTERFACE_FUNC setTypeSort(ComponentTypeSort sort) override;
    ErrCode INTERFACE_FUNC getTypeSort(ComponentTypeSort* sort) override;

    ErrCode INTERFACE_FUNC setName(IString* name) override;
    ErrCode INTERFACE_FUNC getName(IString** name) override;

    ErrCode INTERFACE_FUNC setDescription(IString* description) override;
    ErrCode INTERFACE_FUNC getDescription(IString** description) override;

    ErrCode INTERFACE_FUNC setConnectionStringPrefix(IString* prefix) override;
    ErrCode INTERFACE_FUNC getConnectionStringPrefix(IString** prefix) override;

    ErrCode INTERFACE_FUNC setDefaultConfig(IPropertyObject* defaultConfig) override;
    ErrCode INTERFACE_FUNC getDefaultConfig(IPropertyObject** defaultConfig) override;

private:
    ComponentTypeSort sort;
    StringPtr id;
    StringPtr name;
    StringPtr prefix;
    StringPtr description;
    PropertyObjectPtr defaultConfig;
    ModuleInfoPtr moduleInfo;
};

END_NAMESPACE_OPENDAQ
