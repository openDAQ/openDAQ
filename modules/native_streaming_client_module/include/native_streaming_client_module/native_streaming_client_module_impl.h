/*
 * Copyright 2022-2026 openDAQ d.o.o.
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
#include <opendaq/transport_option_ptr.h>

#include <boost/asio/io_context.hpp>

#include <optional>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_CLIENT_MODULE

enum class NativeType {config, streaming};

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
    DevicePtr onCreateAuthenticatedDevice(const StringPtr& connectionString,
                                          const ComponentPtr& parent,
                                          const PropertyObjectPtr& config,
                                          const LoginParametersPtr& loginParameters) override;
    StreamingPtr onCreateStreaming(const StringPtr& connectionString, const PropertyObjectPtr& config) override;
    StreamingPtr onCreateAuthenticatedStreaming(const StringPtr& connectionString,
                                                const PropertyObjectPtr& config,
                                                const LoginParametersPtr& loginParameters) override;
    Bool onCompleteServerCapability(const ServerCapabilityPtr& source, const ServerCapabilityConfigPtr& target) override;

private:
    DeviceTypePtr createPseudoDeviceType();
    DeviceTypePtr createDeviceType();
    StreamingTypePtr createStreamingType();

    /// @brief reads the transport option the type config names in its "TransportOption" property
    /// (TransportOptionConfigProperty)
    /// @return the selected option of the types, with its default port; the first ("plain") when the config
    /// names none
    /// @throw InvalidParameterException the selected option is not supported
    TransportOptionPtr getSelectedTransportOption(const PropertyObjectPtr& config) const;

    /// @brief the options of `serverOptions` this module supports too, in the server's order, with the server's ports
    ListPtr<ITransportOption> supportedTransportOptions(const ListPtr<ITransportOption>& serverOptions) const;

    DevicePtr createDeviceInternal(const StringPtr& connectionString,
                                   const ComponentPtr& parent,
                                   const PropertyObjectPtr& config,
                                   const LoginParametersPtr& loginParameters);
    StreamingPtr createStreamingInternal(const StringPtr& connectionString,
                                         const PropertyObjectPtr& config,
                                         const LoginParametersPtr& loginParameters);

    static bool ConnectionStringHasPrefix(const StringPtr& connectionString, const char* prefix);
    static bool ValidateConnectionString(const StringPtr& connectionString);

    /// @brief recognises the connection string prefix
    /// @return the type, or nothing if the prefix belongs to some other module
    static std::optional<NativeType> ParseConnectionType(const StringPtr& connectionString);

#if NATIVE_STREAMING_ENABLE_TLS
    /// @brief collects the TLS settings the transport handler needs out of the connection config
    /// @return the object to hand to the handler
    static PropertyObjectPtr parseTlsConfig(const PropertyObjectPtr& config);
#endif

    /// @brief the transport options a discovered server lists that this module supports too, in the server's order,
    /// at the server's ports
    /// @return empty if the two have none in common
    ListPtr<ITransportOption> supportedDiscoveredTransportOptions(const discovery::MdnsDiscoveredDevice& discoveredDevice) const;
    DeviceInfoPtr populateDiscoveredConfigurationDevice(const discovery::MdnsDiscoveredDevice& discoveredDevice,
                                                        const ListPtr<ITransportOption>& discoveredOptions);
    DeviceInfoPtr populateDiscoveredStreamingDevice(const discovery::MdnsDiscoveredDevice& discoveredDevice,
                                                    const ListPtr<ITransportOption>& discoveredOptions);
    static void SetupProtocolAddresses(const discovery::MdnsDiscoveredDevice& discoveredDevice,
                                       ServerCapabilityConfigPtr& cap,
                                       std::string protocolPrefix,
                                       const IntegerPtr& port);

    static StringPtr CreateUrlConnectionString(std::string prefix,
                                               const StringPtr& host,
                                               const IntegerPtr& port,
                                               const StringPtr& path);

    std::shared_ptr<boost::asio::io_context> addStreamingProcessingContext(const StringPtr& connectionString);
    opendaq_native_streaming_protocol::NativeStreamingClientHandlerPtr createAndConnectTransportClient(
        const StringPtr& host,
        Int port,
        const StringPtr& path,
        const PropertyObjectPtr& config,
        const StringPtr& transport,
        const LoginParametersPtr& loginParameters);
    static DictPtr<IString, IString> ParseConfigCredentials(const PropertyObjectPtr& config);
    static DictPtr<IString, IString> GetLoginCredentials(const LoginParametersPtr& loginParameters);
    void copyConfigPropertyValue(const StringPtr& propName, const PropertyObjectPtr& srcObject, PropertyObjectPtr& targetObject);

    StreamingPtr createNativeStreaming(const StringPtr& connectionString,
                                       opendaq_native_streaming_protocol::NativeStreamingClientHandlerPtr transportClientHandler,
                                       Int streamingInitTimeout);

    DevicePtr createNativeDevice(const ContextPtr& context,
                                 const ComponentPtr& parent,
                                 const StringPtr& connectionString,
                                 const PropertyObjectPtr& config,
                                 const StringPtr& host,
                                 Int port,
                                 const StringPtr& path,
                                 uint16_t& protocolVersion,
                                 const StringPtr& transport,
                                 const LoginParametersPtr& loginParameters);

    PropertyObjectPtr createConnectionDefaultConfig(NativeType nativeConfigType);
    bool acceptsConnectionParameters(const StringPtr& connectionString, const PropertyObjectPtr& config);
    bool acceptsStreamingConnectionParameters(const StringPtr& connectionString, const PropertyObjectPtr& config);
    void populateDeviceConfigFromContext(PropertyObjectPtr deviceConfig);
    void populateTransportLayerConfigFromContext(PropertyObjectPtr transportLayerConfig);
    PropertyObjectPtr populateDefaultConfig(const PropertyObjectPtr& config, NativeType nativeType);
    void populateDefaultTransportLayerConfig(PropertyObjectPtr& defaultConfig, const PropertyObjectPtr& config);
    PropertyObjectPtr createTransportLayerDefaultConfig();
    bool validateDeviceConfig(const PropertyObjectPtr& config);
    bool validateConnectionConfig(const PropertyObjectPtr& config);
    bool validateTransportLayerConfig(const PropertyObjectPtr& config);

    std::mutex sync;
    size_t pseudoDeviceIndex;
    size_t transportClientIndex;
    std::string transportClientUuidBase;
    discovery::DiscoveryClient discoveryClient;
    // the transport options every native type lists, with the default port of each; plain comes first.
    // Fixed at build time, so it is built once and frozen
    const ListPtr<ITransportOption> transportOptions;

    using ProcessingContext = std::tuple<StringPtr, std::thread, std::shared_ptr<boost::asio::io_context>>;
    std::vector<ProcessingContext> processingContextPool;
};

END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_CLIENT_MODULE
