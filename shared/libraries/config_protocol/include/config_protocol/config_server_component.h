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
#include <opendaq/device_ptr.h>
#include <coreobjects/property_object_protected.h>
#include <config_protocol/config_server_access_control.h>
#include <opendaq/update_parameters_factory.h>

namespace daq::config_protocol
{

class ConfigServerComponent
{
public:
    static BaseObjectPtr getPropertyValue(const RpcContext& context, const ComponentPtr& component, const ParamsDictPtr& params);
    static BaseObjectPtr setPropertyValue(const RpcContext& context, const ComponentPtr& component, const ParamsDictPtr& params);
    static BaseObjectPtr setProtectedPropertyValue(const RpcContext& context, const ComponentPtr& component, const ParamsDictPtr& params);
    static BaseObjectPtr clearPropertyValue(const RpcContext& context, const ComponentPtr& component, const ParamsDictPtr& params);
    static BaseObjectPtr callProperty(const RpcContext& context, const ComponentPtr& component, const ParamsDictPtr& params);
    static BaseObjectPtr beginUpdate(const RpcContext& context, const ComponentPtr& component, const ParamsDictPtr& params);
    static BaseObjectPtr endUpdate(const RpcContext& context, const ComponentPtr& component, const ParamsDictPtr& params);
    static BaseObjectPtr setAttributeValue(const RpcContext& context, const ComponentPtr& component, const ParamsDictPtr& params);
    static BaseObjectPtr update(const RpcContext& context, const ComponentPtr& component, const ParamsDictPtr& params);

private:
    static void applyProps(uint16_t protocolVersion, const PropertyObjectPtr& obj, const ListPtr<IDict>& props);
};

inline BaseObjectPtr ConfigServerComponent::getPropertyValue(const RpcContext& context,
                                                             const ComponentPtr& component,
                                                             const ParamsDictPtr& params)
{
    const auto propertyName = static_cast<std::string>(params["PropertyName"]);
    const auto value = component.getPropertyValue(propertyName);
    const auto propertyParent = ConfigServerAccessControl::getFirstPropertyParent(component, propertyName);

    ConfigServerAccessControl::protectObject(propertyParent, context.user, Permission::Read);

    return value;
}

inline BaseObjectPtr ConfigServerComponent::setPropertyValue(const RpcContext& context,
                                                             const ComponentPtr& component,
                                                             const ParamsDictPtr& params)
{
    ConfigServerAccessControl::protectLockedComponent(component);
    ConfigServerAccessControl::protectViewOnlyConnection(context.connectionType);

    const auto propertyName = static_cast<std::string>(params["PropertyName"]);
    const auto propertyValue = params["PropertyValue"];
    const auto propertyParent = ConfigServerAccessControl::getFirstPropertyParent(component, propertyName);

    ConfigServerAccessControl::protectObject(propertyParent, context.user, {Permission::Read, Permission::Write});

    component.setPropertyValue(propertyName, propertyValue);

    return nullptr;
}

inline BaseObjectPtr ConfigServerComponent::setProtectedPropertyValue(const RpcContext& context,
                                                                      const ComponentPtr& component,
                                                                      const ParamsDictPtr& params)
{
    ConfigServerAccessControl::protectLockedComponent(component);
    ConfigServerAccessControl::protectViewOnlyConnection(context.connectionType);

    const auto propertyName = static_cast<std::string>(params["PropertyName"]);
    const auto propertyValue = static_cast<std::string>(params["PropertyValue"]);
    const auto propertyParent = ConfigServerAccessControl::getFirstPropertyParent(component, propertyName);

    ConfigServerAccessControl::protectObject(propertyParent, context.user, {Permission::Read, Permission::Write});

    component.asPtr<IPropertyObjectProtected>(true).setProtectedPropertyValue(propertyName, propertyValue);

    return nullptr;
}

inline BaseObjectPtr ConfigServerComponent::clearPropertyValue(const RpcContext& context,
                                                               const ComponentPtr& component,
                                                               const ParamsDictPtr& params)
{
    ConfigServerAccessControl::protectLockedComponent(component);
    ConfigServerAccessControl::protectViewOnlyConnection(context.connectionType);

    const auto propertyName = static_cast<std::string>(params["PropertyName"]);
    const auto propertyParent = ConfigServerAccessControl::getFirstPropertyParent(component, propertyName);

    ConfigServerAccessControl::protectObject(propertyParent, context.user, {Permission::Read, Permission::Write});

    component.clearPropertyValue(propertyName);

    return nullptr;
}

inline BaseObjectPtr ConfigServerComponent::callProperty(const RpcContext& context,
                                                         const ComponentPtr& component,
                                                         const ParamsDictPtr& params)
{
    const auto propertyName = static_cast<std::string>(params["PropertyName"]);
    BaseObjectPtr callParams;
    if (params.hasKey("Params"))
        callParams = params.get("Params");

    const auto prop = component.getProperty(propertyName);
    const auto propValue = component.getPropertyValue(propertyName);
    const auto propertyParent = ConfigServerAccessControl::getFirstPropertyParent(component, propertyName);

    ConfigServerAccessControl::protectObject(propertyParent, context.user, {Permission::Read, Permission::Execute});

    const auto propValueCoreType = propValue.getCoreType();

    switch (propValueCoreType)
    {
        case CoreType::ctProc:
        case CoreType::ctFunc:
            break;
        default:
            throw InvalidPropertyException("Property not callable");
    }

    if (!prop.getCallableInfo().isConst())
    {
        ConfigServerAccessControl::protectLockedComponent(component);
        ConfigServerAccessControl::protectViewOnlyConnection(context.connectionType);
    }

    if (propValueCoreType == CoreType::ctFunc)
    {
        BaseObjectPtr result;
        checkErrorInfo(propValue.asPtr<IFunction>()->call(callParams, &result));
        return result;
    }

    propValue.dispatch(callParams);
    return nullptr;
}

inline BaseObjectPtr ConfigServerComponent::beginUpdate(const RpcContext& context,
                                                        const ComponentPtr& component,
                                                        const ParamsDictPtr& params)
{
    ConfigServerAccessControl::protectLockedComponent(component);
    ConfigServerAccessControl::protectObject(component, context.user, {Permission::Read, Permission::Write});
    ConfigServerAccessControl::protectViewOnlyConnection(context.connectionType);

    if (params.hasKey("Path"))
    {
        const PropertyObjectPtr obj = component.getPropertyValue(params.get("Path"));
        obj.beginUpdate();
    }
    else
        component.beginUpdate();
    return nullptr;
}

inline BaseObjectPtr ConfigServerComponent::endUpdate(const RpcContext& context, const ComponentPtr& component, const ParamsDictPtr& params)
{
    ConfigServerAccessControl::protectLockedComponent(component);
    ConfigServerAccessControl::protectObject(component, context.user, {Permission::Read, Permission::Write});
    ConfigServerAccessControl::protectViewOnlyConnection(context.connectionType);

    PropertyObjectPtr obj;
    if (params.hasKey("Path"))
    {
        obj = component.getPropertyValue(params.get("Path"));
    }
    else
        obj = component;

    if (params.hasKey("Props"))
    {
        if (context.protocolVersion < 1)
            throw NotSupportedException();

        const ListPtr<IDict> props = params.get("Props");
        applyProps(context.protocolVersion, obj, props);
    }

    obj.endUpdate();
    return nullptr;
}

inline BaseObjectPtr ConfigServerComponent::setAttributeValue(const RpcContext& context,
                                                              const ComponentPtr& component,
                                                              const ParamsDictPtr& params)
{
    ConfigServerAccessControl::protectLockedComponent(component);
    ConfigServerAccessControl::protectObject(component, context.user, {Permission::Read, Permission::Write});
    ConfigServerAccessControl::protectViewOnlyConnection(context.connectionType);

    const auto attributeName = static_cast<std::string>(params["AttributeName"]);
    const BaseObjectPtr attributeValue = params["AttributeValue"];

    if (attributeName == "Name")
        component.setName(attributeValue);
    else if (attributeName == "Description")
        component.setDescription(attributeValue);
    else if (attributeName == "Active")
        component.setActive(attributeValue);
    else
        throw InvalidParameterException("Attribute not available or not supported via native config protocol");

    return nullptr;
}

inline BaseObjectPtr ConfigServerComponent::update(const RpcContext& context, const ComponentPtr& component, const ParamsDictPtr& params)
{
    ConfigServerAccessControl::protectLockedComponent(component);
    ConfigServerAccessControl::protectObject(component, context.user, {Permission::Read, Permission::Write});
    ConfigServerAccessControl::protectViewOnlyConnection(context.connectionType);

    const auto serializedString = static_cast<std::string>(params["Serialized"]);
    const auto path = static_cast<std::string>(params["Path"]);

    UpdatablePtr updatable;
    if (!path.empty())
        updatable = component.getPropertyValue(path);
    else
        updatable = component;

    const auto deserializer = JsonDeserializer();
    const auto updateParams = UpdateParameters();
    updateParams.setPropertyValue("RemoteUpdate", true);
    deserializer.update(updatable, serializedString, updateParams);

    return nullptr;
}

inline void ConfigServerComponent::applyProps(uint16_t protocolVersion, const PropertyObjectPtr& obj, const ListPtr<IDict>& props)
{
    for (DictPtr<IString, IBaseObject> item: props)
    {
        const auto setValue = static_cast<Bool>(item.get("SetValue"));
        const StringPtr name = item.get("Name");
        if (setValue)
        {
            const auto value = item.get("Value");
            if (static_cast<Bool>(item.get("ProtectedAccess")))
                obj.asPtr<IPropertyObjectProtected>(true).setProtectedPropertyValue(name, value);
            else
                obj.setPropertyValue(name, value);
        }
        else
            obj.clearPropertyValue(name);
    }
}

}
