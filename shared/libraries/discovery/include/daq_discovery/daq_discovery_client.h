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
#include <coretypes/listobject_factory.h>
#include <opendaq/device_info_ptr.h>
#include <daq_discovery/mdnsdiscovery_client.h>

BEGIN_NAMESPACE_DISCOVERY

class DiscoveryClient
{
public:
    explicit DiscoveryClient(std::function<std::string(MdnsDiscoveredDevice)> connectionStringFormatCb, std::unordered_set<std::string> requiredCaps = {});
    
    void initMdnsClient(const std::string& serviceName, std::chrono::milliseconds discoveryDuration = 500ms);
    virtual ListPtr<IDeviceInfo> discoverDevices();

protected:
    ListPtr<IDeviceInfo> discoveredDevices;
    std::shared_ptr<MDNSDiscoveryClient> mdnsClient;
    std::vector<std::thread> threadPool;
    std::unordered_set<std::string> requiredCaps;

    void runInThread(std::function<void()> func);
    void joinThreads();
    void discoverMdnsDevices();
    DeviceInfoPtr createDeviceInfo(MdnsDiscoveredDevice discoveredDevice) const;
    std::function<std::string(MdnsDiscoveredDevice)> connectionStringFormatCb;
};

END_NAMESPACE_DISCOVERY
