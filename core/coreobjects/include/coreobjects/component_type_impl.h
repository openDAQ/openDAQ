/*
 * Copyright 2022-2024 Blueberry d.o.o.
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
#include <coreobjects/property_object_factory.h>

BEGIN_NAMESPACE_OPENDAQ

template <typename Intf = IComponentType, typename... Interfaces>
class GenericComponentTypeImpl : public GenericStructImpl<Intf, IStruct, Interfaces...>
{
public:
    explicit GenericComponentTypeImpl(const StructTypePtr& type,
                                      const StringPtr& id,
                                      const StringPtr& name,
                                      const StringPtr& description,
                                      const FunctionPtr& createDefaultConfigCallback);

    ErrCode INTERFACE_FUNC getId(IString** id) override;
    ErrCode INTERFACE_FUNC getName(IString** name) override;
    ErrCode INTERFACE_FUNC getDescription(IString** description) override;
    ErrCode INTERFACE_FUNC createDefaultConfig(IPropertyObject** defaultConfig) override;

protected:
    StringPtr id;
    StringPtr name;
    StringPtr description;
    FunctionPtr createDefaultConfigCallback;

    void initCreateDefaultConfig(const FunctionPtr& createDefaultConfigCallback);
};

template <class Intf, class... Interfaces>
GenericComponentTypeImpl<Intf, Interfaces...>::GenericComponentTypeImpl(const StructTypePtr& type,
                                                                        const StringPtr& id,
                                                                        const StringPtr& name,
                                                                        const StringPtr& description,
                                                                        const FunctionPtr& createDefaultConfigCallback)
    : GenericStructImpl<Intf, IStruct, Interfaces...>(
          type, Dict<IString, IBaseObject>({{"id", id}, {"name", name}, {"description", description}}))
    , id(id)
    , name(name)
    , description(description)
{
    initCreateDefaultConfig(createDefaultConfigCallback);
}

template <class Intf, class... Interfaces>
void GenericComponentTypeImpl<Intf, Interfaces...>::initCreateDefaultConfig(const FunctionPtr& createDefaultConfigCallback)
{
    if (createDefaultConfigCallback.assigned())
    {
        this->createDefaultConfigCallback = createDefaultConfigCallback;
        return;
    }

    this->createDefaultConfigCallback = [](IBaseObject* /*params*/, IBaseObject** result)
    {
        auto obj = PropertyObject();
        *result = obj.detach();
        return OPENDAQ_SUCCESS;
    };
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

    *defaultConfig = nullptr;

    if (createDefaultConfigCallback.assigned())
    {
        daq::BaseObjectPtr callResult;
        auto errCode = createDefaultConfigCallback->call(nullptr, &callResult);
        if (OPENDAQ_FAILED(errCode))
            return errCode;

        if (callResult.assigned())
        {
            PropertyObjectPtr callResultPropObj = callResult.asPtrOrNull<IPropertyObject>();
            if (!callResultPropObj.assigned())
                return OPENDAQ_ERR_CONVERSIONFAILED;

            *defaultConfig = callResultPropObj.detach();
        }
    }
    return OPENDAQ_SUCCESS;
}

END_NAMESPACE_OPENDAQ
