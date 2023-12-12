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
#include <coreobjects/core_event_args_factory.h>
#include <coreobjects/property_object_impl.h>
#include <opendaq/component_ptr.h>
#include <coretypes/weakrefptr.h>
#include <opendaq/tags_factory.h>
#include <mutex>
#include <opendaq/custom_log.h>

BEGIN_NAMESPACE_OPENDAQ
    static constexpr int ComponentSerializeFlag_SerializeActiveProp = 1;

template <class Intf = IComponent, class ... Intfs>
class ComponentImpl : public GenericPropertyObjectImpl<Intf, IRemovable, Intfs ...>
{
public:
    using Super = GenericPropertyObjectImpl<Intf, IRemovable, Intfs ...>;

    ComponentImpl(const ContextPtr& context,
                  const ComponentPtr& parent,
                  const StringPtr& localId,
                  const StringPtr& className = nullptr,
                  ComponentStandardProps propsMode = ComponentStandardProps::Add);

    ErrCode INTERFACE_FUNC getLocalId(IString** localId) override;
    ErrCode INTERFACE_FUNC getGlobalId(IString** globalId) override;
    ErrCode INTERFACE_FUNC getActive(Bool* active) override;
    ErrCode INTERFACE_FUNC setActive(Bool active) override;
    ErrCode INTERFACE_FUNC getContext(IContext** context) override;
    ErrCode INTERFACE_FUNC getParent(IComponent** parent) override;
    ErrCode INTERFACE_FUNC getName(IString** name) override;
    ErrCode INTERFACE_FUNC setName(IString* name) override;
    ErrCode INTERFACE_FUNC getDescription(IString** description) override;
    ErrCode INTERFACE_FUNC setDescription(IString* description) override;
    ErrCode INTERFACE_FUNC getTags(ITagsConfig** tags) override;

    ErrCode INTERFACE_FUNC remove() override;
    ErrCode INTERFACE_FUNC isRemoved(Bool* removed) override;

    // IUpdatable
    ErrCode INTERFACE_FUNC update(ISerializedObject* obj) override;
protected:
    virtual void activeChanged();
    virtual void removed();

    std::mutex sync;
    ContextPtr context;

    bool active;
    bool isComponentRemoved;
    WeakRefPtr<IComponent> parent;
    StringPtr localId;
    TagsConfigPtr tags;
    StringPtr globalId;
    EventPtr<const ComponentPtr, const CoreEventArgsPtr> coreEvent;

    ErrCode serializeCustomValues(ISerializer* serializer) override;
    virtual int getSerializeFlags();

    std::unordered_map<std::string, SerializedObjectPtr> getSerializedItems(const SerializedObjectPtr& object);

    virtual void updateObject(const SerializedObjectPtr& obj);
    virtual void serializeCustomObjectValues(const SerializerPtr& serializer);
    static std::string getRelativeGlobalId(const std::string& globalId);

private:
    void initComponentProperties(ComponentStandardProps propsMode);
};

template <class Intf, class ... Intfs>
ComponentImpl<Intf, Intfs...>::ComponentImpl(
    const ContextPtr& context,
    const ComponentPtr& parent,
    const StringPtr& localId,
    const StringPtr& className,
    const ComponentStandardProps propsMode)
    : GenericPropertyObjectImpl<Intf, IRemovable, Intfs ...>(
        context.assigned() ? context.getTypeManager() : nullptr,
        className,
        [&](const CoreEventArgsPtr& args)
         {
             const ComponentPtr thisPtr = this->template borrowPtr<ComponentPtr>();
             this->coreEvent(thisPtr, args);
         })
      , context(context)
      , active(true)
      , isComponentRemoved(false)
      , parent(parent)
      , localId(localId)
      , tags(Tags())
{
    if (!localId.assigned() || localId.toStdString().empty())
        throw GeneralErrorException("Local id not assigned");

    if (parent.assigned())
        globalId = parent.getGlobalId().toStdString() + "/" + static_cast<std::string>(localId);
    else
        globalId = localId;

    if (!context.assigned())
        throw InvalidParameterException{"Context must be assigned on component creation"};

    context->getOnCoreEvent(&this->coreEvent);
    initComponentProperties(propsMode);
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
ErrCode ComponentImpl<Intf, Intfs ...>::setActive(Bool active)
{
    std::scoped_lock lock(sync);

    if (static_cast<bool>(active) == this->active)
        return  OPENDAQ_IGNORED;

    if (active && isComponentRemoved)
        return OPENDAQ_ERR_INVALIDSTATE;

    this->active = active;
    activeChanged();

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

    auto objPtr = this->template borrowPtr<ComponentPtr>();

    return daqTry([this, &name, &objPtr]()
        {
            if (!objPtr.hasProperty("Name"))
                *name = localId.addRefAndReturn();
            else
                *name = objPtr.getPropertyValue("Name").template asPtr<IString>().detach();
            return OPENDAQ_SUCCESS;
        });
}

template <class Intf, class ... Intfs>
ErrCode ComponentImpl<Intf, Intfs...>::setName(IString* name)
{
    auto namePtr = StringPtr::Borrow(name);
    auto objPtr = this->template borrowPtr<ComponentPtr>();

    if (!objPtr.hasProperty("Name"))
    {
        if (context.assigned() && context.getLogger().assigned())
        {
            const auto loggerComponent = context.getLogger().getOrAddComponent("Component");
            StringPtr nameObj;
            this->getName(&nameObj);
            LOG_I("Name of {} cannot be changed", nameObj);
        }
        return OPENDAQ_IGNORED;
    }

    return daqTry(
        [&namePtr, &objPtr, this]()
        {
            objPtr.setPropertyValue("Name", namePtr);
            return OPENDAQ_SUCCESS;
        });
}

template <class Intf, class ... Intfs>
ErrCode ComponentImpl<Intf, Intfs...>::getDescription(IString** description)
{
    OPENDAQ_PARAM_NOT_NULL(description);

    auto objPtr = this->template borrowPtr<ComponentPtr>();

    return daqTry(
        [&description, &objPtr]()
        {
            if (!objPtr.hasProperty("Description"))
                *description = String("").detach();
            else
                *description = objPtr.getPropertyValue("Description").template asPtr<IString>().detach();
            return OPENDAQ_SUCCESS;
        });
}

template <class Intf, class ... Intfs>
ErrCode ComponentImpl<Intf, Intfs...>::setDescription(IString* description)
{
    auto descPtr = StringPtr::Borrow(description);
    auto objPtr = this->template borrowPtr<ComponentPtr>();

    if (!objPtr.hasProperty("Description"))
    {
        if (context.assigned() && context.getLogger().assigned())
        {
            const auto loggerComponent = context.getLogger().getOrAddComponent("Component");
            StringPtr descObj;
            this->getDescription(&descObj);
            LOG_I("Description of {} cannot be changed", descObj);
        }
        return OPENDAQ_IGNORED;
    }

    return daqTry(
        [&descPtr, &objPtr, this]()
        {
            objPtr.setPropertyValue("Description", descPtr);
            return OPENDAQ_SUCCESS;
        });
}

template <class Intf, class ... Intfs>
ErrCode ComponentImpl<Intf, Intfs...>::getTags(ITagsConfig** tags)
{
    OPENDAQ_PARAM_NOT_NULL(tags);

    *tags = this->tags.addRefAndReturn();

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
            const auto err = Super::update(objPtr);

            updateObject(objPtr);

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
void ComponentImpl<Intf, Intfs...>::updateObject(const SerializedObjectPtr& /* obj */)
{
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

    if (!tags.getList().empty())
    {
        serializer.key("tags");
        tags.serialize(serializer);
    }
}

template <class Intf, class ... Intfs>
void ComponentImpl<Intf, Intfs...>::initComponentProperties(const ComponentStandardProps propsMode)
{
    if (propsMode == ComponentStandardProps::Skip)
        return;

    auto objPtr = this->template borrowPtr<ComponentPtr>();
    const auto nameProp = StringPropertyBuilder("Name", objPtr.getLocalId()).setReadOnly(propsMode == ComponentStandardProps::AddReadOnly).build();
    objPtr.addProperty(nameProp);

    const auto descProp = StringPropertyBuilder("Description", "").setReadOnly(propsMode == ComponentStandardProps::AddReadOnly).build();
    objPtr.addProperty(descProp);
}

template <class Intf, class... Intfs>
std::string ComponentImpl<Intf, Intfs...>::getRelativeGlobalId(const std::string& globalId)
{
    const auto equalsIdx = globalId.find_first_of('/');
    if (std::string::npos != equalsIdx)
        return globalId.substr(equalsIdx + 1);

    return globalId;
}

END_NAMESPACE_OPENDAQ
