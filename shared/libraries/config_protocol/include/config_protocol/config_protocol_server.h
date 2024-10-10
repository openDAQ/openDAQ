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
#include <config_protocol/config_protocol_streaming_consumer.h>
#include <opendaq/device_ptr.h>

#include <opendaq/component_holder_ptr.h>

namespace daq::config_protocol
{

using NotificationReadyCallback = std::function<void(const PacketBuffer&)>;

class IComponentFinder
{
public:
    virtual ComponentPtr findComponent(const std::string& globalId) = 0;
    virtual ~IComponentFinder() = default;
};

class ComponentFinderRootDevice: public IComponentFinder
{
public:
    ComponentFinderRootDevice(DevicePtr rootDevice);
    ComponentPtr findComponent(const std::string& globalId) override;
private:
    DevicePtr rootDevice;
    static ComponentPtr findComponentInternal(const ComponentPtr& component, const std::string& id);
};


class ConfigProtocolServer
{
public:
    ConfigProtocolServer(DevicePtr rootDevice,
                         NotificationReadyCallback notificationReadyCallback,
                         const UserPtr& user,
                         const FolderConfigPtr& externalSignalsFolder = nullptr);
    ~ConfigProtocolServer();

    void buildRpcDispatchStructure();

    // called from transport layer
    PacketBuffer processRequestAndGetReply(const PacketBuffer& packetBuffer);
    PacketBuffer processRequestAndGetReply(void *mem);
    static PacketBuffer generateConnectionRejectedReply(uint64_t requestId,
                                                        ErrCode errCode,
                                                        const StringPtr& message,
                                                        const SerializerPtr& serializer);

    // called from transport layer
    void processNoReplyRequest(const PacketBuffer& packetBuffer);
    void processNoReplyRequest(void *mem);

    // called internally from component updates or from the external code
    void sendNotification(const char* json, size_t jsonSize) const;
    void sendNotification(const BaseObjectPtr& obj);

    DevicePtr getRootDevice();

    void setComponentFinder(std::unique_ptr<IComponentFinder>& componentFinder);
    std::unique_ptr<IComponentFinder>& getComponentFinder();

    void processClientToServerStreamingPacket(SignalNumericIdType signalNumericId, const PacketPtr& packet);

    uint16_t getProtocolVersion() const;
    void setProtocolVersion(uint16_t protocolVersion);

private:
    using DispatchFunction = std::function<BaseObjectPtr(const ParamsDictPtr&)>;
    template <typename T>
    using RpcHandlerFunction = std::function<BaseObjectPtr(const RpcContext& context, const T& component, const ParamsDictPtr& params)>;

    DevicePtr rootDevice;
    ContextPtr daqContext;
    NotificationReadyCallback notificationReadyCallback;
    DeserializerPtr deserializer;
    SerializerPtr serializer;
    SerializerPtr notificationSerializer;
    std::unordered_map<std::string, DispatchFunction> rpcDispatch;
    std::mutex notificationSerializerLock;
    std::unique_ptr<IComponentFinder> componentFinder;
    UserPtr user;
    uint16_t protocolVersion;
    const std::set<uint16_t> supportedServerVersions;
    ConfigProtocolStreamingConsumer streamingConsumer;

    PacketBuffer processPacketAndGetReply(const PacketBuffer& packetBuffer);
    void processNoReplyPacket(const PacketBuffer& packetBuffer);
    StringPtr processRpcAndGetReply(const StringPtr& jsonStr);
    void processNoReplyRpc(const StringPtr& jsonStr);
    static StringPtr prepareErrorResponse(Int errorCode, const StringPtr& message, const SerializerPtr& serializer);

    BaseObjectPtr callRpc(const StringPtr& name, const ParamsDictPtr& params);
    ComponentPtr findComponent(const std::string& componentGlobalId) const;

    BaseObjectPtr getComponent(const ParamsDictPtr& params) const;
    BaseObjectPtr getTypeManager(const ParamsDictPtr& params) const;
    BaseObjectPtr getSerializedRootDevice(const ParamsDictPtr& params);
    BaseObjectPtr connectSignal(const RpcContext& context, const InputPortPtr& inputPort, const ParamsDictPtr& params);
    BaseObjectPtr connectExternalSignal(const RpcContext& context, const InputPortPtr& inputPort, const ParamsDictPtr& params);
    BaseObjectPtr removeExternalSignals(const ParamsDictPtr& params);
    BaseObjectPtr acceptsSignal(const RpcContext& context, const InputPortPtr& inputPort, const ParamsDictPtr& params);

    template <class SmartPtr>
    void addHandler(const std::string& name, const RpcHandlerFunction<SmartPtr>& handler);
    
    void coreEventCallback(ComponentPtr& component, CoreEventArgsPtr& eventArgs);
    
    ListPtr<IBaseObject> packCoreEvent(const ComponentPtr& component, const CoreEventArgsPtr& args);
    CoreEventArgsPtr processCoreEventArgs(const CoreEventArgsPtr& args);
    CoreEventArgsPtr processUpdateEndCoreEvent(const ComponentPtr& component, const CoreEventArgsPtr& args);
};

}
