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
#include <opendaq/server_capability_config_ptr.h>

#include <boost/asio/io_context.hpp>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_CLIENT_MODULE

class NativeStreamingClientModule final : public Module
{
public:
    explicit NativeStreamingClientModule(ContextPtr context);
    ~NativeStreamingClientModule() override;

    ListPtr<IDeviceInfo> onGetAvailableDevices() override;
    DictPtr<IString, IDeviceType> onGetAvailableDeviceTypes() override;
    DictPtr<IString, IStreamingType> onGetAvailableStreamingTypes() override;
    DevicePtr onCreateDevice(const StringPtr& deviceConnectionString,
                             const ComponentPtr& parent,
                             const PropertyObjectPtr& config) override;
    StreamingPtr onCreateStreaming(const StringPtr& connectionString, const PropertyObjectPtr& config) override;
    StringPtr onCreateConnectionString(const ServerCapabilityPtr& serverCapability) override;

private:
    DeviceTypePtr createPseudoDeviceType();
    DeviceTypePtr createDeviceType();
    StreamingTypePtr createStreamingType();

    static StringPtr GetHost(const StringPtr& url);
    static StringPtr GetPort(const StringPtr& url, const PropertyObjectPtr& config = nullptr);
    static StringPtr GetPath(const StringPtr& url);

    static bool ConnectionStringHasPrefix(const StringPtr& connectionString, const char* prefix);
    static bool ValidateConnectionString(const StringPtr& connectionString);

    static void SetupProtocolAddresses(const discovery::MdnsDiscoveredDevice& discoveredDevice, ServerCapabilityConfigPtr& cap, std::string protocolPrefix);

    /// adds address to server capabilities
    /// @param capabilities The list of device server capabilities
    /// @param address IPv4 or IPv6 device address
    static void CompleteServerCapabilities(const ListPtr<IServerCapability>& capabilities,
                                           const StringPtr& address);

    static StringPtr CreateUrlConnectionString(std::string prefix,
                                               const StringPtr& host,
                                               const IntegerPtr& port,
                                               const StringPtr& path);

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
    PropertyObjectPtr createConnectionDefaultConfig();
    bool acceptsConnectionParameters(const StringPtr& connectionString, const PropertyObjectPtr& config);
    bool acceptsStreamingConnectionParameters(const StringPtr& connectionString, const PropertyObjectPtr& config);
    void populateTransportLayerConfigFromContext(PropertyObjectPtr transportLayerConfig);
    PropertyObjectPtr populateDefaultConfig(const PropertyObjectPtr& config);
    PropertyObjectPtr createTransportLayerDefaultConfig();
    bool validateConnectionConfig(const PropertyObjectPtr& config);
    bool validateTransportLayerConfig(const PropertyObjectPtr& config);

    std::mutex sync;
    size_t pseudoDeviceIndex;
    size_t transportClientIndex;
    std::string transportClientUuidBase;
    discovery::DiscoveryClient discoveryClient;

    using ProcessingContext = std::tuple<StringPtr, std::thread, std::shared_ptr<boost::asio::io_context>>;
    std::vector<ProcessingContext> processingContextPool;
};

END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_CLIENT_MODULE
