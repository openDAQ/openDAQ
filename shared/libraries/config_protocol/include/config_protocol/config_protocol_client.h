/*
 * Copyright 2022-2023 Blueberry d.o.o.
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

namespace daq::config_protocol
{

using SendRequestCallback = std::function<PacketBuffer(PacketBuffer&)>;
using ServerNotificationReceivedCallback = std::function<bool(const BaseObjectPtr& obj)>;

class ConfigProtocolClientComm : public std::enable_shared_from_this<ConfigProtocolClientComm>
{
public:
    friend class ConfigProtocolClient;
    explicit ConfigProtocolClientComm(const ContextPtr& daqContext, SendRequestCallback sendRequestCallback);

    void setPropertyValue(const std::string& globalId, const std::string& propertyName, const BaseObjectPtr& propertyValue);
    void setProtectedPropertyValue(const std::string& globalId, const std::string& propertyName, const BaseObjectPtr& propertyValue);
    BaseObjectPtr getPropertyValue(const std::string& globalId, const std::string& propertyName);
    void clearPropertyValue(const std::string& globalId, const std::string& propertyName);
    BaseObjectPtr callProperty(const std::string& globalId, const std::string& propertyName, const BaseObjectPtr& params);

    bool getConnected() const;

    BaseObjectPtr sendComponentCommand(const StringPtr& globalId,
                                       const StringPtr& command,
                                       ParamsDictPtr& params,
                                       const ComponentPtr& parentComponent = nullptr);
    BaseObjectPtr sendComponentCommand(const StringPtr& globalId, const StringPtr& command, const ComponentPtr& parentComponent = nullptr);
    BaseObjectPtr sendCommand(const StringPtr& command, const ParamsDictPtr& params = nullptr);

private:
    ContextPtr daqContext;
    size_t id;
    SendRequestCallback sendRequestCallback;
    SerializerPtr serializer;
    DeserializerPtr deserializer;
    bool connected;

    BaseObjectPtr createRpcRequest(const StringPtr& name, const ParamsDictPtr& params) const;
    StringPtr createRpcRequestJson(const StringPtr& name, const ParamsDictPtr& params);
    PacketBuffer createRpcRequestPacketBuffer(size_t id, const StringPtr& name, const ParamsDictPtr& params);
    BaseObjectPtr parseRpcReplyPacketBuffer(const PacketBuffer& packetBuffer, const ComponentDeserializeContextPtr& context = nullptr);
    size_t generateId();

    BaseObjectPtr deserializeConfigComponent(const StringPtr& typeId,
                                             const SerializedObjectPtr& serObj,
                                             const BaseObjectPtr& context,
                                             const FunctionPtr& factoryCallback);

    BaseObjectPtr sendComponentCommandInternal(const StringPtr& command,
                                               const ParamsDictPtr& params,
                                               const ComponentPtr& parentComponent = nullptr);
};

using ConfigProtocolClientCommPtr = std::shared_ptr<ConfigProtocolClientComm>;

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
                                  const ServerNotificationReceivedCallback& serverNotificationReceivedCallback);

    // called from client module
    void connect(const ComponentPtr& parent = nullptr);
    DevicePtr getDevice();
    ConfigProtocolClientCommPtr getClientComm();


    // called from transport layer when notification packet is available. This will call serverNotificationReceivedCallback and
    // triggerNotificationObject method
    void triggerNotificationPacket(const PacketBuffer& packet);

private:
    ContextPtr daqContext;
    SendRequestCallback sendRequestCallback;
    ServerNotificationReceivedCallback serverNotificationReceivedCallback;
    DeserializerPtr deserializer;

    ConfigProtocolClientCommPtr clientComm;
    DevicePtr device;

    // this should handle server component updates
    void triggerNotificationObject(const BaseObjectPtr& object);

    // called on connect to build initial device tree
    void buildDevice(const DevicePtr& device);

};


}
