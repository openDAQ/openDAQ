/*
 * Copyright 2022-2024 Blueberry d.o.o.
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
#include <websocket_streaming_client_module/common.h>
#include <opendaq/module_impl.h>
#include <daq_discovery/daq_discovery_client.h>

BEGIN_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING_CLIENT_MODULE

class WebsocketStreamingClientModule final : public Module
{
public:
    WebsocketStreamingClientModule(ContextPtr context);

    ListPtr<IDeviceInfo> onGetAvailableDevices() override;
    DictPtr<IString, IDeviceType> onGetAvailableDeviceTypes() override;
    DevicePtr onCreateDevice(const StringPtr& connectionString,
                             const ComponentPtr& parent,
                             const PropertyObjectPtr& config) override;
    bool onAcceptsConnectionParameters(const StringPtr& connectionString, const PropertyObjectPtr& config) override;
    bool onAcceptsStreamingConnectionParameters(const StringPtr& connectionString, const StreamingInfoPtr& config) override;
    StreamingPtr onCreateStreaming(const StringPtr& connectionString, const StreamingInfoPtr& config) override;

private:
    static StringPtr tryCreateWebsocketConnectionString(const StreamingInfoPtr& config);
    static DeviceTypePtr createWebsocketDeviceType();
    static DeviceTypePtr createTcpsocketDeviceType();

    std::mutex sync;
    size_t deviceIndex;
    discovery::DiscoveryClient discoveryClient;
};

END_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING_CLIENT_MODULE
