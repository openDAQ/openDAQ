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
#include <coretypes/validation.h>
#include <opendaq/component.h>
#include <opendaq/context_ptr.h>
#include <opendaq/removable.h>
#include <coreobjects/core_event_args_ptr.h>
#include <coreobjects/property_object_impl.h>
#include <opendaq/component_ptr.h>
#include <coretypes/weakrefptr.h>
#include <opendaq/tags_factory.h>
#include <opendaq/search_filter_ptr.h>
#include <opendaq/folder_ptr.h>
#include <mutex>
#include <opendaq/component_keys.h>
#include <tsl/ordered_set.h>
#include <opendaq/custom_log.h>
#include <coreobjects/core_event_args_impl.h>
#include <opendaq/recursive_search_ptr.h>
#include <opendaq/component_private_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

static constexpr int ComponentSerializeFlag_SerializeActiveProp = 1;

template <class Intf = IComponent, class ... Intfs>
class ComponentImpl : public GenericPropertyObjectImpl<Intf, IRemovable, IComponentPrivate, Intfs ...>
{
public:
    using Super = GenericPropertyObjectImpl<Intf, IRemovable, IComponentPrivate, Intfs ...>;

    ComponentImpl(const ContextPtr& context,
                  const ComponentPtr& parent,
                  const StringPtr& localId,
                  const StringPtr& className = nullptr);

    ErrCode INTERFACE_FUNC getLocalId(IString** localId) override;
    ErrCode INTERFACE_FUNC getGlobalId(IString** globalId) override;
    ErrCode INTERFACE_FUNC getActive(Bool* active) override;
    virtual ErrCode INTERFACE_FUNC setActive(Bool active) override;
    ErrCode INTERFACE_FUNC getContext(IContext** context) override;
    ErrCode INTERFACE_FUNC getParent(IComponent** parent) override;
    ErrCode INTERFACE_FUNC getName(IString** name) override;
    virtual ErrCode INTERFACE_FUNC setName(IString* name) override;
    ErrCode INTERFACE_FUNC getDescription(IString** description) override;
    virtual ErrCode INTERFACE_FUNC setDescription(IString* description) override;
    ErrCode INTERFACE_FUNC getTags(ITagsConfig** tags) override;
    ErrCode INTERFACE_FUNC getVisible(Bool* visible) override;
    virtual ErrCode INTERFACE_FUNC setVisible(Bool visible) override;
    ErrCode INTERFACE_FUNC getOnComponentCoreEvent(IEvent** event) override;

    // IComponentPrivate
    ErrCode INTERFACE_FUNC lockAttributes(IList* attributes) override;
    virtual ErrCode INTERFACE_FUNC lockAllAttributes() override;
    ErrCode INTERFACE_FUNC unlockAttributes(IList* attributes) override;
    ErrCode INTERFACE_FUNC unlockAllAttributes() override;
    ErrCode INTERFACE_FUNC getLockedAttributes(IList** attributes) override;
    ErrCode INTERFACE_FUNC triggerComponentCoreEvent(ICoreEventArgs* args) override;

    ErrCode INTERFACE_FUNC remove() override;
    ErrCode INTERFACE_FUNC isRemoved(Bool* removed) override;

    // IUpdatable
    ErrCode INTERFACE_FUNC update(ISerializedObject* obj) override;


protected:
    virtual void activeChanged();
    virtual void removed();
    virtual ErrCode lockAllAttributesInternal();
    ListPtr<IComponent> searchItems(const SearchFilterPtr& searchFilter, const std::vector<ComponentPtr>& items);

    std::mutex sync;
    ContextPtr context;

    bool isComponentRemoved;
    WeakRefPtr<IComponent> parent;
    StringPtr localId;
    TagsConfigPtr tags;
    StringPtr globalId;
    EventPtr<const ComponentPtr, const CoreEventArgsPtr> coreEvent;
    
    inline static std::unordered_set<std::string> componentAvailableAttributes = {"Name", "Description", "Visible", "Active"};
    std::unordered_set<std::string> lockedAttributes;
    bool visible;
    bool active;
    StringPtr name;
    StringPtr description;


    ErrCode serializeCustomValues(ISerializer* serializer) override;
    virtual int getSerializeFlags();

    std::unordered_map<std::string, SerializedObjectPtr> getSerializedItems(const SerializedObjectPtr& object);

    virtual void updateObject(const SerializedObjectPtr& obj);
    virtual void serializeCustomObjectValues(const SerializerPtr& serializer);
    static std::string getRelativeGlobalId(const std::string& globalId);
    void triggerCoreEvent(const CoreEventArgsPtr& args);

private:
    EventEmitter<const ComponentPtr, const CoreEventArgsPtr> componentCoreEvent;
};

template <class Intf, class ... Intfs>
ComponentImpl<Intf, Intfs...>::ComponentImpl(
    const ContextPtr& context,
    const ComponentPtr& parent,
    const StringPtr& localId,
    const StringPtr& className)
    : GenericPropertyObjectImpl<Intf, IRemovable, IComponentPrivate, Intfs ...>(
        context.assigned() ? context.getTypeManager() : nullptr,
        className,
        [&](const CoreEventArgsPtr& args){ triggerCoreEvent(args); })
      , context(context)
      , isComponentRemoved(false)
      , parent(parent)
      , localId(localId)
      , tags(Tags())
      , visible(true)
      , active(true)
      , name(localId)
      , description("")
{
    if (!localId.assigned() || localId.toStdString().empty())
        throw GeneralErrorException("Local id not assigned");

    if (parent.assigned())
        globalId = parent.getGlobalId().toStdString() + "/" + static_cast<std::string>(localId);
    else
        globalId = "/" + localId;

    if (!context.assigned())
        throw InvalidParameterException{"Context must be assigned on component creation"};

    context->getOnCoreEvent(&this->coreEvent);
    lockedAttributes.insert("Visible");
}

template <class Intf, class ... Intfs>
ErrCode ComponentImpl<Intf, Intfs ...>::getLocalId(IString** localId)
{
    OPENDAQ_PARAM_NOT_NULL(localId);

    *localId = this->localId.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

template <class Intf, class ... Intfs>
ErrCode ComponentImpl<Intf, Intfs ...>::getGlobalId(IString** globalId)
{
    OPENDAQ_PARAM_NOT_NULL(globalId);

    *globalId = this->globalId.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

template <class Intf, class ... Intfs>
ErrCode ComponentImpl<Intf, Intfs ...>::getActive(Bool* active)
{
    OPENDAQ_PARAM_NOT_NULL(active);

    std::scoped_lock lock(sync);

    *active = this->active;
    return OPENDAQ_SUCCESS;
}

template <class Intf, class ... Intfs>
ErrCode ComponentImpl<Intf, Intfs...>::setActive(Bool active)
{
    if (this->frozen)
        return OPENDAQ_ERR_FROZEN;

    {
        std::scoped_lock lock(sync);
    
        if (lockedAttributes.count("Active"))
        {
            if (context.assigned() && context.getLogger().assigned())
            {
                const auto loggerComponent = context.getLogger().getOrAddComponent("Component");
                StringPtr descObj;
                this->getName(&descObj);
                LOG_I("Active attribute of {} is locked", descObj);
            }

            return OPENDAQ_IGNORED;
        }

        if (static_cast<bool>(active) == this->active)
            return OPENDAQ_IGNORED;

        if (active && isComponentRemoved)
            return OPENDAQ_ERR_INVALIDSTATE;

        this->active = active;
        activeChanged();
    }

    if (!this->coreEventMuted && this->coreEvent.assigned())
    {
        const CoreEventArgsPtr args = createWithImplementation<ICoreEventArgs, CoreEventArgsImpl>(
            core_event_ids::AttributeChanged, Dict<IString, IBaseObject>({{"AttributeName", "Active"}, {"Active", this->active}}));
        triggerCoreEvent(args);
    }

    return OPENDAQ_SUCCESS;
}

template <class Intf, class ... Intfs>
ErrCode ComponentImpl<Intf, Intfs...>::getContext(IContext** context)
{
    OPENDAQ_PARAM_NOT_NULL(context);

    *context = this->context.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

template <class Intf, class... Intfs>
ErrCode ComponentImpl<Intf, Intfs...>::getParent(IComponent** parent)
{
    OPENDAQ_PARAM_NOT_NULL(parent);

    ComponentPtr parentPtr;

    if (this->parent.assigned())
        parentPtr = this->parent.getRef();
    else
        parentPtr = nullptr;

    *parent = parentPtr.addRefAndReturn();

    return OPENDAQ_SUCCESS;
}

template <class Intf, class ... Intfs>
ErrCode ComponentImpl<Intf, Intfs...>::getName(IString** name)
{
    OPENDAQ_PARAM_NOT_NULL(name);

    if (this->name.assigned())
        *name = this->name.addRefAndReturn();
    else
        *name = localId.addRefAndReturn();

    return OPENDAQ_SUCCESS;
}

template <class Intf, class ... Intfs>
ErrCode ComponentImpl<Intf, Intfs...>::setName(IString* name)
{
    if (this->frozen)
        return OPENDAQ_ERR_FROZEN;

    {
        std::scoped_lock lock(sync);

        if (lockedAttributes.count("Name"))
        {
            if (context.assigned() && context.getLogger().assigned())
            {
                const auto loggerComponent = context.getLogger().getOrAddComponent("Component");
                StringPtr descObj;
                this->getName(&descObj);
                LOG_I("Name of {} is locked", descObj);
            }

            return OPENDAQ_IGNORED;
        }

        this->name = name;
        
    }

    if (!this->coreEventMuted && this->coreEvent.assigned())
    {
        const CoreEventArgsPtr args = createWithImplementation<ICoreEventArgs, CoreEventArgsImpl>(
            core_event_ids::AttributeChanged,
            Dict<IString, IBaseObject>({{"AttributeName", "Name"}, {"Name", this->name}}));
        triggerCoreEvent(args);
    }

    return OPENDAQ_SUCCESS;
}

template <class Intf, class ... Intfs>
ErrCode ComponentImpl<Intf, Intfs...>::getDescription(IString** description)
{
    OPENDAQ_PARAM_NOT_NULL(description);

    *description = this->description.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

template <class Intf, class ... Intfs>
ErrCode ComponentImpl<Intf, Intfs...>::setDescription(IString* description)
{
    if (this->frozen)
        return OPENDAQ_ERR_FROZEN;

    {
        std::scoped_lock lock(sync);

        if (lockedAttributes.count("Description"))
        {
            if (context.assigned() && context.getLogger().assigned())
            {
                const auto loggerComponent = context.getLogger().getOrAddComponent("Component");
                StringPtr descObj;
                this->getName(&descObj);
                LOG_I("Description of {} is locked", descObj);
            }

            return OPENDAQ_IGNORED;
        }

        this->description = description;
    }

    if (!this->coreEventMuted && this->coreEvent.assigned())
    {
        const CoreEventArgsPtr args = createWithImplementation<ICoreEventArgs, CoreEventArgsImpl>(
            core_event_ids::AttributeChanged,
            Dict<IString, IBaseObject>({{"AttributeName", "Description"}, {"Description", this->description}}));
        triggerCoreEvent(args);
    }

    return OPENDAQ_SUCCESS;
}

template <class Intf, class ... Intfs>
ErrCode ComponentImpl<Intf, Intfs...>::getTags(ITagsConfig** tags)
{
    OPENDAQ_PARAM_NOT_NULL(tags);

    *tags = this->tags.addRefAndReturn();

    return OPENDAQ_SUCCESS;
}

template <class Intf, class ... Intfs>
ErrCode ComponentImpl<Intf, Intfs...>::getVisible(Bool* visible)
{
    OPENDAQ_PARAM_NOT_NULL(visible);

    *visible = this->visible;
    return OPENDAQ_SUCCESS;
}

template <class Intf, class ... Intfs>
ErrCode ComponentImpl<Intf, Intfs...>::setVisible(Bool visible)
{
    if (this->frozen)
        return OPENDAQ_ERR_FROZEN;

    {
        std::scoped_lock lock(sync);

        if (lockedAttributes.count("Visible"))
        {
            if (context.assigned() && context.getLogger().assigned())
            {
                const auto loggerComponent = context.getLogger().getOrAddComponent("Component");
                StringPtr descObj;
                this->getName(&descObj);
                LOG_I("Visible attribute of {} is locked", descObj);
            }

            return OPENDAQ_IGNORED;
        }

        this->visible = visible;
    }

    if (!this->coreEventMuted && this->coreEvent.assigned())
    {
        const CoreEventArgsPtr args = createWithImplementation<ICoreEventArgs, CoreEventArgsImpl>(
            core_event_ids::AttributeChanged, Dict<IString, IBaseObject>({{"AttributeName", "Visible"}, {"Visible", this->visible}}));
        triggerCoreEvent(args);
    }

    return OPENDAQ_SUCCESS;
}

template <class Intf, class ... Intfs>
ErrCode ComponentImpl<Intf, Intfs...>::lockAttributes(IList* attributes)
{
    if (!attributes)
        return OPENDAQ_SUCCESS;

    std::scoped_lock lock(sync);

    const auto attributesPtr = ListPtr<IString>::Borrow(attributes);
    for (const auto& strPtr : attributesPtr)
    {
        std::string str = strPtr;
        std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c){ return std::tolower(c); });
        str[0] = std::toupper(str[0]);
        lockedAttributes.insert(str);
    }

    return OPENDAQ_SUCCESS;
}

template <class Intf, class ... Intfs>
ErrCode ComponentImpl<Intf, Intfs...>::lockAllAttributes()
{
    std::scoped_lock lock(sync);
    return lockAllAttributesInternal();
}

template <class Intf, class ... Intfs>
ErrCode ComponentImpl<Intf, Intfs...>::unlockAttributes(IList* attributes)
{
    if (!attributes)
        return OPENDAQ_SUCCESS;

    std::scoped_lock lock(sync);

    const auto attributesPtr = ListPtr<IString>::Borrow(attributes);
    for (const auto& strPtr : attributesPtr)
    {
        std::string str = strPtr;
        std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c){ return std::tolower(c); });
        str[0] = std::toupper(str[0]);
        lockedAttributes.erase(str);
    }

    return OPENDAQ_SUCCESS;
}

template <class Intf, class ... Intfs>
ErrCode ComponentImpl<Intf, Intfs...>::unlockAllAttributes()
{
    std::scoped_lock lock(sync);
    lockedAttributes.clear();
    return OPENDAQ_SUCCESS;
}

template <class Intf, class ... Intfs>
ErrCode ComponentImpl<Intf, Intfs...>::getLockedAttributes(IList** attributes)
{
    OPENDAQ_PARAM_NOT_NULL(attributes);
    
    std::scoped_lock lock(sync);

    ListPtr<IString> attributesList = List<IString>();
    for (const auto& str : lockedAttributes)
        attributesList.pushBack(str);

    *attributes = attributesList.detach();
    return OPENDAQ_SUCCESS;
}

template <class Intf, class ... Intfs>
ErrCode ComponentImpl<Intf, Intfs...>::triggerComponentCoreEvent(ICoreEventArgs* args)
{
    OPENDAQ_PARAM_NOT_NULL(args);

    const auto argsPtr = CoreEventArgsPtr::Borrow(args);

    try
    {
        const ComponentPtr thisPtr = this->template borrowPtr<ComponentPtr>();
        this->componentCoreEvent(thisPtr, argsPtr);
    }
    catch (const std::exception& e)
    {
        const auto loggerComponent = context.getLogger().getOrAddComponent("Component");
        LOG_W("Component {} failed while triggering core event {} with: {}", this->localId, argsPtr.getEventName(), e.what())
    }
    catch (...)
    {
        const auto loggerComponent = context.getLogger().getOrAddComponent("Component");
        LOG_W("Component {} failed while triggering core event {}", this->localId, argsPtr.getEventName())
    }

    return OPENDAQ_SUCCESS;
}

template <class Intf, class ... Intfs>
ErrCode ComponentImpl<Intf, Intfs...>::getOnComponentCoreEvent(IEvent** event)
{
    OPENDAQ_PARAM_NOT_NULL(event);

    *event = componentCoreEvent.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

template<class Intf, class ... Intfs>
ErrCode ComponentImpl<Intf, Intfs ...>::remove()
{
    std::scoped_lock lock(sync);

    if (isComponentRemoved)
        return  OPENDAQ_IGNORED;

    isComponentRemoved = true;

    if (active)
    {
        active = false;
        activeChanged();
    }

    this->disableCoreEventTrigger();
    removed();

    return OPENDAQ_SUCCESS;
}

template<class Intf, class ... Intfs>
ErrCode ComponentImpl<Intf, Intfs ...>::isRemoved(Bool* removed)
{
    OPENDAQ_PARAM_NOT_NULL(removed);

    *removed = this->isComponentRemoved;

    return OPENDAQ_SUCCESS;
}

template <class Intf, class... Intfs>
ErrCode INTERFACE_FUNC ComponentImpl<Intf, Intfs...>::update(ISerializedObject* obj)
{
    const auto objPtr = SerializedObjectPtr::Borrow(obj);

    return daqTry(
        [&objPtr, this]()
        {
            const bool muted = this->coreEventMuted;
            const auto thisPtr = this->template borrowPtr<ComponentPtr>();
            const auto propInternalPtr = this->template borrowPtr<PropertyObjectInternalPtr>();
            if (!muted)
                propInternalPtr.disableCoreEventTrigger();

            const auto err = Super::update(objPtr);

            updateObject(objPtr);

            if (!muted && this->coreEvent.assigned())
            {
                const CoreEventArgsPtr args = createWithImplementation<ICoreEventArgs, CoreEventArgsImpl>(core_event_ids::ComponentUpdateEnd, Dict<IString, IBaseObject>());
                triggerCoreEvent(args);
                propInternalPtr.enableCoreEventTrigger();
            }
            return err;
        });
}

template <class Intf, class... Intfs>
void ComponentImpl<Intf, Intfs...>::activeChanged()
{
}

template <class Intf, class... Intfs>
void ComponentImpl<Intf, Intfs...>::removed()
{
}

template <class Intf, class ... Intfs>
ErrCode ComponentImpl<Intf, Intfs...>::lockAllAttributesInternal()
{
    for (const auto& str : componentAvailableAttributes)
        lockedAttributes.insert(str);
    return OPENDAQ_SUCCESS;
}

template <class Intf, class ... Intfs>
ListPtr<IComponent> ComponentImpl<Intf, Intfs...>::searchItems(const SearchFilterPtr& searchFilter, const std::vector<ComponentPtr>& items)
{
    tsl::ordered_set<ComponentPtr, ComponentHash, ComponentEqualTo> allItems;
    for (const auto& item : items)
        if (searchFilter.acceptsComponent(item))
            allItems.insert(item);

    if (searchFilter.asPtrOrNull<IRecursiveSearch>().assigned())
    {
        for (const auto& item : items)
        {
            if (!searchFilter.visitChildren(item))
                continue;

            if (const auto folder = item.asPtrOrNull<IFolder>(); folder.assigned())
                for (const auto& child : folder.getItems(searchFilter))
                    allItems.insert(child);
        }
    }
    
    ListPtr<IComponent> childList = List<IComponent>();
    for (const auto& signal : allItems)
        childList.pushBack(signal);

    return childList.detach();
}

template <class Intf, class... Intfs>
ErrCode ComponentImpl<Intf, Intfs...>::serializeCustomValues(ISerializer* serializer)
{
    const auto serializerPtr = SerializerPtr::Borrow(serializer);

    auto errCode = Super::serializeCustomValues(serializer);
    if (OPENDAQ_FAILED(errCode))
        return errCode;

    return daqTry(
        [&serializerPtr, this]()
        {
            serializeCustomObjectValues(serializerPtr);

            return OPENDAQ_SUCCESS;
        });
}
 
template <class Intf, class... Intfs>
int ComponentImpl<Intf, Intfs...>::getSerializeFlags()
{
    return 0;
}

template <class Intf, class... Intfs>
std::unordered_map<std::string, SerializedObjectPtr> ComponentImpl<Intf, Intfs...>::getSerializedItems(const SerializedObjectPtr& object)
{
    std::unordered_map<std::string, SerializedObjectPtr> serializedItems;
    if (object.hasKey("items"))
    {
        const auto itemsObject = object.readSerializedObject("items");
        const auto itemsObjectKeys = itemsObject.getKeys();
        for (const auto& key : itemsObjectKeys)
        {
            auto itemObject = itemsObject.readSerializedObject(key);
            serializedItems.insert(std::pair(key.toStdString(), std::move(itemObject)));
        }
    }

    return serializedItems;
}

template <class Intf, class... Intfs>
void ComponentImpl<Intf, Intfs...>::updateObject(const SerializedObjectPtr& obj)
{
    const auto flags = getSerializeFlags();
    if (flags & ComponentSerializeFlag_SerializeActiveProp && obj.hasKey("active"))
        active = obj.readBool("active");

    if (obj.hasKey("visible"))
        visible = obj.readBool("visible");

    if (obj.hasKey("description"))
        description = obj.readString("description");

    if (obj.hasKey("name"))
        name = obj.readString("name");
}

template <class Intf, class... Intfs>
void ComponentImpl<Intf, Intfs...>::serializeCustomObjectValues(const SerializerPtr& serializer)
{
    auto flags = getSerializeFlags();

    if (flags & ComponentSerializeFlag_SerializeActiveProp && !active)
    {
        serializer.key("active");
        serializer.writeBool(active);
    }

    if (!visible)
    {
        serializer.key("visible");
        serializer.writeBool(visible);
    }

    if (description != "")
    {
        serializer.key("description");
        serializer.writeString(description);
    }

    if (name != localId)
    {
        serializer.key("name");
        serializer.writeString(name);
    }

    if (!tags.getList().empty())
    {
        serializer.key("tags");
        tags.serialize(serializer);
    }
}

template <class Intf, class... Intfs>
std::string ComponentImpl<Intf, Intfs...>::getRelativeGlobalId(const std::string& globalId)
{
    const auto equalsIdx = globalId.find_first_of('/');
    if (std::string::npos != equalsIdx)
        return globalId.substr(equalsIdx + 1);

    return globalId;
}

template <class Intf, class ... Intfs>
void ComponentImpl<Intf, Intfs...>::triggerCoreEvent(const CoreEventArgsPtr& args)
{
    try
    {
        const ComponentPtr thisPtr = this->template borrowPtr<ComponentPtr>();
        this->coreEvent(thisPtr, args);
    }
    catch (const std::exception& e)
    {
        const auto loggerComponent = context.getLogger().getOrAddComponent("Component");
        LOG_W("Component {} failed while triggering core event {} with: {}", this->localId, args.getEventName(), e.what())
    }
    catch (...)
    {
        const auto loggerComponent = context.getLogger().getOrAddComponent("Component");
        LOG_W("Component {} failed while triggering core event {}", this->localId, args.getEventName())
    }
}

END_NAMESPACE_OPENDAQ
