#include <config_protocol/config_protocol_client.h>

namespace daq::config_protocol
{

ConfigProtocolClientComm::ConfigProtocolClientComm(SendRequestCallback sendRequestCallback)
        : id(0)
        , sendRequestCallback(std::move(sendRequestCallback))
        , serializer(JsonSerializer())
        , deserializer(JsonDeserializer())
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

BaseObjectPtr ConfigProtocolClientComm::createRpcRequest(const StringPtr& name, const DictPtr<IString, IBaseObject>& params) const
{
    auto obj = Dict<IString, IBaseObject>();
    obj.set("Name", name);
    if (params.assigned() && params.getCount() > 0)
        obj.set("Params", params);

    return obj;
}

StringPtr ConfigProtocolClientComm::createRpcRequestJson(const StringPtr& name, const DictPtr<IString, IBaseObject>& params)
{
    const auto obj = createRpcRequest(name, params);
    serializer.reset();
    obj.serialize(serializer);
    return serializer.getOutput();
}

PacketBuffer ConfigProtocolClientComm::createRpcRequestPacketBuffer(const size_t id,
                                                                const StringPtr& name,
                                                                const DictPtr<IString, IBaseObject>& params)
{
    const auto jsonStr = createRpcRequestJson(name, params);

    auto packetBuffer = PacketBuffer(PacketType::rpc, id, jsonStr.getCharPtr(), jsonStr.getLength());
    return packetBuffer;
}

BaseObjectPtr ConfigProtocolClientComm::parseRpcReplyPacketBuffer(const PacketBuffer& packetBuffer) const
{
    const auto jsonStr = packetBuffer.parseRpcRequestOrReply();

    DictPtr<IString, IBaseObject> reply;
    try
    {
        reply = deserializer.deserialize(jsonStr);
    }
    catch (const std::exception&)
    {
        throw ConfigProtocolException("Invalid reply");
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


ConfigProtocolClient::ConfigProtocolClient(const SendRequestCallback& sendRequestCallback,
                                           const ServerNotificationReceivedCallback& serverNotificationReceivedCallback)
    : sendRequestCallback(sendRequestCallback)
    , serverNotificationReceivedCallback(serverNotificationReceivedCallback)
    , deserializer(JsonDeserializer())
    , clientComm(std::make_shared<ConfigProtocolClientComm>(sendRequestCallback))
{
}

DevicePtr ConfigProtocolClient::getDevice()
{
    return device;
}

ConfigProtocolClientCommPtr ConfigProtocolClient::getClientComm()
{
    return clientComm;
}

void ConfigProtocolClient::connect()
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

    auto dict = Dict<IString, IBaseObject>();
    dict.set("ComponentGlobalId", "//root");
    auto getComponentRpcRequestPacketBuffer = clientComm->createRpcRequestPacketBuffer(clientComm->generateId(), "GetComponent", dict);
    const auto getComponentRpcReplyPacketBuffer = sendRequestCallback(getComponentRpcRequestPacketBuffer);

    const auto component = clientComm->parseRpcReplyPacketBuffer(getComponentRpcReplyPacketBuffer);
    buildDevice(component);
}

void ConfigProtocolClient::triggerNotificationPacket(const PacketBuffer& packet)
{
    const auto json = packet.parseServerNotification();

    const auto obj = deserializer.deserialize(json);
    // handle notifications in callback provided in constructor
    const bool processed = serverNotificationReceivedCallback ? serverNotificationReceivedCallback(obj) : false;
    // if callback not processed by callback, process it internally
    if (!processed)
        triggerNotificationObject(obj);
}

void ConfigProtocolClient::buildDevice(const BaseObjectPtr& obj)
{
    device = nullptr;
}

void ConfigProtocolClient::triggerNotificationObject(const BaseObjectPtr& object)
{
    // handle notifications from server
}

}
