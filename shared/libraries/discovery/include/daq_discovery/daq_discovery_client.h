/*
 * Copyright 2022-2025 openDAQ d.o.o.
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

using ServerCapabilityCallback = std::function<ServerCapabilityPtr(MdnsDiscoveredDevice)>;

class DiscoveryClient final
{
public:
    explicit DiscoveryClient(std::vector<ServerCapabilityCallback> serverCapabilityCbs, std::unordered_set<std::string> requiredCaps = {});
    
    void initMdnsClient(const ListPtr<IString>& serviceNames, std::chrono::milliseconds discoveryDuration = 500ms);
    ListPtr<IDeviceInfo> discoverDevices() const;

protected:
    ListPtr<IDeviceInfo> discoverMdnsDevices() const;
    DeviceInfoPtr createDeviceInfo(MdnsDiscoveredDevice discoveredDevice) const;

    std::shared_ptr<MDNSDiscoveryClient> mdnsClient;
    std::unordered_set<std::string> requiredCaps;
    std::vector<ServerCapabilityCallback> serverCapabilityCallbacks;
};

END_NAMESPACE_DISCOVERY
