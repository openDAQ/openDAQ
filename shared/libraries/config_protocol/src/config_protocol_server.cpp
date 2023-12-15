#include <config_protocol/config_protocol_server.h>

#include "opendaq/ids_parser.h"

namespace daq::config_protocol
{

ComponentFinderRootDevice::ComponentFinderRootDevice(DevicePtr rootDevice)
    : rootDevice(std::move(rootDevice))
{
}

ComponentPtr ComponentFinderRootDevice::findComponentInternal(const ComponentPtr& component, const std::string& id)
{
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
        auto subComponent = folder.getItem(startStr);
        if (hasSubComponentStr)
            return findComponentInternal(subComponent, restStr);

        return subComponent;
    }

    return nullptr;
}

ComponentPtr ComponentFinderRootDevice::findComponent(const std::string& globalId)
{
    return findComponentInternal(rootDevice, globalId);
}

ConfigProtocolServer::ConfigProtocolServer(DevicePtr rootDevice,
                                           NotificationReadyCallback notificationReadyCallback)
    : rootDevice(std::move(rootDevice))
    , notificationReadyCallback(std::move(notificationReadyCallback))
    , deserializer(JsonDeserializer())
    , serializer(JsonSerializer())
    , notificationSerializer(JsonSerializer())
    , componentFinder(std::make_unique<ComponentFinderRootDevice>(this->rootDevice))
{
    buildRpcDispatchStructure();
}

void ConfigProtocolServer::buildRpcDispatchStructure()
{
    rpcDispatch.insert({"GetComponent", std::bind(&ConfigProtocolServer::getComponent, this, std::placeholders::_1)});
    rpcDispatch.insert({"SetPropertyValue", std::bind(&ConfigProtocolServer::setPropertyValue, this, std::placeholders::_1)});
}

PacketBuffer ConfigProtocolServer::processRequestAndGetReply(const PacketBuffer& packetBuffer)
{
    return processPacket(packetBuffer);
}

PacketBuffer ConfigProtocolServer::processRequestAndGetReply(void* mem)
{
    const PacketBuffer packetBuffer(mem, false);
    return processPacket(packetBuffer);
}

void ConfigProtocolServer::sendNotification(const char* json, const size_t jsonSize) const
{
    if (notificationReadyCallback)
    {
        const auto packet = PacketBuffer::createServerNotification(json, jsonSize);
        notificationReadyCallback(packet);
    }
}

void ConfigProtocolServer::sendNotification(const BaseObjectPtr& obj)
{
    StringPtr jsonStr;
    {
        std::scoped_lock lock(notificationSerializerLock);
        notificationSerializer.reset();
        obj.serialize(notificationSerializer);

        jsonStr = notificationSerializer.getOutput();
    }

    sendNotification(jsonStr.getCharPtr(), jsonStr.getLength());
}

void ConfigProtocolServer::setComponentFinder(std::unique_ptr<IComponentFinder>& componentFinder)
{
    this->componentFinder = std::move(componentFinder);
}

std::unique_ptr<IComponentFinder>& ConfigProtocolServer::getComponentFinder()
{
    return componentFinder;
}

PacketBuffer ConfigProtocolServer::processPacket(const PacketBuffer& packetBuffer)
{
    const auto requestId = packetBuffer.getId();
    switch (packetBuffer.getPacketType())
    {
        case PacketType::getProtocolInfo:
            {
                packetBuffer.parseProtocolInfoRequest();
                auto reply = PacketBuffer::createGetProtocolInfoReply(requestId, 0, {0});
                return reply;
            }
        case PacketType::upgradeProtocol:
            {
                uint16_t version;
                packetBuffer.parseProtocolUpgradeRequest(version);
                auto reply = PacketBuffer::createUpgradeProtocolReply(requestId, version == 0);
                return reply;
            }
        case PacketType::rpc:
            {
                std::unique_ptr<char[]> json;

                const auto jsonRequest = packetBuffer.parseRpcRequestOrReply();
                const auto jsonReply = processRpc(jsonRequest);

                auto reply = PacketBuffer::createRpcRequestOrReply(requestId, jsonReply.getCharPtr(), jsonReply.getLength());
                return reply;
            }
        default:
            auto reply = PacketBuffer::createInvalidRequestReply(requestId);
            return reply;

    }
}

StringPtr ConfigProtocolServer::processRpc(const StringPtr& jsonStr)
{
    auto retObj = Dict<IString, IBaseObject>();
    try
    {
        const auto obj = deserializer.deserialize(jsonStr, nullptr);
        const auto dictObj = obj.asPtr<IDict>(true);

        const auto funcName = dictObj["Name"];
        const auto funcParams = dictObj["Params"];

        const auto retValue = callRpc(funcName, funcParams);

        retObj.set("ErrorCode", OPENDAQ_SUCCESS);
        if (retValue.assigned())
            retObj.set("ReturnValue", retValue);
    }
    catch (const daq::DaqException& e)
    {
        retObj.set("ErrorCode", e.getErrCode());
        retObj.set("ErrorMessage", e.what());
    }
    catch (const std::exception& e)
    {
        retObj.set("ErrorCode", OPENDAQ_ERR_GENERALERROR);
        retObj.set("ErrorMessage", e.what());
    }

    serializer.reset();
    retObj.serialize(serializer);
    return serializer.getOutput();
}

BaseObjectPtr ConfigProtocolServer::callRpc(const StringPtr& name, const DictPtr<IString, IBaseObject>& params)
{
    const auto it = rpcDispatch.find(name.toStdString());
    if (it == rpcDispatch.end())
        throw ConfigProtocolException("Invalid function call");

    return it->second(params);
}

ComponentPtr ConfigProtocolServer::findComponent(const std::string componentGlobalId) const
{
    ComponentPtr component;
    if (componentGlobalId == "//root")
        component = rootDevice;
    else
        component = componentFinder->findComponent(componentGlobalId);
    return component;
}

BaseObjectPtr ConfigProtocolServer::getComponent(const DictPtr<IString, IBaseObject>& params) const
{
    const auto componentGlobalId = static_cast<std::string>(params["ComponentGlobalId"]);
    const auto component = findComponent(componentGlobalId);

    if (!component.assigned())
        throw NotFoundException("Component not found");

    return component;
}

BaseObjectPtr ConfigProtocolServer::setPropertyValue(const DictPtr<IString, IBaseObject>& params) const
{
    const auto componentGlobalId = static_cast<std::string>(params["ComponentGlobalId"]);
    const auto component = findComponent(componentGlobalId);

    if (!component.assigned())
        throw NotFoundException("Component not found");

    const auto propertyName = static_cast<std::string>(params["PropertyName"]);
    const auto propertyValue = static_cast<std::string>(params["PropertyValue"]);

    component.setPropertyValue(propertyName, propertyValue);

    return nullptr;
}

}
