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
#include <coretypes/updatable_context.h>
#include <coretypes/stringobject_factory.h>
#include <coretypes/dictobject_factory.h>
#include <set>

BEGIN_NAMESPACE_OPENDAQ

class UpdatableContextImpl : public ImplementationOf<IUpdatableContext>
{
public:
    UpdatableContextImpl()
        : connections(Dict<IString, IBaseObject>())
    {
    }

    ErrCode INTERFACE_FUNC setInputPortConnection(IString* parentId, IString* portId, IString* signalId) override;
    ErrCode INTERFACE_FUNC getInputPortConnection(IString* parentId, IDict** connections) override;
    ErrCode INTERFACE_FUNC setNotMutedComponent(IString* componentId) override;
    ErrCode INTERFACE_FUNC getComponentIsMuted(IString* componentId, Bool* muted) override;

private:
    DictPtr<IString, IBaseObject> connections;
    std::set<std::string> notMutedComponents;
};

inline ErrCode UpdatableContextImpl::setInputPortConnection(IString* parentId, IString* portId, IString* signalId)
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

inline ErrCode UpdatableContextImpl::getInputPortConnection(IString* parentId, IDict** connections)
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

inline ErrCode UpdatableContextImpl::setNotMutedComponent(IString* componentId)
{
    if (componentId != nullptr)
    {
        notMutedComponents.insert(StringPtr(componentId).toString());
    }
    return OPENDAQ_SUCCESS;
}

inline ErrCode UpdatableContextImpl::getComponentIsMuted(IString* componentId, Bool* muted)
{
    if (componentId == nullptr || muted == nullptr)
        return OPENDAQ_ERR_INVALID_ARGUMENT;

    *muted = notMutedComponents.find(StringPtr(componentId).toString()) == notMutedComponents.end();
    return OPENDAQ_SUCCESS;
}

END_NAMESPACE_OPENDAQ