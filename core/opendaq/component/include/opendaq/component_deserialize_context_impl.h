/*
 * Copyright 2022-2023 Blueberry d.o.o.
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
#include <opendaq/component_deserialize_context.h>
#include <opendaq/component_ptr.h>
#include <coretypes/validation.h>

namespace daq
{

template <class MainInterface, class ... Interfaces>
class GenericComponentDeserializeContextImpl : public ImplementationOf<MainInterface, Interfaces...>
{
public:
    GenericComponentDeserializeContextImpl(const ContextPtr& context,
                                           const ComponentPtr& parent,
                                           const StringPtr& localId,
                                           const TypeManagerPtr& typeManager);


    ErrCode INTERFACE_FUNC getParent(IComponent** parent) override;
    ErrCode INTERFACE_FUNC getLocalId(IString** localId) override;
    ErrCode INTERFACE_FUNC getContext(IContext** context) override;
    ErrCode INTERFACE_FUNC getTypeManager(ITypeManager** typeManager) override;

    ErrCode INTERFACE_FUNC clone(IComponent* newParent,
                                 IString* newLocalId,
                                 IComponentDeserializeContext** newComponentDeserializeContext) override;

protected:
    ContextPtr context;
    ComponentPtr parent;
    StringPtr localId;
    TypeManagerPtr typeManager;
};

using ComponentDeserializeContextImpl = GenericComponentDeserializeContextImpl<IComponentDeserializeContext>;

template <class MainInterface, class ... Interfaces>
GenericComponentDeserializeContextImpl<MainInterface, Interfaces...>::GenericComponentDeserializeContextImpl(const ContextPtr& context,
    const ComponentPtr& parent,
    const StringPtr& localId,
    const TypeManagerPtr& typeManager): context(context)
                                      , parent(parent)
                                      , localId(localId)
                                      , typeManager(typeManager)
{
}

template <class MainInterface, class... Interfaces>
ErrCode GenericComponentDeserializeContextImpl<MainInterface, Interfaces...>::getParent(IComponent** parent)
{
    OPENDAQ_PARAM_NOT_NULL(parent);

    *parent = this->parent.addRefAndReturn();

    return OPENDAQ_SUCCESS;
}

template <class MainInterface, class... Interfaces>
ErrCode GenericComponentDeserializeContextImpl<MainInterface, Interfaces...>::getLocalId(IString** localId)
{
    OPENDAQ_PARAM_NOT_NULL(localId);

    *localId = this->localId.addRefAndReturn();

    return OPENDAQ_SUCCESS;
}

template <class MainInterface, class... Interfaces>
ErrCode GenericComponentDeserializeContextImpl<MainInterface, Interfaces...>::getContext(IContext** context)
{
    OPENDAQ_PARAM_NOT_NULL(context);

    *context = this->context.addRefAndReturn();

    return OPENDAQ_SUCCESS;
}

template <class MainInterface, class... Interfaces>
ErrCode GenericComponentDeserializeContextImpl<MainInterface, Interfaces...>::getTypeManager(ITypeManager** typeManager)
{
    OPENDAQ_PARAM_NOT_NULL(typeManager);

    *typeManager = this->typeManager.addRefAndReturn();

    return OPENDAQ_SUCCESS;
}

template <class MainInterface, class ... Interfaces>
ErrCode GenericComponentDeserializeContextImpl<MainInterface, Interfaces...>::clone(
    IComponent* newParent,
    IString* newLocalId,
    IComponentDeserializeContext** newComponentDeserializeContext)
{
    OPENDAQ_PARAM_NOT_NULL(newLocalId);
    OPENDAQ_PARAM_NOT_NULL(newComponentDeserializeContext);

    return createComponentDeserializeContext(newComponentDeserializeContext, context, newParent, newLocalId, typeManager);
}

}
