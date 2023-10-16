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
#include <opcua_client_module/common.h>
#include <opendaq/module_impl.h>
#include <daq_discovery/daq_discovery_client.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA_CLIENT_MODULE

class OpcUaClientModule final : public Module
{
public:
    OpcUaClientModule(ContextPtr context);

    ListPtr<IDeviceInfo> onGetAvailableDevices() override;
    DictPtr<IString, IDeviceType> onGetAvailableDeviceTypes() override;
    DevicePtr onCreateDevice(const StringPtr& connectionString,
                             const ComponentPtr& parent,
                             const PropertyObjectPtr& config) override;
    bool onAcceptsConnectionParameters(const StringPtr& connectionString, const PropertyObjectPtr& config) override;

private:
    static std::string GetUrlFromConnectionString(const StringPtr& connectionString);
    static bool acceptDeviceProperties(const PropertyObjectPtr& config);
    static PropertyObjectPtr createDeviceDefaultConfig();
    static void configureStreamingSources(const PropertyObjectPtr& deviceConfig, const DevicePtr& device);
    static DeviceTypePtr createDeviceType();
    discovery::DiscoveryClient discoveryClient;

    std::mutex sync;
};

END_NAMESPACE_OPENDAQ_OPCUA_CLIENT_MODULE
