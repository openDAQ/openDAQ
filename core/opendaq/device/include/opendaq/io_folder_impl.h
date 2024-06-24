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
#include <opendaq/folder_impl.h>
#include <opendaq/io_folder_config.h>
#include <opendaq/channel.h>

BEGIN_NAMESPACE_OPENDAQ

template <class... Intfs>
class IoFolderImpl : public FolderImpl<IIoFolderConfig, Intfs ...>
{
public:
    using Super = FolderImpl<IIoFolderConfig, Intfs...>;

    IoFolderImpl(const ContextPtr& context,
                 const ComponentPtr& parent,
                 const StringPtr& localId,
                 const StringPtr& className = nullptr);

    IoFolderImpl(const IntfID& itemId,
                 const ContextPtr& context,
                 const ComponentPtr& parent,
                 const StringPtr& localId,
                 const StringPtr& className = nullptr);

    // ISerializable
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;

    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);
protected:
    bool addItemInternal(const ComponentPtr& component) override;

};

template <class... Intfs>
IoFolderImpl<Intfs...>::IoFolderImpl(const ContextPtr& context,
                           const ComponentPtr& parent,
                           const StringPtr& localId,
                           const StringPtr& className)
    : Super(context, parent, localId, className)
{
}

template <class ... Intfs>
IoFolderImpl<Intfs...>::IoFolderImpl(
    const IntfID& itemId,
    const ContextPtr& context,
    const ComponentPtr& parent,
    const StringPtr& localId,
    const StringPtr& className)
    : Super(itemId, context, parent, localId, className)
{
}

template <class... Intfs>
ErrCode IoFolderImpl<Intfs...>::getSerializeId(ConstCharPtr* id) const
{
    OPENDAQ_PARAM_NOT_NULL(id);

    *id = SerializeId();

    return OPENDAQ_SUCCESS;
}

template <class... Intfs>
ConstCharPtr IoFolderImpl<Intfs...>::SerializeId()
{
    return "IOFolder";
}

template <class... Intfs>
ErrCode IoFolderImpl<Intfs...>::Deserialize(ISerializedObject* serialized,
                                                   IBaseObject* context,
                                                   IFunction* factoryCallback,
                                                   IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(context);

    return daqTry([&obj, &serialized, &context, &factoryCallback]()
    {
        *obj = Super::template DeserializeFolder<IIoFolderConfig, IoFolderImpl>(serialized, context, factoryCallback).detach();
    });
}

template <class... Intfs>
bool IoFolderImpl<Intfs...>::addItemInternal(const ComponentPtr& component)
{
    if (!component.supportsInterface<IIoFolderConfig>() && !component.supportsInterface<IChannel>())
        throw InvalidParameterException("Type of item not allowed in the folder");

    return Super::addItemInternal(component);
}

END_NAMESPACE_OPENDAQ
