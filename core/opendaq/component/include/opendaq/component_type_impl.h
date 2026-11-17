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
#include <opendaq/authentication_method_factory.h>
#include <opendaq/authentication_config_ptr.h>
#include <opendaq/transport_option_ptr.h>
#include <coretypes/listobject_factory.h>
#include <unordered_set>

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
                                      const DictPtr<IString, IAuthenticationMethod>& supportedAuthenticationMethods = Dict<IString, IAuthenticationMethod>(),
                                      const ListPtr<ITransportOption>& transportOptions = List<ITransportOption>());

    explicit GenericComponentTypeImpl(const StructTypePtr& type,
                                      const StringPtr& id,
                                      const StringPtr& name,
                                      const StringPtr& description,
                                      const StringPtr& prefix,
                                      const PropertyObjectPtr& defaultConfig,
                                      const DictPtr<IString, IAuthenticationMethod>& supportedAuthenticationMethods = Dict<IString, IAuthenticationMethod>(),
                                      const ListPtr<ITransportOption>& transportOptions = List<ITransportOption>());

    ErrCode INTERFACE_FUNC getId(IString** id) override;
    ErrCode INTERFACE_FUNC getName(IString** name) override;
    ErrCode INTERFACE_FUNC getDescription(IString** description) override;
    ErrCode INTERFACE_FUNC createDefaultConfig(IPropertyObject** defaultConfig) override;
    ErrCode INTERFACE_FUNC getModuleInfo(IModuleInfo** moduleInfo) override;
    ErrCode INTERFACE_FUNC createAuthenticationConfig(IAuthenticationConfig** config) override;
    ErrCode INTERFACE_FUNC getTransportOptions(IList** transportOptions) override;

    // IComponentTypePrivate
    ErrCode INTERFACE_FUNC setModuleInfo(IModuleInfo* info) override;
    ErrCode INTERFACE_FUNC getSupportedAuthenticationMethods(IDict** authenticationMethods) override;

protected:
    StringPtr id;
    StringPtr name;
    StringPtr description;
    StringPtr prefix;
    PropertyObjectPtr defaultConfig;
    ModuleInfoPtr moduleInfo;
    DictPtr<IString, IAuthenticationMethod> supportedAuthenticationMethods;
    ListPtr<ITransportOption> transportOptions;

private:
    static ListPtr<ITransportOption> FrozenTransportOptionsCopy(const ListPtr<ITransportOption>& transportOptions);
};

template <class Intf, class... Interfaces>
GenericComponentTypeImpl<Intf, Interfaces...>::GenericComponentTypeImpl(const StructTypePtr& type,
                                                                        const StringPtr& id,
                                                                        const StringPtr& name,
                                                                        const StringPtr& description,
                                                                        const PropertyObjectPtr& defaultConfig,
                                                                        const DictPtr<IString, IAuthenticationMethod>& supportedAuthenticationMethods,
                                                                        const ListPtr<ITransportOption>& transportOptions)
    : GenericStructImpl<Intf, IStruct, IComponentTypePrivate, Interfaces...>(type, Dict<IString, IBaseObject>({{"Id", id}, {"Name", name}, {"Description", description}}))
    , id(id)
    , name(name)
    , description(description)
    , prefix("")
    , defaultConfig(defaultConfig)
    , supportedAuthenticationMethods(supportedAuthenticationMethods)
    , transportOptions(FrozenTransportOptionsCopy(transportOptions))
{
}

template <typename Intf, typename... Interfaces>
GenericComponentTypeImpl<Intf, Interfaces...>::GenericComponentTypeImpl(const StructTypePtr& type,
                                                                        const StringPtr& id,
                                                                        const StringPtr& name,
                                                                        const StringPtr& description,
                                                                        const StringPtr& prefix,
                                                                        const PropertyObjectPtr& defaultConfig,
                                                                        const DictPtr<IString, IAuthenticationMethod>& supportedAuthenticationMethods,
                                                                        const ListPtr<ITransportOption>& transportOptions)
    : GenericStructImpl<Intf, IStruct, IComponentTypePrivate, Interfaces...>(type, Dict<IString, IBaseObject>({{"Id", id}, {"Name", name}, {"Description", description}, {"Prefix", prefix}}))
    , id(id)
    , name(name)
    , description(description)
    , prefix(prefix)
    , defaultConfig(defaultConfig)
    , supportedAuthenticationMethods(supportedAuthenticationMethods)
    , transportOptions(FrozenTransportOptionsCopy(transportOptions))
{
}

template <class Intf, class... Interfaces>
ListPtr<ITransportOption> GenericComponentTypeImpl<Intf, Interfaces...>::FrozenTransportOptionsCopy(
    const ListPtr<ITransportOption>& transportOptions)
{
    auto copy = List<ITransportOption>();
    if (transportOptions.assigned())
    {
        std::unordered_set<std::string> names;
        IList* items = transportOptions;
        for (const auto& item : ListPtr<IBaseObject>::Borrow(items))
        {
            const auto option = item.asPtrOrNull<ITransportOption, TransportOptionPtr>();
            if (!option.assigned())
                DAQ_THROW_EXCEPTION(InvalidTypeException, "Transport options must be ITransportOption objects");

            if (!names.insert(option.getName().toStdString()).second)
                DAQ_THROW_EXCEPTION(InvalidValueException, "Transport option \"{}\" is listed more than once", option.getName());

            copy.pushBack(option);
        }
    }

    copy.freeze();
    return copy;
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

template <class Intf, class... Interfaces>
ErrCode GenericComponentTypeImpl<Intf, Interfaces...>::createAuthenticationConfig(IAuthenticationConfig** config)
{
    OPENDAQ_PARAM_NOT_NULL(config);

    if (this->supportedAuthenticationMethods.getCount() == 0)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOT_SUPPORTED, "This component type supports no authentication method");

    return daqTry([&]()
    {
        AuthenticationConfigPtr configPtr = AuthenticationConfig_Create(this->supportedAuthenticationMethods);
        *config = configPtr.detach();
        return OPENDAQ_SUCCESS;
    });
}

template <typename Intf, typename... Interfaces>
inline ErrCode GenericComponentTypeImpl<Intf, Interfaces...>::setModuleInfo(IModuleInfo* info)
{
    this->moduleInfo = info;

    return OPENDAQ_SUCCESS;
}

template <class Intf, class... Interfaces>
ErrCode GenericComponentTypeImpl<Intf, Interfaces...>::getSupportedAuthenticationMethods(IDict** authenticationMethods)
{
    OPENDAQ_PARAM_NOT_NULL(authenticationMethods);

    *authenticationMethods = this->supportedAuthenticationMethods.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

template <typename Intf, typename... Interfaces>
inline ErrCode INTERFACE_FUNC GenericComponentTypeImpl<Intf, Interfaces...>::getModuleInfo(IModuleInfo** moduleInfo)
{
    OPENDAQ_PARAM_NOT_NULL(moduleInfo);

    *moduleInfo = this->moduleInfo.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

template <typename Intf, typename... Interfaces>
ErrCode GenericComponentTypeImpl<Intf, Interfaces...>::getTransportOptions(IList** transportOptions)
{
    OPENDAQ_PARAM_NOT_NULL(transportOptions);

    *transportOptions = this->transportOptions.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

END_NAMESPACE_OPENDAQ
