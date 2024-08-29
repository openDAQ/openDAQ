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
    ComponentUpdateContextImpl(const ComponentPtr& curComponent)
        : connections(Dict<IString, IBaseObject>())
        , rootComponent(getRootComponent(curComponent))
    {
    }

    ErrCode INTERFACE_FUNC setInputPortConnection(IString* parentId, IString* portId, IString* signalId) override;
    ErrCode INTERFACE_FUNC getInputPortConnection(IString* parentId, IDict** connections) override;
    ErrCode INTERFACE_FUNC getRootComponent(IComponent** rootComponent) override;
    ErrCode INTERFACE_FUNC getSignal(IString* parentId, IString* portId, ISignal** signal) override;

private:

    static ComponentPtr getRootComponent(const ComponentPtr& curComponent);
    static StringPtr getRemoteId(const std::string& globalId);

    DictPtr<IString, IBaseObject> connections;
    ComponentPtr rootComponent;
};

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

inline ErrCode ComponentUpdateContextImpl::getInputPortConnection(IString* parentId, IDict** connections)
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
    if (firstSlashPos == std::string::npos) {
        // No slash found, return the original path
        return globalId;
    }

    // Find the position of the second slash
    size_t secondSlashPos = globalId.find('/', firstSlashPos + 1);
    if (secondSlashPos == std::string::npos) {
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
    getInputPortConnection(parentId, &connections);
    if (!connections.hasKey(portId))
        return OPENDAQ_NOTFOUND;
    
    auto signalId = connections.get(portId);
    auto overridenSignalId = rootComponent.getGlobalId() + getRemoteId(signalId);

    ComponentPtr signalPtr;
    rootComponent->findComponent(overridenSignalId, &signalPtr);

    if (!signalPtr.assigned())
        return OPENDAQ_NOTFOUND;

    *signal = signalPtr.asPtrOrNull<ISignal>().detach();
    return OPENDAQ_SUCCESS;
}

END_NAMESPACE_OPENDAQ