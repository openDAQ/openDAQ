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
#include <coreobjects/component_type.h>
#include <coretypes/string_ptr.h>
#include <coretypes/function_ptr.h>
#include <coretypes/validation.h>
#include <coreobjects/property_object_ptr.h>
#include <coretypes/struct_impl.h>
#include <coretypes/struct_type_factory.h>
#include <coreobjects/property_object_internal_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

template <typename Intf = IComponentType, typename... Interfaces>
class GenericComponentTypeImpl : public GenericStructImpl<Intf, IStruct, Interfaces...>
{
public:
    explicit GenericComponentTypeImpl(const StructTypePtr& type,
                                      const StringPtr& id,
                                      const StringPtr& name,
                                      const StringPtr& description,
                                      const PropertyObjectPtr& defaultConfig);

    explicit GenericComponentTypeImpl(const StructTypePtr& type,
                                      const StringPtr& id,
                                      const StringPtr& name,
                                      const StringPtr& description,
                                      const StringPtr& prefix,
                                      const PropertyObjectPtr& defaultConfig);

    ErrCode INTERFACE_FUNC getId(IString** id) override;
    ErrCode INTERFACE_FUNC getName(IString** name) override;
    ErrCode INTERFACE_FUNC getDescription(IString** description) override;
    ErrCode INTERFACE_FUNC createDefaultConfig(IPropertyObject** defaultConfig) override;

protected:
    StringPtr id;
    StringPtr name;
    StringPtr description;
    StringPtr prefix;
    PropertyObjectPtr defaultConfig;
};

template <class Intf, class... Interfaces>
GenericComponentTypeImpl<Intf, Interfaces...>::GenericComponentTypeImpl(const StructTypePtr& type,
                                                                        const StringPtr& id,
                                                                        const StringPtr& name,
                                                                        const StringPtr& description,
                                                                        const PropertyObjectPtr& defaultConfig)
    : GenericStructImpl<Intf, IStruct, Interfaces...>(
          type, Dict<IString, IBaseObject>({{"Id", id}, {"Name", name}, {"Description", description}}))
    , id(id)
    , name(name)
    , description(description)
    , prefix("")
    , defaultConfig(defaultConfig)
{
}

template <typename Intf, typename... Interfaces>
GenericComponentTypeImpl<Intf, Interfaces...>::GenericComponentTypeImpl(const StructTypePtr& type,
                                                                        const StringPtr& id,
                                                                        const StringPtr& name,
                                                                        const StringPtr& description,
                                                                        const StringPtr& prefix,
                                                                        const PropertyObjectPtr& defaultConfig)
    : GenericStructImpl<Intf, IStruct, Interfaces...>(
          type, Dict<IString, IBaseObject>({{"Id", id}, {"Name", name}, {"Description", description}, {"Prefix", prefix}}))
    , id(id)
    , name(name)
    , description(description)
    , prefix(prefix)
    , defaultConfig(defaultConfig)
{
}

template <class Intf, class... Interfaces>
ErrCode GenericComponentTypeImpl<Intf, Interfaces...>::getId(IString** id)
{
    OPENDAQ_PARAM_NOT_NULL(id);

    *id = this->id.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

template <class Intf, class... Interfaces>
ErrCode GenericComponentTypeImpl<Intf, Interfaces...>::getName(IString** name)
{
    OPENDAQ_PARAM_NOT_NULL(name);

    *name = this->name.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

template <class Intf, class... Interfaces>
ErrCode GenericComponentTypeImpl<Intf, Interfaces...>::getDescription(IString** description)
{
    OPENDAQ_PARAM_NOT_NULL(description);

    *description = this->description.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

template <class Intf, class... Interfaces>
ErrCode GenericComponentTypeImpl<Intf, Interfaces...>::createDefaultConfig(IPropertyObject** defaultConfig)
{
    OPENDAQ_PARAM_NOT_NULL(defaultConfig);

    if (this->defaultConfig.assigned())
        return this->defaultConfig.template asPtr<IPropertyObjectInternal>()->clone(defaultConfig);

    *defaultConfig = PropertyObject().detach();
    return OPENDAQ_SUCCESS;
}

END_NAMESPACE_OPENDAQ
