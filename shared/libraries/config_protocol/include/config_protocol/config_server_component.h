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

namespace daq::config_protocol
{

class ConfigServerComponent
{
public:
    static BaseObjectPtr getPropertyValue(uint16_t protocolVersion, const ComponentPtr& component, const ParamsDictPtr& params);
    static BaseObjectPtr setPropertyValue(uint16_t protocolVersion, const ComponentPtr& component, const ParamsDictPtr& params);
    static BaseObjectPtr setProtectedPropertyValue(uint16_t protocolVersion, const ComponentPtr& component, const ParamsDictPtr& params);
    static BaseObjectPtr clearPropertyValue(uint16_t protocolVersion, const ComponentPtr& component, const ParamsDictPtr& params);
    static BaseObjectPtr callProperty(uint16_t protocolVersion, const ComponentPtr& component, const ParamsDictPtr& params);
    static BaseObjectPtr beginUpdate(uint16_t protocolVersion, const ComponentPtr& component, const ParamsDictPtr& params);
    static BaseObjectPtr endUpdate(uint16_t protocolVersion, const ComponentPtr& component, const ParamsDictPtr& params);
    static BaseObjectPtr setAttributeValue(uint16_t protocolVersion, const ComponentPtr& component, const ParamsDictPtr& params);
    static BaseObjectPtr update(uint16_t protocolVersion, const ComponentPtr& component, const ParamsDictPtr& params);

private:

    static void applyProps(uint16_t protocolVersion, const PropertyObjectPtr& obj, const ListPtr<IDict>& props);
};

inline BaseObjectPtr ConfigServerComponent::getPropertyValue(uint16_t protocolVersion,
                                                             const ComponentPtr& component,
                                                             const ParamsDictPtr& params)
{
    const auto propertyName = static_cast<std::string>(params["PropertyName"]);

    const auto value = component.getPropertyValue(propertyName);
    return value;
}

inline BaseObjectPtr ConfigServerComponent::setPropertyValue(uint16_t protocolVersion,
                                                             const ComponentPtr& component,
                                                             const ParamsDictPtr& params)
{
    const auto propertyName = static_cast<std::string>(params["PropertyName"]);
    const auto propertyValue = params["PropertyValue"];

    component.setPropertyValue(propertyName, propertyValue);

    return nullptr;
}

inline BaseObjectPtr ConfigServerComponent::setProtectedPropertyValue(uint16_t protocolVersion,
                                                                      const ComponentPtr& component,
                                                                      const ParamsDictPtr& params)
{
    const auto propertyName = static_cast<std::string>(params["PropertyName"]);
    const auto propertyValue = static_cast<std::string>(params["PropertyValue"]);

    component.asPtr<IPropertyObjectProtected>(true).setProtectedPropertyValue(propertyName, propertyValue);

    return nullptr;
}

inline BaseObjectPtr ConfigServerComponent::clearPropertyValue(uint16_t protocolVersion,
                                                               const ComponentPtr& component,
                                                               const ParamsDictPtr& params)
{
    const auto propertyName = static_cast<std::string>(params["PropertyName"]);

    component.clearPropertyValue(propertyName);

    return nullptr;
}

inline BaseObjectPtr ConfigServerComponent::callProperty(uint16_t protocolVersion,
                                                         const ComponentPtr& component,
                                                         const ParamsDictPtr& params)
{
    const auto propertyName = static_cast<std::string>(params["PropertyName"]);
    BaseObjectPtr callParams;
    if (params.hasKey("Params"))
        callParams = params.get("Params");

    const auto propValue = component.getPropertyValue(propertyName);

    const auto propValueCoreType = propValue.getCoreType();

    if (propValueCoreType == CoreType::ctProc)
    {
        propValue.dispatch(callParams);
        return nullptr;
    }

    if (propValueCoreType == CoreType::ctFunc)
    {
        BaseObjectPtr result;
        checkErrorInfo(propValue.asPtrOrNull<IFunction>()->call(callParams, &result));
        return result;
    }

    throw InvalidPropertyException("Property not callable");
}

inline BaseObjectPtr ConfigServerComponent::beginUpdate(uint16_t protocolVersion,
                                                        const ComponentPtr& component,
                                                        const ParamsDictPtr& params)
{
    if (params.hasKey("Path"))
    {
        const PropertyObjectPtr obj = component.getPropertyValue(params.get("Path"));
        obj.beginUpdate();
    }
    else
        component.beginUpdate();
    return nullptr;
}

inline BaseObjectPtr ConfigServerComponent::endUpdate(uint16_t protocolVersion, const ComponentPtr& component, const ParamsDictPtr& params)
{
    PropertyObjectPtr obj;
    if (params.hasKey("Path"))
    {
        obj = component.getPropertyValue(params.get("Path"));
    }
    else
        obj = component;

    if (params.hasKey("Props"))
    {
        if (protocolVersion < 1)
            throw NotSupportedException();

        const ListPtr<IDict> props = params.get("Props");
        applyProps(protocolVersion, obj, props);
    }

    obj.endUpdate();
    return nullptr;
}

inline BaseObjectPtr ConfigServerComponent::setAttributeValue(uint16_t protocolVersion,
                                                              const ComponentPtr& component,
                                                              const ParamsDictPtr& params)
{
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

inline BaseObjectPtr ConfigServerComponent::update(uint16_t protocolVersion, const ComponentPtr& component, const ParamsDictPtr& params)
{
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
