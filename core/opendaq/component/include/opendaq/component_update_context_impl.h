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
#include <opendaq/component_update_context.h>
#include <opendaq/component_ptr.h>
#include <opendaq/signal_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

class ComponentUpdateContextImpl : public ImplementationOf<IComponentUpdateContext>
{
public:

    ComponentUpdateContextImpl(const ComponentPtr& curComponent, const PropertyObjectPtr& config)
        : config(populateConfig(config))
        , connections(Dict<IString, IBaseObject>())
        , signalDependencies(Dict<IString, IString>())
        , parentDependencies(List<IString>())
        , rootComponent(getRootComponent(curComponent))
    {
    }

    ErrCode INTERFACE_FUNC setInputPortConnection(IString* parentId, IString* portId, IString* signalId) override;
    ErrCode INTERFACE_FUNC getInputPortConnections(IString* parentId, IDict** connections) override;
    ErrCode INTERFACE_FUNC removeInputPortConnection(IString* parentId) override;
    ErrCode INTERFACE_FUNC getRootComponent(IComponent** rootComponent) override;
    ErrCode INTERFACE_FUNC getSignal(IString* parentId, IString* portId, ISignal** signal) override;
    ErrCode INTERFACE_FUNC setSignalDependency(IString* signalId, IString* parentId) override;

    ErrCode INTERFACE_FUNC getReAddDevicesEnabled(Bool* enabled) override;

    static PropertyObjectPtr getDefaultConfig();
    static PropertyObjectPtr populateConfig(const PropertyObjectPtr& config);

private:

    ErrCode INTERFACE_FUNC resolveSignalDependency(IString* signalId, ISignal** signal);

    static ComponentPtr getRootComponent(const ComponentPtr& curComponent);
    static StringPtr getRemoteId(const std::string& globalId);

    PropertyObjectPtr config;

    DictPtr<IString, IBaseObject> connections;
    DictPtr<IString, IString> signalDependencies;
    ListPtr<IString> parentDependencies;
    ComponentPtr rootComponent;
};

inline PropertyObjectPtr ComponentUpdateContextImpl::getDefaultConfig()
{
    auto config = PropertyObject();
    config.addProperty(BoolProperty("ReAddDevices", false));
    return config;
}

inline PropertyObjectPtr ComponentUpdateContextImpl::populateConfig(const PropertyObjectPtr& config)
{
    auto defaultConfig = getDefaultConfig();
    if (config.assigned())
    {
        for (const auto & prop : config.getAllProperties())
        {
            if (defaultConfig.hasProperty(prop.getName()))
                defaultConfig.setPropertyValue(prop.getName(), prop.getValue());
        }
    }
    return defaultConfig;
}

inline ComponentPtr ComponentUpdateContextImpl::getRootComponent(const ComponentPtr& curComponent)
{
    const auto parent = curComponent.getParent();
    if (!parent.assigned())
        return curComponent;
    return getRootComponent(parent);
}

inline ErrCode ComponentUpdateContextImpl::setInputPortConnection(IString* parentId, IString* portId, IString* signalId)
{
    if (!parentId || !portId || !signalId)
        return OPENDAQ_ERR_INVALID_ARGUMENT;

    DictPtr<IString, IString> ports;
    
    if (!connections.hasKey(parentId))
    {
        ports = Dict<IString, IString>();
        connections.set(parentId, ports);
    }
    else
    {
        ports = connections.get(parentId);
    }
    ports.set(portId, signalId);

    connections->set(portId, signalId);

    return OPENDAQ_SUCCESS;
}

inline ErrCode ComponentUpdateContextImpl::getInputPortConnections(IString* parentId, IDict** connections)
{
    if (!parentId || !connections)
        return OPENDAQ_ERR_INVALID_ARGUMENT;

    DictPtr<IString, IBaseObject> ports;
    if (!this->connections.hasKey(parentId))
    {
        ports = Dict<IString, IBaseObject>();
    }
    else
    {
        ports = this->connections.get(parentId);
    }
    *connections = ports.detach();
    return OPENDAQ_SUCCESS;
}

inline ErrCode ComponentUpdateContextImpl::removeInputPortConnection(IString* parentId)
{
    if (!parentId)
        return OPENDAQ_SUCCESS;

    connections->deleteItem(parentId);
    return OPENDAQ_SUCCESS;
}

inline ErrCode ComponentUpdateContextImpl::getRootComponent(IComponent** rootComponent)
{
    if (!rootComponent)
        return OPENDAQ_ERR_INVALID_ARGUMENT;

    *rootComponent = this->rootComponent.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

inline StringPtr ComponentUpdateContextImpl::getRemoteId(const std::string& globalId)
{
    size_t firstSlashPos = globalId.find('/');
    if (firstSlashPos == std::string::npos) 
    {
        // No slash found, return the original path
        return globalId;
    }

    // Find the position of the second slash
    size_t secondSlashPos = globalId.find('/', firstSlashPos + 1);
    if (secondSlashPos == std::string::npos) 
    {
        // Only one segment found, return an empty string
        return "";
    }

    // Erase the first segment
    return globalId.substr(secondSlashPos);
}

inline ErrCode ComponentUpdateContextImpl::getSignal(IString* parentId, IString* portId, ISignal** signal)
{
    if (parentId == nullptr)
        return OPENDAQ_ERR_INVALID_ARGUMENT;
    if (portId == nullptr)
        return OPENDAQ_ERR_INVALID_ARGUMENT;
    if (signal == nullptr)
        return OPENDAQ_ERR_INVALID_ARGUMENT;

    *signal = nullptr;

    DictPtr<IString, IBaseObject> connections;
    getInputPortConnections(parentId, &connections);
    if (!connections.hasKey(portId))
        return OPENDAQ_NOTFOUND;
    
    auto signalId = connections.get(portId);
    auto overridenSignalId = rootComponent.getGlobalId() + getRemoteId(signalId);

    Bool isCircle = false;
    for (SizeT i = 0; i < parentDependencies.getCount(); i++)
    {
        const auto & parentDep = parentDependencies.getItemAt(i);
        if (parentDep == parentId)
        {
            std::string deps;
            for (SizeT j = i; j < parentDependencies.getCount(); j++)
            {
                deps += parentDependencies.getItemAt(j).toStdString() + " -> ";
            }
            auto loggerComponent = rootComponent.getContext().getLogger().getOrAddComponent("Component");
            LOG_W("Circular dependency detected: {}{}", deps, parentDep);
            isCircle = true;
            break;
        }
    }

    if (isCircle == false)
    {
        parentDependencies.pushBack(parentId);
        ErrCode errCode = resolveSignalDependency(overridenSignalId, signal);
        parentDependencies.popBack();
        if (errCode == OPENDAQ_SUCCESS)
            return OPENDAQ_SUCCESS;
    }

    ComponentPtr signalPtr;
    rootComponent->findComponent(overridenSignalId, &signalPtr);

    if (!signalPtr.assigned())
        return OPENDAQ_NOTFOUND;

    *signal = signalPtr.asPtrOrNull<ISignal>().detach();
    return OPENDAQ_SUCCESS;
}

inline ErrCode ComponentUpdateContextImpl::setSignalDependency(IString* signalId, IString* parentId)
{
    if (signalId == nullptr)
        return OPENDAQ_ERR_INVALID_ARGUMENT;
    if (parentId == nullptr)
        return OPENDAQ_ERR_INVALID_ARGUMENT;

    signalDependencies.set(signalId, parentId);
    return OPENDAQ_SUCCESS;
}

inline ErrCode ComponentUpdateContextImpl::resolveSignalDependency(IString* signalId, ISignal** signal)
{
    if (signalId == nullptr)
        return OPENDAQ_ERR_INVALID_ARGUMENT;

    // Check that signal has parent
    if (!signalDependencies.hasKey(signalId))
        return OPENDAQ_NOTFOUND;
    
    auto parentId = signalDependencies.get(signalId);

    // Check that the parent is function block which is not finished with the update
    if (!connections.hasKey(parentId + "/IP"))
        return OPENDAQ_NOTFOUND;

    // Find the function block
    ComponentPtr parentComponent;
    rootComponent->findComponent(parentId, &parentComponent);

    if (!parentComponent.assigned())
        return OPENDAQ_NOTFOUND;

    // Call the parent component to finish the update
    parentComponent.as<IUpdatable>(true)->updateEnded(this->borrowInterface<IBaseObject>());

    // unregister dependency
    signalDependencies->deleteItem(signalId);

    // Find the signal in parent component
    auto signalIdPtr = StringPtr::Borrow(signalId);
    if (parentComponent.getGlobalId().toStdString().find(signalIdPtr.toStdString()) != 0)
        return OPENDAQ_ERR_INVALIDSTATE;
    
    StringPtr signalLocalId = parentComponent.getGlobalId().toStdString().substr(signalIdPtr.getLength());
    ComponentPtr signalComponent;
    parentComponent->findComponent(signalLocalId, &signalComponent);
    *signal = signalComponent.asPtrOrNull<ISignal>().detach();
    return OPENDAQ_SUCCESS;
}

inline ErrCode ComponentUpdateContextImpl::getReAddDevicesEnabled(Bool* enabled)
{
    if (!enabled)
        return OPENDAQ_ERR_INVALID_ARGUMENT;

    *enabled = config.getPropertyValue("ReAddDevices").asPtr<IBoolean>(true);
    return OPENDAQ_SUCCESS;
}

END_NAMESPACE_OPENDAQ