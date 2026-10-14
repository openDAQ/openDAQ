/*
 * Copyright 2022-2026 openDAQ d.o.o.
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
#include <opendaq/component_type.h>
#include <coretypes/string_ptr.h>
#include <coretypes/function_ptr.h>
#include <coretypes/validation.h>
#include <coreobjects/property_object_factory.h>
#include <coretypes/struct_impl.h>
#include <coretypes/struct_type_factory.h>
#include <coreobjects/property_object_internal_ptr.h>
#include <opendaq/module_info_ptr.h>
#include <opendaq/component_type_private.h>

BEGIN_NAMESPACE_OPENDAQ

template <typename Intf = IComponentType, typename... Interfaces>
class GenericComponentTypeImpl : public GenericStructImpl<Intf, IStruct, IComponentTypePrivate, Interfaces...>
{
public:
    explicit GenericComponentTypeImpl(const StructTypePtr& type,
                                      const StringPtr& id,
                                      const StringPtr& name,
                                      const StringPtr& description,
                                      const PropertyObjectPtr& defaultConfig,
                                      const DictPtr<IString, IBaseObject>& extraFields = nullptr);

    ErrCode INTERFACE_FUNC getId(IString** id) override;
    ErrCode INTERFACE_FUNC getName(IString** name) override;
    ErrCode INTERFACE_FUNC getDescription(IString** description) override;
    ErrCode INTERFACE_FUNC createDefaultConfig(IPropertyObject** defaultConfig) override;
    ErrCode INTERFACE_FUNC getModuleInfo(IModuleInfo** moduleInfo) override;

    // IComponentTypePrivate
    ErrCode INTERFACE_FUNC setModuleInfo(IModuleInfo* info) override;

private:
    static DictPtr<IString, IBaseObject> createFields(const StructTypePtr& type,
                                                      const StringPtr& id,
                                                      const StringPtr& name,
                                                      const StringPtr& description,
                                                      const DictPtr<IString, IBaseObject>& extraFields);

protected:
    StringPtr id;
    StringPtr name;
    StringPtr description;
    PropertyObjectPtr defaultConfig;
    ModuleInfoPtr moduleInfo;
};

template <class Intf, class... Interfaces>
DictPtr<IString, IBaseObject> GenericComponentTypeImpl<Intf, Interfaces...>::createFields([[maybe_unused]] const StructTypePtr& type,
    const StringPtr& id,
    const StringPtr& name,
    const StringPtr& description,
    const DictPtr<IString, IBaseObject>& extraFields)
{
    auto fields = Dict<IString, IBaseObject>({{"Id", id}, {"Name", name}, {"Description", description}});

    if (extraFields.assigned())
    {
        for (const auto& [key, value] : extraFields)
        {
            // Id, Name and Description are owned by the base and must not be overwritten by a derived type
            if (fields.hasKey(key))
                DAQ_THROW_EXCEPTION(InvalidParameterException, "Extra component type field \"{}\" is reserved", key);

            fields.set(key, value);
        }
    }

#ifndef NDEBUG
    // GenericStructImpl does not validate the fields against the struct type, so a misspelled
    // or misordered extra field would otherwise go unnoticed
    if (fields.getKeyList() != type.getFieldNames())
        DAQ_THROW_EXCEPTION(InvalidTypeException, "Component type fields do not match the fields of struct type \"{}\"", type.getName());
#endif

    return fields;
}

template <class Intf, class... Interfaces>
GenericComponentTypeImpl<Intf, Interfaces...>::GenericComponentTypeImpl(const StructTypePtr& type,
                                                                        const StringPtr& id,
                                                                        const StringPtr& name,
                                                                        const StringPtr& description,
                                                                        const PropertyObjectPtr& defaultConfig,
                                                                        const DictPtr<IString, IBaseObject>& extraFields)
    : GenericStructImpl<Intf, IStruct, IComponentTypePrivate, Interfaces...>(
          type, createFields(type, id, name, description, extraFields))
    , id(id)
    , name(name)
    , description(description)
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

template <typename Intf, typename... Interfaces>
inline ErrCode GenericComponentTypeImpl<Intf, Interfaces...>::setModuleInfo(IModuleInfo* info)
{
    this->moduleInfo = info;

    return OPENDAQ_SUCCESS;
}

template <typename Intf, typename... Interfaces>
inline ErrCode INTERFACE_FUNC GenericComponentTypeImpl<Intf, Interfaces...>::getModuleInfo(IModuleInfo** moduleInfo)
{
    OPENDAQ_PARAM_NOT_NULL(moduleInfo);

    *moduleInfo = this->moduleInfo.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

END_NAMESPACE_OPENDAQ
