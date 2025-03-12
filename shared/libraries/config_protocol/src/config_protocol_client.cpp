#include <config_protocol/config_protocol_client.h>
#include <config_protocol/config_client_function_block_impl.h>
#include <config_protocol/config_client_input_port_impl.h>
#include <config_protocol/config_client_folder_impl.h>
#include <config_protocol/config_client_signal_impl.h>
#include <config_protocol/config_client_io_folder_impl.h>
#include <config_protocol/config_client_device_impl.h>
#include <config_protocol/config_client_channel_impl.h>
#include <config_protocol/config_client_sync_component_impl.h>
#include <config_protocol/config_client_server_impl.h>
#include <config_protocol/config_client_device_info_impl.h>
#include <config_protocol/config_protocol_deserialize_context_impl.h>
#include <opendaq/exceptions.h>

namespace daq::config_protocol
{

// ClientCommand

ClientCommand::ClientCommand(const std::string& name)
    : ClientCommand(name, 0)
{
}

daq::config_protocol::ClientCommand::ClientCommand(const std::string& name, uint16_t minServerVersion)
    : name(name)
    , minServerVersion(minServerVersion)
{
}

std::string ClientCommand::getName() const
{
    return name;
}

uint16_t ClientCommand::getMinServerVersion() const
{
    return minServerVersion;
}

// ConfigProtocolClientComm

ConfigProtocolClientComm::ConfigProtocolClientComm(const ContextPtr& daqContext,
                                                   SendRequestCallback sendRequestCallback,
                                                   SendNoReplyRequestCallback sendNoReplyRequestCallback,
                                                   const ConfigProtocolStreamingProducerPtr& streamingProducer,
                                                   ComponentDeserializeCallback rootDeviceDeserializeCallback)
    : daqContext(daqContext)
    , id(0)
    , sendRequestCallback(std::move(sendRequestCallback))
    , sendNoReplyRequestCallback(std::move(sendNoReplyRequestCallback))
    , rootDeviceDeserializeCallback(std::move(rootDeviceDeserializeCallback))
    , connected(false)
    , protocolVersion(0)
    , streamingProducerRef(streamingProducer)
    , loggerComponent(daqContext.getLogger().getOrAddComponent("NativeClient"))
{
}

uint64_t ConfigProtocolClientComm::generateId()
{
    return std::atomic_fetch_add_explicit(&id, uint64_t(1), std::memory_order_relaxed);
}

void ConfigProtocolClientComm::setPropertyValue(
    const std::string& globalId,
    const std::string& propertyName,
    const BaseObjectPtr& propertyValue)
{
    auto dict = Dict<IString, IBaseObject>();
    dict.set("ComponentGlobalId", String(globalId));
    dict.set("PropertyName", String(propertyName));
    dict.set("PropertyValue", propertyValue);
    auto setPropertyValueRpcRequestPacketBuffer = createRpcRequestPacketBuffer(generateId(), "SetPropertyValue", dict);
    const auto setPropertyValueRpcReplyPacketBuffer = sendRequestCallback(setPropertyValueRpcRequestPacketBuffer);

    // ReSharper disable once CppExpressionWithoutSideEffects
    parseRpcOrRejectReply(setPropertyValueRpcReplyPacketBuffer.parseRpcRequestOrReply());
}

void ConfigProtocolClientComm::setProtectedPropertyValue(const std::string& globalId,
                                                         const std::string& propertyName,
                                                         const BaseObjectPtr& propertyValue)
{
    auto dict = Dict<IString, IBaseObject>();
    dict.set("ComponentGlobalId", String(globalId));
    dict.set("PropertyName", String(propertyName));
    dict.set("PropertyValue", String(propertyValue));
    auto setProtectedPropertyValueRpcRequestPacketBuffer = createRpcRequestPacketBuffer(generateId(), "SetProtectedPropertyValue", dict);
    const auto setProtectedPropertyValueRpcReplyPacketBuffer = sendRequestCallback(setProtectedPropertyValueRpcRequestPacketBuffer);

    // ReSharper disable once CppExpressionWithoutSideEffects
    parseRpcOrRejectReply(setProtectedPropertyValueRpcReplyPacketBuffer.parseRpcRequestOrReply());
}

BaseObjectPtr ConfigProtocolClientComm::getPropertyValue(const std::string& globalId, const std::string& propertyName)
{
    auto dict = Dict<IString, IBaseObject>();
    dict.set("ComponentGlobalId", String(globalId));
    dict.set("PropertyName", String(propertyName));
    auto getPropertyValueRpcRequestPacketBuffer = createRpcRequestPacketBuffer(generateId(), "GetPropertyValue", dict);
    const auto getPropertyValueRpcReplyPacketBuffer = sendRequestCallback(getPropertyValueRpcRequestPacketBuffer);

    const auto deserializeContext = createDeserializeContext(std::string{}, daqContext, nullptr, nullptr, nullptr, nullptr);

    return parseRpcOrRejectReply(getPropertyValueRpcReplyPacketBuffer.parseRpcRequestOrReply(), deserializeContext);
}

void ConfigProtocolClientComm::clearPropertyValue(
    const std::string& globalId,
    const std::string& propertyName)
{
    auto dict = Dict<IString, IBaseObject>();
    dict.set("ComponentGlobalId", String(globalId));
    dict.set("PropertyName", String(propertyName));
    auto clearPropertyValueRpcRequestPacketBuffer = createRpcRequestPacketBuffer(generateId(), "ClearPropertyValue", dict);
    const auto clearPropertyValueRpcReplyPacketBuffer = sendRequestCallback(clearPropertyValueRpcRequestPacketBuffer);

    // ReSharper disable once CppExpressionWithoutSideEffects
    parseRpcOrRejectReply(clearPropertyValueRpcReplyPacketBuffer.parseRpcRequestOrReply());
}

void ConfigProtocolClientComm::update(const std::string& globalId, const std::string& serialized, const std::string& path)
{
    auto dict = Dict<IString, IBaseObject>();
    dict.set("ComponentGlobalId", String(globalId));
    dict.set("Serialized", String(serialized));
    dict.set("Path", String(path));
    auto updateRpcRequestPacketBuffer = createRpcRequestPacketBuffer(generateId(), "Update", dict);
    const auto updateRpcReplyPacketBuffer = sendRequestCallback(updateRpcRequestPacketBuffer );

    // ReSharper disable once CppExpressionWithoutSideEffects
    parseRpcOrRejectReply(updateRpcReplyPacketBuffer.parseRpcRequestOrReply());
}

BaseObjectPtr ConfigProtocolClientComm::callProperty(const std::string& globalId,
                                                     const std::string& propertyName,
                                                     const BaseObjectPtr& params)
{
    auto dict = Dict<IString, IBaseObject>();
    dict.set("ComponentGlobalId", String(globalId));
    dict.set("PropertyName", propertyName);
    if (params.assigned())
        dict.set("Params", params);
    auto callPropertyRpcRequestPacketBuffer = createRpcRequestPacketBuffer(generateId(), "CallProperty", dict);
    const auto callPropertyRpcReplyPacketBuffer = sendRequestCallback(callPropertyRpcRequestPacketBuffer);

    const auto result = parseRpcOrRejectReply(callPropertyRpcReplyPacketBuffer.parseRpcRequestOrReply());
    return result;
}

void ConfigProtocolClientComm::setAttributeValue(const std::string& globalId,
    const std::string& attributeName,
    const BaseObjectPtr& attributeValue)
{
    auto dict = Dict<IString, IBaseObject>();
    dict.set("ComponentGlobalId", String(globalId));
    dict.set("AttributeName", String(attributeName));
    dict.set("AttributeValue", String(attributeValue));
    auto setAttributeValueRpcRequestPacketBuffer = createRpcRequestPacketBuffer(generateId(), "SetAttributeValue", dict);
    const auto setAttributeValueRpcReplyPacketBuffer = sendRequestCallback(setAttributeValueRpcRequestPacketBuffer);

    // ReSharper disable once CppExpressionWithoutSideEffects
    parseRpcOrRejectReply(setAttributeValueRpcReplyPacketBuffer.parseRpcRequestOrReply());
}

void ConfigProtocolClientComm::beginUpdate(const std::string& globalId, const std::string& path)
{
    auto dict = Dict<IString, IBaseObject>();
    dict.set("ComponentGlobalId", String(globalId));
    if (!path.empty())
        dict.set("Path", String(path));
    auto setPropertyValueRpcRequestPacketBuffer = createRpcRequestPacketBuffer(generateId(), "BeginUpdate", dict);
    const auto setPropertyValueRpcReplyPacketBuffer = sendRequestCallback(setPropertyValueRpcRequestPacketBuffer);

    // ReSharper disable once CppExpressionWithoutSideEffects
    parseRpcOrRejectReply(setPropertyValueRpcReplyPacketBuffer.parseRpcRequestOrReply());
}

void ConfigProtocolClientComm::endUpdate(const std::string& globalId, const std::string& path, const ListPtr<IDict>& props)
{
    auto dict = Dict<IString, IBaseObject>();
    dict.set("ComponentGlobalId", String(globalId));
    if (!path.empty())
        dict.set("Path", String(path));
    if (props.assigned())
        dict.set("Props", props);
    auto setPropertyValueRpcRequestPacketBuffer = createRpcRequestPacketBuffer(generateId(), "EndUpdate", dict);
    const auto setPropertyValueRpcReplyPacketBuffer = sendRequestCallback(setPropertyValueRpcRequestPacketBuffer);

    // ReSharper disable once CppExpressionWithoutSideEffects
    parseRpcOrRejectReply(setPropertyValueRpcReplyPacketBuffer.parseRpcRequestOrReply());
}

DictPtr<IString, IFunctionBlockType> ConfigProtocolClientComm::getAvailableFunctionBlockTypes(const std::string& globalId, bool isFb)
{
    auto command = isFb ? ClientCommand("GetAvailableFunctionBlockTypes", 9) : ClientCommand("GetAvailableFunctionBlockTypes");
    return sendComponentCommand(globalId, command);
}

ComponentHolderPtr ConfigProtocolClientComm::addFunctionBlock(const std::string& globalId,
                                                              const StringPtr& typeId,
                                                              const PropertyObjectPtr& config,
                                                              const ComponentPtr& parentComponent,
                                                              bool isFb)
{
    auto command = isFb ? ClientCommand("AddFunctionBlock", 9) : ClientCommand("AddFunctionBlock");
    auto params = Dict<IString, IBaseObject>({{"TypeId", typeId}, {"Config", config}});
    return sendComponentCommand(globalId, command, params, parentComponent);
}

void ConfigProtocolClientComm::removeFunctionBlock(const std::string& globalId, const StringPtr& functionBlockLocalId, bool isFb)
{
    auto command = isFb ? ClientCommand("RemoveFunctionBlock", 9) : ClientCommand("RemoveFunctionBlock");
    auto params = Dict<IString, IBaseObject>({{"LocalId", functionBlockLocalId}});
    sendComponentCommand(globalId, command, params);
}

uint64_t ConfigProtocolClientComm::getTicksSinceOrigin(const std::string& globalId)
{
    return sendComponentCommand(globalId, ClientCommand("GetTicksSinceOrigin"));
}

ListPtr<IDeviceInfo> ConfigProtocolClientComm::getAvailableDevices(const std::string& globalId)
{
    return sendComponentCommand(globalId, ClientCommand("GetAvailableDevices", 4));
}

DictPtr<IString, IDeviceType> ConfigProtocolClientComm::getAvailableDeviceTypes(const std::string& globalId)
{
    return sendComponentCommand(globalId, ClientCommand("GetAvailableDeviceTypes", 4));
}

ComponentHolderPtr ConfigProtocolClientComm::addDevice(const std::string& globalId,
                                                       const StringPtr& connectionString,
                                                       const PropertyObjectPtr& config,
                                                       const ComponentPtr& parentComponent)
{
    auto params = Dict<IString, IBaseObject>({{"ConnectionString", connectionString}, {"Config", config}});
    return sendComponentCommand(globalId, ClientCommand("AddDevice", 4), params, parentComponent);
}

void ConfigProtocolClientComm::removeDevice(const std::string& globalId, const StringPtr& deviceLocalId)
{    auto params = Dict<IString, IBaseObject>({{"LocalId", deviceLocalId}});
    sendComponentCommand(globalId, ClientCommand("RemoveDevice", 4), params);
}

void ConfigProtocolClientComm::connectSignal(const std::string& globalId, const std::string& globaSignallId)
{
    auto params = ParamsDict({{"SignalId", globaSignallId}});
    sendComponentCommand(globalId, ClientCommand("ConnectSignal"), params, nullptr);
}

void ConfigProtocolClientComm::disconnectSignal(const std::string& globalId)
{
    sendComponentCommand(globalId, ClientCommand("DisconnectSignal"), nullptr);
}

BooleanPtr ConfigProtocolClientComm::acceptsSignal(const std::string& globalId, const std::string& globaSignallId)
{
    auto params = ParamsDict({{"SignalId", globaSignallId}});
    return sendComponentCommand(globalId, ClientCommand("AcceptsSignal"), params, nullptr);
}

DeviceInfoPtr ConfigProtocolClientComm::getInfo(const std::string& globalId)
{
    return sendComponentCommand(globalId, ClientCommand("GetInfo"));
}

TypeManagerPtr ConfigProtocolClientComm::getTypeManager()
{
    return sendCommand(ClientCommand("GetTypeManager"));
}

ListPtr<ILogFileInfo> ConfigProtocolClientComm::getLogFileInfos(const std::string& globalId)
{
    return sendComponentCommand(globalId, ClientCommand("getLogFileInfos", 5));
}

StringPtr ConfigProtocolClientComm::getLog(const std::string& globalId, const StringPtr& id, Int size, Int offset)
{
    auto params = Dict<IString, IBaseObject>({{"Id", id}, {"Size", size}, {"Offset", offset}});
    return sendComponentCommand(globalId, ClientCommand("GetLog", 5), params);
}

ListPtr<IString> ConfigProtocolClientComm::getAvailableOperationModes(const std::string& globalId)
{
    return sendComponentCommand(globalId, ClientCommand("GetAvailableOperationModes", 9));
}

void ConfigProtocolClientComm::setOperationMode(const std::string& globalId, const StringPtr& modeType)
{
    auto params = Dict<IString, IBaseObject>({{"ModeType", modeType}});
    sendComponentCommand(globalId, ClientCommand("SetOperationMode", 9), params);
}

void ConfigProtocolClientComm::setOperationModeRecursive(const std::string& globalId, const StringPtr& modeType)
{
    auto params = Dict<IString, IBaseObject>({{"ModeType", modeType}});
    sendComponentCommand(globalId, ClientCommand("SetOperationModeRecursive", 9), params);
}

StringPtr ConfigProtocolClientComm::getOperationMode(const std::string& globalId)
{
    return sendComponentCommand(globalId, ClientCommand("GetOperationMode", 9));
}

BaseObjectPtr ConfigProtocolClientComm::getLastValue(const std::string& globalId)
{
    auto dict = Dict<IString, IBaseObject>();
    dict.set("ComponentGlobalId", String(globalId));
    auto getPropertyValueRpcRequestPacketBuffer = createRpcRequestPacketBuffer(generateId(), "GetLastValue", dict);
    const auto getPropertyValueRpcReplyPacketBuffer = sendRequestCallback(getPropertyValueRpcRequestPacketBuffer);

    const auto deserializeContext = createDeserializeContext(std::string{}, daqContext, nullptr, nullptr, nullptr, nullptr);
    return parseRpcOrRejectReply(getPropertyValueRpcReplyPacketBuffer.parseRpcRequestOrReply(), deserializeContext);
}

void ConfigProtocolClientComm::lock(const std::string& globalId)
{
    auto params = Dict<IString, IBaseObject>();
    params.set("ComponentGlobalId", String(globalId));

    sendCommand(ClientCommand("Lock", 6), params);
}

void ConfigProtocolClientComm::unlock(const std::string& globalId)
{
    auto params = Dict<IString, IBaseObject>();
    params.set("ComponentGlobalId", String(globalId));

    sendCommand(ClientCommand("Unlock", 6), params);
}

void ConfigProtocolClientComm::forceUnlock(const std::string& globalId)
{
    auto params = Dict<IString, IBaseObject>();
    params.set("ComponentGlobalId", String(globalId));

    sendCommand(ClientCommand("ForceUnlock", 6), params);
}

bool ConfigProtocolClientComm::isLocked(const std::string& globalId)
{
    auto params = Dict<IString, IBaseObject>();
    params.set("ComponentGlobalId", String(globalId));

    return sendCommand(ClientCommand("IsLocked", 6), params);
}

BaseObjectPtr ConfigProtocolClientComm::createRpcRequest(const StringPtr& name, const ParamsDictPtr& params) const
{
    auto obj = Dict<IString, IBaseObject>();
    obj.set("Name", name);
    if (params.assigned() && params.getCount() > 0)
        obj.set("Params", params);

    return obj;
}

StringPtr ConfigProtocolClientComm::createRpcRequestJson(const StringPtr& name, const ParamsDictPtr& params)
{
    const auto obj = createRpcRequest(name, params);
    auto serializer = JsonSerializer();
    obj.serialize(serializer);
    return serializer.getOutput();
}

PacketBuffer ConfigProtocolClientComm::createRpcRequestPacketBuffer(const uint64_t id,
                                                                    const StringPtr& name,
                                                                    const ParamsDictPtr& params)
{
    const auto jsonStr = createRpcRequestJson(name, params);

    auto packetBuffer = PacketBuffer(PacketType::Rpc, id, jsonStr.getCharPtr(), jsonStr.getLength());
    return packetBuffer;
}

PacketBuffer ConfigProtocolClientComm::createNoReplyRpcRequestPacketBuffer(const StringPtr& name, const ParamsDictPtr& params)
{
    const auto jsonStr = createRpcRequestJson(name, params);

    auto packetBuffer = PacketBuffer::createNoReplyRpcRequest(jsonStr.getCharPtr(), jsonStr.getLength());
    return packetBuffer;
}

BaseObjectPtr ConfigProtocolClientComm::parseRpcOrRejectReply(const StringPtr& jsonReply,
                                                              const ComponentDeserializeContextPtr& context,
                                                              bool isGetRootDeviceReply)
{
    ParamsDictPtr reply;
    try
    {
        ComponentDeserializeCallback customDeviceDeserilazeCallback = isGetRootDeviceReply ? rootDeviceDeserializeCallback : nullptr;
        const auto deserializer = JsonDeserializer();
        reply = deserializer.deserialize(
            jsonReply,
            context,
            [this, &customDeviceDeserilazeCallback](const StringPtr& typeId, const SerializedObjectPtr& object, const BaseObjectPtr& context, const FunctionPtr& factoryCallback)
            {
                return deserializeConfigComponent(typeId, object, context, factoryCallback, customDeviceDeserilazeCallback);
            });
    }
    catch (const std::exception& e)
    {
        throw ConfigProtocolException(fmt::format("Invalid reply: {}", e.what()));
    }

    if (!reply.hasKey("ErrorCode"))
        throw ConfigProtocolException("Invalid reply");

    const ErrCode errCode = reply["ErrorCode"];
    if (OPENDAQ_FAILED(errCode))
    {
        std::string msg = reply.getOrDefault("ErrorMessage", "");
        throwExceptionFromErrorCode(errCode, msg);
    }

    return reply.getOrDefault("ReturnValue");
}

BaseObjectPtr ConfigProtocolClientComm::deserializeConfigComponent(const StringPtr& typeId,
                                                                   const SerializedObjectPtr& serObj,
                                                                   const BaseObjectPtr& context,
                                                                   const FunctionPtr& factoryCallback,
                                                                   ComponentDeserializeCallback deviceDeserialzeCallback)
{
    if (typeId == "Folder")
    {
        BaseObjectPtr obj;
        checkErrorInfo(ConfigClientFolderImpl::Deserialize(serObj, context, factoryCallback, &obj));
        return obj;
    }

    if (typeId == "Component")
    {
        BaseObjectPtr obj;
        checkErrorInfo(ConfigClientComponentImpl::Deserialize(serObj, context, factoryCallback, &obj));
        return obj;
    }

    if (typeId == "ComponentHolder")
    {
        const auto remoteContext = context.asPtr<IConfigProtocolDeserializeContext>(true);
        if(serObj.hasKey("parentGlobalId") && context.assigned())
            remoteContext->setRemoteGlobalId(serObj.readString("parentGlobalId"));
    }

    if (typeId == "IoFolder")
    {
        BaseObjectPtr obj;
        checkErrorInfo(ConfigClientIoFolderImpl::Deserialize(serObj, context, factoryCallback, &obj));
        return obj;
    }

    if (typeId == "InputPort")
    {
        BaseObjectPtr obj;
        checkErrorInfo(ConfigClientInputPortImpl::Deserialize(serObj, context, factoryCallback, &obj));
        return obj;
    }

    if (typeId == "Signal")
    {
        BaseObjectPtr obj;
        checkErrorInfo(ConfigClientSignalImpl::Deserialize(serObj, context, factoryCallback, &obj));
        return obj;
    }

    if (typeId == "Channel")
    {
        BaseObjectPtr obj;
        checkErrorInfo(ConfigClientChannelImpl::Deserialize(serObj, context, factoryCallback, &obj));
        return obj;
    }

    if (typeId == "FunctionBlock")
    {
        BaseObjectPtr obj;
        checkErrorInfo(ConfigClientFunctionBlockImpl::Deserialize(serObj, context, factoryCallback, &obj));
        return obj;
    }

    if (typeId == "Device" || typeId == "Instance")
    {
        BaseObjectPtr obj;
        if (deviceDeserialzeCallback)
            checkErrorInfo(deviceDeserialzeCallback(serObj, context, factoryCallback, &obj));
        else
            checkErrorInfo(ConfigClientDeviceImpl::Deserialize(serObj, context, factoryCallback, &obj));
        return obj;
    }

    if (typeId == "DeviceInfo")
    {
        if (protocolVersion < 8)
            return nullptr;
        BaseObjectPtr obj;
        checkErrorInfo(ConfigClientDeviceInfoImpl::Deserialize(serObj, context, factoryCallback, &obj));
        return obj;
    }

    if (typeId == "PropertyObject")
    {
        BaseObjectPtr obj;
        checkErrorInfo(ConfigClientPropertyObjectImpl::Deserialize(serObj, context, factoryCallback, &obj));
        return obj;
    }    

    if (typeId == "SyncComponent")
    {
        BaseObjectPtr obj;
        checkErrorInfo(ConfigClientSyncComponentImpl::Deserialize(serObj, context, factoryCallback, &obj));
        return obj;
    }

    if (typeId == "Server")
    {
        BaseObjectPtr obj;
        checkErrorInfo(ConfigClientServerImpl::Deserialize(serObj, context, factoryCallback, &obj));
        return obj;
    }

    return nullptr;
}

uint16_t ConfigProtocolClientComm::getProtocolVersion() const
{
    return protocolVersion;
}

bool ConfigProtocolClientComm::isComponentNested(const StringPtr& componentGlobalId)
{
    const auto dev = getRootDevice();
    if (!dev.assigned())
        return false;

    return IdsParser::isNestedComponentId(dev.getGlobalId().toStdString(), componentGlobalId.toStdString());
}

std::tuple<uint32_t, StringPtr, StringPtr> ConfigProtocolClientComm::getExternalSignalParams(
    const SignalPtr& signal,
    const ConfigProtocolStreamingProducerPtr& streamingProducer)
{
    auto serializer = JsonSerializer();
    signal.serialize(serializer);

    return std::make_tuple(streamingProducer->registerOrUpdateSignal(signal),
                           signal.getGlobalId(),
                           serializer.getOutput());
}

void ConfigProtocolClientComm::disconnectExternalSignalFromServerInputPort(const SignalPtr& signal,
                                                                           const StringPtr& inputPortRemoteGlobalId)
{
    const auto streamingProducer = streamingProducerRef.lock();
    if (!streamingProducer)
        return;

    std::vector<SignalNumericIdType> unusedSignals;
    streamingProducer->removeConnection(signal, inputPortRemoteGlobalId, unusedSignals);

    if (!unusedSignals.empty())
    {
        auto params = ParamsDict({{"SignalNumericIds", ListPtr<IInteger>::FromVector(unusedSignals)}});
        sendNoReplyCommand(ClientCommand("RemoveExternalSignals"), params);
    }
}

void ConfigProtocolClientComm::connectExternalSignalToServerInputPort(const SignalPtr& signal,
                                                                      const StringPtr& inputPortRemoteGlobalId)
{
    const auto streamingProducer = streamingProducerRef.lock();
    if (!streamingProducer)
        DAQ_THROW_EXCEPTION(NotAssignedException, "StreamingProducer is not assigned.");

    auto domainSignal = signal.getDomainSignal();
    const auto [domainSignalNumericId, domainSignalGlobalId, serializedDomainSignal] =
        (domainSignal.assigned())
            ? getExternalSignalParams(domainSignal, streamingProducer)
            : std::make_tuple(0, nullptr, nullptr);

    const auto [signalNumericId, signalGlobalId, serializedSignal] = getExternalSignalParams(signal, streamingProducer);

    auto params = ParamsDict({{"DomainSignalNumericId", domainSignalNumericId},
                              {"DomainSignalStringId", domainSignalGlobalId},
                              {"DomainSerializedSignal", serializedDomainSignal},
                              {"SignalNumericId", signalNumericId},
                              {"SignalStringId", signal.getGlobalId()},
                              {"SerializedSignal", serializedSignal}});

    sendComponentCommand(inputPortRemoteGlobalId, ClientCommand("ConnectExternalSignal"), params, nullptr);

    streamingProducer->addConnection(signal, inputPortRemoteGlobalId);
}

void ConfigProtocolClientComm::requireMinServerVersion(const ClientCommand& command)
{
    if (protocolVersion < command.getMinServerVersion())
    {
        LOG_W("The client attempted to call a function \"{}\" that requires a version {} of openDAQ server. Actual server version is {}.",
              command.getName(),
              command.getMinServerVersion(),
              protocolVersion);

        DAQ_THROW_EXCEPTION(ServerVersionTooLowException);
    }
}

ComponentDeserializeContextPtr ConfigProtocolClientComm::createDeserializeContext(const std::string& remoteGlobalId,
                                                                                  const ContextPtr& context,
                                                                                  const ComponentPtr& root,
                                                                                  const ComponentPtr& parent,
                                                                                  const StringPtr& localId,
                                                                                  IntfID* intfID)
{
    return createWithImplementation<IComponentDeserializeContext, ConfigProtocolDeserializeContextImpl>(
        shared_from_this(), remoteGlobalId, context, root, parent, localId, intfID);
}

bool ConfigProtocolClientComm::getConnected() const
{
    return connected;
}

ContextPtr ConfigProtocolClientComm::getDaqContext()
{
    return daqContext;
}

BaseObjectPtr ConfigProtocolClientComm::sendComponentCommand(const StringPtr& globalId,
                                                             const ClientCommand& command,
                                                             ParamsDictPtr& params,
                                                             const ComponentPtr& parentComponent)
{
    params.set("ComponentGlobalId", globalId);
    return sendComponentCommandInternal(command, params, parentComponent);
}

BaseObjectPtr ConfigProtocolClientComm::sendComponentCommand(const StringPtr& globalId,
                                                             const ClientCommand& command,
                                                             const ComponentPtr& parentComponent)
{
    auto params = Dict<IString, IBaseObject>();
    params.set("ComponentGlobalId", globalId);
    return sendComponentCommandInternal(command, params, parentComponent);
}

BaseObjectPtr ConfigProtocolClientComm::requestRootDevice(const ComponentPtr& parentComponent)
{
    auto params = Dict<IString, IBaseObject>();
    params.set("ComponentGlobalId", "//root");
    return sendComponentCommandInternal(ClientCommand("GetComponent"), params, parentComponent, true);
}

StringPtr ConfigProtocolClientComm::requestSerializedRootDevice()
{
    auto params = Dict<IString, IBaseObject>();
    return sendComponentCommandInternal(ClientCommand("GetSerializedRootDevice"), params, nullptr);
}

BaseObjectPtr ConfigProtocolClientComm::sendCommand(const ClientCommand& command, const ParamsDictPtr& params)
{
    requireMinServerVersion(command);

    auto sendCommandRpcRequestPacketBuffer = createRpcRequestPacketBuffer(generateId(), command.getName(), params);
    const auto sendCommandRpcReplyPacketBuffer = sendRequestCallback(sendCommandRpcRequestPacketBuffer);

    return parseRpcOrRejectReply(sendCommandRpcReplyPacketBuffer.parseRpcRequestOrReply(), nullptr);
}

void ConfigProtocolClientComm::sendNoReplyCommand(const ClientCommand& command, const ParamsDictPtr& params)
{
    requireMinServerVersion(command);

    auto sendNoReplyCommandRpcRequestPacketBuffer = createNoReplyRpcRequestPacketBuffer(command.getName(), params);
    sendNoReplyRequestCallback(sendNoReplyCommandRpcRequestPacketBuffer);
}

void ConfigProtocolClientComm::setRootDevice(const DevicePtr& rootDevice)
{
    this->rootDeviceRef = rootDevice;
}

DevicePtr ConfigProtocolClientComm::getRootDevice() const
{
    return rootDeviceRef.assigned() ? rootDeviceRef.getRef() : nullptr;
}

void ConfigProtocolClientComm::connectDomainSignals(const ComponentPtr& component)
{
    const auto dev = getRootDevice();
    if (!dev.assigned())
        return;
    const auto topComponent = ComponentPtr::Borrow(component);

    forEachComponent<ISignal>(
        component,
        [&dev, &topComponent](const SignalPtr& signal)
        {
            const StringPtr domainSignalId = signal.asPtr<IDeserializeComponent>(true).getDeserializedParameter("domainSignalId");
            if (domainSignalId.assigned())
            {
                SignalPtr domainSignal;

                // try to find domain signal recursively starting from top component
                {
                    const auto domainSingalRemoteId = domainSignalId.toStdString();
                    StringPtr topComponentRemoteId;
                    checkErrorInfo(topComponent.asPtr<IConfigClientObject>(true)->getRemoteGlobalId(&topComponentRemoteId));
                    if (domainSingalRemoteId.find(topComponentRemoteId.toStdString() + "/") == 0)
                    {
                        auto restStr = domainSingalRemoteId.substr(topComponentRemoteId.toStdString().size() + 1);
                        domainSignal = findSignalByRemoteGlobalIdWithComponent(topComponent, restStr);
                    }
                }

                // try to find domain signal recursively starting from root device
                if (!domainSignal.assigned())
                    domainSignal = findSignalByRemoteGlobalId(dev, domainSignalId);
                if (domainSignal.assigned())
                    signal.asPtr<IConfigClientSignalPrivate>(true)->assignDomainSignal(domainSignal);
                else
                    signal.asPtr<IConfigClientSignalPrivate>(true)->assignDomainSignal(nullptr);
            }
            else
            {
                signal.asPtr<IConfigClientSignalPrivate>(true)->assignDomainSignal(nullptr);
            }
        });
}

void ConfigProtocolClientComm::setRemoteGlobalIds(const ComponentPtr& component, const StringPtr& parentRemoteId)
{
    forEachComponent<IComponent>(
        component,
        [&parentRemoteId](const ComponentPtr& comp)
        {
            StringPtr compRemoteId;
            comp.asPtr<IConfigClientObject>(true)->getRemoteGlobalId(&compRemoteId);
            comp.asPtr<IConfigClientObject>(true)->setRemoteGlobalId(parentRemoteId + compRemoteId);
        });
}

void ConfigProtocolClientComm::setProtocolVersion(uint16_t protocolVersion)
{
    this->protocolVersion = protocolVersion;
}

void ConfigProtocolClientComm::disconnectExternalSignals()
{
    const auto rootDevice = getRootDevice();
    if (!rootDevice.assigned())
        return;

    forEachComponent<IInputPort>(rootDevice,
                                 [this](const InputPortPtr& inputPort)
                                 {
                                     auto connectedSignal = inputPort.getSignal();
                                     const auto configClientInputPort = inputPort.asPtr<IConfigClientInputPort>(true);
                                     if (connectedSignal.assigned() && !isComponentNested(connectedSignal.getGlobalId()))
                                         configClientInputPort->assignSignal(nullptr);
                                 });
}

void ConfigProtocolClientComm::connectInputPorts(const ComponentPtr& component)
{
    const auto dev = getRootDevice();
    if (!dev.assigned())
        return;

    forEachComponent<IInputPort>(component,
                                 [&dev](const InputPortPtr& inputPort)
                                 {
                                     const auto signalId = inputPort.asPtr<IDeserializeComponent>(true).
                                                                     getDeserializedParameter("signalId");
                                     const auto configClientInputPort = inputPort.asPtr<IConfigClientInputPort>(true);
                                     if (signalId.assigned())
                                     {
                                         const auto signal = findSignalByRemoteGlobalId(dev, signalId);
                                         checkErrorInfo(configClientInputPort->assignSignal(signal));
                                     }
                                     else
                                     {
                                         configClientInputPort->assignSignal(nullptr);
                                     }
                                 });
}

BaseObjectPtr ConfigProtocolClientComm::sendComponentCommandInternal(const ClientCommand& command,
                                                                     const ParamsDictPtr& params,
                                                                     const ComponentPtr& parentComponent,
                                                                     bool isGetRootDeviceCommand)
{
    requireMinServerVersion(command);

    auto sendCommandRpcRequestPacketBuffer = createRpcRequestPacketBuffer(generateId(), command.getName(), params);
    const auto sendCommandRpcReplyPacketBuffer = sendRequestCallback(sendCommandRpcRequestPacketBuffer);

    std::string remoteGlobalId{};
    if (parentComponent.supportsInterface<IConfigClientObject>())
    {
        const auto configClientObject = parentComponent.asPtr<IConfigClientObject>(true);
        StringPtr temp;
        checkErrorInfo(configClientObject->getRemoteGlobalId(&temp));
        remoteGlobalId = temp.toStdString();
    }

    const auto deserializeContext = createDeserializeContext(remoteGlobalId, daqContext, nullptr, parentComponent, nullptr, nullptr);

    return parseRpcOrRejectReply(sendCommandRpcReplyPacketBuffer.parseRpcRequestOrReply(), deserializeContext, isGetRootDeviceCommand);
}

template <class Interface, class F>
void ConfigProtocolClientComm::forEachComponent(const ComponentPtr& component, const F& f)
{
    const auto comp = component.asPtrOrNull<Interface>(true);
    if (comp.assigned())
        f(comp);

    const auto folder = component.asPtrOrNull<IFolder>(true);
    if (folder.assigned())
    {
        for (const auto item : folder.getItems())
            forEachComponent<Interface>(item, f);
    }
}

SignalPtr ConfigProtocolClientComm::findSignalByRemoteGlobalIdWithComponent(const ComponentPtr& component,
                                                                            const std::string& remoteGlobalId)
{
    std::string startStr;
    std::string restStr;
    const bool hasSubComponentStr = IdsParser::splitRelativeId(remoteGlobalId, startStr, restStr);
    if (!hasSubComponentStr)
        startStr = remoteGlobalId;

    const auto folder = component.asPtrOrNull<IFolder>(true);
    if (!folder.assigned())
        return nullptr;

    if (folder.hasItem(startStr))
    {
        auto subComponent = folder.getItem(startStr);
        if (hasSubComponentStr)
            return findSignalByRemoteGlobalIdWithComponent(subComponent, restStr);

        if (subComponent.supportsInterface<ISignal>())
            return subComponent;
    }

    return nullptr;
}

SignalPtr ConfigProtocolClientComm::findSignalByRemoteGlobalId(const DevicePtr& device, const std::string& remoteGlobalId)
{
    if (remoteGlobalId.find("/") != 0)
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Global id must start with /");

    const std::string globalIdWithoutSlash = remoteGlobalId.substr(1);

    std::string startStr;
    std::string restStr;
    const bool hasSubComponentStr = IdsParser::splitRelativeId(globalIdWithoutSlash, startStr, restStr);
    if (!hasSubComponentStr)
        return nullptr;

    if (startStr == device.getLocalId())
        return findSignalByRemoteGlobalIdWithComponent(device, restStr);

    return nullptr;
}

}
