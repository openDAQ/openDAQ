/*
 * Copyright 2022-2025 openDAQ d.o.o.
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
#include <opendaq/component_status_container_ptr.h>
#include <opendaq/component_status_container_private.h>
#include <opendaq/context_ptr.h>
#include <coretypes/intfs.h>
#include <coretypes/validation.h>
#include <coreobjects/core_event_args_impl.h>
#include <opendaq/component_deserialize_context_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

template <typename TInterface, typename... Interfaces>
class StatusContainerBase : public ImplementationOf<TInterface, Interfaces...>
{
public:
    explicit StatusContainerBase();
    explicit StatusContainerBase(const ProcedurePtr& coreEventCallback);

    // IComponentStatusContainer
    ErrCode INTERFACE_FUNC getStatus(IString* name, IEnumeration** value) override;
    ErrCode INTERFACE_FUNC getStatuses(IDict** statuses) override;
    ErrCode INTERFACE_FUNC getStatusMessage(IString* name, IString** message) override;

protected:
    std::recursive_mutex sync;

    DictPtr<IString, IEnumeration> statuses;
    DictPtr<IString, IString> messages;
    ProcedurePtr triggerCoreEvent;
};

template <typename TInterface, typename... Interfaces>
StatusContainerBase<TInterface, Interfaces...>::StatusContainerBase()
    : StatusContainerBase(nullptr)
{
}

template <typename TInterface, typename... Interfaces>
StatusContainerBase<TInterface, Interfaces...>::StatusContainerBase(const ProcedurePtr& coreEventCallback)
    : statuses(Dict<IString, IEnumeration>())
    , messages(Dict<IString, IString>())
    , triggerCoreEvent(coreEventCallback)
{
}

template <typename TInterface, typename... Interfaces>
ErrCode StatusContainerBase<TInterface, Interfaces...>::getStatus(IString* name, IEnumeration** value)
{
    OPENDAQ_PARAM_NOT_NULL(name);
    OPENDAQ_PARAM_NOT_NULL(value);

    std::scoped_lock lock(sync);

    if (!statuses.hasKey(name))
        return OPENDAQ_ERR_NOTFOUND;

    *value = statuses.get(name).addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... Interfaces>
ErrCode StatusContainerBase<TInterface, Interfaces...>::getStatuses(IDict** statuses)
{
    OPENDAQ_PARAM_NOT_NULL(statuses);

    std::scoped_lock lock(sync);

    auto dict = Dict<IString, IEnumeration>();

    for (const auto& [name, value]: this->statuses)
    {
        dict.set(name, value);
    }

    dict.freeze();

    *statuses = dict.detach();
    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... Interfaces>
ErrCode StatusContainerBase<TInterface, Interfaces...>::getStatusMessage(IString* name, IString** message)
{
    OPENDAQ_PARAM_NOT_NULL(name);
    OPENDAQ_PARAM_NOT_NULL(message);
    std::scoped_lock lock(sync);
    if (!messages.hasKey(name))
        return OPENDAQ_ERR_NOTFOUND;
    *message = messages.get(name).addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

// ComponentStatusContainer
class ComponentStatusContainerImpl final : public StatusContainerBase<IComponentStatusContainer, IComponentStatusContainerPrivate, ISerializable>
{
public:
    using Super = StatusContainerBase<IComponentStatusContainer, IComponentStatusContainerPrivate, ISerializable>;

    explicit ComponentStatusContainerImpl();
    explicit ComponentStatusContainerImpl(const ProcedurePtr& coreEventCallback);

    // IComponentStatusContainerPrivate
    ErrCode INTERFACE_FUNC addStatus(IString* name, IEnumeration* initialValue) override;
    ErrCode INTERFACE_FUNC addStatusWithMessage(IString* name, IEnumeration* initialValue, IString* message) override;
    ErrCode INTERFACE_FUNC setStatus(IString* name, IEnumeration* value) override;
    ErrCode INTERFACE_FUNC setStatusWithMessage(IString* name, IEnumeration* value, IString* message) override;

    // ISerializable
    ErrCode INTERFACE_FUNC serialize(ISerializer* serializer) override;
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;

    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);
};

inline ComponentStatusContainerImpl::ComponentStatusContainerImpl()
    : ComponentStatusContainerImpl(nullptr)
{
}

inline ComponentStatusContainerImpl::ComponentStatusContainerImpl(const ProcedurePtr& coreEventCallback)
    : Super(coreEventCallback)
{
}

inline ErrCode ComponentStatusContainerImpl::addStatus(IString* name, IEnumeration* initialValue)
{
    return addStatusWithMessage(name, initialValue, String(""));
}

inline ErrCode ComponentStatusContainerImpl::addStatusWithMessage(IString* name, IEnumeration* initialValue, IString* message)
{
    OPENDAQ_PARAM_NOT_NULL(name);
    OPENDAQ_PARAM_NOT_NULL(initialValue);
    OPENDAQ_PARAM_NOT_NULL(message);

    const auto nameObj = StringPtr::Borrow(name);
    if (nameObj == "")
        return OPENDAQ_ERR_INVALIDPARAMETER;

    std::scoped_lock lock(sync);

    if (statuses.hasKey(name))
        return OPENDAQ_ERR_ALREADYEXISTS;

    auto errCode = statuses->set(name, initialValue);
    if (OPENDAQ_FAILED(errCode))
        return errCode;

    errCode = messages->set(name, message);
    if (OPENDAQ_FAILED(errCode))
    {
        // Rollback
        statuses.remove(name);
        // Return error
        return errCode;
    }

    return OPENDAQ_SUCCESS;
}

inline ErrCode ComponentStatusContainerImpl::setStatus(IString *name, IEnumeration *value)
{
    return setStatusWithMessage(name, value, String(""));
}

inline ErrCode ComponentStatusContainerImpl::setStatusWithMessage(IString* name, IEnumeration* value, IString* message)
{
    OPENDAQ_PARAM_NOT_NULL(name);
    OPENDAQ_PARAM_NOT_NULL(value);
    OPENDAQ_PARAM_NOT_NULL(message);

    const auto nameObj = StringPtr::Borrow(name);
    if (nameObj == "")
        return OPENDAQ_ERR_INVALIDPARAMETER;
    const auto messageObj = StringPtr::Borrow(message);

    std::scoped_lock lock(sync);

    if (!statuses.hasKey(name))
        return OPENDAQ_ERR_NOTFOUND;

    const auto valueObj = EnumerationPtr::Borrow(value);
    const auto oldValue = statuses.get(name);
    if (valueObj.getEnumerationType() != oldValue.getEnumerationType())
        return OPENDAQ_ERR_INVALIDTYPE;
    if (valueObj == oldValue)
    {
        if (messages.get(name) == messageObj)
        {
            // No change in value or message
            return OPENDAQ_IGNORED;
        }
        // No change in value, change in message
        auto errCode = messages->set(name, messageObj);
        if (OPENDAQ_FAILED(errCode))
            return errCode;
    }
    else
    {
        // Change in value
        auto errCode = statuses->set(name, value);
        if (OPENDAQ_FAILED(errCode))
            return errCode;

        if (messages.get(name) != messageObj)
        {
            // Change in message
            errCode = messages->set(name, messageObj);
            if (OPENDAQ_FAILED(errCode))
            {
                // Rollback
                statuses.set(name, oldValue);
                // Return error
                return errCode;
            }
        }
    }

    if (triggerCoreEvent.assigned())
    {
        const CoreEventArgsPtr args = createWithImplementation<ICoreEventArgs, CoreEventArgsImpl>(
            CoreEventId::StatusChanged, Dict<IString, IBaseObject>({{name, value}, {"Message", message}}));
        triggerCoreEvent(args);
    }

    return OPENDAQ_SUCCESS;
}

inline ErrCode ComponentStatusContainerImpl::serialize(ISerializer* serializer)
{
    OPENDAQ_PARAM_NOT_NULL(serializer);

    serializer->startTaggedObject(this);

    serializer->key("statuses");
    statuses.serialize(serializer);

    serializer->key("messages");
    messages.serialize(serializer);

    serializer->endObject();

    return OPENDAQ_SUCCESS;
}

inline ErrCode ComponentStatusContainerImpl::getSerializeId(ConstCharPtr* id) const
{
    OPENDAQ_PARAM_NOT_NULL(id);

    *id = SerializeId();

    return OPENDAQ_SUCCESS;
}

inline ConstCharPtr ComponentStatusContainerImpl::SerializeId()
{
    return "ComponentStatusContainer";
}

inline ErrCode ComponentStatusContainerImpl::Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(serialized);
    OPENDAQ_PARAM_NOT_NULL(context);
    OPENDAQ_PARAM_NOT_NULL(obj);

    const auto contextPtr = BaseObjectPtr::Borrow(context);
    const auto deserializerContext = contextPtr.assigned() ? contextPtr.asPtrOrNull<IComponentDeserializeContext>() : nullptr;
    const ProcedurePtr triggerCoreEvent = deserializerContext.assigned() ? deserializerContext.getTriggerCoreEvent() : nullptr;

    ObjectPtr<IComponentStatusContainerPrivate> statusContainer;
    auto errCode = createObject<IComponentStatusContainerPrivate, ComponentStatusContainerImpl>(&statusContainer, triggerCoreEvent);
    if (OPENDAQ_FAILED(errCode))
        return errCode;

    const auto serializedObj = SerializedObjectPtr::Borrow(serialized);

    DictPtr<IString, IEnumeration> statuses = serializedObj.readObject("statuses", context, factoryCallback);

    if (serializedObj.hasKey("messages"))
    {
        DictPtr<IString, IString> messages = serializedObj.readObject("messages", context, factoryCallback);

        for (const auto& [name, value] : statuses)
        {
            errCode = statusContainer->addStatusWithMessage(name, value, messages.get(name));
            if (OPENDAQ_FAILED(errCode))
                return errCode;
        }
    }
    else
    {
        // For backwards compatibility without messages
        for (const auto& [name, value] : statuses)
        {
            errCode = statusContainer->addStatus(name, value);
            if (OPENDAQ_FAILED(errCode))
                return errCode;
        }
    }

    *obj = statusContainer.detach();

    return OPENDAQ_SUCCESS;
}

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(ComponentStatusContainerImpl)

END_NAMESPACE_OPENDAQ
