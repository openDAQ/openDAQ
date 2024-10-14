/*
 * Copyright 2022-2024 openDAQ d.o.o.
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

#include <config_protocol/config_protocol.h>
#include <opendaq/component_deserialize_context_ptr.h>
#include <opendaq/device_ptr.h>
#include <opendaq/component_holder_ptr.h>
#include <opendaq/ids_parser.h>
#include <opendaq/deserialize_component_ptr.h>
#include <opendaq/mirrored_signal_private_ptr.h>
#include <config_protocol/config_client_object.h>
#include <coretypes/cloneable.h>
#include <coreobjects/core_event_args_factory.h>
#include <set>
#include <opendaq/custom_log.h>
#include <config_protocol/config_protocol_streaming_producer.h>

namespace daq::config_protocol
{

using SendRequestCallback = std::function<PacketBuffer(PacketBuffer&)>;
using SendNoReplyRequestCallback = std::function<void(PacketBuffer&)>;
using ServerNotificationReceivedCallback = std::function<bool(const BaseObjectPtr& obj)>;
using ComponentDeserializeCallback = std::function<ErrCode(ISerializedObject*, IBaseObject*, IFunction*, IBaseObject**)>;

using ConfigProtocolStreamingProducerPtr = std::shared_ptr<ConfigProtocolStreamingProducer>;

class ConfigProtocolClientComm : public std::enable_shared_from_this<ConfigProtocolClientComm>
{
public:
    template <class TRootDeviceImpl>
    friend class ConfigProtocolClient;
    explicit ConfigProtocolClientComm(const ContextPtr& daqContext,
                                      SendRequestCallback sendRequestCallback,
                                      SendNoReplyRequestCallback sendNoReplyRequestCallback,
                                      const ConfigProtocolStreamingProducerPtr& streamingProducer,
                                      ComponentDeserializeCallback rootDeviceDeserializeCallback);

    void setPropertyValue(const std::string& globalId, const std::string& propertyName, const BaseObjectPtr& propertyValue);
    void setProtectedPropertyValue(const std::string& globalId, const std::string& propertyName, const BaseObjectPtr& propertyValue);
    BaseObjectPtr getPropertyValue(const std::string& globalId, const std::string& propertyName);
    void clearPropertyValue(const std::string& globalId, const std::string& propertyName);
    void update(const std::string& globalId, const std::string& serialized, const std::string& path);
    BaseObjectPtr callProperty(const std::string& globalId, const std::string& propertyName, const BaseObjectPtr& params);
    void setAttributeValue(const std::string& globalId, const std::string& attributeName, const BaseObjectPtr& attributeValue);
    BaseObjectPtr getLastValue(const std::string& globalId);
    void lock(const std::string& globalId);
    void unlock(const std::string& globalId);
    bool isLocked(const std::string& globalId);

    void beginUpdate(const std::string& globalId, const std::string& path);
    void endUpdate(const std::string& globalId, const std::string& path, const ListPtr<IDict>& props = nullptr);

    bool getConnected() const;
    ContextPtr getDaqContext();

    BaseObjectPtr sendComponentCommand(const StringPtr& globalId,
                                       const StringPtr& command,
                                       ParamsDictPtr& params,
                                       const ComponentPtr& parentComponent = nullptr);
    BaseObjectPtr sendComponentCommand(const StringPtr& globalId, const StringPtr& command, const ComponentPtr& parentComponent = nullptr);
    BaseObjectPtr sendCommand(const StringPtr& command, const ParamsDictPtr& params = nullptr);
    void sendNoReplyCommand(const StringPtr& command, const ParamsDictPtr& params = nullptr);

    static SignalPtr findSignalByRemoteGlobalId(const DevicePtr& device, const std::string& remoteGlobalId);

    void setRootDevice(const DevicePtr& rootDevice);
    DevicePtr getRootDevice() const;

    void connectDomainSignals(const ComponentPtr& component);
    void connectInputPorts(const ComponentPtr& component);
    void disconnectExternalSignals();

    BaseObjectPtr deserializeConfigComponent(const StringPtr& typeId,
                                             const SerializedObjectPtr& serObj,
                                             const BaseObjectPtr& context,
                                             const FunctionPtr& factoryCallback,
                                             ComponentDeserializeCallback deviceDeserialzeCallback);
    bool isComponentNested(const StringPtr& componentGlobalId);
    void connectExternalSignalToServerInputPort(const SignalPtr& signal, const StringPtr& inputPortRemoteGlobalId);
    void disconnectExternalSignalFromServerInputPort(const SignalPtr& signal, const StringPtr& inputPortRemoteGlobalId);

    uint16_t getProtocolVersion() const;

private:
    ContextPtr daqContext;
    std::atomic<uint64_t> id;
    SendRequestCallback sendRequestCallback;
    SendNoReplyRequestCallback sendNoReplyRequestCallback;
    ComponentDeserializeCallback rootDeviceDeserializeCallback;
    bool connected;
    WeakRefPtr<IDevice> rootDeviceRef;
    uint16_t protocolVersion;
    std::weak_ptr<ConfigProtocolStreamingProducer> streamingProducerRef;

    ComponentDeserializeContextPtr createDeserializeContext(const std::string& remoteGlobalId,
                                                            const ContextPtr& context,
                                                            const ComponentPtr& root,
                                                            const ComponentPtr& parent,
                                                            const StringPtr& localId,
                                                            IntfID* intfID);
    BaseObjectPtr createRpcRequest(const StringPtr& name, const ParamsDictPtr& params) const;
    StringPtr createRpcRequestJson(const StringPtr& name, const ParamsDictPtr& params);
    PacketBuffer createRpcRequestPacketBuffer(uint64_t id, const StringPtr& name, const ParamsDictPtr& params);
    PacketBuffer createNoReplyRpcRequestPacketBuffer(const StringPtr& name, const ParamsDictPtr& params);
    BaseObjectPtr parseRpcOrRejectReply(const StringPtr& jsonReply,
                                        const ComponentDeserializeContextPtr& context = nullptr,
                                        bool isGetRootDeviceReply = false);
    uint64_t generateId();

    BaseObjectPtr sendComponentCommandInternal(const StringPtr& command,
                                               const ParamsDictPtr& params,
                                               const ComponentPtr& parentComponent = nullptr,
                                               bool isGetRootDeviceCommand = false);

    BaseObjectPtr requestRootDevice(const ComponentPtr& parentComponent);
    StringPtr requestSerializedRootDevice();

    static SignalPtr findSignalByRemoteGlobalIdWithComponent(const ComponentPtr& component, const std::string& remoteGlobalId);

    template <class Interface, class F>
    void forEachComponent(const ComponentPtr& component, const F& f);
    [[maybe_unused]]
    void setRemoteGlobalIds(const ComponentPtr& component, const StringPtr& parentRemoteId);

    void setProtocolVersion(uint16_t protocolVersion);
    std::tuple<uint32_t, StringPtr, StringPtr> getExternalSignalParams(const SignalPtr& signal,
                                                                       const ConfigProtocolStreamingProducerPtr& streamingProducer);
};

using ConfigProtocolClientCommPtr = std::shared_ptr<ConfigProtocolClientComm>;

template <class TRootDeviceImpl>
class ConfigProtocolClient
{
public:
    // sendRequestCallback is called from this object when a request is available
    // it should send the packet and return reply packet
    //
    // serverNotificationReceivedCallback is used by external code if for any reason needs to preprocess
    // server notification. it should return false when the notification should be handled by the ConfigProtocolClient

    explicit ConfigProtocolClient(const ContextPtr& daqContext,
                                  const SendRequestCallback& sendRequestCallback,
                                  const SendNoReplyRequestCallback& sendNoReplyRequestCallback,
                                  const SendDaqPacketCallback& sendDaqPacketCallback,
                                  const ServerNotificationReceivedCallback& serverNotificationReceivedCallback);

    // called from client module
    DevicePtr connect(const ComponentPtr& parent = nullptr, uint16_t protocolVersion = std::numeric_limits<uint16_t>::max());
    void reconnect(Bool restoreClientConfigOnReconnect);
    void disconnectExternalSignals();

    DevicePtr getDevice();
    ConfigProtocolClientCommPtr getClientComm();

    // called from transport layer when notification packet is available. This will call serverNotificationReceivedCallback and
    // triggerNotificationObject method
    void triggerNotificationPacket(const PacketBuffer& packet);

private:
    ContextPtr daqContext;
    SendRequestCallback sendRequestCallback;
    SendNoReplyRequestCallback sendNoReplyRequestCallback;
    SendDaqPacketCallback sendDaqPacketCallback;
    ServerNotificationReceivedCallback serverNotificationReceivedCallback;
    DeserializerPtr deserializer;
    ConfigProtocolStreamingProducerPtr streamingProducer;

    ConfigProtocolClientCommPtr clientComm;
    
    ComponentPtr findComponent(std::string globalId);

    void protocolHandshake(uint16_t protocolVersion);
    void enumerateTypes();

    // this should handle server component updates
    void triggerNotificationObject(const BaseObjectPtr& object);
    CoreEventArgsPtr unpackCoreEvents(const CoreEventArgsPtr& args);
    void handleNonComponentEvent(const CoreEventArgsPtr& args) const;
};

template<class TRootDeviceImpl>
ConfigProtocolClient<TRootDeviceImpl>::ConfigProtocolClient(const ContextPtr& daqContext,
                                                            const SendRequestCallback& sendRequestCallback,
                                                            const SendNoReplyRequestCallback& sendNoReplyRequestCallback,
                                                            const SendDaqPacketCallback& sendDaqPacketCallback,
                                                            const ServerNotificationReceivedCallback& serverNotificationReceivedCallback)
    : daqContext(daqContext)
    , sendRequestCallback(sendRequestCallback)
    , sendDaqPacketCallback(sendDaqPacketCallback)
    , serverNotificationReceivedCallback(serverNotificationReceivedCallback)
    , deserializer(JsonDeserializer())
    , streamingProducer(std::make_shared<ConfigProtocolStreamingProducer>(daqContext, sendDaqPacketCallback))
    , clientComm(
          std::make_shared<ConfigProtocolClientComm>(
              daqContext,
              sendRequestCallback,
              sendNoReplyRequestCallback,
              streamingProducer,
              [](ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj)
              {
                  return TRootDeviceImpl::Deserialize(serialized, context, factoryCallback, obj);
              }))
{
}

template<class TRootDeviceImpl>
void ConfigProtocolClient<TRootDeviceImpl>::protocolHandshake(uint16_t protocolVersion)
{
    auto getProtocolInfoRequestPacketBuffer = PacketBuffer::createGetProtocolInfoRequest(clientComm->generateId());
    const auto replyPacketBuffer = sendRequestCallback(getProtocolInfoRequestPacketBuffer);

    if (replyPacketBuffer.getPacketType() == PacketType::ConnectionRejected)
        clientComm->parseRpcOrRejectReply(replyPacketBuffer.parseConnectionRejectedReply(), nullptr);

    const std::set<uint16_t> supportedClientVersions {0, 1, 2, 3, 4};

    uint16_t currentVersion;
    std::set<uint16_t> supportedServerVersions;
    replyPacketBuffer.parseProtocolInfoReply(currentVersion, supportedServerVersions);

    if (protocolVersion != std::numeric_limits<uint16_t>::max())
    {
        if (std::find(supportedClientVersions.begin(), supportedClientVersions.end(), protocolVersion) == supportedClientVersions.end())
            throw ConfigProtocolException("Protocol not supported on client");

        if (std::find(supportedServerVersions.begin(), supportedServerVersions.end(), protocolVersion) == supportedServerVersions.end())
            throw ConfigProtocolException("Protocol not supported on server");
    }
    else
    {
        std::set<uint16_t> commonVersions;
        std::set_intersection(supportedClientVersions.begin(),
                              supportedClientVersions.end(),
                              supportedServerVersions.begin(),
                              supportedServerVersions.end(),
                              std::inserter(commonVersions, commonVersions.begin()));

        if (commonVersions.empty())
            throw ConfigProtocolException("Cannot handshake a common protocol version");

        protocolVersion = *commonVersions.rbegin();
    }

    auto upgradeProtocolRequestPacketBuffer = PacketBuffer::createUpgradeProtocolRequest(clientComm->generateId(), protocolVersion);
    const auto upgradeProtocolReplyPacketBuffer = sendRequestCallback(upgradeProtocolRequestPacketBuffer);

    bool success;
    upgradeProtocolReplyPacketBuffer.parseProtocolUpgradeReply(success);

    if (!success)
        throw ConfigProtocolException("Protocol upgrade failed");

    clientComm->setProtocolVersion(protocolVersion);
    const auto loggerComponent = daqContext.getLogger().getOrAddComponent("ConfigProtocolClient");
    LOG_I("Config protocol version {} used", protocolVersion);
}

template<class TRootDeviceImpl>
void ConfigProtocolClient<TRootDeviceImpl>::enumerateTypes()
{
    const auto localTypeManager = daqContext.getTypeManager();
    const TypeManagerPtr typeManager = clientComm->sendCommand("GetTypeManager");
    const auto types = typeManager.getTypes();

    for (const auto& typeName : types)
    {
        const auto type = typeManager.getType(typeName);
        try
        {
            ErrCode errCode = localTypeManager->addType(type);
            if (errCode == OPENDAQ_ERR_ALREADYEXISTS)
            {
                const auto loggerComponent = daqContext.getLogger().getOrAddComponent("ConfigProtocolClient");
                LOG_D("Type {} already exists in local type manager", type.getName());
            }
            else if (OPENDAQ_FAILED(errCode))
            {
                ObjectPtr<IErrorInfo> errorInfo;
                daqGetErrorInfo(&errorInfo);
                StringPtr message;
                if (errorInfo.assigned())
                    errorInfo->getMessage(&message);

                const auto loggerComponent = daqContext.getLogger().getOrAddComponent("ConfigProtocolClient");
                LOG_W("Couldn't add type {} to local type manager: {}", type.getName(), message.assigned() ? message: "Unknown error");
            }
        }
        catch (const std::exception& e)
        {
            const auto loggerComponent = daqContext.getLogger().getOrAddComponent("ConfigProtocolClient");
            LOG_W("Couldn't add type {} to local type manager: {}", type.getName(), e.what());
        }
        catch (...)
        {
            const auto loggerComponent = daqContext.getLogger().getOrAddComponent("ConfigProtocolClient");
            LOG_W("Couldn't add type {} to local type manager!", type.getName());
        }
    }
}

template<class TRootDeviceImpl>
void ConfigProtocolClient<TRootDeviceImpl>::reconnect(Bool restoreClientConfigOnReconnect)
{
    if (!clientComm->getConnected())
        throw ConfigProtocolException("The 'reconnect' called without a prior successful 'connect' call.");

    auto rootDevice = clientComm->getRootDevice();
    if (!rootDevice.assigned())
        throw NotAssignedException("Root device is not assigned.");

    protocolHandshake(clientComm->getProtocolVersion());
    enumerateTypes();

    if (restoreClientConfigOnReconnect)
    {
        const auto serializer = JsonSerializer();
        rootDevice.asPtr<IUpdatable>().serializeForUpdate(serializer);
        StringPtr serializedClientRootDevice = serializer.getOutput();

        const auto deserializer = JsonDeserializer();
        deserializer.update(rootDevice.asPtr<IUpdatable>(), serializedClientRootDevice);
    }
    else
    {
        const StringPtr serializedServerRootDevice = clientComm->requestSerializedRootDevice();

        auto dict = Dict<IString, IBaseObject>();
        dict.set("SerializedComponent", serializedServerRootDevice);

        auto args = CoreEventArgs(CoreEventId::ComponentUpdateEnd, nullptr, dict);
        rootDevice.asPtr<IConfigClientObject>()->handleRemoteCoreEvent(rootDevice, args);
    }
}

template<class TRootDeviceImpl>
void ConfigProtocolClient<TRootDeviceImpl>::disconnectExternalSignals()
{
    clientComm->disconnectExternalSignals();
}

template<class TRootDeviceImpl>
DevicePtr ConfigProtocolClient<TRootDeviceImpl>::connect(const ComponentPtr& parent, uint16_t protocolVersion)
{
    protocolHandshake(protocolVersion);
    enumerateTypes();

    const ComponentHolderPtr deviceHolder = clientComm->requestRootDevice(parent);
    auto device = deviceHolder.getComponent();

    clientComm->setRootDevice(device);
    clientComm->connectDomainSignals(device);
    clientComm->connectInputPorts(device);

    clientComm->connected = true;

    return device;
}

template<class TRootDeviceImpl>
DevicePtr ConfigProtocolClient<TRootDeviceImpl>::getDevice()
{
    return clientComm->getRootDevice();
}

template<class TRootDeviceImpl>
ConfigProtocolClientCommPtr ConfigProtocolClient<TRootDeviceImpl>::getClientComm()
{
    return clientComm;
}

template<class TRootDeviceImpl>
void ConfigProtocolClient<TRootDeviceImpl>::triggerNotificationPacket(const PacketBuffer& packet)
{
    const auto json = packet.parseServerNotification();

    const auto deserializeContext = clientComm->createDeserializeContext(std::string{}, daqContext, clientComm->getRootDevice(), nullptr, nullptr, nullptr);
    const auto obj = deserializer.deserialize(json, deserializeContext,
                                              [this](const StringPtr& typeId, const SerializedObjectPtr& object, const BaseObjectPtr& context, const FunctionPtr& factoryCallback)
                                              {
                                                  return clientComm->deserializeConfigComponent(typeId, object, context, factoryCallback, nullptr);
                                              });
    // handle notifications in callback provided in constructor
    const bool processed = serverNotificationReceivedCallback ? serverNotificationReceivedCallback(obj) : false;
    // if callback not processed by callback, process it internally
    if (!processed)
        triggerNotificationObject(obj);
}

template<class TRootDeviceImpl>
ComponentPtr ConfigProtocolClient<TRootDeviceImpl>::findComponent(std::string globalId)
{
    auto rootDevice = clientComm->getRootDevice();
    if (!rootDevice.assigned())
        throw NotAssignedException{"Root device is not assigned."};

    if (globalId.empty())
        return nullptr;

    globalId.erase(globalId.begin(), globalId.begin() + rootDevice.getLocalId().getLength() + 1);
    if (globalId.find_first_of('/') == 0)
        globalId.erase(globalId.begin(), globalId.begin() + 1);

    return rootDevice.findComponent(globalId);
}

template<class TRootDeviceImpl>
void ConfigProtocolClient<TRootDeviceImpl>::triggerNotificationObject(const BaseObjectPtr& object)
{
    ListPtr<IBaseObject> packedEvent = object.asPtrOrNull<IList>();
    if (!packedEvent.assigned() || packedEvent.getCount() != 2)
        return;

    const ComponentPtr component = findComponent(packedEvent[0]);
    const CoreEventArgsPtr argsPtr = unpackCoreEvents(packedEvent[1]);
    if (component.assigned())
    {
        component.asPtr<IConfigClientObject>()->handleRemoteCoreEvent(component, argsPtr);
    }
    else
    {
        try
        {
            handleNonComponentEvent(argsPtr);
        }
        catch([[maybe_unused]] const std::exception& e)
        {
            const auto loggerComponent = daqContext.getLogger().getOrAddComponent("ConfigProtocolClient");
            LOG_D("Failed to handle non-component event {}: {}", argsPtr.getEventName(), e.what());
        }
        catch(...)
        {
            const auto loggerComponent = daqContext.getLogger().getOrAddComponent("ConfigProtocolClient");
            LOG_D("Failed to handle non-component event {}", argsPtr.getEventName());
        }
    }
}

template <class TRootDeviceImpl>
void ConfigProtocolClient<TRootDeviceImpl>::handleNonComponentEvent(const CoreEventArgsPtr& args) const
{
    const auto params = args.getParameters();
    switch (static_cast<CoreEventId>(args.getEventId()))
    {
        case CoreEventId::TypeAdded:
            daqContext.getTypeManager().addType(params.get("Type"));
            break;
        case CoreEventId::TypeRemoved:
            daqContext.getTypeManager().removeType(params.get("TypeName"));
            break;
        default:
            break;
    }
}

template<class TRootDeviceImpl>
CoreEventArgsPtr ConfigProtocolClient<TRootDeviceImpl>::unpackCoreEvents(const CoreEventArgsPtr& args)
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

        dict.set("Component", comp);
    }

    return CoreEventArgs(static_cast<CoreEventId>(args.getEventId()), args.getEventName(), dict);
}

}
