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
#include <opendaq/folder_ptr.h>
#include <tsl/ordered_map.h>
#include <opendaq/component_deserialize_context_factory.h>

BEGIN_NAMESPACE_OPENDAQ

template <class Intf = IFolderConfig, class... Intfs>
class FolderImpl : public ComponentImpl<Intf, Intfs...>
{
public:
    using Super = ComponentImpl<Intf, Intfs ...>;

    FolderImpl(const IntfID& itemId,
               const ContextPtr& context,
               const ComponentPtr& parent,
               const StringPtr& localId,
               const StringPtr& className = nullptr);

    FolderImpl(const ContextPtr& context,
               const ComponentPtr& parent,
               const StringPtr& localId,
               const StringPtr& className = nullptr);

    // IFolder
    ErrCode INTERFACE_FUNC getItems(IList** items, ISearchFilter* searchFilter = nullptr) override;
    ErrCode INTERFACE_FUNC getItem(IString* localId, IComponent** item) override;
    ErrCode INTERFACE_FUNC isEmpty(Bool* empty) override;
    ErrCode INTERFACE_FUNC hasItem(IString* localId, Bool* value) override;

    // IFolderConfig
    ErrCode INTERFACE_FUNC addItem(IComponent* item) override;
    ErrCode INTERFACE_FUNC removeItem(IComponent* item) override;
    ErrCode INTERFACE_FUNC removeItemWithLocalId(IString* localId) override;
    ErrCode INTERFACE_FUNC clear() override;

    // ISerializable
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;

    // IPropertyObjectInternal
    ErrCode INTERFACE_FUNC enableCoreEventTrigger() override;
    ErrCode INTERFACE_FUNC disableCoreEventTrigger() override;

    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

protected:
    tsl::ordered_map<std::string, ComponentPtr> items;

    void removed() override;

    virtual bool addItemInternal(const ComponentPtr& component);
    void serializeCustomObjectValues(const SerializerPtr& serializer, bool forUpdate) override;

    void deserializeCustomObjectValues(const SerializedObjectPtr& serializedObject,
                                       const BaseObjectPtr& context,
                                       const FunctionPtr& factoryCallback) override;

    template <class Interface, class Implementation>
    static BaseObjectPtr DeserializeFolder(const SerializedObjectPtr& serialized,
                                           const BaseObjectPtr& context,
                                           const FunctionPtr& factoryCallback);

    void callBeginUpdateOnChildren() override;
    void callEndUpdateOnChildren() override;
    void onUpdatableUpdateEnd() override;

private:
    bool removeItemWithLocalIdInternal(const std::string& str);
    void clearInternal();

    IntfID itemId;
};

template <class Intf, class... Intfs>
FolderImpl<Intf, Intfs...>::FolderImpl(const IntfID& itemId,
                                       const ContextPtr& context,
                                       const ComponentPtr& parent,
                                       const StringPtr& localId,
                                       const StringPtr& className)
    : ComponentImpl<Intf, Intfs...>(context, parent, localId, className)
    , itemId(itemId)
{
}

template <class Intf, class... Intfs>
FolderImpl<Intf, Intfs...>::FolderImpl(const ContextPtr& context,
                                       const ComponentPtr& parent,
                                       const StringPtr& localId,
                                       const StringPtr& className)
    : FolderImpl(IComponent::Id, context, parent, localId, className)
{
}

template <class Intf, class... Intfs>
ErrCode FolderImpl<Intf, Intfs...>::getItems(IList** items, ISearchFilter* searchFilter)
{
    OPENDAQ_PARAM_NOT_NULL(items);

    std::scoped_lock lock(this->sync);

    if (searchFilter)
    {
        return daqTry([&]
        {
            std::vector<ComponentPtr> itemsVec;
            for (const auto& item : this->items)
                itemsVec.emplace_back(item.second);

            *items = this->searchItems(searchFilter, itemsVec).detach();
            return OPENDAQ_SUCCESS;
        });
    }

    IList* list;
    auto err = createListWithElementType(&list, itemId);
    if (OPENDAQ_FAILED(err))
        return err;

    ListPtr<IComponent> childList = ListPtr<IComponent>::Adopt(list);
    for (const auto& item : this->items)
        if (item.second.getVisible())
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

    {
        std::scoped_lock lock(this->sync);

        const ErrCode err = daqTry(
            [this, &item]
            {
                if (!addItemInternal(item))
                    return OPENDAQ_ERR_DUPLICATEITEM;

                return OPENDAQ_SUCCESS;
            });

        if (OPENDAQ_FAILED(err))
            return err;
    }

    if (!this->coreEventMuted && this->coreEvent.assigned())
    {
        const auto component = ComponentPtr::Borrow(item);
        const auto args = createWithImplementation<ICoreEventArgs, CoreEventArgsImpl>(
                CoreEventId::ComponentAdded,
                Dict<IString, IBaseObject>({{"Component", component}}));
         this->triggerCoreEvent(args);
         component.asPtr<IPropertyObjectInternal>().enableCoreEventTrigger();
    }

    return OPENDAQ_SUCCESS;
}

template <class Intf, class... Intfs>
ErrCode FolderImpl<Intf, Intfs...>::removeItem(IComponent* item)
{
    OPENDAQ_PARAM_NOT_NULL(item);

    const auto str = ComponentPtr::Borrow(item).getLocalId().toStdString();

    {
        std::scoped_lock lock(this->sync);

        const ErrCode err = daqTry(
            [this, &str]
            {
                if (!removeItemWithLocalIdInternal(str))
                    return OPENDAQ_ERR_NOTFOUND;

                return OPENDAQ_SUCCESS;
            });

        if (OPENDAQ_FAILED(err))
            return err;
    }
    
    if (!this->coreEventMuted && this->coreEvent.assigned())
    {
        const auto args = createWithImplementation<ICoreEventArgs, CoreEventArgsImpl>(
                CoreEventId::ComponentRemoved,
                Dict<IString, IBaseObject>({{"Id", str}}));
        
        this->triggerCoreEvent(args);
    }

    return OPENDAQ_SUCCESS;
}

template <class Intf, class... Intfs>
ErrCode FolderImpl<Intf, Intfs...>::removeItemWithLocalId(IString* localId)
{
    OPENDAQ_PARAM_NOT_NULL(localId);

    const auto str = StringPtr::Borrow(localId).toStdString();

    {
        std::scoped_lock lock(this->sync);

        const ErrCode err = daqTry(
            [this, &str]
            {
                if (!removeItemWithLocalIdInternal(str))
                    return OPENDAQ_ERR_NOTFOUND;

                return OPENDAQ_SUCCESS;
            });

        if (OPENDAQ_FAILED(err))
             return err;
    }

    if (!this->coreEventMuted && this->coreEvent.assigned())
    {
        const auto args = createWithImplementation<ICoreEventArgs, CoreEventArgsImpl>(
                CoreEventId::ComponentRemoved,
                Dict<IString, IBaseObject>({{"Id", str}}));
        
        this->triggerCoreEvent(args);
    }

    return OPENDAQ_SUCCESS;
}

template <class Intf, class... Intfs>
ErrCode FolderImpl<Intf, Intfs...>::clear()
{
    std::scoped_lock lock(this->sync);
    clearInternal();

    return OPENDAQ_SUCCESS;
}

template <class Intf, class... Intfs>
ErrCode INTERFACE_FUNC FolderImpl<Intf, Intfs...>::getSerializeId(ConstCharPtr* id) const
{
    OPENDAQ_PARAM_NOT_NULL(id);

    *id = SerializeId();

    return OPENDAQ_SUCCESS;
}

template <class Intf, class ... Intfs>
ErrCode FolderImpl<Intf, Intfs...>::enableCoreEventTrigger()
{
    for (const auto& child : items)
    {
        const ErrCode err = child.second.template asPtr<IPropertyObjectInternal>()->enableCoreEventTrigger();
        if (OPENDAQ_FAILED(err))
            return err;
    }

    return ComponentImpl<Intf, Intfs...>::enableCoreEventTrigger();
}

template <class Intf, class ... Intfs>
ErrCode FolderImpl<Intf, Intfs...>::disableCoreEventTrigger()
{
    for (const auto& child : items)
    {
        const ErrCode err = child.second.template asPtr<IPropertyObjectInternal>()->disableCoreEventTrigger();
        if (OPENDAQ_FAILED(err))
            return err;
    }

    return ComponentImpl<Intf, Intfs...>::disableCoreEventTrigger();
}

template <class Intf, class... Intfs>
ConstCharPtr FolderImpl<Intf, Intfs...>::SerializeId()
{
    return "Folder";
}

template <class Intf, class... Intfs>
ErrCode FolderImpl<Intf, Intfs...>::Deserialize(ISerializedObject* serialized,
                                                IBaseObject* context,
                                                IFunction* factoryCallback,
                                                IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(context);

    return daqTry(
        [&obj, &serialized, &context, &factoryCallback]()
        {
            *obj = DeserializeFolder<IFolderConfig, FolderImpl>(serialized, context, factoryCallback).detach();
        });
}

template <class Intf, class... Intfs>
template <class Interface, class Implementation>
BaseObjectPtr FolderImpl<Intf, Intfs...>::DeserializeFolder(const SerializedObjectPtr& serialized,
                                                            const BaseObjectPtr& context,
                                                            const FunctionPtr& factoryCallback)
{
    return Super::DeserializeComponent(
        serialized,
        context,
        factoryCallback,
        [](const SerializedObjectPtr& serialized, const ComponentDeserializeContextPtr& deserializeContext, const StringPtr& className)
        {
            IntfID intfID;
            const auto errCode = deserializeContext->getIntfID(&intfID);
            if (errCode == OPENDAQ_SUCCESS)
            {
                return createWithImplementation<Interface, Implementation>(
                    intfID,
                    deserializeContext.getContext(),
                    deserializeContext.getParent(),
                    deserializeContext.getLocalId(),
                    className);
            }
            if (errCode == OPENDAQ_NOTFOUND)
            {
                return createWithImplementation<Interface, Implementation>(
                    deserializeContext.getContext(),
                    deserializeContext.getParent(),
                    deserializeContext.getLocalId(),
                    className);
            }
            checkErrorInfo(errCode);

            return typename InterfaceToSmartPtr<Interface>::SmartPtr();
        });
}


template <class Intf, class... Intfs>
void FolderImpl<Intf, Intfs...>::clearInternal()
{
    for (auto& item : items)
    {
        item.second.template asPtr<IPropertyObjectInternal>().disableCoreEventTrigger();
        item.second.remove();

        if (!this->coreEventMuted && this->coreEvent.assigned())
        {
            const auto args = createWithImplementation<ICoreEventArgs, CoreEventArgsImpl>(
                CoreEventId::ComponentRemoved,
                Dict<IString, IBaseObject>({{"Id", item.second.getLocalId()}}));
            
            this->triggerCoreEvent(args);
        }
    }

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
void FolderImpl<Intf, Intfs...>::serializeCustomObjectValues(const SerializerPtr& serializer, bool forUpdate)
{
    Super::serializeCustomObjectValues(serializer, forUpdate);

    if (!items.empty())
    {
        serializer.key("items");
        serializer.startObject();
        for (const auto& item : items)
        {
            serializer.key(item.first.c_str());
            if (forUpdate)
                item.second.template asPtr<IUpdatable>(true).serializeForUpdate(serializer);
            else
                item.second.serialize(serializer);
        }
        serializer.endObject();
    }
}

template <class Intf, class ... Intfs>
void FolderImpl<Intf, Intfs...>::deserializeCustomObjectValues(
    const SerializedObjectPtr& serializedObject,
    const BaseObjectPtr& context,
    const FunctionPtr& factoryCallback)
{
    Super::deserializeCustomObjectValues(serializedObject, context, factoryCallback);

    const auto deserializeContext = context.asPtr<IComponentDeserializeContext>(true);

    if (serializedObject.hasKey("items"))
    {
        const auto items = serializedObject.readSerializedObject("items");
        const auto keys = items.getKeys();
        for (const auto& key: keys)
        {
            const auto newDeserializeContext = deserializeContext.clone(this->template borrowPtr<ComponentPtr>(), key, nullptr);
            const auto item = items.readObject(key, newDeserializeContext, factoryCallback);

            const auto comp = item.template asPtr<IComponent>(true);
            addItemInternal(comp);
        }
    }
}

template <class Intf, class... Intfs>
void FolderImpl<Intf, Intfs...>::callBeginUpdateOnChildren()
{
    Super::callBeginUpdateOnChildren();

    for (const auto& item : items)
    {
        const auto& comp = item.second;
        comp.beginUpdate();
    }
}

template <class Intf, class... Intfs>
void FolderImpl<Intf, Intfs...>::callEndUpdateOnChildren()
{
    Super::callEndUpdateOnChildren();

    for (const auto& item : items)
    {
        const auto& comp = item.second;
        comp.endUpdate();
    }
}

template <class Intf, class ... Intfs>
void FolderImpl<Intf, Intfs...>::onUpdatableUpdateEnd()
{
    ComponentImpl<Intf, Intfs...>::onUpdatableUpdateEnd();

    for (const auto& item : items)
    {
        const auto updatable = item.second.template asPtrOrNull<IUpdatable>();
        if (updatable.assigned())
            updatable.updateEnded();
    }
}

template <class Intf, class... Intfs>
bool FolderImpl<Intf, Intfs...>::removeItemWithLocalIdInternal(const std::string& str)
{
    const auto it = items.find(str);
    if (it == items.end())
        return false;

    it->second.template asPtr<IPropertyObjectInternal>().disableCoreEventTrigger();
    it->second.remove();
    items.erase(it);
    return true;
}

using StandardFolder = FolderImpl<>;

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(StandardFolder)

END_NAMESPACE_OPENDAQ
