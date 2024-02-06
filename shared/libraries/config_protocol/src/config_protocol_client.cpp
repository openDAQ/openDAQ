#include <config_protocol/config_protocol_client.h>
#include <config_protocol/config_client_function_block_impl.h>
#include <config_protocol/config_client_input_port_impl.h>
#include <config_protocol/config_client_folder_impl.h>
#include <config_protocol/config_client_signal_impl.h>
#include <config_protocol/config_client_io_folder_impl.h>
#include <config_protocol/config_client_device_impl.h>
#include <config_protocol/config_client_channel_impl.h>
#include <config_protocol/config_protocol_deserialize_context_impl.h>
#include <config_protocol/component_holder_ptr.h>

namespace daq::config_protocol
{

ConfigProtocolClientComm::ConfigProtocolClientComm(const ContextPtr& daqContext, SendRequestCallback sendRequestCallback)
        : daqContext(daqContext)
        , id(0)
        , sendRequestCallback(std::move(sendRequestCallback))
        , serializer(JsonSerializer())
        , deserializer(JsonDeserializer())
        , connected(false)
{
}

size_t ConfigProtocolClientComm::generateId()
{
    return id++;
}

void ConfigProtocolClientComm::setPropertyValue(
    const std::string& globalId,
    const std::string& propertyName,
    const BaseObjectPtr& propertyValue)
{
    auto dict = Dict<IString, IBaseObject>();
    dict.set("ComponentGlobalId", String(globalId));
    dict.set("PropertyName", String(propertyName));
    dict.set("PropertyValue", String(propertyValue));
    auto setPropertyValueRpcRequestPacketBuffer = createRpcRequestPacketBuffer(generateId(), "SetPropertyValue", dict);
    const auto setPropertyValueRpcReplyPacketBuffer = sendRequestCallback(setPropertyValueRpcRequestPacketBuffer);

    // ReSharper disable once CppExpressionWithoutSideEffects
    parseRpcReplyPacketBuffer(setPropertyValueRpcReplyPacketBuffer);
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
    parseRpcReplyPacketBuffer(setProtectedPropertyValueRpcReplyPacketBuffer);
}

BaseObjectPtr ConfigProtocolClientComm::getPropertyValue(const std::string& globalId, const std::string& propertyName)
{
    auto dict = Dict<IString, IBaseObject>();
    dict.set("ComponentGlobalId", String(globalId));
    dict.set("PropertyName", String(propertyName));
    auto getPropertyValueRpcRequestPacketBuffer = createRpcRequestPacketBuffer(generateId(), "GetPropertyValue", dict);
    const auto getPropertyValueRpcReplyPacketBuffer = sendRequestCallback(getPropertyValueRpcRequestPacketBuffer);

    const auto deserializeContext = createWithImplementation<IComponentDeserializeContext, ConfigProtocolDeserializeContextImpl>(
        shared_from_this(), std::string{}, daqContext, nullptr, nullptr, nullptr);

    return parseRpcReplyPacketBuffer(getPropertyValueRpcReplyPacketBuffer, deserializeContext);
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
    parseRpcReplyPacketBuffer(clearPropertyValueRpcReplyPacketBuffer);
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

    const auto result = parseRpcReplyPacketBuffer(callPropertyRpcReplyPacketBuffer);
    return result;
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
    serializer.reset();
    obj.serialize(serializer);
    return serializer.getOutput();
}

PacketBuffer ConfigProtocolClientComm::createRpcRequestPacketBuffer(const size_t id,
                                                                    const StringPtr& name,
                                                                    const ParamsDictPtr& params)
{
    const auto jsonStr = createRpcRequestJson(name, params);

    auto packetBuffer = PacketBuffer(PacketType::rpc, id, jsonStr.getCharPtr(), jsonStr.getLength());
    return packetBuffer;
}

BaseObjectPtr ConfigProtocolClientComm::parseRpcReplyPacketBuffer(const PacketBuffer& packetBuffer,
                                                                  const ComponentDeserializeContextPtr& context)
{
    const auto jsonStr = packetBuffer.parseRpcRequestOrReply();

    ParamsDictPtr reply;
    try
    {
        reply = deserializer.deserialize(
            jsonStr,
            context,
            [this](const StringPtr& typeId, const SerializedObjectPtr& object, const BaseObjectPtr& context, const FunctionPtr& factoryCallback)
            {
                return deserializeConfigComponent(typeId, object, context, factoryCallback);
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
        std::string msg;
        if (reply.hasKey("ErrorMessage"))
            msg = static_cast<std::string>(reply.get("ErrorMessage"));
        throwExceptionFromErrorCode(errCode, msg);
    }

    if (reply.hasKey("ReturnValue"))
        return reply.get("ReturnValue");

    return {};
}

BaseObjectPtr ConfigProtocolClientComm::deserializeConfigComponent(const StringPtr& typeId,
                                                                   const SerializedObjectPtr& serObj,
                                                                   const BaseObjectPtr& context,
                                                                   const FunctionPtr& factoryCallback)
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
        checkErrorInfo(ConfigClientDeviceImpl::Deserialize(serObj, context, factoryCallback, &obj));
        return obj;
    }

    return nullptr;
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
                                                             const StringPtr& command,
                                                             ParamsDictPtr& params,
                                                             const ComponentPtr& parentComponent)
{
    params.set("ComponentGlobalId", globalId);
    return sendComponentCommandInternal(command, params, parentComponent);
}

BaseObjectPtr ConfigProtocolClientComm::sendComponentCommand(const StringPtr& globalId,
                                                             const StringPtr& command,
                                                             const ComponentPtr& parentComponent)
{
    auto params = Dict<IString, IBaseObject>();
    params.set("ComponentGlobalId", globalId);
    return sendComponentCommandInternal(command, params, parentComponent);
}

BaseObjectPtr ConfigProtocolClientComm::sendCommand(const StringPtr& command, const ParamsDictPtr& params)
{
    auto sendCommandRpcRequestPacketBuffer = createRpcRequestPacketBuffer(generateId(), command, params);
    const auto sendCommandRpcReplyPacketBuffer = sendRequestCallback(sendCommandRpcRequestPacketBuffer);

    return parseRpcReplyPacketBuffer(sendCommandRpcReplyPacketBuffer, nullptr);
}

BaseObjectPtr ConfigProtocolClientComm::sendComponentCommandInternal(const StringPtr& command,
                                                                     const ParamsDictPtr& params,
                                                                     const ComponentPtr& parentComponent)
{
    auto sendCommandRpcRequestPacketBuffer = createRpcRequestPacketBuffer(generateId(), command, params);
    const auto sendCommandRpcReplyPacketBuffer = sendRequestCallback(sendCommandRpcRequestPacketBuffer);

    std::string remoteGlobalId{};
    if (parentComponent.assigned() && parentComponent.supportsInterface<IConfigClientObject>())
    {
        const auto configClientObject = parentComponent.asPtr<IConfigClientObject>(true);
        StringPtr temp;
        checkErrorInfo(configClientObject->getRemoteGlobalId(&temp));
        remoteGlobalId = temp.toStdString();
    }

    const auto deserializeContext = createWithImplementation<IComponentDeserializeContext, ConfigProtocolDeserializeContextImpl>(
        shared_from_this(), remoteGlobalId, daqContext, nullptr, parentComponent, nullptr);

    return parseRpcReplyPacketBuffer(sendCommandRpcReplyPacketBuffer, deserializeContext);
}

ConfigProtocolClientCommPtr ConfigProtocolClient::getClientComm()
{
    return clientComm;
}

ConfigProtocolClient::ConfigProtocolClient(const ContextPtr& daqContext,
                                           const SendRequestCallback& sendRequestCallback,
                                           const ServerNotificationReceivedCallback& serverNotificationReceivedCallback)
    : daqContext(daqContext)
    , sendRequestCallback(sendRequestCallback)
    , serverNotificationReceivedCallback(serverNotificationReceivedCallback)
    , deserializer(JsonDeserializer())
    , clientComm(std::make_shared<ConfigProtocolClientComm>(daqContext, sendRequestCallback))
{
}

DevicePtr ConfigProtocolClient::getDevice()
{
    return device;
}

void ConfigProtocolClient::connect(const ComponentPtr& parent)
{
    auto getProtocolInfoRequestPacketBuffer = PacketBuffer::createGetProtocolInfoRequest(clientComm->generateId());
    const auto getProtocolInfoReplyPacketBuffer = sendRequestCallback(getProtocolInfoRequestPacketBuffer);

    uint16_t currentVersion;
    std::vector<uint16_t> supportedVersions;
    getProtocolInfoReplyPacketBuffer.parseProtocolInfoReply(currentVersion, supportedVersions);

    if (currentVersion != 0)
        throw ConfigProtocolException("Invalid server protocol version");

    if (std::find(supportedVersions.begin(), supportedVersions.end(), 0) == supportedVersions.end())
        throw ConfigProtocolException("Protocol not supported on server");

    auto upgradeProtocolRequestPacketBuffer = PacketBuffer::createUpgradeProtocolRequest(clientComm->generateId(), 0);
    const auto upgradeProtocolReplyPacketBuffer = sendRequestCallback(upgradeProtocolRequestPacketBuffer);

    bool success;
    upgradeProtocolReplyPacketBuffer.parseProtocolUpgradeReply(success);

    if (!success)
        throw ConfigProtocolException("Protocol upgrade failed");

    const auto localTypeManager = daqContext.getTypeManager();
    const TypeManagerPtr typeManager = clientComm->sendCommand("GetTypeManager");
    const auto types = typeManager.getTypes();

    for (const auto& typeName : types)
    {
        const auto type = typeManager.getType(typeName);
        if (localTypeManager.hasType(type.getName()))
        {
            const auto localType = localTypeManager.getType(type.getName());
            if (localType != type)
                throw InvalidValueException("Remote type different than local");
            continue;
        }

        localTypeManager.addType(type);
    }

    const ComponentHolderPtr deviceHolder = clientComm->sendComponentCommand("//root", "GetComponent", parent);
    device = deviceHolder.getComponent();

    clientComm->connected = true;
}

void ConfigProtocolClient::triggerNotificationPacket(const PacketBuffer& packet)
{
    const auto json = packet.parseServerNotification();
    
    const auto deserializeContext = createWithImplementation<IComponentDeserializeContext, ConfigProtocolDeserializeContextImpl>(
        clientComm, std::string{}, daqContext, device, nullptr, nullptr);
    const auto obj = deserializer.deserialize(json, deserializeContext, 
            [this](const StringPtr& typeId, const SerializedObjectPtr& object, const BaseObjectPtr& context, const FunctionPtr& factoryCallback)
            {
                return clientComm->deserializeConfigComponent(typeId, object, context, factoryCallback);
            });
    // handle notifications in callback provided in constructor
    const bool processed = serverNotificationReceivedCallback ? serverNotificationReceivedCallback(obj) : false;
    // if callback not processed by callback, process it internally
    if (!processed)
        triggerNotificationObject(obj);
}

ComponentPtr ConfigProtocolClient::findComponent(std::string globalId)
{
    globalId.erase(globalId.begin(), globalId.begin() + device.getLocalId().getLength() + 1);
    if (globalId.find_first_of('/') == 0)
        globalId.erase(globalId.begin(), globalId.begin() + 1);

    return device.findComponent(globalId);
}

void ConfigProtocolClient::triggerNotificationObject(const BaseObjectPtr& object)
{
    ListPtr<IBaseObject> packedEvent = object.asPtrOrNull<IList>();
    if (!packedEvent.assigned() || packedEvent.getCount() != 2)
        return;

    const ComponentPtr component = findComponent(packedEvent[0]);
    if (component.assigned())
    {
        CoreEventArgsPtr argsPtr = unpackCoreEvents(packedEvent[1]);
        component.asPtr<IConfigClientObject>()->handleRemoteCoreEvent(component, argsPtr);
    }
}

CoreEventArgsPtr ConfigProtocolClient::unpackCoreEvents(const CoreEventArgsPtr& args)
{
    
    BaseObjectPtr cloned;
    checkErrorInfo(args.getParameters().asPtr<ICloneable>()->clone(&cloned));
    DictPtr<IString, IBaseObject> dict = cloned;

    if (dict.hasKey("Signal"))
    {
        const auto globalId = dict.get("Signal");
        dict.set("Signal", findComponent(globalId));
    }
    
    if (dict.hasKey("DomainSignal"))
    {
        const auto globalId = dict.get("DomainSignal");
        dict.set("DomainSignal", findComponent(globalId));
    }
    
    if (dict.hasKey("RelatedSignals"))
    {
        ListPtr<ISignal> signals = List<ISignal>();
        const ListPtr<IString> relatedSignals = dict.get("RelatedSignals");
        for (const auto& id : relatedSignals)
            signals.pushBack(findComponent(id));
        dict.set("RelatedSignals", signals);
    }

    if (dict.hasKey("Component"))
    {
        const ComponentHolderPtr compHolder = dict.get("Component");
        const ComponentPtr comp = compHolder.getComponent();
        StringPtr parentRemoteId;
        comp.getParent().asPtr<IConfigClientObject>()->getRemoteGlobalId(&parentRemoteId);

        comp.asPtr<IConfigClientObject>()->setRemoteGlobalId(parentRemoteId + "/" + comp.getLocalId());
        dict.set("Component", comp);
    }

    return CoreEventArgs(static_cast<CoreEventId>(args.getEventId()), args.getEventName(), dict);
}
}
