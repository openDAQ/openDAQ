/*
 * Copyright 2022-2024 openDAQ d. o. o.
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
#include <thread>
#include <opendaq/device_impl.h>
#include <opendaq/context_ptr.h>
#include <opendaq/device_info_config_ptr.h>
#include <opendaq/mock/mock_channel.h>

BEGIN_NAMESPACE_OPENDAQ

class MockPhysicalDeviceImpl : public Device
{
public:
    MockPhysicalDeviceImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId, const PropertyObjectPtr& config);
    ~MockPhysicalDeviceImpl();

    daq::DeviceInfoPtr onGetInfo() override;
    uint64_t onGetTicksSinceOrigin() override;
    bool allowAddDevicesFromModules() override;

    void setDeviceDomainHelper(const DeviceDomainPtr& deviceDomain);

protected:
    void startAcq();
    void stopAcq();
    void generatePackets(size_t packetCount);
    void registerProperties();
    void registerTestConfigProperties();

    PropertyObjectPtr config;
    FolderConfigPtr mockFolderA; // InputsOutputs/mockFolderA
    FolderConfigPtr mockFolderB; // InputsOutputs/mockFolderB
    ChannelPtr mockChannel1; // InputsOutputs/mockChannel1
    ChannelPtr mockChannelA1; // InputsOutputs/mockFolderA/mockChannelA1
    ChannelPtr mockChannelB1; // InputsOutputs/mockFolderB/mockChannelB1
    ChannelPtr mockChannelB2; // InputsOutputs/mockFolderB/mockChannelB1
    std::thread generateThread;
    DeviceInfoConfigPtr deviceInfo;

    FolderPtr componentA;
    ComponentPtr componentA1;
    ComponentPtr componentB;
    uint64_t time;
};

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    INTERNAL_FACTORY,
    MockPhysicalDevice, daq::IDevice,
    daq::IContext*, ctx,
    daq::IComponent*, parent,
    daq::IString*, localId,
    daq::IPropertyObject*, config)

END_NAMESPACE_OPENDAQ
