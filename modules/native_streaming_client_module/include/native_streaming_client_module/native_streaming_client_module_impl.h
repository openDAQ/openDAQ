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
#include <native_streaming_client_module/common.h>
#include <opendaq/module_impl.h>
#include <daq_discovery/daq_discovery_client.h>
#include <native_streaming_protocol/native_streaming_client_handler.h>

#include <boost/asio/io_context.hpp>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_CLIENT_MODULE

class NativeStreamingClientModule final : public Module
{
public:
    explicit NativeStreamingClientModule(ContextPtr context);
    ~NativeStreamingClientModule() override;

    ListPtr<IDeviceInfo> onGetAvailableDevices() override;
    DictPtr<IString, IDeviceType> onGetAvailableDeviceTypes() override;
    DevicePtr onCreateDevice(const StringPtr& deviceConnectionString,
                             const ComponentPtr& parent,
                             const PropertyObjectPtr& config) override;
    bool onAcceptsConnectionParameters(const StringPtr& connectionString, const PropertyObjectPtr& config) override;
    bool onAcceptsStreamingConnectionParameters(const StringPtr& connectionString, const PropertyObjectPtr& config) override;
    StreamingPtr onCreateStreaming(const StringPtr& connectionString, const PropertyObjectPtr& config) override;

private:
    static bool connectionStringHasPrefix(const StringPtr& connectionString, const char* prefix);
    DeviceTypePtr createPseudoDeviceType();
    DeviceTypePtr createDeviceType();
    static StringPtr getHost(const StringPtr& url);
    static StringPtr getPort(const StringPtr& url);
    static StringPtr getPath(const StringPtr& url);
    static bool validateConnectionString(const StringPtr& connectionString);

    std::shared_ptr<boost::asio::io_context> addStreamingProcessingContext(const StringPtr& connectionString);
    opendaq_native_streaming_protocol::NativeStreamingClientHandlerPtr createAndConnectTransportClient(
        const StringPtr& host,
        const StringPtr& port,
        const StringPtr& path,
        const PropertyObjectPtr& transportLayerConfig);

    StreamingPtr createNativeStreaming(const StringPtr& connectionString,
                                       opendaq_native_streaming_protocol::NativeStreamingClientHandlerPtr transportClientHandler,
                                       Int streamingInitTimeout);

    DevicePtr createNativeDevice(const ContextPtr& context,
                                 const ComponentPtr& parent,
                                 const StringPtr& connectionString,
                                 const PropertyObjectPtr& config,
                                 const StringPtr& host,
                                 const StringPtr& port,
                                 const StringPtr& path);
    PropertyObjectPtr createDeviceDefaultConfig();
    void populateTransportLayerConfigFromContext(PropertyObjectPtr transportLayerConfig);
    PropertyObjectPtr createTransportLayerDefaultConfig();
    bool validateDeviceConfig(const PropertyObjectPtr& config);
    bool validateTransportLayerConfig(const PropertyObjectPtr& config);

    std::mutex sync;
    size_t pseudoDeviceIndex;
    discovery::DiscoveryClient discoveryClient;

    using ProcessingContext = std::tuple<StringPtr, std::thread, std::shared_ptr<boost::asio::io_context>>;
    std::vector<ProcessingContext> processingContextPool;
};

END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_CLIENT_MODULE
