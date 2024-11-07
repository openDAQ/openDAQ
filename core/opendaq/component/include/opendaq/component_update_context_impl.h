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
#include <opendaq/update_parameters_factory.h>

BEGIN_NAMESPACE_OPENDAQ

class ComponentUpdateContextImpl : public ImplementationOf<IComponentUpdateContext>
{
public:

    ComponentUpdateContextImpl(const ComponentPtr& curComponent, const UpdateParametersPtr& config)
        : config(config.assigned() ? config : UpdateParameters())
        , connections(Dict<IString, IBaseObject>())
        , signalDependencies(Dict<IString, IString>())
        , parentDependencies(List<IString>())
        , rootComponent(GetRootComponent(curComponent))
    {
    }

    ErrCode INTERFACE_FUNC setInputPortConnection(IString* parentId, IString* portId, IString* signalId) override;
    ErrCode INTERFACE_FUNC getInputPortConnections(IString* parentId, IDict** connections) override;
    ErrCode INTERFACE_FUNC removeInputPortConnection(IString* parentId) override;
    ErrCode INTERFACE_FUNC getRootComponent(IComponent** rootComponent) override;
    ErrCode INTERFACE_FUNC getSignal(IString* parentId, IString* portId, ISignal** signal) override;
    ErrCode INTERFACE_FUNC setSignalDependency(IString* signalId, IString* parentId) override;

    ErrCode INTERFACE_FUNC getReAddDevicesEnabled(Bool* enabled) override;

private:
    ErrCode INTERFACE_FUNC resolveSignalDependency(IString* signalId, ISignal** signal);

    static ComponentPtr GetRootComponent(const ComponentPtr& curComponent);
    static StringPtr GetRemoteId(const std::string& globalId);

    UpdateParametersPtr config;

    DictPtr<IString, IDict> connections;
    DictPtr<IString, IString> signalDependencies;
    ListPtr<IString> parentDependencies;
    ComponentPtr rootComponent;
};

inline ComponentPtr ComponentUpdateContextImpl::GetRootComponent(const ComponentPtr& curComponent)
{
    const auto parent = curComponent.getParent();
    if (!parent.assigned())
        return curComponent;
    return GetRootComponent(parent);
}

inline StringPtr ComponentUpdateContextImpl::GetRemoteId(const std::string& globalId)
{
    if (globalId.size() < 2)
        return globalId;
    
    if (globalId[0] != '/')
        return globalId;
    
    // Find the position of the second slash
    size_t secondSlashPos = globalId.find('/', 1);
    if (secondSlashPos == std::string::npos)
        return globalId;

    // Erase the first segment
    return globalId.substr(secondSlashPos);
}

inline ErrCode ComponentUpdateContextImpl::setInputPortConnection(IString* parentId, IString* portId, IString* signalId)
{
    if (!parentId || !portId || !signalId)
        return OPENDAQ_ERR_ARGUMENT_NULL;

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

    auto signalRemoteId = GetRemoteId(StringPtr::Borrow(signalId));
    ports.set(portId, signalRemoteId);

    return OPENDAQ_SUCCESS;
}

inline ErrCode ComponentUpdateContextImpl::getInputPortConnections(IString* parentId, IDict** connections)
{
    if (!parentId || !connections)
        return OPENDAQ_ERR_ARGUMENT_NULL;

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
        return OPENDAQ_ERR_ARGUMENT_NULL;

    connections->deleteItem(parentId);
    return OPENDAQ_SUCCESS;
}

inline ErrCode ComponentUpdateContextImpl::getRootComponent(IComponent** rootComponent)
{
    if (!rootComponent)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *rootComponent = this->rootComponent.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

inline ErrCode ComponentUpdateContextImpl::getSignal(IString* parentId, IString* portId, ISignal** signal)
{
    if (parentId == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;
    if (portId == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;
    if (signal == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *signal = nullptr;

    DictPtr<IString, IString> signalConnections;
    getInputPortConnections(parentId, &signalConnections);
    if (!signalConnections.hasKey(portId))
        return OPENDAQ_NOTFOUND;
    
    auto signalId = signalConnections.get(portId);

    Bool isCircle = false;
    for (SizeT i = 0; i < parentDependencies.getCount(); i++)
    {
        const auto & parentDep = parentDependencies.getItemAt(i);
        if (parentDep == parentId)
        {
            std::ostringstream deps;
            for (SizeT j = i; j < parentDependencies.getCount(); j++)
            {
                deps << parentDependencies.getItemAt(j).toStdString() + " -> ";
            }
            auto loggerComponent = rootComponent.getContext().getLogger().getOrAddComponent("Component");
            LOG_W("Circular dependency detected: {}{}", deps.str(), parentDep);
            isCircle = true;
            break;
        }
    }

    if (isCircle == false)
    {
        parentDependencies.pushBack(parentId);
        ErrCode errCode = resolveSignalDependency(signalId, signal);
        parentDependencies.popBack();
        if (errCode == OPENDAQ_SUCCESS)
            return OPENDAQ_SUCCESS;
    }

    ComponentPtr signalPtr;
    rootComponent->findComponent(signalId, &signalPtr);

    if (!signalPtr.assigned())
        return OPENDAQ_NOTFOUND;

    *signal = signalPtr.asPtrOrNull<ISignal>().detach();
    return OPENDAQ_SUCCESS;
}

inline ErrCode ComponentUpdateContextImpl::setSignalDependency(IString* signalId, IString* parentId)
{
    if (signalId == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;
    if (parentId == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;
    
    StringPtr signalRemoteId = GetRemoteId(StringPtr::Borrow(signalId));
    StringPtr parentRemoteId = GetRemoteId(StringPtr::Borrow(parentId));

    if (signalRemoteId.toStdString().find(parentRemoteId.toStdString()) != 0)
        return this->makeErrorInfo(OPENDAQ_ERR_INVALIDSTATE, "%s is not parent of signal %s", parentRemoteId.getCharPtr(), signalRemoteId.getCharPtr());

    signalDependencies.set(signalRemoteId, parentId);
    return OPENDAQ_SUCCESS;
}

inline ErrCode ComponentUpdateContextImpl::resolveSignalDependency(IString* signalId, ISignal** signal)
{
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
    
    auto signalIdPtr = StringPtr::Borrow(signalId);
    StringPtr signalLocalId = signalIdPtr.toStdString().substr(GetRemoteId(parentId).getLength());
    
    ComponentPtr signalComponent;
    parentComponent->findComponent(signalLocalId, &signalComponent);
    if (!signalComponent.assigned())
        return OPENDAQ_NOTFOUND;

    SignalPtr singalPtr = signalComponent.asPtrOrNull<ISignal>();
    if (!singalPtr.assigned())
        return OPENDAQ_NOTFOUND;

    *signal = singalPtr.detach();
    return OPENDAQ_SUCCESS;
}

inline ErrCode ComponentUpdateContextImpl::getReAddDevicesEnabled(Bool* enabled)
{
    return config->getReAddDevicesEnabled(enabled);
}

END_NAMESPACE_OPENDAQ