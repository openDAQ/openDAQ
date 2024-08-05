#include <config_protocol/config_protocol_server.h>
#include <coreobjects/property_object_protected_ptr.h>
#include <opendaq/ids_parser.h>
#include <config_protocol/config_server_component.h>
#include <config_protocol/config_server_device.h>
#include <config_protocol/config_server_input_port.h>
#include <config_protocol/config_server_signal.h>
#include <coreobjects/core_event_args_factory.h>
#include <coretypes/cloneable.h>

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

ConfigProtocolServer::ConfigProtocolServer(DevicePtr rootDevice, NotificationReadyCallback notificationReadyCallback, const UserPtr& user)
    : rootDevice(std::move(rootDevice))
    , daqContext(this->rootDevice.getContext())
    , notificationReadyCallback(std::move(notificationReadyCallback))
    , deserializer(JsonDeserializer())
    , serializer(JsonSerializer())
    , notificationSerializer(JsonSerializer())
    , componentFinder(std::make_unique<ComponentFinderRootDevice>(this->rootDevice))
    , user(user)
{
    assert(user.assigned());
    serializer.setUser(user);

    buildRpcDispatchStructure();

    if (daqContext.assigned())
        daqContext.getOnCoreEvent() += event(this, &ConfigProtocolServer::coreEventCallback);
}

ConfigProtocolServer::~ConfigProtocolServer()
{
    if (daqContext.assigned())
        daqContext.getOnCoreEvent() -= event(this, &ConfigProtocolServer::coreEventCallback);
}

template <class SmartPtr, class F>
BaseObjectPtr ConfigProtocolServer::bindComponentWrapper(const F& f, const ParamsDictPtr& params)
{
    const auto componentGlobalId = static_cast<std::string>(params["ComponentGlobalId"]);
    const auto component = findComponent(componentGlobalId);

    if (!component.assigned())
        throw NotFoundException("Component not found");

    const auto ptr = component.asPtr<typename SmartPtr::DeclaredInterface>();

    return f(ptr, params);
}

template <class SmartPtr, class Handler>
void ConfigProtocolServer::addHandler(const std::string& name, const Handler& handler, const std::vector<Permission>& requiredPermissions)
{
    using namespace std::placeholders;

    auto h = std::bind(handler, _1, _2);

    rpcDispatch.insert({name,
                        [this, h, requiredPermissions](const ParamsDictPtr& params) -> BaseObjectPtr
                        {
                            return bindComponentWrapper<SmartPtr>(
                                [this, &h, &requiredPermissions](const SmartPtr& component, const ParamsDictPtr& params) -> BaseObjectPtr
                                {
                                    protectComponent(component, requiredPermissions);
                                    return h(component, params);
                                },
                                params);
                        }});
}

void ConfigProtocolServer::buildRpcDispatchStructure()
{
    using namespace std::placeholders;

    rpcDispatch.insert({"GetComponent", std::bind(&ConfigProtocolServer::getComponent, this,  _1)});
    rpcDispatch.insert({"GetTypeManager", std::bind(&ConfigProtocolServer::getTypeManager, this, _1)});
    rpcDispatch.insert({"GetSerializedRootDevice", std::bind(&ConfigProtocolServer::getSerializedRootDevice, this,  _1)});

    addHandler<ComponentPtr>("SetPropertyValue", &ConfigServerComponent::setPropertyValue, {Permission::Read, Permission::Write});
    addHandler<ComponentPtr>("GetPropertyValue", &ConfigServerComponent::getPropertyValue, {Permission::Read});
    addHandler<ComponentPtr>("SetProtectedPropertyValue", &ConfigServerComponent::setProtectedPropertyValue, {Permission::Read, Permission::Write});
    addHandler<ComponentPtr>("ClearPropertyValue", &ConfigServerComponent::clearPropertyValue, {Permission::Read, Permission::Write});
    addHandler<ComponentPtr>("CallProperty", &ConfigServerComponent::callProperty, {Permission::Read, Permission::Execute});
    addHandler<ComponentPtr>("BeginUpdate", &ConfigServerComponent::beginUpdate, {Permission::Read, Permission::Write});
    addHandler<ComponentPtr>("EndUpdate", &ConfigServerComponent::endUpdate, {Permission::Read, Permission::Write});
    addHandler<ComponentPtr>("SetAttributeValue", &ConfigServerComponent::setAttributeValue, {Permission::Read, Permission::Write});
    addHandler<ComponentPtr>("Update", &ConfigServerComponent::update, {Permission::Read, Permission::Write});

    addHandler<DevicePtr>("GetInfo", &ConfigServerDevice::getInfo, {Permission::Read});
    addHandler<DevicePtr>("GetAvailableFunctionBlockTypes", &ConfigServerDevice::getAvailableFunctionBlockTypes, {Permission::Read});
    addHandler<DevicePtr>("AddFunctionBlock", &ConfigServerDevice::addFunctionBlock, {Permission::Read, Permission::Write});
    addHandler<DevicePtr>("RemoveFunctionBlock", &ConfigServerDevice::removeFunctionBlock, {Permission::Read, Permission::Write});
    addHandler<DevicePtr>("GetTicksSinceOrigin", &ConfigServerDevice::getTicksSinceOrigin, {Permission::Read});

    addHandler<SignalPtr>("GetLastValue", &ConfigServerSignal::getLastValue, {Permission::Read});

    addHandler<InputPortPtr>("ConnectSignal", std::bind(&ConfigProtocolServer::connectSignal, this, _1, _2), {Permission::Read, Permission::Write});
    addHandler<InputPortPtr>("DisconnectSignal", &ConfigServerInputPort::disconnect, {Permission::Read, Permission::Write});
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

DevicePtr ConfigProtocolServer::getRootDevice()
{
    return rootDevice;
}

PacketBuffer ConfigProtocolServer::processPacket(const PacketBuffer& packetBuffer)
{
    const auto requestId = packetBuffer.getId();
    switch (packetBuffer.getPacketType())
    {
        case PacketType::GetProtocolInfo:
            {
                packetBuffer.parseProtocolInfoRequest();
                auto reply = PacketBuffer::createGetProtocolInfoReply(requestId, 0, {0});
                return reply;
            }
        case PacketType::UpgradeProtocol:
            {
                uint16_t version;
                packetBuffer.parseProtocolUpgradeRequest(version);
                auto reply = PacketBuffer::createUpgradeProtocolReply(requestId, version == 0);
                return reply;
            }
        case PacketType::Rpc:
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
        const DictPtr<IString, IBaseObject> dictObj = obj.asPtr<IDict>(true);

        const auto funcName = dictObj.get("Name");
        ParamsDictPtr funcParams;
        if (dictObj.hasKey("Params"))
            funcParams = dictObj.get("Params");

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

    protectComponent(component, Permission::Read);
    return ComponentHolder(component);
}

BaseObjectPtr ConfigProtocolServer::getSerializedRootDevice(const ParamsDictPtr& params)
{
    protectComponent(rootDevice, Permission::Read);

    serializer.reset();
    rootDevice.serialize(serializer);

    return serializer.getOutput();
}

BaseObjectPtr ConfigProtocolServer::connectSignal(const InputPortPtr& inputPort, const ParamsDictPtr& params)
{
    const StringPtr signalId = params.get("SignalId");
    const SignalPtr signal = findComponent(signalId);
    return ConfigServerInputPort::connect(inputPort, signal);
}

void ConfigProtocolServer::coreEventCallback(ComponentPtr& component, CoreEventArgsPtr& eventArgs)
{
    const auto packed = packCoreEvent(component, eventArgs);
    sendNotification(packed);
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

void ConfigProtocolServer::protectComponent(const ComponentPtr& component, const std::vector<Permission>& requiredPermissions) const
{
    auto permissionManager = component.getPermissionManager();

    for (const auto permission : requiredPermissions)
    {
        if (!permissionManager.isAuthorized(user, permission))
            throw AccessDeniedException();
    }
}

void ConfigProtocolServer::protectComponent(const ComponentPtr& component, Permission requiredPermission) const 
{
    const std::vector<Permission> requiredPermissions = {requiredPermission};
    protectComponent(component, requiredPermissions);
}

BaseObjectPtr ConfigProtocolServer::getTypeManager(const ParamsDictPtr& params) const
{
    protectComponent(rootDevice, Permission::Read);
    const auto typeManager = rootDevice.getContext().getTypeManager();
    return typeManager;
}

void ConfigProtocolServer::processClientToDeviceStreamingPacket(uint32_t signalNumericId, const PacketPtr& packet)
{
    // TODO
}

}
