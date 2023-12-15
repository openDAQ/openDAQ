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
    ConfigProtocolServer(DevicePtr rootDevice, NotificationReadyCallback notificationReadyCallback);
    void buildRpcDispatchStructure();

    // called from transport layer
    PacketBuffer processRequestAndGetReply(const PacketBuffer& packetBuffer);
    PacketBuffer processRequestAndGetReply(void *mem);

    // called internally from component updates or from the external code
    void sendNotification(const char* json, size_t jsonSize) const;
    void sendNotification(const BaseObjectPtr& obj);

    void setComponentFinder(std::unique_ptr<IComponentFinder>& componentFinder);
    std::unique_ptr<IComponentFinder>& getComponentFinder();

private:
    using DispatchFunction = std::function<BaseObjectPtr(const DictPtr<IString, IBaseObject>&)>;

    DevicePtr rootDevice;
    NotificationReadyCallback notificationReadyCallback;
    DeserializerPtr deserializer;
    SerializerPtr serializer;
    SerializerPtr notificationSerializer;
    std::unordered_map<std::string, DispatchFunction> rpcDispatch;
    std::mutex notificationSerializerLock;
    std::unique_ptr<IComponentFinder> componentFinder;

    PacketBuffer processPacket(const PacketBuffer& packetBuffer);
    StringPtr processRpc(const StringPtr& jsonStr);

    BaseObjectPtr callRpc(const StringPtr& name, const DictPtr<IString, IBaseObject>& params);
    ComponentPtr findComponent(std::string componentGlobalId) const;

    BaseObjectPtr getComponent(const DictPtr<IString, IBaseObject>& params) const;
    BaseObjectPtr setPropertyValue(const DictPtr<IString, IBaseObject>& params) const;
};

}
