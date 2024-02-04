#include <config_protocol/config_protocol_client.h>
#include <config_protocol/config_client_function_block_impl.h>
#include <config_protocol/config_client_input_port_impl.h>
#include <config_protocol/config_client_folder_impl.h>
#include <config_protocol/config_client_signal_impl.h>
#include <config_protocol/config_client_io_folder_impl.h>
#include <config_protocol/config_client_device_impl.h>
#include <config_protocol/config_client_channel_impl.h>
#include <config_protocol/config_protocol_deserialize_context_impl.h>

namespace daq::config_protocol
{

ConfigProtocolClientComm::ConfigProtocolClientComm(const ContextPtr& daqContext,
                                                   SendRequestCallback sendRequestCallback,
                                                   ComponentDeserializeCallback rootDeviceDeserializeCallback)
        : daqContext(daqContext)
        , id(0)
        , sendRequestCallback(std::move(sendRequestCallback))
        , rootDeviceDeserializeCallback(std::move(rootDeviceDeserializeCallback))
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
                                                                  const ComponentDeserializeContextPtr& context,
                                                                  bool isGetRootDeviceReply)
{
    const auto jsonStr = packetBuffer.parseRpcRequestOrReply();

    ParamsDictPtr reply;
    try
    {
        ComponentDeserializeCallback customDeviceDeserilazeCallback = isGetRootDeviceReply ? rootDeviceDeserializeCallback : nullptr;
        reply = deserializer.deserialize(
            jsonStr,
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
                                                                   const FunctionPtr& factoryCallback,
                                                                   ComponentDeserializeCallback deviceDeserialzeCallback)
{
    if (typeId == "Folder")
    {
        BaseObjectPtr obj;
        checkErrorInfo(ConfigClientFolderImpl::Deserialize(serObj, context, factoryCallback, &obj));
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
        if (deviceDeserialzeCallback)
            checkErrorInfo(deviceDeserialzeCallback(serObj, context, factoryCallback, &obj));
        else
            checkErrorInfo(ConfigClientDeviceImpl::Deserialize(serObj, context, factoryCallback, &obj));
        return obj;
    }

    return nullptr;
}

bool ConfigProtocolClientComm::getConnected() const
{
    return connected;
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

BaseObjectPtr ConfigProtocolClientComm::requestRootDevice(const ComponentPtr& parentComponent)
{
    auto params = Dict<IString, IBaseObject>();
    params.set("ComponentGlobalId", "//root");
    return sendComponentCommandInternal("GetComponent", params, parentComponent, true);
}

BaseObjectPtr ConfigProtocolClientComm::sendCommand(const StringPtr& command, const ParamsDictPtr& params)
{
    auto sendCommandRpcRequestPacketBuffer = createRpcRequestPacketBuffer(generateId(), command, params);
    const auto sendCommandRpcReplyPacketBuffer = sendRequestCallback(sendCommandRpcRequestPacketBuffer);

    return parseRpcReplyPacketBuffer(sendCommandRpcReplyPacketBuffer, nullptr);
}

BaseObjectPtr ConfigProtocolClientComm::sendComponentCommandInternal(const StringPtr& command,
                                                                     const ParamsDictPtr& params,
                                                                     const ComponentPtr& parentComponent,
                                                                     bool isGetRootDeviceCommand)
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
        shared_from_this(), remoteGlobalId, daqContext, parentComponent, nullptr, nullptr);

    return parseRpcReplyPacketBuffer(sendCommandRpcReplyPacketBuffer, deserializeContext, isGetRootDeviceCommand);
}













}
