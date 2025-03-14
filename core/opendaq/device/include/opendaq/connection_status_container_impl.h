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
#include <opendaq/connection_status_container_private.h>
#include <opendaq/component_status_container_impl.h>
#include <coretypes/validation.h>
#include <coreobjects/core_event_args_impl.h>

#include <opendaq/server_capability.h>
#include <opendaq/module_manager_utils_ptr.h>
#include <opendaq/component_deserialize_context_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

static const char* ConfigurationConnectionStatusAlias = "ConfigurationStatus";

class ConnectionStatusContainerImpl final : public StatusContainerBase<IComponentStatusContainer, IConnectionStatusContainerPrivate, ISerializable>
{
public:
    using Super = StatusContainerBase<IComponentStatusContainer, IConnectionStatusContainerPrivate, ISerializable>;

    explicit ConnectionStatusContainerImpl();
    explicit ConnectionStatusContainerImpl(const ContextPtr& context, const ProcedurePtr& coreEventCallback);

    // IComponentStatusContainer
    ErrCode INTERFACE_FUNC getStatus(IString* name, IEnumeration** value) override;
    ErrCode INTERFACE_FUNC getStatuses(IDict** statuses) override;
    ErrCode INTERFACE_FUNC getStatusMessage(IString* name, IString** message) override;

    // IConnectionStatusContainerPrivate
    ErrCode INTERFACE_FUNC addConfigurationConnectionStatus(IString* connectionString, IEnumeration* initialValue) override;
    ErrCode INTERFACE_FUNC addStreamingConnectionStatus(IString* connectionString, IEnumeration* initialValue, IStreaming* streamingObject) override;
    ErrCode INTERFACE_FUNC removeStreamingConnectionStatus(IString* connectionString) override;
    ErrCode INTERFACE_FUNC updateConnectionStatus(IString* connectionString, IEnumeration* value, IStreaming* streamingObject) override;
    ErrCode INTERFACE_FUNC updateConnectionStatusWithMessage(IString* connectionString, IEnumeration* value, IStreaming* streamingObject, IString* message) override;

    // ISerializable
    ErrCode INTERFACE_FUNC serialize(ISerializer* serializer) override;
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;

    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

private:
    StringPtr getStreamingStatusNameAlias(const StringPtr& connectionString);

    ContextPtr context;
    DictPtr<IString, IString> statusNameAliases;
    SizeT streamingConnectionsCounter;
    bool configConnectionStatusAdded{false};
};

inline ConnectionStatusContainerImpl::ConnectionStatusContainerImpl()
    : ConnectionStatusContainerImpl(nullptr, nullptr)
{
}

inline ConnectionStatusContainerImpl::ConnectionStatusContainerImpl(const ContextPtr& context, const ProcedurePtr& coreEventCallback)
    : Super(coreEventCallback)
    , context(context)
    , statusNameAliases(Dict<IString, IString>())
    , streamingConnectionsCounter(0)
{
}

inline ErrCode ConnectionStatusContainerImpl::getStatus(IString* name, IEnumeration** value)
{
    OPENDAQ_PARAM_NOT_NULL(name);
    OPENDAQ_PARAM_NOT_NULL(value);

    const auto nameObj = StringPtr::Borrow(name);
    if (nameObj == "")
        return OPENDAQ_ERR_INVALIDPARAMETER;

    std::scoped_lock lock(sync);

    for (const auto& [connectionString, nameAlias] : statusNameAliases)
    {
        if (nameAlias == nameObj && statuses.hasKey(connectionString))
        {
            *value = statuses.get(connectionString).addRefAndReturn();
            return OPENDAQ_SUCCESS;
        }
    }

    return OPENDAQ_ERR_NOTFOUND;
}

inline ErrCode ConnectionStatusContainerImpl::getStatusMessage(IString* name, IString** message)
{
    OPENDAQ_PARAM_NOT_NULL(name);
    OPENDAQ_PARAM_NOT_NULL(message);

    const auto nameObj = StringPtr::Borrow(name);
    if (nameObj == "")
        return OPENDAQ_ERR_INVALIDPARAMETER;

    std::scoped_lock lock(sync);

    for (const auto& [connectionString, nameAlias] : statusNameAliases)
    {
        if (nameAlias == nameObj && messages.hasKey(connectionString))
        {
            *message = messages.get(connectionString).addRefAndReturn();
            return OPENDAQ_SUCCESS;
        }
    }

    return OPENDAQ_ERR_NOTFOUND;
}

inline ErrCode ConnectionStatusContainerImpl::getStatuses(IDict** statuses)
{
    OPENDAQ_PARAM_NOT_NULL(statuses);

    auto statusesAliased = Dict<IString, IEnumeration>();

    std::scoped_lock lock(sync);

    for (const auto& [connectionString, nameAlias] : statusNameAliases)
    {
        if (this->statuses.hasKey(connectionString))
            statusesAliased[nameAlias] = this->statuses.get(connectionString);
    }

    *statuses = statusesAliased.detach();
    return OPENDAQ_SUCCESS;
}

inline ErrCode ConnectionStatusContainerImpl::addConfigurationConnectionStatus(IString* connectionString, IEnumeration* initialValue)
{
    OPENDAQ_PARAM_NOT_NULL(connectionString);
    OPENDAQ_PARAM_NOT_NULL(initialValue);

    const auto connectionStringObj = StringPtr::Borrow(connectionString);
    if (connectionStringObj == "")
        return OPENDAQ_ERR_INVALIDPARAMETER;

    std::scoped_lock lock(sync);

    if (configConnectionStatusAdded || statuses.hasKey(connectionStringObj) || messages.hasKey(connectionStringObj))
        return OPENDAQ_ERR_ALREADYEXISTS;

    const auto message = String("");
    statuses[connectionStringObj] = initialValue;
    messages[connectionStringObj] = message;
    statusNameAliases[connectionStringObj] = ConfigurationConnectionStatusAlias;
    configConnectionStatusAdded = true;

    if (triggerCoreEvent.assigned())
    {
        const CoreEventArgsPtr args = createWithImplementation<ICoreEventArgs, CoreEventArgsImpl>(
            CoreEventId::ConnectionStatusChanged, Dict<IString, IBaseObject>({
                                        {"StatusName", ConfigurationConnectionStatusAlias},
                                        {"StatusValue", initialValue},
                                        {"ConnectionString", connectionStringObj},
                                        {"ProtocolType", Integer((Int)ProtocolType::Configuration)},
                                        {"StreamingObject", nullptr},
                                        {"Message", message}}));
        triggerCoreEvent(args);
    }

    return OPENDAQ_SUCCESS;
}

inline ErrCode ConnectionStatusContainerImpl::addStreamingConnectionStatus(IString* connectionString, IEnumeration* initialValue, IStreaming* streamingObject)
{
    OPENDAQ_PARAM_NOT_NULL(connectionString);
    OPENDAQ_PARAM_NOT_NULL(initialValue);

    const auto connectionStringObj = StringPtr::Borrow(connectionString);
    if (connectionStringObj == "")
        return OPENDAQ_ERR_INVALIDPARAMETER;

    std::scoped_lock lock(sync);

    if (statuses.hasKey(connectionStringObj) || messages.hasKey(connectionStringObj))
        return OPENDAQ_ERR_ALREADYEXISTS;

    ++streamingConnectionsCounter;
    const auto message = String("");
    statuses[connectionStringObj] = initialValue;
    messages[connectionStringObj] = message;
    const StringPtr statusNameAlias = getStreamingStatusNameAlias(connectionStringObj);
    statusNameAliases[connectionStringObj] = statusNameAlias;

    if (triggerCoreEvent.assigned())
    {
        const CoreEventArgsPtr args = createWithImplementation<ICoreEventArgs, CoreEventArgsImpl>(
            CoreEventId::ConnectionStatusChanged, Dict<IString, IBaseObject>({
                                        {"StatusName", statusNameAlias},
                                        {"StatusValue", initialValue},
                                        {"ConnectionString", connectionStringObj},
                                        {"ProtocolType", Integer((Int)ProtocolType::Streaming)},
                                        {"StreamingObject", streamingObject},
                                        {"Message", message}}));
        triggerCoreEvent(args);
    }

    return OPENDAQ_SUCCESS;
}

inline ErrCode ConnectionStatusContainerImpl::removeStreamingConnectionStatus(IString* connectionString)
{
    OPENDAQ_PARAM_NOT_NULL(connectionString);

    std::scoped_lock lock(sync);

    if (!statuses.hasKey(connectionString) || !messages.hasKey(connectionString))
        return OPENDAQ_ERR_NOTFOUND;

    const StringPtr statusNameAlias =
        statusNameAliases.hasKey(connectionString)
            ? statusNameAliases.remove(connectionString)
            : nullptr;

    messages.remove(connectionString);
    auto value = statuses.remove(connectionString);
    value = "Removed";

    if (triggerCoreEvent.assigned())
    {
        const CoreEventArgsPtr args = createWithImplementation<ICoreEventArgs, CoreEventArgsImpl>(
            CoreEventId::ConnectionStatusChanged, Dict<IString, IBaseObject>({
                                        {"StatusName", statusNameAlias},
                                        {"StatusValue", value},
                                        {"ConnectionString", connectionString},
                                        {"ProtocolType", Integer((Int)ProtocolType::Streaming)},
                                        {"StreamingObject", nullptr},
                                        {"Message", nullptr}}));
        triggerCoreEvent(args);
    }

    return OPENDAQ_SUCCESS;
}

inline ErrCode ConnectionStatusContainerImpl::updateConnectionStatus(IString* connectionString, IEnumeration* value, IStreaming* streamingObject)
{
    return updateConnectionStatusWithMessage(connectionString, value, streamingObject, String(""));
}

inline ErrCode ConnectionStatusContainerImpl::updateConnectionStatusWithMessage(IString* connectionString, IEnumeration* value, IStreaming* streamingObject, IString* message)
{
    OPENDAQ_PARAM_NOT_NULL(connectionString);
    OPENDAQ_PARAM_NOT_NULL(value);
    OPENDAQ_PARAM_NOT_NULL(message);

    const auto connectionStringObj = StringPtr::Borrow(connectionString);
    if (connectionStringObj == "")
        return OPENDAQ_ERR_INVALIDPARAMETER;
    const auto messageObj = StringPtr::Borrow(message);

    std::scoped_lock lock(sync);

    if (!statuses.hasKey(connectionStringObj) || !messages.hasKey(connectionStringObj))
        return OPENDAQ_ERR_NOTFOUND;

    const auto valueObj = EnumerationPtr::Borrow(value);
    const auto oldValue = statuses.get(connectionStringObj);
    const auto oldMessage = messages.get(connectionStringObj);

    if (valueObj.getEnumerationType() != oldValue.getEnumerationType())
        return OPENDAQ_ERR_INVALIDTYPE;
    if (valueObj == oldValue && oldMessage == messageObj)
        return OPENDAQ_IGNORED;

    auto errCode = statuses->set(connectionStringObj, value);
    if (OPENDAQ_FAILED(errCode))
        return errCode;

    errCode = messages->set(connectionStringObj, message);
    if (OPENDAQ_FAILED(errCode))
        return errCode;

    const StringPtr statusNameAlias = statusNameAliases.getOrDefault(connectionStringObj);

    const IntegerPtr connectionType =
        (statusNameAlias != ConfigurationConnectionStatusAlias)
            ? (Int)ProtocolType::Streaming
            : (Int)ProtocolType::Configuration;

    if (triggerCoreEvent.assigned())
    {
        const CoreEventArgsPtr args = createWithImplementation<ICoreEventArgs, CoreEventArgsImpl>(
            CoreEventId::ConnectionStatusChanged, Dict<IString, IBaseObject>({
                                        {"StatusName", statusNameAlias},
                                        {"StatusValue", value},
                                        {"ConnectionString", connectionStringObj},
                                        {"ProtocolType", connectionType},
                                        {"StreamingObject", streamingObject},
                                        {"Message", message}}));
        triggerCoreEvent(args);
    }

    return OPENDAQ_SUCCESS;
}

inline ErrCode ConnectionStatusContainerImpl::serialize(ISerializer* serializer)
{
    OPENDAQ_PARAM_NOT_NULL(serializer);

    serializer->startTaggedObject(this);

    serializer->key("connectionStatuses");
    statuses.serialize(serializer);

    serializer->key("statusNames");
    statusNameAliases.serialize(serializer);

    serializer->key("messages");
    messages.serialize(serializer);

    serializer->endObject();

    return OPENDAQ_SUCCESS;
}

inline ErrCode ConnectionStatusContainerImpl::getSerializeId(ConstCharPtr *id) const
{
    OPENDAQ_PARAM_NOT_NULL(id);

    *id = SerializeId();

    return OPENDAQ_SUCCESS;
}

inline ConstCharPtr ConnectionStatusContainerImpl::SerializeId()
{
    return "ConnectionStatusContainer";
}

inline ErrCode ConnectionStatusContainerImpl::Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(serialized);
    OPENDAQ_PARAM_NOT_NULL(context);
    OPENDAQ_PARAM_NOT_NULL(obj);

    const auto contextPtr = BaseObjectPtr::Borrow(context);
    const auto deserializerContext = contextPtr.assigned() ? contextPtr.asPtrOrNull<IComponentDeserializeContext>() : nullptr;
    const ProcedurePtr triggerCoreEvent = deserializerContext.assigned() ? deserializerContext.getTriggerCoreEvent() : nullptr;
    const ContextPtr daqContext = deserializerContext.assigned() ? deserializerContext.getContext() : nullptr;

    ObjectPtr<IConnectionStatusContainerPrivate> statusContainer;
    auto errCode = createObject<IConnectionStatusContainerPrivate, ConnectionStatusContainerImpl>(&statusContainer, daqContext, triggerCoreEvent);
    if (OPENDAQ_FAILED(errCode))
        return errCode;

    const auto serializedObj = SerializedObjectPtr::Borrow(serialized);

    DictPtr<IString, IEnumeration> statuses = serializedObj.readObject("connectionStatuses", context, factoryCallback);
    DictPtr<IString, IString> statusNameAliases = serializedObj.readObject("statusNames", context, factoryCallback);

    // Supports backwards compatibility without messages
    DictPtr<IString, IString> messages = Dict<IString, IString>();
    if (serializedObj.hasKey("messages"))
        messages = serializedObj.readObject("messages", context, factoryCallback);

    for (const auto& [connString, nameAlias] : statusNameAliases)
    {
        if (nameAlias == ConfigurationConnectionStatusAlias && statuses.hasKey(connString))
        {
            errCode = statusContainer->addConfigurationConnectionStatus(connString, statuses.get(connString));
            if (OPENDAQ_FAILED(errCode))
                return errCode;
            if (messages.hasKey(connString))
            {
                errCode = statusContainer->updateConnectionStatusWithMessage(connString, statuses.get(connString), nullptr, messages.get(connString));
                if (OPENDAQ_FAILED(errCode))
                    return errCode;
            }
            break;
        }
    }

    *obj = statusContainer.detach();

    return OPENDAQ_SUCCESS;
}

inline StringPtr ConnectionStatusContainerImpl::getStreamingStatusNameAlias(const StringPtr& connectionString)
{
    if (this->context.assigned() && this->context.getModuleManager().assigned())
    {
        const ModuleManagerUtilsPtr managerUtils = this->context.getModuleManager().template asPtr<IModuleManagerUtils>();
        auto streamingTypes = managerUtils.getAvailableStreamingTypes();

        for (const auto& [typeId, streamingType] : streamingTypes)
        {
            auto prefix = streamingType.getConnectionStringPrefix().toStdString();
            if (connectionString.toStdString().find(prefix) == 0)
                return String(fmt::format("StreamingStatus_{}_{}", typeId, streamingConnectionsCounter));
        }
    }
    return String(fmt::format("StreamingStatus_{}", streamingConnectionsCounter));
}

OPENDAQ_REGISTER_DESERIALIZE_FACTORY( ConnectionStatusContainerImpl)

END_NAMESPACE_OPENDAQ
