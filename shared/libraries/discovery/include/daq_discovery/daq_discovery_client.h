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
#include <coreobjects/property_object_ptr.h>
#include <daq_discovery/mdnsdiscovery_client.h>

BEGIN_NAMESPACE_DISCOVERY

class DiscoveryClient final
{
public:
    explicit DiscoveryClient(std::unordered_set<std::string> requiredCaps = {});
    
    void initMdnsClient(const ListPtr<IString>& serviceNames, std::chrono::milliseconds discoveryDuration = 500ms);
    std::vector<MdnsDiscoveredDevice> discoverMdnsDevices() const;

    static void populateDiscoveredInfoProperties(PropertyObjectPtr& info, const MdnsDiscoveredDevice& device);

protected:
    bool verifyDiscoveredDevice(const MdnsDiscoveredDevice& discoveredDevice) const;

    std::shared_ptr<MDNSDiscoveryClient> mdnsClient;
    std::unordered_set<std::string> requiredCaps;
};

END_NAMESPACE_DISCOVERY
