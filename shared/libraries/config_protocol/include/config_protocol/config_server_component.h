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
#include <opendaq/device_ptr.h>
#include <coreobjects/property_object_protected.h>
#include <config_protocol/config_server_access_control.h>

namespace daq::config_protocol
{

class ConfigServerComponent
{
public:
    static BaseObjectPtr getPropertyValue(const ComponentPtr& component, const ParamsDictPtr& params, const UserPtr& user);
    static BaseObjectPtr setPropertyValue(const ComponentPtr& component, const ParamsDictPtr& params, const UserPtr& user);
    static BaseObjectPtr setProtectedPropertyValue(const ComponentPtr& component, const ParamsDictPtr& params, const UserPtr& user);
    static BaseObjectPtr clearPropertyValue(const ComponentPtr& component, const ParamsDictPtr& params, const UserPtr& user);
    static BaseObjectPtr callProperty(const ComponentPtr& component, const ParamsDictPtr& params, const UserPtr& user);
    static BaseObjectPtr beginUpdate(const ComponentPtr& component, const ParamsDictPtr& params, const UserPtr& user);
    static BaseObjectPtr endUpdate(const ComponentPtr& component, const ParamsDictPtr& params, const UserPtr& user);
    static BaseObjectPtr setAttributeValue(const ComponentPtr& component, const ParamsDictPtr& params, const UserPtr& user);
    static BaseObjectPtr update(const ComponentPtr& component, const ParamsDictPtr& params, const UserPtr& user);
};

inline BaseObjectPtr ConfigServerComponent::getPropertyValue(const ComponentPtr& component,
                                                             const ParamsDictPtr& params,
                                                             const UserPtr& user)
{
    const auto propertyName = static_cast<std::string>(params["PropertyName"]);
    const auto value = component.getPropertyValue(propertyName);
    const auto propertyParent = ConfigServerAccessControl::getFirstPropertyParent(component, propertyName);

    ConfigServerAccessControl::protectObject(propertyParent, user, Permission::Read);

    return value;
}

inline BaseObjectPtr ConfigServerComponent::setPropertyValue(const ComponentPtr& component,
                                                             const ParamsDictPtr& params,
                                                             const UserPtr& user)
{
    const auto propertyName = static_cast<std::string>(params["PropertyName"]);
    const auto propertyValue = params["PropertyValue"];
    const auto propertyParent = ConfigServerAccessControl::getFirstPropertyParent(component, propertyName);

    ConfigServerAccessControl::protectObject(propertyParent, user, {Permission::Read, Permission::Write});

    component.setPropertyValue(propertyName, propertyValue);

    return nullptr;
}

inline BaseObjectPtr ConfigServerComponent::setProtectedPropertyValue(const ComponentPtr& component,
                                                                      const ParamsDictPtr& params,
                                                                      const UserPtr& user)
{
    const auto propertyName = static_cast<std::string>(params["PropertyName"]);
    const auto propertyValue = static_cast<std::string>(params["PropertyValue"]);
    const auto propertyParent = ConfigServerAccessControl::getFirstPropertyParent(component, propertyName);

    ConfigServerAccessControl::protectObject(propertyParent, user, {Permission::Read, Permission::Write});

    component.asPtr<IPropertyObjectProtected>(true).setProtectedPropertyValue(propertyName, propertyValue);

    return nullptr;
}

inline BaseObjectPtr ConfigServerComponent::clearPropertyValue(const ComponentPtr& component,
                                                               const ParamsDictPtr& params,
                                                               const UserPtr& user)
{
    const auto propertyName = static_cast<std::string>(params["PropertyName"]);
    const auto propertyParent = ConfigServerAccessControl::getFirstPropertyParent(component, propertyName);

    ConfigServerAccessControl::protectObject(propertyParent, user, {Permission::Read, Permission::Write});

    component.clearPropertyValue(propertyName);

    return nullptr;
}

inline BaseObjectPtr ConfigServerComponent::callProperty(const ComponentPtr& component, const ParamsDictPtr& params, const UserPtr& user)
{
    const auto propertyName = static_cast<std::string>(params["PropertyName"]);
    BaseObjectPtr callParams;
    if (params.hasKey("Params"))
        callParams = params.get("Params");

    const auto propValue = component.getPropertyValue(propertyName);
    const auto propertyParent = ConfigServerAccessControl::getFirstPropertyParent(component, propertyName);

    ConfigServerAccessControl::protectObject(propertyParent, user, {Permission::Read, Permission::Execute});

    const auto propValueCoreType = propValue.getCoreType();

    if (propValueCoreType == CoreType::ctProc)
    {
        propValue.dispatch(callParams);
        return nullptr;
    }

    if (propValueCoreType == CoreType::ctFunc)
    {
        BaseObjectPtr result;
        checkErrorInfo(propValue.asPtr<IFunction>()->call(callParams, &result));
        return result;
    }

    throw InvalidPropertyException("Property not callable");
}

inline BaseObjectPtr ConfigServerComponent::beginUpdate(const ComponentPtr& component, const ParamsDictPtr& params, const UserPtr& user)
{
    ConfigServerAccessControl::protectObject(component, user, {Permission::Read, Permission::Write});

    if (params.hasKey("Path"))
    {
        const PropertyObjectPtr obj = component.getPropertyValue(params.get("Path"));
        obj.beginUpdate();
    }
    else
        component.beginUpdate();
    return nullptr;
}

inline BaseObjectPtr ConfigServerComponent::endUpdate(const ComponentPtr& component, const ParamsDictPtr& params, const UserPtr& user)
{
    ConfigServerAccessControl::protectObject(component, user, {Permission::Read, Permission::Write});

    if (params.hasKey("Path"))
    {
        const PropertyObjectPtr obj = component.getPropertyValue(params.get("Path"));
        obj.endUpdate();
    }
    else
        component.endUpdate();
    return nullptr;
}

inline BaseObjectPtr ConfigServerComponent::setAttributeValue(const ComponentPtr& component,
                                                              const ParamsDictPtr& params,
                                                              const UserPtr& user)
{
    ConfigServerAccessControl::protectObject(component, user, {Permission::Read, Permission::Write});

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

inline BaseObjectPtr ConfigServerComponent::update(const ComponentPtr& component, const ParamsDictPtr& params, const UserPtr& user)
{
    ConfigServerAccessControl::protectObject(component, user, {Permission::Read, Permission::Write});

    const auto serializedString = static_cast<std::string>(params["Serialized"]);
    const auto path = static_cast<std::string>(params["Path"]);

    UpdatablePtr updatable;
    if (!path.empty())
        updatable = component.getPropertyValue(path);
    else
        updatable = component;

    const auto deserializer = JsonDeserializer();
    deserializer.update(updatable, serializedString);

    return nullptr;
}

}
