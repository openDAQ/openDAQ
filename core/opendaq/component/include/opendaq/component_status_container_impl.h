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
#include <opendaq/component_status_container_ptr.h>
#include <opendaq/component_status_container_private.h>
#include <opendaq/context_ptr.h>
#include <coretypes/intfs.h>
#include <coretypes/validation.h>
#include <coreobjects/core_event_args_impl.h>

BEGIN_NAMESPACE_OPENDAQ

class ComponentStatusContainerImpl final : public ImplementationOf<IComponentStatusContainer, IComponentStatusContainerPrivate, ISerializable>
{
public:
    explicit ComponentStatusContainerImpl();
    explicit ComponentStatusContainerImpl(const ProcedurePtr& coreEventCallback);

    // IComponentStatusContainer
    ErrCode INTERFACE_FUNC getStatus(IString* name, IEnumeration** value) override;
    ErrCode INTERFACE_FUNC getStatuses(IDict** statuses) override;

    // IComponentStatusContainerPrivate
    ErrCode INTERFACE_FUNC addStatus(IString* name, IEnumeration* initialValue) override;
    ErrCode INTERFACE_FUNC setStatus(IString* name, IEnumeration* value) override;

    // ISerializable
    ErrCode INTERFACE_FUNC serialize(ISerializer* serializer) override;
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;

    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

private:
    std::mutex sync;

    DictPtr<IString, IEnumeration> statuses;
    ProcedurePtr triggerCoreEvent;
};

inline ComponentStatusContainerImpl::ComponentStatusContainerImpl()
    : statuses(Dict<IString, IEnumeration>())
{
}

inline ComponentStatusContainerImpl::ComponentStatusContainerImpl(const ProcedurePtr &coreEventCallback)
    : statuses(Dict<IString, IEnumeration>())
    , triggerCoreEvent(coreEventCallback)
{
}

inline ErrCode ComponentStatusContainerImpl::getStatus(IString *name, IEnumeration **value)
{
    OPENDAQ_PARAM_NOT_NULL(name);
    OPENDAQ_PARAM_NOT_NULL(value);

    std::scoped_lock lock(sync);

    if (!statuses.hasKey(name))
        return OPENDAQ_ERR_NOTFOUND;

    *value = statuses.get(name).addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

inline ErrCode ComponentStatusContainerImpl::getStatuses(IDict **statuses)
{
    OPENDAQ_PARAM_NOT_NULL(statuses);

    std::scoped_lock lock(sync);

    *statuses = this->statuses.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

inline ErrCode ComponentStatusContainerImpl::addStatus(IString *name, IEnumeration *initialValue)
{
    OPENDAQ_PARAM_NOT_NULL(name);
    OPENDAQ_PARAM_NOT_NULL(initialValue);

    const auto nameObj = StringPtr::Borrow(name);
    if (nameObj == "")
        return OPENDAQ_ERR_INVALIDPARAMETER;

    std::scoped_lock lock(sync);

    if (statuses.hasKey(name))
        return OPENDAQ_ERR_ALREADYEXISTS;

    return statuses->set(name, initialValue);
}

inline ErrCode ComponentStatusContainerImpl::setStatus(IString *name, IEnumeration *value)
{
    OPENDAQ_PARAM_NOT_NULL(name);
    OPENDAQ_PARAM_NOT_NULL(value);

    const auto nameObj = StringPtr::Borrow(name);
    if (nameObj == "")
        return OPENDAQ_ERR_INVALIDPARAMETER;

    std::scoped_lock lock(sync);

    if (!statuses.hasKey(name))
        return OPENDAQ_ERR_NOTFOUND;

    const auto valueObj = EnumerationPtr::Borrow(value);
    const auto oldValue = statuses.get(name);
    if (valueObj.getEnumerationType() != oldValue.getEnumerationType())
        return OPENDAQ_ERR_INVALIDTYPE;
    if (valueObj == oldValue)
        return OPENDAQ_IGNORED;

    auto errCode = statuses->set(name, value);
    if (OPENDAQ_FAILED(errCode))
        return errCode;

    if (triggerCoreEvent.assigned())
    {
        const CoreEventArgsPtr args = createWithImplementation<ICoreEventArgs, CoreEventArgsImpl>(
            CoreEventId::StatusChanged, Dict<IString, IBaseObject>({{name, value}}));
        triggerCoreEvent(args);
    }

    return OPENDAQ_SUCCESS;
}

inline ErrCode ComponentStatusContainerImpl::serialize(ISerializer* serializer)
{
    OPENDAQ_PARAM_NOT_NULL(serializer);

    serializer->startTaggedObject(this);
    {
        serializer->key("statuses");
        statuses.serialize(serializer);
    }
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

    ObjectPtr<IComponentStatusContainerPrivate> statusContainer;
    auto errCode = createObject<IComponentStatusContainerPrivate, ComponentStatusContainerImpl>(&statusContainer);
    if (OPENDAQ_FAILED(errCode))
        return errCode;

    const auto serializedObj = SerializedObjectPtr::Borrow(serialized);

    DictPtr<IString, IEnumeration> statuses = serializedObj.readObject("statuses", context, factoryCallback);
    for (const auto& [name, value] : statuses)
        statusContainer->addStatus(name, value);

    *obj = statusContainer.detach();

    return OPENDAQ_SUCCESS;
}

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(ComponentStatusContainerImpl)

END_NAMESPACE_OPENDAQ
