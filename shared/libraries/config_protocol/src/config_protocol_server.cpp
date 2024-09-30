#include <config_protocol/config_protocol_server.h>
#include <coreobjects/property_object_protected_ptr.h>
#include <opendaq/ids_parser.h>
#include <config_protocol/config_server_component.h>
#include <config_protocol/config_server_device.h>
#include <config_protocol/config_server_input_port.h>
#include <config_protocol/config_server_signal.h>
#include <coreobjects/core_event_args_factory.h>
#include <coretypes/cloneable.h>
#include <config_protocol/config_server_access_control.h>
#include <opendaq/custom_log.h>

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
    if (globalId.find("/") != 0)
        throw InvalidParameterException("Global id must start with /");

    const std::string globalIdWithoutSlash = globalId.substr(1);

    std::string startStr;
    std::string restStr;
    const bool hasSubComponentStr = IdsParser::splitRelativeId(globalIdWithoutSlash, startStr, restStr);
    if (!hasSubComponentStr)
    {
        if (globalIdWithoutSlash == rootDevice.getLocalId())
            return rootDevice;
        return nullptr;
    }

    if (startStr == rootDevice.getLocalId())
        return findComponentInternal(rootDevice, restStr);

    return nullptr;
}

ConfigProtocolServer::ConfigProtocolServer(DevicePtr rootDevice,
                                           NotificationReadyCallback notificationReadyCallback,
                                           const UserPtr& user,
                                           const FolderConfigPtr& externalSignalsFolder)
    : rootDevice(std::move(rootDevice))
    , daqContext(this->rootDevice.getContext())
    , notificationReadyCallback(std::move(notificationReadyCallback))
    , deserializer(JsonDeserializer())
    , serializer(JsonSerializer())
    , notificationSerializer(JsonSerializer())
    , componentFinder(std::make_unique<ComponentFinderRootDevice>(this->rootDevice))
    , user(user)
    , protocolVersion(0)
    , supportedServerVersions({0, 1, 2, 3, 4})
    , streamingConsumer(this->daqContext, externalSignalsFolder)
{
    assert(user.assigned());
    serializer.setUser(user);
    notificationSerializer.setUser(user);

    buildRpcDispatchStructure();

    if (daqContext.assigned())
        daqContext.getOnCoreEvent() += event(this, &ConfigProtocolServer::coreEventCallback);
}

ConfigProtocolServer::~ConfigProtocolServer()
{
    if (daqContext.assigned())
        daqContext.getOnCoreEvent() -= event(this, &ConfigProtocolServer::coreEventCallback);
}

template <class SmartPtr>
void ConfigProtocolServer::addHandler(const std::string& name, const RpcHandlerFunction<SmartPtr>& handler)
{
    auto wrappedHanler = [this, handler](const ParamsDictPtr& params)
    {
        RpcContext context;
        context.protocolVersion = this->protocolVersion;
        context.user = this->user;

        const auto componentGlobalId = static_cast<std::string>(params["ComponentGlobalId"]);
        const auto component = findComponent(componentGlobalId);

        if (!component.assigned())
            throw NotFoundException("Component not found");

        const auto componentPtr = component.asPtr<typename SmartPtr::DeclaredInterface>();
        return handler(context, componentPtr, params);
    };

    rpcDispatch.insert({name, wrappedHanler});
}

void ConfigProtocolServer::buildRpcDispatchStructure()
{
    using namespace std::placeholders;

    rpcDispatch.insert({"GetComponent", std::bind(&ConfigProtocolServer::getComponent, this,  _1)});
    rpcDispatch.insert({"GetTypeManager", std::bind(&ConfigProtocolServer::getTypeManager, this, _1)});
    rpcDispatch.insert({"GetSerializedRootDevice", std::bind(&ConfigProtocolServer::getSerializedRootDevice, this,  _1)});
    rpcDispatch.insert({"RemoveExternalSignals", std::bind(&ConfigProtocolServer::removeExternalSignals, this,  _1)});

    addHandler<ComponentPtr>("SetPropertyValue", &ConfigServerComponent::setPropertyValue);
    addHandler<ComponentPtr>("GetPropertyValue", &ConfigServerComponent::getPropertyValue);
    addHandler<ComponentPtr>("SetProtectedPropertyValue", &ConfigServerComponent::setProtectedPropertyValue);
    addHandler<ComponentPtr>("ClearPropertyValue", &ConfigServerComponent::clearPropertyValue);
    addHandler<ComponentPtr>("CallProperty", &ConfigServerComponent::callProperty);
    addHandler<ComponentPtr>("BeginUpdate", &ConfigServerComponent::beginUpdate);
    addHandler<ComponentPtr>("EndUpdate", &ConfigServerComponent::endUpdate);
    addHandler<ComponentPtr>("SetAttributeValue", &ConfigServerComponent::setAttributeValue);
    addHandler<ComponentPtr>("Update", &ConfigServerComponent::update);

    addHandler<DevicePtr>("GetInfo", &ConfigServerDevice::getInfo);
    addHandler<DevicePtr>("GetAvailableFunctionBlockTypes", &ConfigServerDevice::getAvailableFunctionBlockTypes);
    addHandler<DevicePtr>("AddFunctionBlock", &ConfigServerDevice::addFunctionBlock);
    addHandler<DevicePtr>("RemoveFunctionBlock", &ConfigServerDevice::removeFunctionBlock);
    addHandler<DevicePtr>("GetTicksSinceOrigin", &ConfigServerDevice::getTicksSinceOrigin);
    addHandler<DevicePtr>("Lock", &ConfigServerDevice::lock);
    addHandler<DevicePtr>("Unlock", &ConfigServerDevice::unlock);
    addHandler<DevicePtr>("IsLocked", &ConfigServerDevice::isLocked);

    addHandler<SignalPtr>("GetLastValue", &ConfigServerSignal::getLastValue);

    addHandler<InputPortPtr>("ConnectSignal", std::bind(&ConfigProtocolServer::connectSignal, this, _1, _2, _3));
    addHandler<InputPortPtr>("ConnectExternalSignal", std::bind(&ConfigProtocolServer::connectExternalSignal, this, _1, _2, _3));

    addHandler<InputPortPtr>("DisconnectSignal", &ConfigServerInputPort::disconnect);
    addHandler<InputPortPtr>("AcceptsSignal", std::bind(&ConfigProtocolServer::acceptsSignal, this, _1, _2, _3));
}

PacketBuffer ConfigProtocolServer::processRequestAndGetReply(const PacketBuffer& packetBuffer)
{
    return processPacketAndGetReply(packetBuffer);
}

PacketBuffer ConfigProtocolServer::processRequestAndGetReply(void* mem)
{
    const PacketBuffer packetBuffer(mem, false);
    return processPacketAndGetReply(packetBuffer);
}

PacketBuffer ConfigProtocolServer::generateConnectionRejectedReply(uint64_t requestId,
                                                                   ErrCode errCode,
                                                                   const StringPtr& message,
                                                                   const SerializerPtr& serializer)
{
    assert(OPENDAQ_FAILED(errCode));

    const auto jsonReply = prepareErrorResponse(errCode, message, serializer);

    auto reply = PacketBuffer::createConnectionRejectedReply(requestId, jsonReply.getCharPtr(), jsonReply.getLength());
    return reply;
}

void ConfigProtocolServer::processNoReplyRequest(const PacketBuffer& packetBuffer)
{
    processNoReplyPacket(packetBuffer);
}

void ConfigProtocolServer::processNoReplyRequest(void* mem)
{
    const PacketBuffer packetBuffer(mem, false);
    processNoReplyPacket(packetBuffer);
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

DevicePtr ConfigProtocolServer::getRootDevice()
{
    return rootDevice;
}

PacketBuffer ConfigProtocolServer::processPacketAndGetReply(const PacketBuffer& packetBuffer)
{
    const auto requestId = packetBuffer.getId();
    switch (packetBuffer.getPacketType())
    {
        case PacketType::GetProtocolInfo:
            {
                packetBuffer.parseProtocolInfoRequest();
                auto reply = PacketBuffer::createGetProtocolInfoReply(requestId, 0, supportedServerVersions);
                return reply;
            }
        case PacketType::UpgradeProtocol:
            {
                uint16_t version;
                packetBuffer.parseProtocolUpgradeRequest(version);
                auto reply = PacketBuffer::createUpgradeProtocolReply(requestId, supportedServerVersions.find(version) != supportedServerVersions.end());
                protocolVersion = version;
                return reply;
            }
        case PacketType::Rpc:
            {
                const auto jsonRequest = packetBuffer.parseRpcRequestOrReply();
                const auto jsonReply = processRpcAndGetReply(jsonRequest);

                auto reply = PacketBuffer::createRpcRequestOrReply(requestId, jsonReply.getCharPtr(), jsonReply.getLength());
                return reply;
            }
        default:
            auto reply = PacketBuffer::createInvalidRequestReply(requestId);
            return reply;
    }
}

void ConfigProtocolServer::processNoReplyPacket(const PacketBuffer& packetBuffer)
{
    const auto jsonRequest = packetBuffer.parseNoReplyRpcRequest();
    processNoReplyRpc(jsonRequest);
}

StringPtr ConfigProtocolServer::processRpcAndGetReply(const StringPtr& jsonStr)
{
    try
    {
        auto retObj = Dict<IString, IBaseObject>();

        const auto obj = deserializer.deserialize(jsonStr, nullptr);
        const DictPtr<IString, IBaseObject> dictObj = obj.asPtr<IDict>(true);

        const auto funcName = dictObj.get("Name");
        ParamsDictPtr funcParams;
        if (dictObj.hasKey("Params"))
            funcParams = dictObj.get("Params");

        const auto retValue = callRpc(funcName, funcParams);

        retObj.set("ErrorCode", OPENDAQ_SUCCESS);
        if (retValue.assigned())
            retObj.set("ReturnValue", retValue);

        serializer.reset();
        retObj.serialize(serializer);
        return serializer.getOutput();
    }
    catch (const daq::DaqException& e)
    {
        return prepareErrorResponse(e.getErrCode(), e.what(), this->serializer);
    }
    catch (const std::exception& e)
    {
        return prepareErrorResponse(OPENDAQ_ERR_GENERALERROR, e.what(), this->serializer);
    }

    return prepareErrorResponse(OPENDAQ_ERR_GENERALERROR, "General error during serialization", this->serializer);
}

StringPtr ConfigProtocolServer::prepareErrorResponse(Int errorCode, const StringPtr& message, const SerializerPtr& serializer)
{
    auto errorObject = Dict<IString, IBaseObject>();
    errorObject.set("ErrorCode", errorCode);
    errorObject.set("ErrorMessage", message);

    serializer.reset();
    errorObject.serialize(serializer);
    return serializer.getOutput();
}

void ConfigProtocolServer::processNoReplyRpc(const StringPtr& jsonStr)
{
    StringPtr funcName;
    try
    {
        const auto obj = deserializer.deserialize(jsonStr, nullptr);
        const DictPtr<IString, IBaseObject> dictObj = obj.asPtr<IDict>(true);

        funcName = dictObj.get("Name");
        ParamsDictPtr funcParams;
        if (dictObj.hasKey("Params"))
            funcParams = dictObj.get("Params");

        callRpc(funcName, funcParams);
    }
    catch (const std::exception& e)
    {
        auto loggerComponent = daqContext.getLogger().getOrAddComponent("ConfigProtocolServer");
        LOG_W("RPC call {} failed with: {}", funcName.assigned() ? funcName : "not recognized", e.what());
    }
}

BaseObjectPtr ConfigProtocolServer::callRpc(const StringPtr& name, const ParamsDictPtr& params)
{
    const auto it = rpcDispatch.find(name.toStdString());
    if (it == rpcDispatch.end())
        throw ConfigProtocolException("Invalid function call");

    return it->second(params);
}

ComponentPtr ConfigProtocolServer::findComponent(const std::string& componentGlobalId) const
{
    ComponentPtr component;
    if (componentGlobalId == "//root")
        component = rootDevice;
    else
        component = componentFinder->findComponent(componentGlobalId);
    return component;
}

BaseObjectPtr ConfigProtocolServer::getComponent(const ParamsDictPtr& params) const
{
    const auto componentGlobalId = static_cast<std::string>(params["ComponentGlobalId"]);
    const auto component = findComponent(componentGlobalId);

    if (!component.assigned())
        throw NotFoundException("Component not found");

    ConfigServerAccessControl::protectObject(component, user, Permission::Read);
    return ComponentHolder(component);
}

BaseObjectPtr ConfigProtocolServer::getSerializedRootDevice(const ParamsDictPtr& params)
{
    ConfigServerAccessControl::protectObject(rootDevice, user, Permission::Read);

    serializer.reset();
    rootDevice.serialize(serializer);

    return serializer.getOutput();
}

BaseObjectPtr ConfigProtocolServer::connectSignal(const RpcContext& context, const InputPortPtr& inputPort, const ParamsDictPtr& params)
{
    const StringPtr signalId = params.get("SignalId");
    const SignalPtr signal = findComponent(signalId);
    if (signal.assigned() && streamingConsumer.isExternalSignal(signal))
        throw InvalidParameterException("Mirrored external signal cannot be connected to server input port");
    return ConfigServerInputPort::connect(context, inputPort, signal, params);
}

BaseObjectPtr ConfigProtocolServer::connectExternalSignal(const RpcContext& context,
                                                          const InputPortPtr& inputPort,
                                                          const ParamsDictPtr& params)
{
    const SignalPtr signal = streamingConsumer.getOrAddExternalSignal(params);
    return ConfigServerInputPort::connect(context, inputPort, signal, params);
}

BaseObjectPtr ConfigProtocolServer::removeExternalSignals(const ParamsDictPtr& params)
{
    ConfigServerAccessControl::protectLockedComponent(rootDevice);

    streamingConsumer.removeExternalSignals(params);
    return nullptr;
}

BaseObjectPtr ConfigProtocolServer::acceptsSignal(uint16_t protocolVersion, const InputPortPtr& inputPort, const ParamsDictPtr& params)
{
    const StringPtr signalId = params.get("SignalId");
    const SignalPtr signal = findComponent(signalId);
    return ConfigServerInputPort::accepts(protocolVersion, inputPort, signal, user);
}

void ConfigProtocolServer::coreEventCallback(ComponentPtr& component, CoreEventArgsPtr& eventArgs)
{
    if (streamingConsumer.isForwardedCoreEvent(component, eventArgs))
    {
        const auto packed = packCoreEvent(component, eventArgs);
        sendNotification(packed);
    }
}

ListPtr<IBaseObject> ConfigProtocolServer::packCoreEvent(const ComponentPtr& component, const CoreEventArgsPtr& args)
{
    const auto globalId = component.assigned() ? component.getGlobalId() : "";
    auto packedEvent = List<IBaseObject>(globalId);

    switch (static_cast<CoreEventId>(args.getEventId()))
    {
        case CoreEventId::PropertyValueChanged:
        case CoreEventId::PropertyObjectUpdateEnd:
        case CoreEventId::PropertyAdded:
        case CoreEventId::TagsChanged:
        case CoreEventId::PropertyRemoved:
        case CoreEventId::SignalConnected:
        case CoreEventId::ComponentAdded:
        case CoreEventId::AttributeChanged:
            packedEvent.pushBack(processCoreEventArgs(args));
            break;
        case CoreEventId::ComponentUpdateEnd:
            packedEvent.pushBack(processUpdateEndCoreEvent(component, args));
            break;
        case CoreEventId::ComponentRemoved:
        case CoreEventId::SignalDisconnected:
        case CoreEventId::DataDescriptorChanged:
        case CoreEventId::StatusChanged:
        case CoreEventId::TypeAdded:
        case CoreEventId::TypeRemoved:
        case CoreEventId::DeviceDomainChanged:
        default:
            packedEvent.pushBack(args);
    }
    
    return packedEvent;
}

CoreEventArgsPtr ConfigProtocolServer::processCoreEventArgs(const CoreEventArgsPtr& args)
{
    BaseObjectPtr cloned;
    checkErrorInfo(args.getParameters().asPtr<ICloneable>()->clone(&cloned));
    DictPtr<IString, IBaseObject> dict = cloned;

    if (dict.hasKey("Owner"))
        dict.remove("Owner");

    if (dict.hasKey("Signal"))
    {
        const auto globalId = dict.get("Signal").asPtr<IComponent>().getGlobalId();
        dict.set("Signal", globalId);
    }
    
    if (dict.hasKey("DomainSignal"))
    {
        const auto globalId = dict.get("DomainSignal").asPtr<IComponent>().getGlobalId();
        dict.set("DomainSignal", globalId);
    }
    
    if (dict.hasKey("RelatedSignals"))
    {
        ListPtr<IString> globalIds = List<IString>();
        const ListPtr<ISignal> relatedSignals = dict.get("RelatedSignals");
        for (const auto& sig : relatedSignals)
            globalIds.pushBack(sig.getGlobalId());
        dict.set("RelatedSignals", globalIds);
    }

    if (dict.hasKey("Component"))
    {
        const ComponentPtr comp = dict.get("Component");
        dict.set("Component", ComponentHolder(comp));
    }

    return CoreEventArgs(static_cast<CoreEventId>(args.getEventId()), args.getEventName(), cloned);
}

CoreEventArgsPtr ConfigProtocolServer::processUpdateEndCoreEvent(const ComponentPtr& component, const CoreEventArgsPtr& args)
{
    std::scoped_lock lock(notificationSerializerLock);
    auto dict = Dict<IString, IBaseObject>();

    notificationSerializer.reset();
    component.serialize(notificationSerializer);
    dict.set("SerializedComponent", notificationSerializer.getOutput());

    return CoreEventArgs(static_cast<CoreEventId>(args.getEventId()), args.getEventName(), dict);
}

BaseObjectPtr ConfigProtocolServer::getTypeManager(const ParamsDictPtr& params) const
{
    ConfigServerAccessControl::protectObject(rootDevice, user, Permission::Read);

    const auto typeManager = rootDevice.getContext().getTypeManager();
    return typeManager;
}

void ConfigProtocolServer::processClientToServerStreamingPacket(SignalNumericIdType signalNumericId, const PacketPtr& packet)
{
    streamingConsumer.processClientToServerStreamingPacket(signalNumericId, packet);
}

uint16_t ConfigProtocolServer::getProtocolVersion() const
{
    return protocolVersion;
}

void ConfigProtocolServer::setProtocolVersion(uint16_t protocolVersion)
{
    this->protocolVersion = protocolVersion;
}

}
