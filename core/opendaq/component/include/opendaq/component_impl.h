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
#include <coretypes/validation.h>
#include <opendaq/component.h>
#include <opendaq/context_ptr.h>
#include <opendaq/removable.h>
#include <coreobjects/core_event_args_ptr.h>
#include <coreobjects/property_object_impl.h>
#include <opendaq/component_ptr.h>
#include <coretypes/weakrefptr.h>
#include <opendaq/tags_private_ptr.h>
#include <opendaq/tags_ptr.h>
#include <opendaq/search_filter_ptr.h>
#include <opendaq/folder_ptr.h>
#include <mutex>
#include <opendaq/component_keys.h>
#include <tsl/ordered_set.h>
#include <opendaq/custom_log.h>
#include <opendaq/component_deserialize_context_ptr.h>
#include <opendaq/deserialize_component_ptr.h>
#include <coreobjects/core_event_args_impl.h>
#include <opendaq/recursive_search_ptr.h>
#include <opendaq/component_private_ptr.h>
#include <opendaq/tags_impl.h>
#include <cctype>
#include <opendaq/ids_parser.h>
#include <opendaq/component_status_container_impl.h>
#include <coreobjects/permission_manager_factory.h>
#include <coreobjects/permissions_builder_factory.h>
#include <coreobjects/permission_mask_builder_factory.h>
#include <opendaq/component_errors.h>

BEGIN_NAMESPACE_OPENDAQ

static constexpr int ComponentSerializeFlag_SerializeActiveProp = 1;

// https://developercommunity.visualstudio.com/t/inline-static-destructors-are-called-multiple-time/1157794
#ifdef _MSC_VER
#if _MSC_VER <= 1927
#define WORKAROUND_MEMBER_INLINE_VARIABLE
#endif
#endif

#define COMPONENT_AVAILABLE_ATTRIBUTES {"Name", "Description", "Visible", "Active"}

template <class Intf = IComponent, class ... Intfs>
class ComponentImpl : public GenericPropertyObjectImpl<Intf, IRemovable, IComponentPrivate, IDeserializeComponent, Intfs ...>
{
public:
    using Super = GenericPropertyObjectImpl<Intf, IRemovable, IComponentPrivate, IDeserializeComponent, Intfs ...>;

    ComponentImpl(const ContextPtr& context,
                  const ComponentPtr& parent,
                  const StringPtr& localId,
                  const StringPtr& className = nullptr,
                  const StringPtr& name = nullptr);

    // IComponent
    ErrCode INTERFACE_FUNC getLocalId(IString** localId) override;
    ErrCode INTERFACE_FUNC getGlobalId(IString** globalId) override;
    ErrCode INTERFACE_FUNC getActive(Bool* active) override;
    virtual ErrCode INTERFACE_FUNC setActive(Bool active) override;
    ErrCode INTERFACE_FUNC getContext(IContext** context) override;
    ErrCode INTERFACE_FUNC getParent(IComponent** parent) override;
    ErrCode INTERFACE_FUNC getName(IString** name) override;
    ErrCode INTERFACE_FUNC setName(IString* name) override;
    ErrCode INTERFACE_FUNC getDescription(IString** description) override;
    ErrCode INTERFACE_FUNC setDescription(IString* description) override;
    ErrCode INTERFACE_FUNC getTags(ITags** tags) override;
    ErrCode INTERFACE_FUNC getVisible(Bool* visible) override;
    ErrCode INTERFACE_FUNC setVisible(Bool visible) override;
    ErrCode INTERFACE_FUNC getOnComponentCoreEvent(IEvent** event) override;
    ErrCode INTERFACE_FUNC getStatusContainer(IComponentStatusContainer** statusContainer) override;
    ErrCode INTERFACE_FUNC findComponent(IString* id, IComponent** outComponent) override;
    ErrCode INTERFACE_FUNC getLockedAttributes(IList** attributes) override;

    // IComponentPrivate
    ErrCode INTERFACE_FUNC lockAttributes(IList* attributes) override;
    ErrCode INTERFACE_FUNC lockAllAttributes() override;
    ErrCode INTERFACE_FUNC unlockAttributes(IList* attributes) override;
    ErrCode INTERFACE_FUNC unlockAllAttributes() override;
    ErrCode INTERFACE_FUNC triggerComponentCoreEvent(ICoreEventArgs* args) override;

    // IRemovable
    ErrCode INTERFACE_FUNC remove() override;
    ErrCode INTERFACE_FUNC isRemoved(Bool* removed) override;

    // IUpdatable
    ErrCode INTERFACE_FUNC update(ISerializedObject* obj) override;
    ErrCode INTERFACE_FUNC updateInternal(ISerializedObject* obj, IBaseObject* context) override;

    // IDeserializeComponent
    ErrCode INTERFACE_FUNC deserializeValues(ISerializedObject* serializedObject, IBaseObject* context, IFunction* callbackFactory) override;
    ErrCode INTERFACE_FUNC complete() override;
    ErrCode INTERFACE_FUNC getDeserializedParameter(IString* parameter, IBaseObject** value) override;

    // ISerializable
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;

    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

protected:
    virtual void activeChanged();
    virtual void visibleChanged();
    virtual void removed();
    virtual ErrCode lockAllAttributesInternal();
    ListPtr<IComponent> searchItems(const SearchFilterPtr& searchFilter, const std::vector<ComponentPtr>& items);
    void setActiveRecursive(const std::vector<ComponentPtr>& items, Bool active);

    std::mutex sync;
    ContextPtr context;

    bool isComponentRemoved;
    WeakRefPtr<IComponent> parent;
    StringPtr localId;
    TagsPrivatePtr tags;
    StringPtr globalId;
    EventPtr<const ComponentPtr, const CoreEventArgsPtr> coreEvent;
    
#ifdef WORKAROUND_MEMBER_INLINE_VARIABLE
    static std::unordered_set<std::string> componentAvailableAttributes;
#else
    inline static std::unordered_set<std::string> componentAvailableAttributes = COMPONENT_AVAILABLE_ATTRIBUTES;
#endif

    std::unordered_set<std::string> lockedAttributes;
    bool visible;
    bool active;
    StringPtr name;
    StringPtr description;
    ComponentStatusContainerPtr statusContainer;

    ErrCode serializeCustomValues(ISerializer* serializer, bool forUpdate) override;
    virtual int getSerializeFlags();

    std::unordered_map<std::string, SerializedObjectPtr> getSerializedItems(const SerializedObjectPtr& object);

    virtual void updateObject(const SerializedObjectPtr& obj, const BaseObjectPtr& context);
    void onUpdatableUpdateEnd() override;
    virtual void serializeCustomObjectValues(const SerializerPtr& serializer, bool forUpdate);
    void triggerCoreEvent(const CoreEventArgsPtr& args);

    virtual void deserializeCustomObjectValues(const SerializedObjectPtr& serializedObject,
                                               const BaseObjectPtr& context,
                                               const FunctionPtr& factoryCallback);

    template <class CreateComponentCallback>
    static BaseObjectPtr DeserializeComponent(const SerializedObjectPtr& serialized,
                                              const BaseObjectPtr& context,
                                              const FunctionPtr& factoryCallback,
                                              CreateComponentCallback&& createComponentCallback);

    virtual BaseObjectPtr getDeserializedParameter(const StringPtr& parameter);
    ComponentPtr findComponentInternal(const ComponentPtr& component, const std::string& id);

    PropertyObjectPtr getPropertyObjectParent() override;

    static bool validateComponentId(const std::string& id);

private:
    EventEmitter<const ComponentPtr, const CoreEventArgsPtr> componentCoreEvent;
};

#ifdef WORKAROUND_MEMBER_INLINE_VARIABLE
template <class Intf, class... Intfs>
std::unordered_set<std::string> ComponentImpl<Intf, Intfs...>::componentAvailableAttributes = COMPONENT_AVAILABLE_ATTRIBUTES;
#endif

template <class Intf, class ... Intfs>
ComponentImpl<Intf, Intfs...>::ComponentImpl(
    const ContextPtr& context,
    const ComponentPtr& parent,
    const StringPtr& localId,
    const StringPtr& className,
    const StringPtr& name)
    : Super(
        context.assigned() ? context.getTypeManager() : nullptr,
        className,
        [&](const CoreEventArgsPtr& args){ triggerCoreEvent(args); })
      , context(context)
      , isComponentRemoved(false)
      , parent(parent)
      , localId(localId)
      , tags(createWithImplementation<ITagsPrivate, TagsImpl>([&](const CoreEventArgsPtr& args)
          {
              if (!this->coreEventMuted)
                  triggerCoreEvent(args);
          }))
      , visible(true)
      , active(true)
      , name(name.assigned() && name != "" ? name : localId)
      , description("")
      , statusContainer(createWithImplementation<IComponentStatusContainer, ComponentStatusContainerImpl>(
          [&](const CoreEventArgsPtr& args)
          {
              if (!this->coreEventMuted)
                  triggerCoreEvent(args);
          }))
{
    if (!localId.assigned() || localId.toStdString().empty())
        throw GeneralErrorException("Local id not assigned");

    if (parent.assigned())
        globalId = parent.getGlobalId().toStdString() + "/" + static_cast<std::string>(localId);
    else
        globalId = "/" + localId;

    if (!context.assigned())
        throw InvalidParameterException{"Context must be assigned on component creation"};

    if (context.getLogger().assigned()) {
        const auto loggerComponent = context.getLogger().getOrAddComponent("Component");
        const auto localIdString = localId.toStdString();
        if (!validateComponentId(localIdString))
            LOG_W("Component has incorrect id '{}': contains whitespaces", localIdString);
    }

    context->getOnCoreEvent(&this->coreEvent);
    lockedAttributes.insert("Visible");

    if (parent.assigned())
    {
        const auto parentManager = parent.getPermissionManager();
        this->permissionManager.template asPtr<IPermissionManagerInternal>(true).setParent(parentManager);
    }
    else
    {
        this->permissionManager.setPermissions(
            PermissionsBuilder().assign("everyone", PermissionMaskBuilder().read().write().execute()).build());
    }
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

        if (this->isComponentRemoved)
            return OPENDAQ_ERR_COMPONENT_REMOVED;
    
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
            CoreEventId::AttributeChanged, Dict<IString, IBaseObject>({{"AttributeName", "Active"}, {"Active", this->active}}));
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

        if (this->isComponentRemoved)
            return OPENDAQ_ERR_COMPONENT_REMOVED;

        if (StringPtr namePtr = name; this->name == namePtr)
            return OPENDAQ_IGNORED;

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
            CoreEventId::AttributeChanged,
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

        if (this->isComponentRemoved)
            return OPENDAQ_ERR_COMPONENT_REMOVED;

        if (StringPtr descriptionPtr = description; this->description == descriptionPtr)
            return OPENDAQ_IGNORED;

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
            CoreEventId::AttributeChanged,
            Dict<IString, IBaseObject>({{"AttributeName", "Description"}, {"Description", this->description}}));
        triggerCoreEvent(args);
    }

    return OPENDAQ_SUCCESS;
}

template <class Intf, class ... Intfs>
ErrCode ComponentImpl<Intf, Intfs...>::getTags(ITags** tags)
{
    OPENDAQ_PARAM_NOT_NULL(tags);

    *tags = this->tags.template asPtr<ITags>().addRefAndReturn();

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

        if (this->isComponentRemoved)
            return OPENDAQ_ERR_COMPONENT_REMOVED;

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
        visibleChanged();
    }

    if (!this->coreEventMuted && this->coreEvent.assigned())
    {
        const CoreEventArgsPtr args = createWithImplementation<ICoreEventArgs, CoreEventArgsImpl>(
            CoreEventId::AttributeChanged, Dict<IString, IBaseObject>({{"AttributeName", "Visible"}, {"Visible", this->visible}}));
        triggerCoreEvent(args);
    }

    return OPENDAQ_SUCCESS;
}

template <class Intf, class ... Intfs>
ErrCode ComponentImpl<Intf, Intfs...>::getStatusContainer(IComponentStatusContainer** statusContainer)
{
    OPENDAQ_PARAM_NOT_NULL(statusContainer);

    *statusContainer = this->statusContainer.addRefAndReturn();

    return OPENDAQ_SUCCESS;
}

template <class Intf, class ... Intfs>
ErrCode ComponentImpl<Intf, Intfs...>::lockAttributes(IList* attributes)
{
    if (!attributes)
        return OPENDAQ_SUCCESS;

    std::scoped_lock lock(sync);

    if (this->isComponentRemoved)
        return OPENDAQ_ERR_COMPONENT_REMOVED;

    const auto attributesPtr = ListPtr<IString>::Borrow(attributes);
    for (const auto& strPtr : attributesPtr)
    {
        std::string str = strPtr;
        std::transform(str.begin(), str.end(), str.begin(), [](char c){ return std::tolower(c); });
        str[0] = std::toupper(str[0]);
        lockedAttributes.insert(str);
    }

    return OPENDAQ_SUCCESS;
}

template <class Intf, class ... Intfs>
ErrCode ComponentImpl<Intf, Intfs...>::lockAllAttributes()
{
    std::scoped_lock lock(sync);

    if (this->isComponentRemoved)
        return OPENDAQ_ERR_COMPONENT_REMOVED;

    return lockAllAttributesInternal();
}

template <class Intf, class ... Intfs>
ErrCode ComponentImpl<Intf, Intfs...>::unlockAttributes(IList* attributes)
{
    if (!attributes)
        return OPENDAQ_SUCCESS;

    std::scoped_lock lock(sync);

    if (this->isComponentRemoved)
        return OPENDAQ_ERR_COMPONENT_REMOVED;

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

    if (this->isComponentRemoved)
        return OPENDAQ_ERR_COMPONENT_REMOVED;

    lockedAttributes.clear();
    return OPENDAQ_SUCCESS;
}

template <class Intf, class ... Intfs>
ErrCode ComponentImpl<Intf, Intfs...>::getLockedAttributes(IList** attributes)
{
    OPENDAQ_PARAM_NOT_NULL(attributes);
    
    std::scoped_lock lock(sync);

    if (this->isComponentRemoved)
        return OPENDAQ_ERR_COMPONENT_REMOVED;

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

template <class Intf, class ... Intfs>
ErrCode ComponentImpl<Intf, Intfs...>::findComponent(IString* id, IComponent** outComponent)
{
    OPENDAQ_PARAM_NOT_NULL(outComponent);
    OPENDAQ_PARAM_NOT_NULL(id);

    return daqTry(
        [&]()
        {
            
            std::string str = StringPtr(id);
            if (str != "" && str[0] == '/')
            {
                str.erase(str.begin(), str.begin() + 1);
                std::string startStr;
                std::string restStr;
                IdsParser::splitRelativeId(str, startStr, restStr);
                if (startStr == localId)
                    str = restStr;
            }

            *outComponent = findComponentInternal(this->template borrowPtr<ComponentPtr>(), str).detach();

            return *outComponent == nullptr ? OPENDAQ_NOTFOUND : OPENDAQ_SUCCESS;
        });
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

template <class Intf, class ... Intfs>
ErrCode INTERFACE_FUNC ComponentImpl<Intf, Intfs...>::updateInternal(ISerializedObject* obj, IBaseObject* context)
{
    const auto objPtr = SerializedObjectPtr::Borrow(obj);
    const auto contextPtr = BaseObjectPtr::Borrow(context);

    return daqTry([&objPtr, &contextPtr, this]
    {
        const bool muted = this->coreEventMuted;
        const auto thisPtr = this->template borrowPtr<ComponentPtr>();
        const auto propInternalPtr = this->template borrowPtr<PropertyObjectInternalPtr>();
        if (!muted)
            propInternalPtr.disableCoreEventTrigger();

        const auto err = Super::updateInternal(objPtr, contextPtr);

        updateObject(objPtr, contextPtr);

        if (!muted && this->coreEvent.assigned())
        {
            const CoreEventArgsPtr args = createWithImplementation<ICoreEventArgs, CoreEventArgsImpl>(CoreEventId::ComponentUpdateEnd, Dict<IString, IBaseObject>());
            triggerCoreEvent(args);
            propInternalPtr.enableCoreEventTrigger();
        }
        return err;
    });
}

template <class Intf, class... Intfs>
ErrCode INTERFACE_FUNC ComponentImpl<Intf, Intfs...>::update(ISerializedObject* obj)
{
    auto errCode = updateInternal(obj, nullptr);
    if (OPENDAQ_FAILED(errCode))
        return errCode;
    return updateEnded();
}

template <class Intf, class ... Intfs>
ErrCode ComponentImpl<Intf, Intfs...>::deserializeValues(ISerializedObject* serializedObject, IBaseObject* context, IFunction* callbackFactory)
{
    auto serializedObjectPtr = SerializedObjectPtr::Borrow(serializedObject);
    auto contextPtr = BaseObjectPtr::Borrow(context);
    auto callbackFactoryPtr = FunctionPtr::Borrow(callbackFactory);

    return daqTry(
        [&serializedObjectPtr, &contextPtr, &callbackFactoryPtr, this]()
        {
            deserializeCustomObjectValues(serializedObjectPtr, contextPtr, callbackFactoryPtr);
            return OPENDAQ_SUCCESS;
        });
}

template <class Intf, class ... Intfs>
ErrCode ComponentImpl<Intf, Intfs...>::complete()
{
    return OPENDAQ_SUCCESS;
}

template <class Intf, class... Intfs>
ErrCode INTERFACE_FUNC ComponentImpl<Intf, Intfs...>::getDeserializedParameter(IString* parameter, IBaseObject** value)
{
    OPENDAQ_PARAM_NOT_NULL(parameter);
    OPENDAQ_PARAM_NOT_NULL(value);

    return daqTry([this, &parameter, &value]
        {
            const auto parameterPtr = StringPtr::Borrow(parameter);
            *value = getDeserializedParameter(parameterPtr).detach();
        });
}

template <class Intf, class ... Intfs>
ErrCode ComponentImpl<Intf, Intfs...>::getSerializeId(ConstCharPtr* id) const
{
    *id = SerializeId();
    return OPENDAQ_SUCCESS;
}

template <class Intf, class ... Intfs>
ConstCharPtr ComponentImpl<Intf, Intfs...>::SerializeId()
{
    return "Component";
}

template <class Intf, class ... Intfs>
ErrCode ComponentImpl<Intf, Intfs...>::Deserialize(ISerializedObject* serialized,
    IBaseObject* context,
    IFunction* factoryCallback,
    IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(obj);

    return daqTry(
        [&obj, &serialized, &context, &factoryCallback]()
        {
            *obj = DeserializeComponent(
                serialized,
                context,
                factoryCallback, 
                [](const SerializedObjectPtr&, const ComponentDeserializeContextPtr& deserializeContext, const StringPtr& className)
                {
                    return createWithImplementation<IComponent, ComponentImpl>(
                        deserializeContext.getContext(),
                        deserializeContext.getParent(),
                        deserializeContext.getLocalId(),
                        className);
                }).detach();
        });
}

template <class Intf, class... Intfs>
template <class CreateComponentCallback>
BaseObjectPtr ComponentImpl<Intf, Intfs...>::DeserializeComponent(const SerializedObjectPtr& serialized,
                                                                  const BaseObjectPtr& context,
                                                                  const FunctionPtr& factoryCallback,
                                                                  CreateComponentCallback&& createComponentCallback)
{
    if (!serialized.assigned())
        throw ArgumentNullException("Serialized object not assigned");

    if (!context.assigned())
        throw ArgumentNullException("Deserialization context not assigned");

    const auto componentDeserializeContext = context.asPtrOrNull<IComponentDeserializeContext>(true);
    if (!componentDeserializeContext.assigned())
        throw InvalidParameterException("Invalid deserialization context");

    ComponentPtr component = Super::DeserializePropertyObject(
        serialized,
        context,
        factoryCallback,
        [&componentDeserializeContext, &createComponentCallback, &factoryCallback](
            const SerializedObjectPtr& serialized, const BaseObjectPtr& context, const StringPtr& className)
        {
            ComponentPtr component = createComponentCallback(serialized, componentDeserializeContext, className);
            component.asPtr<IDeserializeComponent>(true).deserializeValues(serialized, context, factoryCallback);
            return component;
        });

    const auto deserializeComponent = component.asPtr<IDeserializeComponent>(true);
    deserializeComponent.complete();

    return component;
}

template <class Intf, class... Intfs>
void ComponentImpl<Intf, Intfs...>::activeChanged()
{
}

template <class Intf, class... Intfs>
void ComponentImpl<Intf, Intfs...>::visibleChanged()
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

    if (searchFilter.supportsInterface<IRecursiveSearch>())
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

template <class Intf, class ... Intfs>
void ComponentImpl<Intf, Intfs...>::setActiveRecursive(const std::vector<ComponentPtr>& items, Bool active)
{
    const bool muted = this->coreEventMuted;
    const auto propInternalPtr = this->template borrowPtr<PropertyObjectInternalPtr>();
    if (!muted)
        propInternalPtr.disableCoreEventTrigger();

    for (const auto& item : items)
        item.setActive(active);
    
    if (!muted)
        propInternalPtr.enableCoreEventTrigger();
}

template <class Intf, class... Intfs>
ErrCode ComponentImpl<Intf, Intfs...>::serializeCustomValues(ISerializer* serializer, bool forUpdate)
{
    const auto serializerPtr = SerializerPtr::Borrow(serializer);

    auto errCode = Super::serializeCustomValues(serializer, forUpdate);
    if (OPENDAQ_FAILED(errCode))
        return errCode;

    return daqTry(
        [&serializerPtr, forUpdate, this]()
        {
            serializeCustomObjectValues(serializerPtr, forUpdate);

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
void ComponentImpl<Intf, Intfs...>::updateObject(const SerializedObjectPtr& obj, const BaseObjectPtr& /* context */)
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

template <class Intf, class ... Intfs>
void ComponentImpl<Intf, Intfs...>::onUpdatableUpdateEnd()
{
}

template <class Intf, class... Intfs>
void ComponentImpl<Intf, Intfs...>::serializeCustomObjectValues(const SerializerPtr& serializer, bool /* forUpdate */)
{
    const auto flags = getSerializeFlags();

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

    if (!tags.asPtr<ITags>().getList().empty())
    {
        serializer.key("tags");
        tags.serialize(serializer);
    }

    if (statusContainer.getStatuses().getCount() > 0)
    {
        serializer.key("statuses");
        statusContainer.serialize(serializer);
    }
}

template <class Intf, class... Intfs>
BaseObjectPtr ComponentImpl<Intf, Intfs...>::getDeserializedParameter(const StringPtr&)
{
    return {};
}

template <class Intf, class ... Intfs>
ComponentPtr ComponentImpl<Intf, Intfs...>::findComponentInternal(const ComponentPtr& component, const std::string& id)
{
    if (id == "")
        return component;

    std::string startStr;
    std::string restStr;
    const bool hasSubComponentStr = IdsParser::splitRelativeId(id, startStr, restStr);
    if (!hasSubComponentStr)
        startStr = id;

    const auto folder = component.asPtrOrNull<IFolder>(true);
    if (!folder.assigned())
        return nullptr;

    if (folder.hasItem(startStr))
    {
        const auto subComponent = folder.getItem(startStr);
        if (hasSubComponentStr)
            return findComponentInternal(subComponent, restStr);

        return subComponent;
    }

    return nullptr;
}

template <class Intf, class ... Intfs>
PropertyObjectPtr ComponentImpl<Intf, Intfs...>::getPropertyObjectParent()
{
    if (parent.assigned())
        return parent.getRef();

    return nullptr;
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

template <class Intf, class ... Intfs>
void ComponentImpl<Intf, Intfs...>::deserializeCustomObjectValues(const SerializedObjectPtr& serializedObject,
                                                            const BaseObjectPtr& context,
                                                            const FunctionPtr& /*factoryCallback*/)
{
    if (serializedObject.hasKey("active"))
        active = serializedObject.readBool("active");

    if (serializedObject.hasKey("visible"))
        visible = serializedObject.readBool("visible");

    if (serializedObject.hasKey("description"))
        description = serializedObject.readString("description");

    if (serializedObject.hasKey("name"))
        name = serializedObject.readString("name");

    if (serializedObject.hasKey("tags"))
        tags = serializedObject.readObject(
            "tags",
            context,
            [this](const StringPtr& typeId,
                   const SerializedObjectPtr& object,
                   const BaseObjectPtr& context,
                   const FunctionPtr& factoryCallback) -> BaseObjectPtr
            {
                if (typeId == TagsImpl::SerializeId())
                {
                    ObjectPtr<ITagsPrivate> tags;
                    auto errCode = createObject<ITagsPrivate, TagsImpl>(&tags,
                        [this](const CoreEventArgsPtr& args)
                        {
                            if (!this->coreEventMuted)
                                triggerCoreEvent(args);
                        });
                    if (OPENDAQ_FAILED(errCode))
                        return errCode;

                    const auto list = object.readList<IString>("list", context, factoryCallback);
                    for (const auto& tag : list)
                        tags->add(tag);

                    return tags;
                }
                return nullptr;
            });

    if (serializedObject.hasKey("statuses"))
        statusContainer = serializedObject.readObject(
            "statuses",
            context,
            [this](const StringPtr& typeId,
                   const SerializedObjectPtr& object,
                   const BaseObjectPtr& context,
                   const FunctionPtr& factoryCallback) -> BaseObjectPtr
            {
                if (typeId == ComponentStatusContainerImpl::SerializeId())
                {
                    auto container = createWithImplementation<IComponentStatusContainerPrivate, ComponentStatusContainerImpl>(
                        [this](const CoreEventArgsPtr& args)
                        {
                            if (!this->coreEventMuted)
                                triggerCoreEvent(args);
                        });

                    DictPtr<IString, IEnumeration> statuses = object.readObject("statuses", context, factoryCallback);
                    for (const auto& [name, value] : statuses)
                        container->addStatus(name, value);
                    return container;
                }
                return nullptr;
            });
}

template <class Intf, class... Intfs>
bool ComponentImpl<Intf, Intfs...>::validateComponentId(const std::string& id)
{
    return id.find(' ') == std::string::npos;
}

using StandardComponent = ComponentImpl<>;

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(StandardComponent)


END_NAMESPACE_OPENDAQ
