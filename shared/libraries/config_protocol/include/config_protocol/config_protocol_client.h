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
#include <opendaq/device_ptr.h>

namespace daq::config_protocol
{

using SendRequestCallback = std::function<PacketBuffer(PacketBuffer&)>;
using ServerNotificationReceivedCallback = std::function<bool(const BaseObjectPtr& obj)>;

class ConfigProtocolClientComm
{
public:
    friend class ConfigProtocolClient;
    explicit ConfigProtocolClientComm(SendRequestCallback sendRequestCallback);

    void setPropertyValue(const std::string& globalId, const std::string& propertyName, const BaseObjectPtr& propertyValue);
private:
    size_t id;
    SendRequestCallback sendRequestCallback;
    SerializerPtr serializer;
    DeserializerPtr deserializer;

    BaseObjectPtr createRpcRequest(const StringPtr& name, const DictPtr<IString, IBaseObject>& params) const;
    StringPtr createRpcRequestJson(const StringPtr& name, const DictPtr<IString, IBaseObject>& params);
    PacketBuffer createRpcRequestPacketBuffer(size_t id, const StringPtr& name, const DictPtr<IString, IBaseObject>& params);
    BaseObjectPtr parseRpcReplyPacketBuffer(const PacketBuffer& packetBuffer) const;
    size_t generateId();
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

    explicit ConfigProtocolClient(const SendRequestCallback& sendRequestCallback,
                                  const ServerNotificationReceivedCallback& serverNotificationReceivedCallback);

    // called from client module
    void connect();
    DevicePtr getDevice();
    ConfigProtocolClientCommPtr getClientComm();


    // called from transport layer when notification packet is available. This will call serverNotificationReceivedCallback and
    // triggerNotificationObject method
    void triggerNotificationPacket(const PacketBuffer& packet);

private:
    SendRequestCallback sendRequestCallback;
    ServerNotificationReceivedCallback serverNotificationReceivedCallback;
    DeserializerPtr deserializer;

    std::shared_ptr<ConfigProtocolClientComm> clientComm;
    DevicePtr device;

    // this should handle server component updates
    void triggerNotificationObject(const BaseObjectPtr& object);

    // called on connect to build initial device tree
    void buildDevice(const BaseObjectPtr& obj);
};


}
