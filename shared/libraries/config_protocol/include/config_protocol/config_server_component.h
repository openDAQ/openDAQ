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
#include <opendaq/component_private_ptr.h>

#include <opendaq/component_holder_factory.h>
#include <opendaq/search_filter_factory.h>

namespace daq::config_protocol
{

class ConfigServerComponent
{
public:
    // Property object methods
    static BaseObjectPtr getPropertyValue(const RpcContext& context, const PropertyObjectPtr& component, const ParamsDictPtr& params);
    static BaseObjectPtr setPropertyValue(const RpcContext& context, const PropertyObjectPtr& component, const ParamsDictPtr& params);
    static BaseObjectPtr setProtectedPropertyValue(const RpcContext& context, const PropertyObjectPtr& component, const ParamsDictPtr& params);
    static BaseObjectPtr clearPropertyValue(const RpcContext& context, const PropertyObjectPtr& component, const ParamsDictPtr& params);
    static BaseObjectPtr clearProtectedPropertyValue(const RpcContext& context, const PropertyObjectPtr& component, const ParamsDictPtr& params);
    static BaseObjectPtr getSuggestedValues(const RpcContext& context, const PropertyObjectPtr& component, const ParamsDictPtr& params);
    static BaseObjectPtr getSelectionValues(const RpcContext& context, const PropertyObjectPtr& component, const ParamsDictPtr& params);
    static BaseObjectPtr callProperty(const RpcContext& context, const PropertyObjectPtr& component, const ParamsDictPtr& params);

    // Component methods
    static BaseObjectPtr beginUpdate(const RpcContext& context, const ComponentPtr& component, const ParamsDictPtr& params);
    static BaseObjectPtr endUpdate(const RpcContext& context, const ComponentPtr& component, const ParamsDictPtr& params);
    static BaseObjectPtr setAttributeValue(const RpcContext& context, const ComponentPtr& component, const ParamsDictPtr& params);
    static BaseObjectPtr update(const RpcContext& context, const ComponentPtr& component, const ParamsDictPtr& params);
    static BaseObjectPtr getAvailableFunctionBlockTypes(const RpcContext& context, const ComponentPtr& component, const ParamsDictPtr& params);
    static BaseObjectPtr addFunctionBlock(const RpcContext& context, const ComponentPtr& component, const ParamsDictPtr& params);
    static BaseObjectPtr removeFunctionBlock(const RpcContext& context, const ComponentPtr& component, const ParamsDictPtr& params);
    static BaseObjectPtr getComponentConfig(const RpcContext& context, const ComponentPtr& component, const ParamsDictPtr& params);
    static BaseObjectPtr serializeForUpdate(const RpcContext& context, const ComponentPtr& component, const ParamsDictPtr& params);

private:
    static void applyProps(uint16_t protocolVersion, const PropertyObjectPtr& obj, const ListPtr<IDict>& props);
    static void parseAndGetDeviceInfo(PropertyObjectPtr& component, std::string& propName);
};

inline BaseObjectPtr ConfigServerComponent::getPropertyValue(const RpcContext& context,
                                                             const PropertyObjectPtr& component,
                                                             const ParamsDictPtr& params)
{
    std::string propertyName = static_cast<std::string>(params["PropertyName"]);
    PropertyObjectPtr targetComponent = component;
    parseAndGetDeviceInfo(targetComponent, propertyName);

    const auto value = targetComponent.getPropertyValue(propertyName);
    const auto propertyParent = ConfigServerAccessControl::getFirstPropertyParent(targetComponent, propertyName);

    ConfigServerAccessControl::protectObject(propertyParent, context.user, Permission::Read);

    return value;
}

inline BaseObjectPtr ConfigServerComponent::setPropertyValue(const RpcContext& context,
                                                             const PropertyObjectPtr& component,
                                                             const ParamsDictPtr& params)
{
    ConfigServerAccessControl::protectLockedComponent(component);
    ConfigServerAccessControl::protectViewOnlyConnection(context.connectionType);

    const auto propertyValue = params["PropertyValue"];
    
    auto propertyName = static_cast<std::string>(params["PropertyName"]);
    PropertyObjectPtr targetComponent = component;
    parseAndGetDeviceInfo(targetComponent, propertyName);

    const auto propertyParent = ConfigServerAccessControl::getFirstPropertyParent(targetComponent, propertyName);

    ConfigServerAccessControl::protectObject(propertyParent, context.user, {Permission::Read, Permission::Write});

    targetComponent.setPropertyValue(propertyName, propertyValue);

    return nullptr;
}

inline BaseObjectPtr ConfigServerComponent::setProtectedPropertyValue(const RpcContext& context,
                                                                      const PropertyObjectPtr& component,
                                                                      const ParamsDictPtr& params)
{
    ConfigServerAccessControl::protectLockedComponent(component);
    ConfigServerAccessControl::protectViewOnlyConnection(context.connectionType);

    const auto propertyValue = static_cast<std::string>(params["PropertyValue"]);
    auto propertyName = static_cast<std::string>(params["PropertyName"]);
    PropertyObjectPtr targetComponent = component;
    parseAndGetDeviceInfo(targetComponent, propertyName);

    const auto propertyParent = ConfigServerAccessControl::getFirstPropertyParent(targetComponent, propertyName);

    ConfigServerAccessControl::protectObject(propertyParent, context.user, {Permission::Read, Permission::Write});

    targetComponent.asPtr<IPropertyObjectProtected>(true).setProtectedPropertyValue(propertyName, propertyValue);

    return nullptr;
}

inline BaseObjectPtr ConfigServerComponent::clearPropertyValue(const RpcContext& context,
                                                               const PropertyObjectPtr& component,
                                                               const ParamsDictPtr& params)
{
    ConfigServerAccessControl::protectLockedComponent(component);
    ConfigServerAccessControl::protectViewOnlyConnection(context.connectionType);

    auto propertyName = static_cast<std::string>(params["PropertyName"]);
    PropertyObjectPtr targetComponent = component;
    parseAndGetDeviceInfo(targetComponent, propertyName);

    const auto propertyParent = ConfigServerAccessControl::getFirstPropertyParent(targetComponent, propertyName);

    ConfigServerAccessControl::protectObject(propertyParent, context.user, {Permission::Read, Permission::Write});

    targetComponent.clearPropertyValue(propertyName);

    return nullptr;
}

inline BaseObjectPtr ConfigServerComponent::clearProtectedPropertyValue(const RpcContext& context,
                                                                        const PropertyObjectPtr& component,
                                                                        const ParamsDictPtr& params)
{
    ConfigServerAccessControl::protectLockedComponent(component);
    ConfigServerAccessControl::protectViewOnlyConnection(context.connectionType);
    
    auto propertyName = static_cast<std::string>(params["PropertyName"]);
    PropertyObjectPtr targetComponent = component;
    parseAndGetDeviceInfo(targetComponent, propertyName);

    const auto propertyParent = ConfigServerAccessControl::getFirstPropertyParent(targetComponent, propertyName);

    ConfigServerAccessControl::protectObject(propertyParent, context.user, {Permission::Read, Permission::Write});

    targetComponent.asPtr<IPropertyObjectProtected>().clearProtectedPropertyValue(propertyName);

    return nullptr;
}

inline BaseObjectPtr ConfigServerComponent::getSuggestedValues(const RpcContext& context,
                                                               const PropertyObjectPtr& component,
                                                               const ParamsDictPtr& params)
{
    auto propertyName = static_cast<std::string>(params["PropertyName"]);
    PropertyObjectPtr targetComponent = component;
    parseAndGetDeviceInfo(targetComponent, propertyName);

    const auto propertyParent = ConfigServerAccessControl::getFirstPropertyParent(targetComponent, propertyName);

    ConfigServerAccessControl::protectObject(propertyParent, context.user, Permission::Read);

    return targetComponent.getProperty(propertyName).getSuggestedValues();
}

inline BaseObjectPtr ConfigServerComponent::getSelectionValues(const RpcContext& context,
                                                               const PropertyObjectPtr& component,
                                                               const ParamsDictPtr& params)
{
    auto propertyName = static_cast<std::string>(params["PropertyName"]);
    PropertyObjectPtr targetComponent = component;
    parseAndGetDeviceInfo(targetComponent, propertyName);

    const auto propertyParent = ConfigServerAccessControl::getFirstPropertyParent(targetComponent, propertyName);

    ConfigServerAccessControl::protectObject(propertyParent, context.user, Permission::Read);

    return targetComponent.getProperty(propertyName).getSelectionValues();
}

inline BaseObjectPtr ConfigServerComponent::callProperty(const RpcContext& context,
                                                         const PropertyObjectPtr& component,
                                                         const ParamsDictPtr& params)
{
    auto propertyName = static_cast<std::string>(params["PropertyName"]);
    PropertyObjectPtr targetComponent = component;
    parseAndGetDeviceInfo(targetComponent, propertyName);

    BaseObjectPtr callParams = params.getOrDefault("Params");

    const auto prop = targetComponent.getProperty(propertyName);
    const auto propValue = targetComponent.getPropertyValue(propertyName);
    const auto propertyParent = ConfigServerAccessControl::getFirstPropertyParent(targetComponent, propertyName);

    ConfigServerAccessControl::protectObject(propertyParent, context.user, {Permission::Read, Permission::Execute});

    const auto propValueCoreType = propValue.getCoreType();

    switch (propValueCoreType)
    {
        case CoreType::ctProc:
        case CoreType::ctFunc:
            break;
        default:
            DAQ_THROW_EXCEPTION(InvalidPropertyException, "Property not callable");
    }

    if (!prop.getCallableInfo().isConst())
    {
        ConfigServerAccessControl::protectLockedComponent(targetComponent);
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
            DAQ_THROW_EXCEPTION(NotSupportedException);

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
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Attribute not available or not supported via native config protocol");

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

inline void ConfigServerComponent::parseAndGetDeviceInfo(PropertyObjectPtr& component, std::string& propName)

{
    const std::string prefix = "DaqDeviceInfo";
    if (propName.find(prefix) == std::string::npos)
        return;

    propName = propName.substr(prefix.size() + 1);
    component = component.asPtr<IDevice>(true).getInfo();
}

inline BaseObjectPtr ConfigServerComponent::getAvailableFunctionBlockTypes(const RpcContext& context,
                                                                           const ComponentPtr& component,
                                                                           const ParamsDictPtr& /*params*/)
{
    ConfigServerAccessControl::protectObject(component, context.user, Permission::Read);

    BaseObjectPtr fbTypes;
    if (const auto device = component.asPtrOrNull<IDevice>(true); device.assigned())
        fbTypes = device.getAvailableFunctionBlockTypes();
    else if (const auto fb = component.asPtrOrNull<IFunctionBlock>(true); fb.assigned())
        fbTypes = fb.getAvailableFunctionBlockTypes();
    else
        DAQ_THROW_EXCEPTION(InvalidStateException, "Component is not a device or function block");

    return fbTypes;
}

inline BaseObjectPtr ConfigServerComponent::addFunctionBlock(const RpcContext& context,
                                                             const ComponentPtr& component,
                                                             const ParamsDictPtr& params)
{
    ConfigServerAccessControl::protectLockedComponent(component);
    ConfigServerAccessControl::protectObject(component, context.user, {Permission::Read, Permission::Write});
    ConfigServerAccessControl::protectViewOnlyConnection(context.connectionType);

    const auto fbTypeId = params.get("TypeId");
    PropertyObjectPtr config = params.getOrDefault("Config");

    BaseObjectPtr fbNested;
    if (const auto device = component.asPtrOrNull<IDevice>(true); device.assigned())
        fbNested = device.addFunctionBlock(fbTypeId, config);
    else if (const auto fb = component.asPtrOrNull<IFunctionBlock>(true); fb.assigned())
        fbNested = fb.addFunctionBlock(fbTypeId, config);
    else
        DAQ_THROW_EXCEPTION(InvalidStateException, "Component is not a device or function block");

    return ComponentHolder(fbNested);
}

inline BaseObjectPtr ConfigServerComponent::removeFunctionBlock(const RpcContext& context,
                                                                const ComponentPtr& component,
                                                                const ParamsDictPtr& params)
{
    ConfigServerAccessControl::protectLockedComponent(component);
    ConfigServerAccessControl::protectObject(component, context.user, {Permission::Read, Permission::Write});
    ConfigServerAccessControl::protectViewOnlyConnection(context.connectionType);

    const auto localId = params.get("LocalId");

    const auto checkFb = [](const ListPtr<IFunctionBlock>& fbs)
    {
        auto fbsCount = fbs.getCount();
        if (fbsCount == 0)
            DAQ_THROW_EXCEPTION(NotFoundException, "Function block not found");

        if (fbsCount > 1)
            DAQ_THROW_EXCEPTION(InvalidStateException, "Duplicate function block");
    };

    ListPtr<IFunctionBlock> fbs;
    if (const auto device = component.asPtrOrNull<IDevice>(true); device.assigned())
    {
        fbs = device.getFunctionBlocks(search::LocalId(localId));
        checkFb(fbs);
        device.removeFunctionBlock(fbs[0]);
        
    }
    else if (const auto fb = component.asPtrOrNull<IFunctionBlock>(true); fb.assigned())
    {
        fbs = fb.getFunctionBlocks(search::LocalId(localId));
        checkFb(fbs);
        fb.removeFunctionBlock(fbs[0]);
    }
    else
        DAQ_THROW_EXCEPTION(InvalidStateException, "Component is not a device or function block");

    return nullptr;
}

inline BaseObjectPtr ConfigServerComponent::getComponentConfig(const RpcContext& context,
                                                               const ComponentPtr& component,
                                                               const ParamsDictPtr& params)
{
    ConfigServerAccessControl::protectObject(component, context.user, Permission::Read);

    if (const auto & componentPrivate = component.asPtrOrNull<IComponentPrivate>(true); componentPrivate.assigned())
        return componentPrivate.getComponentConfig();
    return nullptr;
}

inline BaseObjectPtr ConfigServerComponent::serializeForUpdate(const RpcContext& context,
                                                               const ComponentPtr& component,
                                                               const ParamsDictPtr& params)
{
    ConfigServerAccessControl::protectObject(component, context.user, Permission::Read);
    if (const auto & updatable = component.asPtrOrNull<IUpdatable>(true); updatable.assigned())
    {
        auto serializer = JsonSerializer(True);

        checkErrorInfo(updatable->serializeForUpdate(serializer));

        StringPtr configuration;
        checkErrorInfo(serializer->getOutput(&configuration));
        return configuration;
    }
    return nullptr;
}

}
