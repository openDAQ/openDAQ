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
#include <opendaq/folder_config.h>
#include <opendaq/component_ptr.h>
#include <opendaq/component_impl.h>
#include <tsl/ordered_map.h>

BEGIN_NAMESPACE_OPENDAQ

template <class Intf = IFolderConfig, class... Intfs>
class FolderImpl : public ComponentImpl<Intf, Intfs...>
{
public:
    using Super = ComponentImpl<Intf, Intfs ...>;

    FolderImpl(const IntfID& itemId, const ContextPtr& context, const ComponentPtr& parent, const StringPtr& localId);
    FolderImpl(const ContextPtr& context, const ComponentPtr& parent, const StringPtr& localId);

    // IComponent
    ErrCode INTERFACE_FUNC getName(IString** name) override;

    // IFolder
    ErrCode INTERFACE_FUNC getItems(IList** items) override;
    ErrCode INTERFACE_FUNC getItem(IString* localId, IComponent** item) override;
    ErrCode INTERFACE_FUNC isEmpty(Bool* empty) override;
    ErrCode INTERFACE_FUNC hasItem(IString* localId, Bool* value) override;

    // IFolderConfig
    ErrCode INTERFACE_FUNC addItem(IComponent* item) override;
    ErrCode INTERFACE_FUNC removeItem(IComponent* item) override;
    ErrCode INTERFACE_FUNC removeItemWithLocalId(IString* localId) override;
    ErrCode INTERFACE_FUNC clear() override;
    ErrCode INTERFACE_FUNC setName(IString* name) override;

    // ISerializable
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;

    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IBaseObject** obj);
protected:
    tsl::ordered_map<std::string, ComponentPtr> items;

    void removed() override;

    virtual bool addItemInternal(const ComponentPtr& component);
    void serializeCustomObjectValues(const SerializerPtr& serializer) override;
private:
    StringPtr name;

    bool removeItemWithLocalIdInternal(const std::string& str);
    void clearInternal();

    IntfID itemId;
};

template <class Intf, class... Intfs>
FolderImpl<Intf, Intfs...>::FolderImpl(const IntfID& itemId,
                                       const ContextPtr& context,
                                       const ComponentPtr& parent,
                                       const StringPtr& localId)
    : ComponentImpl<Intf, Intfs...>(context, parent, localId)
    , itemId(itemId)
{
}

template <class Intf, class... Intfs>
FolderImpl<Intf, Intfs...>::FolderImpl(const ContextPtr& context, const ComponentPtr& parent, const StringPtr& localId)
    : FolderImpl(IComponent::Id, context, parent, localId)
{
}

template <class Intf, class... Intfs>
ErrCode FolderImpl<Intf, Intfs...>::getName(IString** name)
{
    OPENDAQ_PARAM_NOT_NULL(name);

    std::scoped_lock lock(this->sync);

    if (this->name.assigned() && !this->name.toStdString().empty())
        *name = this->name.addRefAndReturn();
    else
        *name = this->localId.addRefAndReturn();

    return OPENDAQ_SUCCESS;
}

template <class Intf, class... Intfs>
ErrCode FolderImpl<Intf, Intfs...>::getItems(IList** items)
{
    OPENDAQ_PARAM_NOT_NULL(items);

    std::scoped_lock lock(this->sync);

    IList* list;
    auto err = createListWithElementType(&list, itemId);
    if (OPENDAQ_FAILED(err))
        return err;

    auto childList = ListPtr<IComponent>::Adopt(list);

    for (const auto& item : this->items)
        childList.pushBack(item.second);

    *items = childList.detach();

    return OPENDAQ_SUCCESS;
}

template <class Intf, class... Intfs>
ErrCode FolderImpl<Intf, Intfs...>::getItem(IString* localId, IComponent** item)
{
    OPENDAQ_PARAM_NOT_NULL(localId);
    OPENDAQ_PARAM_NOT_NULL(item);

    std::scoped_lock lock(this->sync);

    auto it = items.find(StringPtr::Borrow(localId).toStdString());
    if (it == items.end())
        return OPENDAQ_ERR_NOTFOUND;

    *item = it->second.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

template <class Intf, class... Intfs>
ErrCode INTERFACE_FUNC FolderImpl<Intf, Intfs...>::isEmpty(Bool* empty)
{
    OPENDAQ_PARAM_NOT_NULL(empty);

    *empty = items.empty() ? True : False;

    return OPENDAQ_SUCCESS;
}

template <class Intf, class ... Intfs>
ErrCode FolderImpl<Intf, Intfs...>::hasItem(IString* localId, Bool* value)
{
    OPENDAQ_PARAM_NOT_NULL(localId);

    std::scoped_lock lock(this->sync);

    const auto it = items.find(StringPtr::Borrow(localId).toStdString());
    if (it == items.end())
        *value = False;
    else
        *value = True;

    return OPENDAQ_SUCCESS;
}

template <class Intf, class... Intfs>
ErrCode FolderImpl<Intf, Intfs...>::addItem(IComponent* item)
{
    OPENDAQ_PARAM_NOT_NULL(item);

    std::scoped_lock lock(this->sync);

    return daqTry(
        [this, &item]
        {
            if (!addItemInternal(item))
                return OPENDAQ_ERR_DUPLICATEITEM;

            return OPENDAQ_SUCCESS;
        });
}

template <class Intf, class... Intfs>
ErrCode FolderImpl<Intf, Intfs...>::removeItem(IComponent* item)
{
    OPENDAQ_PARAM_NOT_NULL(item);

    auto itemPtr = ComponentPtr::Borrow(item);

    std::scoped_lock lock(this->sync);

    return daqTry(
        [this, &itemPtr]
        {
            if (!removeItemWithLocalIdInternal(itemPtr.getLocalId().toStdString()))
                return OPENDAQ_ERR_NOTFOUND;

            return OPENDAQ_SUCCESS;
        });
}

template <class Intf, class... Intfs>
ErrCode FolderImpl<Intf, Intfs...>::removeItemWithLocalId(IString* localId)
{
    OPENDAQ_PARAM_NOT_NULL(localId);

    auto localIdPtr = StringPtr::Borrow(localId);

    std::scoped_lock lock(this->sync);

    return daqTry(
        [this, &localIdPtr]
        {
            if (!removeItemWithLocalIdInternal(localIdPtr.toStdString()))
                return OPENDAQ_ERR_NOTFOUND;

            return OPENDAQ_SUCCESS;
        });
}

template <class Intf, class... Intfs>
ErrCode FolderImpl<Intf, Intfs...>::clear()
{
    std::scoped_lock lock(this->sync);
    clearInternal();

    return OPENDAQ_SUCCESS;
}

template <class Intf, class... Intfs>
ErrCode FolderImpl<Intf, Intfs...>::setName(IString* name)
{
    OPENDAQ_PARAM_NOT_NULL(name);

    std::scoped_lock lock(this->sync);

    this->name = name;

    return OPENDAQ_SUCCESS;
}

template <class Intf, class... Intfs>
ErrCode INTERFACE_FUNC FolderImpl<Intf, Intfs...>::getSerializeId(ConstCharPtr* id) const
{
    OPENDAQ_PARAM_NOT_NULL(id);

    *id = SerializeId();

    return OPENDAQ_SUCCESS;
}

template <class Intf, class... Intfs>
ConstCharPtr FolderImpl<Intf, Intfs...>::SerializeId()
{
    return "Folder";
}

template <class Intf, class... Intfs>
ErrCode FolderImpl<Intf, Intfs...>::Deserialize(ISerializedObject* serialized, IBaseObject* context, IBaseObject** obj)
{
    return OPENDAQ_ERR_NOTIMPLEMENTED;
}

template <class Intf, class... Intfs>
void FolderImpl<Intf, Intfs...>::clearInternal()
{
    for (auto& item : items)
        item.second.remove();

    items.clear();
}

template <class Intf, class... Intfs>
void FolderImpl<Intf, Intfs...>::removed()
{
    clearInternal();
}

template <class Intf, class... Intfs>
bool FolderImpl<Intf, Intfs...>::addItemInternal(const ComponentPtr& component)
{
    if (!component.supportsInterface(itemId))
        throw InvalidParameterException("Type of item not allowed in the folder");

    const auto res = items.insert({component.getLocalId(), component});
    return res.second;
}

template <class Intf, class ... Intfs>
void FolderImpl<Intf, Intfs...>::serializeCustomObjectValues(const SerializerPtr& serializer)
{
    Super::serializeCustomObjectValues(serializer);

    if (!items.empty())
    {
        serializer.key("items");
        serializer.startObject();
        for (const auto& item : items)
        {
            serializer.key(item.first.c_str());
            item.second.serialize(serializer);
        }
        serializer.endObject();
    }
}

template <class Intf, class... Intfs>
bool FolderImpl<Intf, Intfs...>::removeItemWithLocalIdInternal(const std::string& str)
{
    const auto it = items.find(str);
    if (it == items.end())
        return false;

    it->second.remove();
    items.erase(it);
    return true;
}

END_NAMESPACE_OPENDAQ
