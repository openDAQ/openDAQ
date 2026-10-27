#include <native_streaming_client_module/native_streaming_client_module_impl.h>
#include <native_streaming_protocol/native_streaming_constants.h>
#include <native_streaming_client_module/version.h>
#include <coretypes/version_info_factory.h>
#include <opendaq/device_type_factory.h>
#include <opendaq/mirrored_device_config_ptr.h>
#include <opendaq/streaming_type_factory.h>
#include <opendaq/add_component_config_ptr.h>
#include <native_streaming_client_module/native_streaming_device_impl.h>
#include <native_streaming_client_module/native_streaming_impl.h>
#include <native_streaming_client_module/native_device_impl.h>
#include <string>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <config_protocol/config_protocol_client.h>
#include <opendaq/address_info_factory.h>
#include <opendaq/client_type.h>
#include <opendaq/thread_name.h>
#include <native_streaming_protocol/async_exception_guard.h>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_CLIENT_MODULE
using namespace discovery;
using namespace opendaq_native_streaming_protocol;
using namespace config_protocol;

namespace
{
    DictPtr<IString, IInteger> CreateAvailableTransports()
    {
        auto transports = Dict<IString, IInteger>();
        transports.set(CONST_NATIVE_TRANSPORT_PLAIN, static_cast<Int>(DEFAULT_PORT));
#if NATIVE_STREAMING_ENABLE_TLS
        transports.set(CONST_NATIVE_TRANSPORT_TLS, static_cast<Int>(DEFAULT_TLS_PORT));
#endif
        transports.freeze();
        return transports;
    }
}

NativeStreamingClientModule::NativeStreamingClientModule(ContextPtr context)
    : Module("OpenDAQNativeStreamingClientModule",
            VersionInfo(NATIVE_STREAM_CL_MODULE_MAJOR_VERSION,
                        NATIVE_STREAM_CL_MODULE_MINOR_VERSION,
                        NATIVE_STREAM_CL_MODULE_PATCH_VERSION),
            std::move(context),
            "OpenDAQNativeStreamingClientModule")
    , pseudoDeviceIndex(0)
    , transportClientIndex(0)
    , discoveryClient(
        {CONST_NATIVE_SERVICE_CAPABILITY}
    )
    , availableTransports(CreateAvailableTransports())
{
    loggerComponent = this->context.getLogger().getOrAddComponent("NativeClient");

    boost::uuids::random_generator gen;
    const auto uuidBoost = gen();
    transportClientUuidBase = boost::uuids::to_string(uuidBoost);

    discoveryClient.initMdnsClient(List<IString>(CONST_NATIVE_SERVICE_NAME));
}

NativeStreamingClientModule::~NativeStreamingClientModule()
{
    for (auto& [description, processingThread, processingIOContextPtr] : processingContextPool)
    {
        if (!processingIOContextPtr->stopped())
            processingIOContextPtr->stop();
        if (processingThread.get_id() != std::this_thread::get_id())
        {
            if (processingThread.joinable())
            {
                processingThread.join();
                LOG_I("{} thread joined", description);
            }
            else
            {
                LOG_W("{} thread is not joinable", description);
            }
        }
        else
        {
            LOG_C("{} thread cannot join itself", description);
        }
    }
}

void NativeStreamingClientModule::SetupProtocolAddresses(const MdnsDiscoveredDevice& discoveredDevice,
                                                         ServerCapabilityConfigPtr& cap,
                                                         std::string protocolPrefix,
                                                         const IntegerPtr& port)
{
    // The connection strings use the port of the capability's default transport, the first one this module
    // supports, rather than discoveredDevice.servicePort. The service port is the port of the server's default
    // transport; the two differ only when that transport is one this module does not support, and then the
    // service port is one the client cannot connect to.
    for (const auto& ipAddress : discoveredDevice.ipv4Addresses)
    {
        auto connectionStringIpv4 = CreateUrlConnectionString(
            protocolPrefix,
            ipAddress,
            port,
            discoveredDevice.getPropertyOrDefault("path", "/")
            );
        cap.addConnectionString(connectionStringIpv4);
        cap.addAddress(ipAddress);

        const auto addressInfo = AddressInfoBuilder().setAddress(ipAddress)
                                                     .setReachabilityStatus(AddressReachabilityStatus::Unknown)
                                                     .setType("IPv4")
                                                     .setConnectionString(connectionStringIpv4)
                                                     .build();
        cap.addAddressInfo(addressInfo);
    }
    
    for (const auto& ipAddress : discoveredDevice.ipv6Addresses)
    {
        auto connectionStringIpv6 = CreateUrlConnectionString(
            protocolPrefix,
            ipAddress,
            port,
            discoveredDevice.getPropertyOrDefault("path", "/")
            );
        cap.addConnectionString(connectionStringIpv6);
        cap.addAddress(ipAddress);

        const auto addressInfo = AddressInfoBuilder().setAddress(ipAddress)
                                                     .setReachabilityStatus(AddressReachabilityStatus::Unknown)
                                                     .setType("IPv6")
                                                     .setConnectionString(connectionStringIpv6)
                                                     .build();
        cap.addAddressInfo(addressInfo);
    }

    cap.setConnectionType("TCP/IP");
    cap.setPrefix(protocolPrefix);
}

ListPtr<IDeviceInfo> NativeStreamingClientModule::onGetAvailableDevices()
{
    auto availableDevices = List<IDeviceInfo>();
    for (const auto& device : discoveryClient.discoverMdnsDevices())
    {
        // a server this module shares no transport with cannot be connected to
        const auto transports = SupportedDiscoveredTransports(device);
        if (transports.getCount() == 0)
            continue;

        availableDevices.pushBack(populateDiscoveredConfigurationDevice(device, transports));
        availableDevices.pushBack(populateDiscoveredStreamingDevice(device, transports));
    }
    return availableDevices;
}

DictPtr<IString, IDeviceType> NativeStreamingClientModule::onGetAvailableDeviceTypes()
{
    auto result = Dict<IString, IDeviceType>();

    auto pseudoDeviceType = createPseudoDeviceType();
    result.set(pseudoDeviceType.getId(), pseudoDeviceType);

    auto deviceType = createDeviceType();
    result.set(deviceType.getId(), deviceType);

    return result;
}

DictPtr<IString, IStreamingType> NativeStreamingClientModule::onGetAvailableStreamingTypes()
{
    auto result = Dict<IString, IStreamingType>();

    auto streamingType = createStreamingType();
    result.set(streamingType.getId(), streamingType);

    return result;
}

DevicePtr NativeStreamingClientModule::createNativeDevice(const ContextPtr& context,
                                                          const ComponentPtr& parent,
                                                          const StringPtr& connectionString,
                                                          const PropertyObjectPtr& deviceConfig,
                                                          const StringPtr& host,
                                                          const StringPtr& port,
                                                          const StringPtr& path,
                                                          uint16_t& protocolVersion,
                                                          bool secure)
{
    auto transportClient = createAndConnectTransportClient(host, port, path, deviceConfig, secure);

    auto processingIOContextPtr = std::make_shared<boost::asio::io_context>();
    auto processingThread = std::thread(
        [this, processingIOContextPtr]()
        {
            daqNameThread("NatCliDevCfgProc");
            using namespace boost::asio;
            auto workGuard = make_work_guard(*processingIOContextPtr);
            runGuardedEventLoop(*processingIOContextPtr, loggerComponent, "Native device config processing thread");
            LOG_I("Native device config processing thread finished");
        }
    );

    auto reconnectionProcessingIOContextPtr = std::make_shared<boost::asio::io_context>();
    auto reconnectionProcessingThread = std::thread(
        [this, reconnectionProcessingIOContextPtr]()
        {
            daqNameThread("NatCliDevReconnProc");
            using namespace boost::asio;
            auto workGuard = make_work_guard(*reconnectionProcessingIOContextPtr);
            runGuardedEventLoop(*reconnectionProcessingIOContextPtr, loggerComponent, "Native device reconnection processing thread");
            LOG_I("Native device reconnection processing thread finished");
        }
    );

    try
    {
        PropertyObjectPtr transportLayerConfig = deviceConfig.getPropertyValue("TransportLayerConfig");
        Int reconnectionPeriod = transportLayerConfig.getPropertyValue("ReconnectionPeriod");

        auto deviceHelper = std::make_shared<NativeDeviceHelper>(context,
                                                                 transportClient,
                                                                 deviceConfig.getPropertyValue("ConfigProtocolRequestTimeout"),
                                                                 deviceConfig.getPropertyValue("RestoreClientConfigOnReconnect"),
                                                                 processingIOContextPtr,
                                                                 reconnectionProcessingIOContextPtr,
                                                                 reconnectionProcessingThread.get_id(),
                                                                 connectionString,
                                                                 std::stoi(port.toStdString()),
                                                                 reconnectionPeriod);
        deviceHelper->setupProtocolClients(context);
        auto device = deviceHelper->connectAndGetDevice(parent, protocolVersion);

        deviceHelper->subscribeToCoreEvent(context);

        device.asPtr<INativeDevicePrivate>(true)->completeInitialization(std::move(deviceHelper), connectionString);

        processingContextPool.emplace_back("Device " + device.getGlobalId() + " config protocol processing",
                                                        std::move(processingThread),
                                                        processingIOContextPtr);
        processingContextPool.emplace_back("Device " + device.getGlobalId() + " reconnection processing",
                                                        std::move(reconnectionProcessingThread),
                                                        reconnectionProcessingIOContextPtr);

        return device;
    }
    catch (...)
    {
        processingIOContextPtr->stop();
        processingThread.join();

        reconnectionProcessingIOContextPtr->stop();
        reconnectionProcessingThread.join();

        throw;
    }
}

void NativeStreamingClientModule::populateDeviceConfigFromContext(PropertyObjectPtr deviceConfig)
{
    auto options = context.getModuleOptions(moduleInfo.getId());
    if (options.getCount() == 0)
        return;

    {
        auto value = options.getOrDefault("ProtocolVersion");
        if (value.assigned() && value.getCoreType() == CoreType::ctInt)
            deviceConfig.setPropertyValue("ProtocolVersion", value);
    }

    {
        auto value = options.getOrDefault("ConfigProtocolRequestTimeout");
        if (value.assigned() && value.getCoreType() == CoreType::ctInt)
            deviceConfig.setPropertyValue("ConfigProtocolRequestTimeout", value);
    }

    {
        auto value = options.getOrDefault("RestoreClientConfigOnReconnect");
        if (value.assigned() && value.getCoreType() == CoreType::ctBool)
            deviceConfig.setPropertyValue("RestoreClientConfigOnReconnect", value);
    }
}

#if NATIVE_STREAMING_ENABLE_TLS

PropertyObjectPtr NativeStreamingClientModule::parseTlsConfig(const PropertyObjectPtr& config, bool secure)
{
    if (!secure)
        return nullptr;

#ifdef NATIVE_STREAMING_TLS_WITHOUT_SIGN
    return PropertyObject();
#else
    // values only: the visibility conditions of the originals refer to their siblings, which would
    // not survive being lifted into an object of their own
    auto tlsConfig = PropertyObject();

    const auto copyBool = [&config, &tlsConfig](const char* name, bool defaultValue)
    {
        const bool value = config.assigned() && config.hasProperty(name)
                               ? static_cast<bool>(config.getPropertyValue(name))
                               : defaultValue;
        tlsConfig.addProperty(BoolProperty(name, value));
    };

    const auto copyPath = [&config, &tlsConfig](const char* name)
    {
        const StringPtr value = config.assigned() && config.hasProperty(name)
                                    ? StringPtr(config.getPropertyValue(name))
                                    : String("");
        tlsConfig.addProperty(StringProperty(name, value));
    };

    copyBool(PROPERTY_VERIFY_SERVER_CERT_CLIENT, DEFAULT_VERIFY_SERVER_CERT);
    copyBool(PROPERTY_ENABLE_MTLS_CLIENT, DEFAULT_ENABLE_MTLS_CLIENT);
    copyPath(PROPERTY_CA_CERT_FILE_PATH_CLIENT);
    copyPath(PROPERTY_CERT_FILE_PATH_CLIENT);
    copyPath(PROPERTY_KEY_FILE_PATH_CLIENT);

    return tlsConfig;
#endif
}

#endif

void NativeStreamingClientModule::populateTransportLayerConfigFromContext(PropertyObjectPtr transportLayerConfig)
{
    auto options = context.getModuleOptions(moduleInfo.getId());
    if (options.getCount() == 0)
        return;

    {
        auto value = options.getOrDefault("MonitoringEnabled");
        if (value.assigned() && value.getCoreType() == CoreType::ctBool)
            transportLayerConfig.setPropertyValue("MonitoringEnabled", value);
    }

    {
        auto value = options.getOrDefault("HeartbeatPeriod");
        if (value.assigned() && value.getCoreType() == CoreType::ctInt)
            transportLayerConfig.setPropertyValue("HeartbeatPeriod", value);
    }

    {
        auto value = options.getOrDefault("InactivityTimeout");
        if (value.assigned() && value.getCoreType() == CoreType::ctInt)
            transportLayerConfig.setPropertyValue("InactivityTimeout", value);
    }

    {
        auto value = options.getOrDefault("ConnectionTimeout");
        if (value.assigned() && value.getCoreType() == CoreType::ctInt)
            transportLayerConfig.setPropertyValue("ConnectionTimeout", value);
    }

    {
        auto value = options.getOrDefault("StreamingInitTimeout");
        if (value.assigned() && value.getCoreType() == CoreType::ctInt)
            transportLayerConfig.setPropertyValue("StreamingInitTimeout", value);
    }

    {
        auto value = options.getOrDefault("ReconnectionPeriod");
        if (value.assigned() && value.getCoreType() == CoreType::ctInt)
            transportLayerConfig.setPropertyValue("ReconnectionPeriod", value);
    }
}

PropertyObjectPtr NativeStreamingClientModule::populateDefaultConfig(const PropertyObjectPtr& config, NativeType nativeType)
{
    auto defConfig = createConnectionDefaultConfig(nativeType);
    for (const auto& prop : defConfig.getAllProperties())
    {
        const auto name = prop.getName();
        if (config.hasProperty(name))
        {
            if (name == "TransportLayerConfig")
            {
                const PropertyObjectPtr transportLayerConfig = config.getPropertyValue(name);
                PropertyObjectPtr defTransportLayerConfig = defConfig.getPropertyValue(name);
                populateDefaultTransportLayerConfig(defTransportLayerConfig, transportLayerConfig);
            }
            else
            {
                defConfig.setPropertyValue(name, config.getPropertyValue(name));
            }
        }
    }

    return defConfig;
}

void NativeStreamingClientModule::populateDefaultTransportLayerConfig(PropertyObjectPtr& defaultConfig,
                                                                      const PropertyObjectPtr& config)
{
    for (const auto& prop : defaultConfig.getAllProperties())
    {
        const auto propName = prop.getName();
        if (config.hasProperty(propName))
            defaultConfig.setPropertyValue(propName, config.getPropertyValue(propName));
    }
}

DevicePtr NativeStreamingClientModule::onCreateDevice(const StringPtr& connectionString,
                                                      const ComponentPtr& parent,
                                                      const PropertyObjectPtr& config)
{
    if (!connectionString.assigned())
        DAQ_THROW_EXCEPTION(ArgumentNullException);

    const auto parsedType = ParseConnectionType(connectionString);
    if (!parsedType.has_value())
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Invalid connection string prefix");

    const NativeType nativeType = *parsedType;
    const bool secure = IsTlsSelected(config, "AddDeviceConfig");

    PropertyObjectPtr deviceConfig;
    if (!config.assigned())
        deviceConfig = createConnectionDefaultConfig(nativeType);
    else
        deviceConfig = populateDefaultConfig(config, nativeType);

    if (!acceptsConnectionParameters(connectionString, deviceConfig))
        DAQ_THROW_EXCEPTION(InvalidParameterException);

    if (!context.assigned())
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Context is not available.");

    auto host = ConnectionStringUtils::GetHost(connectionString);
    auto port = ConnectionStringUtils::GetPort(connectionString, deviceConfig, secure ? DEFAULT_TLS_PORT : DEFAULT_PORT);
    auto path = ConnectionStringUtils::GetPath(connectionString);

    DevicePtr device;
    if (nativeType == NativeType::streaming)
    {
        std::string localId;
        {
            std::scoped_lock lock(sync);
            localId = fmt::format("streaming_pseudo_device{}", pseudoDeviceIndex++);
        }

        auto transportClient = createAndConnectTransportClient(host, port, path, deviceConfig, secure);

        PropertyObjectPtr transportLayerConfig = deviceConfig.getPropertyValue("TransportLayerConfig");
        Int initTimeout = transportLayerConfig.getPropertyValue("StreamingInitTimeout");

        auto deviceType = createPseudoDeviceType();
        checkErrorInfo(deviceType.asPtr<IComponentTypePrivate>()->setModuleInfo(moduleInfo));
        device = createWithImplementation<IDevice, NativeStreamingDeviceImpl>(
            context,
            parent,
            localId,
            connectionString,
            transportClient,
            addStreamingProcessingContext(connectionString),
            initTimeout,
            deviceType
            );

        // Set the connection info for the device
        ServerCapabilityConfigPtr connectionInfo = device.getInfo().getConfigurationConnectionInfo();

        const auto addressInfo = AddressInfoBuilder().setAddress(host)
                                     .setReachabilityStatus(AddressReachabilityStatus::Reachable)
                                     .setType(ConnectionStringUtils::GetHostType(connectionString))
                                     .setConnectionString(connectionString)
                                     .build();

        // only the transport the device is connected over
        auto transports = Dict<IString, IInteger>();
        transports.set(secure ? CONST_NATIVE_TRANSPORT_TLS : CONST_NATIVE_TRANSPORT_PLAIN, std::stoi(port.toStdString()));

        connectionInfo.setProtocolId(CONST_NATIVE_STREAMING_ID)
            .setProtocolName(CONST_NATIVE_STREAMING_ID)
            .setProtocolType(ProtocolType::Streaming)
            .setConnectionType("TCP/IP")
            .addAddress(host)
            .setAvailableTransports(transports)
            .setPrefix(CONST_NATIVE_STREAMING_PREFIX)
            .setConnectionString(connectionString)
            .addAddressInfo(addressInfo)
            .freeze();
    }
    else if (nativeType == NativeType::config)
    {
        uint16_t protocolVersion = deviceConfig.getPropertyValue("ProtocolVersion");
        device = createNativeDevice(context, parent, connectionString, deviceConfig, host, port, path, protocolVersion, secure);

        auto deviceType = createDeviceType();
        checkErrorInfo(deviceType.asPtr<IComponentTypePrivate>()->setModuleInfo(moduleInfo));
        device.asPtr<IMirroredDeviceConfig>().setMirroredDeviceType(deviceType);
    }

    device.asPtr<IDevicePrivate>(true).setAsRoot();

    return device;
}

PropertyObjectPtr NativeStreamingClientModule::createTransportLayerDefaultConfig()
{
    auto transportLayerConfig = daq::PropertyObject();

    transportLayerConfig.addProperty(daq::BoolProperty("MonitoringEnabled", daq::True));
    transportLayerConfig.addProperty(daq::IntProperty("HeartbeatPeriod", 1000));
    transportLayerConfig.addProperty(daq::IntProperty("InactivityTimeout", 1500));
    transportLayerConfig.addProperty(daq::IntProperty("ConnectionTimeout", 1000));
    transportLayerConfig.addProperty(daq::IntProperty("StreamingInitTimeout", 1000));
    transportLayerConfig.addProperty(daq::IntProperty("ReconnectionPeriod", 1000));

    daq::ClientTypeTools::DefineConfigProperties(transportLayerConfig);

    populateTransportLayerConfigFromContext(transportLayerConfig);

    return transportLayerConfig;
}

PropertyObjectPtr NativeStreamingClientModule::createConnectionDefaultConfig(NativeType nativeConfigType)
{
    auto defaultConfig = PropertyObject();

    defaultConfig.addProperty(ObjectProperty("TransportLayerConfig", createTransportLayerDefaultConfig()));
    // left at its default, the port of the selected transport is used instead
    defaultConfig.addProperty(IntProperty("Port", DEFAULT_PORT));
    defaultConfig.addProperty(StringProperty("Username", ""));
    defaultConfig.addProperty(StringProperty("Password", ""));

    daq::ClientTypeTools::DefineConfigProperties(defaultConfig);

#if NATIVE_STREAMING_ENABLE_TLS && !defined(NATIVE_STREAMING_TLS_WITHOUT_SIGN)
    // used only when the TLS transport is selected
    {
        const std::string verifyingServer = std::string("$") + PROPERTY_VERIFY_SERVER_CERT_CLIENT + " == 1";
        const std::string mutualTls = std::string("$") + PROPERTY_ENABLE_MTLS_CLIENT + " == 1";

        const auto verifyProp =
            BoolPropertyBuilder(PROPERTY_VERIFY_SERVER_CERT_CLIENT, DEFAULT_VERIFY_SERVER_CERT)
                .setDescription("Verifies the server certificate against the configured certificate authority. "
                                "Turning it off leaves the connection encrypted but unauthenticated, which is no "
                                "protection against an active man-in-the-middle.")
                .build();
        defaultConfig.addProperty(verifyProp);

        const auto caProp = StringPropertyBuilder(PROPERTY_CA_CERT_FILE_PATH_CLIENT, DEFAULT_CA_CERT_FILE_PATH)
                                .setDescription("Path to the trusted CA certificates in PEM format, used to verify the server.")
                                .setVisible(EvalValue(verifyingServer))
                                .build();
        defaultConfig.addProperty(caProp);

        const auto mutualTlsProp = BoolPropertyBuilder(PROPERTY_ENABLE_MTLS_CLIENT, DEFAULT_ENABLE_MTLS_CLIENT)
                                       .setDescription("Presents a client certificate to servers which ask for one.")
                                       .build();
        defaultConfig.addProperty(mutualTlsProp);

        const auto certProp = StringPropertyBuilder(PROPERTY_CERT_FILE_PATH_CLIENT, DEFAULT_CERT_FILE_PATH)
                                  .setDescription("Path to the client certificate chain in PEM format.")
                                  .setVisible(EvalValue(mutualTls))
                                  .build();
        defaultConfig.addProperty(certProp);

        const auto keyProp = StringPropertyBuilder(PROPERTY_KEY_FILE_PATH_CLIENT, DEFAULT_KEY_FILE_PATH)
                                 .setDescription("Path to the client private key in PEM format.")
                                 .setVisible(EvalValue(mutualTls))
                                 .build();
        defaultConfig.addProperty(keyProp);
    }
#endif

    if (nativeConfigType == NativeType::config)
    {
        defaultConfig.addProperty(IntProperty("ProtocolVersion", GetLatestConfigProtocolVersion()));
        defaultConfig.addProperty(IntProperty("ConfigProtocolRequestTimeout", 10000));
        defaultConfig.addProperty(BoolProperty("RestoreClientConfigOnReconnect", False));

        populateDeviceConfigFromContext(defaultConfig);
    }

    return defaultConfig;
}

bool NativeStreamingClientModule::acceptsConnectionParameters(const StringPtr& connectionString,
                                                              const PropertyObjectPtr& config)
{
    const auto parsedType = ParseConnectionType(connectionString);
    if (!parsedType.has_value() || !ValidateConnectionString(connectionString))
    {
        return false;
    }

    const bool pseudoDevicePrefixFound = *parsedType == NativeType::streaming;
    const bool devicePrefixFound = *parsedType == NativeType::config;

    if (config.assigned() &&
        ((pseudoDevicePrefixFound && !validateConnectionConfig(config)) || (devicePrefixFound && !validateDeviceConfig(config))))
    {
        LOG_W("Connection string \"{}\" is accepted but config is incomplete", connectionString);
        return false;
    }
    else
    {
        return true;
    }
}

bool NativeStreamingClientModule::acceptsStreamingConnectionParameters(const StringPtr& connectionString,
                                                                       const PropertyObjectPtr& /*config*/)
{
    if (connectionString.assigned() && connectionString != "")
    {
        const auto parsedType = ParseConnectionType(connectionString);
        return parsedType == NativeType::streaming && ValidateConnectionString(connectionString);
    }
    return false;
}

std::shared_ptr<boost::asio::io_context> NativeStreamingClientModule::addStreamingProcessingContext(const StringPtr& connectionString)
{
    auto processingIOContextPtr = std::make_shared<boost::asio::io_context>();
    auto processingThread = std::thread(
        [this, processingIOContextPtr, connectionString]()
        {
            daqNameThread("NatCliStreamProc");
            using namespace boost::asio;
            auto workGuard = make_work_guard(*processingIOContextPtr);
            runGuardedEventLoop(*processingIOContextPtr, loggerComponent, "Streaming " + connectionString.toStdString() + " processing thread");
            LOG_I("Streaming {}: processing thread finished", connectionString);
        }
    );
    processingContextPool.emplace_back("Streaming " + connectionString + " processing",
                                       std::move(processingThread),
                                       processingIOContextPtr);

    return processingIOContextPtr;
}

NativeStreamingClientHandlerPtr NativeStreamingClientModule::createAndConnectTransportClient(
    const StringPtr& host,
    const StringPtr& port,
    const StringPtr& path,
    const PropertyObjectPtr& config,
    bool secure)
{
    PropertyObjectPtr transportLayerConfig = config.getPropertyValue("TransportLayerConfig");
    PropertyObjectPtr authenticationConfig = parseAuthenticationConfig(config);

    copyConfigPropertyValue("ClientType", config, transportLayerConfig);
    copyConfigPropertyValue("ExclusiveControlDropOthers", config, transportLayerConfig);

    StringPtr modifiedHost = host;
    if (modifiedHost.assigned() && modifiedHost.getLength() > 1 && modifiedHost.getCharPtr()[0] == '[' && modifiedHost.getCharPtr()[modifiedHost.getLength() - 1] == ']')
    {
        modifiedHost = modifiedHost.toStdString().substr(1, modifiedHost.getLength() - 2);
    }

    {
        std::scoped_lock lock(sync);
        transportLayerConfig.addProperty(StringProperty("ClientId", fmt::format("{}/{}", transportClientUuidBase, transportClientIndex++)));
    }

#if NATIVE_STREAMING_ENABLE_TLS
    auto transportClientHandler =
        secure ? std::make_shared<NativeStreamingClientHandler>(context, transportLayerConfig, authenticationConfig, parseTlsConfig(config, secure))
               : std::make_shared<NativeStreamingClientHandler>(context, transportLayerConfig, authenticationConfig);
#else
    auto transportClientHandler = std::make_shared<NativeStreamingClientHandler>(context, transportLayerConfig, authenticationConfig);
#endif
    if (!transportClientHandler->connect(modifiedHost.toStdString(), port.toStdString(), path.toStdString()))
        DAQ_THROW_EXCEPTION(NotFoundException, "Failed to connect to native streaming server - host {} port {} path {}", modifiedHost, port, path);

    return transportClientHandler;
}

PropertyObjectPtr NativeStreamingClientModule::parseAuthenticationConfig(const PropertyObjectPtr& config)
{
    auto normalizedObject = PropertyObject();

    normalizedObject.addProperty(StringProperty("Username", ""));
    normalizedObject.addProperty(StringProperty("Password", ""));

    if (config.assigned())
    {
        if (config.hasProperty("Username"))
            normalizedObject.setPropertyValue("Username", config.getPropertyValue("Username"));

        if (config.hasProperty("Password"))
            normalizedObject.setPropertyValue("Password", config.getPropertyValue("Password"));
    }

    return normalizedObject;
}

void NativeStreamingClientModule::copyConfigPropertyValue(const StringPtr& propName,
                                                          const PropertyObjectPtr& srcObject,
                                                          PropertyObjectPtr& targetObject)
{
    if (srcObject.hasProperty(propName))
    {
        const auto prop = srcObject.getProperty(propName);
        const auto value = srcObject.getPropertyValue(propName);
        const auto defaultValue = prop.getDefaultValue();

        if (targetObject.hasProperty(propName) && targetObject.getPropertyValue(propName) == defaultValue)
            targetObject.setPropertyValue(propName, value);
    }
}

StreamingPtr NativeStreamingClientModule::createNativeStreaming(const StringPtr& connectionString,
                                                                NativeStreamingClientHandlerPtr transportClientHandler,
                                                                Int streamingInitTimeout)
{
    StreamingPtr nativeStreaming;
    if (transportClientHandler->supportsToDeviceStreaming())
        nativeStreaming = createWithImplementation<IStreaming, NativeStreamingToDeviceImpl>(
            connectionString,
            context,
            transportClientHandler,
            addStreamingProcessingContext(connectionString),
            streamingInitTimeout
        );
    else
        nativeStreaming = createWithImplementation<IStreaming, NativeStreamingBasicImpl>(
            connectionString,
            context,
            transportClientHandler,
            addStreamingProcessingContext(connectionString),
            streamingInitTimeout,
            nullptr,
            nullptr,
            nullptr,
            false
        );
    nativeStreaming.asPtr<INativeStreamingPrivate>()->upgradeToSafeProcessingCallbacks();
    return nativeStreaming;
}

StreamingPtr NativeStreamingClientModule::onCreateStreaming(const StringPtr& connectionString, const PropertyObjectPtr& config)
{
    if (!acceptsStreamingConnectionParameters(connectionString, config))
        DAQ_THROW_EXCEPTION(InvalidParameterException);

    const bool secure = IsTlsSelected(config, "AddStreamingConfig");

    PropertyObjectPtr parsedConfig = config.assigned() ? populateDefaultConfig(config, NativeType::streaming)
                                                       : createConnectionDefaultConfig(NativeType::streaming);

    StringPtr host = ConnectionStringUtils::GetHost(connectionString);
    StringPtr port = ConnectionStringUtils::GetPort(connectionString, parsedConfig, secure ? DEFAULT_TLS_PORT : DEFAULT_PORT);
    StringPtr path = ConnectionStringUtils::GetPath(connectionString);

    PropertyObjectPtr transportLayerConfig = parsedConfig.getPropertyValue("TransportLayerConfig");
    Int initTimeout = transportLayerConfig.getPropertyValue("StreamingInitTimeout");

    auto transportClient = createAndConnectTransportClient(host, port, path, parsedConfig, secure);
    return createNativeStreaming(connectionString, transportClient, initTimeout);
}

Bool NativeStreamingClientModule::onCompleteServerCapability(const ServerCapabilityPtr& source, const ServerCapabilityConfigPtr& target)
{
    const auto targetProtocolId = target.getProtocolId();
    if (targetProtocolId != CONST_NATIVE_STREAMING_ID && targetProtocolId != CONST_NATIVE_CONFIG_ID)
        return false;

    if (source.getConnectionType() != "TCP/IP")
        return false;

    // only the transports both the server and this module support
    if (const auto serverTransports = target.getAvailableTransports(); serverTransports.getCount() > 0)
    {
        auto transports = Dict<IString, IInteger>();
        for (const auto& [name, transportPort] : serverTransports)
        {
            if (availableTransports.hasKey(name))
                transports.set(name, transportPort);
        }

        if (transports.getCount() == 0)
        {
            LOG_W("Native server capability lists no transport this client supports.")
            return false;
        }

        if (transports.getCount() != serverTransports.getCount())
            target.setAvailableTransports(transports);
    }

    if (!source.getAddresses().assigned() || !source.getAddresses().getCount())
    {
        LOG_W("Source server capability address is not available when filling in missing Native capability information.")
        return false;
    }

    const auto addrInfos = source.getAddressInfo();
    if (!addrInfos.assigned() || !addrInfos.getCount())
    {
        LOG_W("Source server capability addressInfo is not available when filling in missing Native capability information.")
        return false;
    }

    // the port of the default transport
    auto port = target.getPort();
    if (port == -1)
    {
        port = DEFAULT_PORT;
        target.setPort(port);
        LOG_W("Native server capability is missing port. Defaulting to {}.", port)
    }

    // a server which does not list its transports serves plain on the capability port
    if (target.getAvailableTransports().getCount() == 0)
    {
        auto transports = Dict<IString, IInteger>();
        transports.set(CONST_NATIVE_TRANSPORT_PLAIN, port);
        target.setAvailableTransports(transports);
    }

    const auto path = target.hasProperty("Path") ? target.getPropertyValue("Path") : "";
    const auto targetAddress = target.getAddresses();
    for (const auto& addrInfo : addrInfos)
    {
        const auto address = addrInfo.getAddress();
        if (auto it = std::find(targetAddress.begin(), targetAddress.end(), address); it != targetAddress.end())
            continue;

        const auto prefix = target.getPrefix();
        StringPtr connectionString;
        if (source.getPrefix() == prefix)
            connectionString = addrInfo.getConnectionString();
        else
            connectionString = CreateUrlConnectionString(prefix, address, port, path);

        const auto targetAddrInfo = AddressInfoBuilder()
                                        .setAddress(address)
                                        .setReachabilityStatus(addrInfo.getReachabilityStatus())
                                        .setType(addrInfo.getType())
                                        .setConnectionString(connectionString)
                                        .build();

        target.addAddressInfo(targetAddrInfo)
              .setConnectionString(connectionString)
              .addAddress(address);
    }

    return true;
}

bool NativeStreamingClientModule::ConnectionStringHasPrefix(const StringPtr& connectionString,
                                                            const char* prefix)
{
    std::string connStr = connectionString;
    auto found = connStr.find(std::string(prefix) + "://");
    return (found == 0);
}

StringPtr NativeStreamingClientModule::CreateUrlConnectionString(std::string prefix,
                                                                 const StringPtr& host,
                                                                 const IntegerPtr& port,
                                                                 const StringPtr& path)
{
    return String(fmt::format("{}://{}:{}{}", prefix, host, port, path));
}

DeviceTypePtr NativeStreamingClientModule::createPseudoDeviceType()
{
    return DeviceTypeBuilder()
        .setId(CONST_NATIVE_STREAMING_ID)
        .setName("PseudoDevice")
        .setDescription("Pseudo device, provides only signals of the remote device as flat list")
        .setConnectionStringPrefix(CONST_NATIVE_STREAMING_PREFIX)
        .setDefaultConfig(NativeStreamingClientModule::createConnectionDefaultConfig(NativeType::streaming))
        .setAvailableTransports(availableTransports)
        .build();
}

DeviceTypePtr NativeStreamingClientModule::createDeviceType()
{
    return DeviceTypeBuilder()
        .setId(CONST_NATIVE_CONFIG_ID)
        .setName("Device")
        .setDescription("Network device connected over Native configuration protocol")
        .setConnectionStringPrefix(CONST_NATIVE_CONFIG_PREFIX)
        .setDefaultConfig(NativeStreamingClientModule::createConnectionDefaultConfig(NativeType::config))
        .setAvailableTransports(availableTransports)
        .build();
}

StreamingTypePtr NativeStreamingClientModule::createStreamingType()
{
    return StreamingTypeBuilder()
        .setId(CONST_NATIVE_STREAMING_ID)
        .setName("NativeStreaming")
        .setDescription("openDAQ native streaming protocol client")
        .setConnectionStringPrefix(CONST_NATIVE_STREAMING_PREFIX)
        .setDefaultConfig(NativeStreamingClientModule::createConnectionDefaultConfig(NativeType::streaming))
        .setAvailableTransports(availableTransports)
        .build();
}

bool NativeStreamingClientModule::IsTlsSelected(const PropertyObjectPtr& config, const char* addConfigName)
{
    if (!config.assigned() || !config.hasProperty(addConfigName))
        return false;

    const auto addConfig = config.getPropertyValue(addConfigName).asPtrOrNull<IAddComponentConfig>();
    if (!addConfig.assigned())
        return false;

    const StringPtr transport = addConfig.getTransport();
    if (!transport.assigned() || transport == CONST_NATIVE_TRANSPORT_PLAIN)
        return false;

#if NATIVE_STREAMING_ENABLE_TLS
    if (transport == CONST_NATIVE_TRANSPORT_TLS)
        return true;
#endif

    DAQ_THROW_EXCEPTION(InvalidParameterException, "Transport \"{}\" is not supported by the native streaming client", transport);
}

std::optional<NativeType> NativeStreamingClientModule::ParseConnectionType(const StringPtr& connectionString)
{
    if (ConnectionStringHasPrefix(connectionString, CONST_NATIVE_STREAMING_PREFIX))
        return NativeType::streaming;
    if (ConnectionStringHasPrefix(connectionString, CONST_NATIVE_CONFIG_PREFIX))
        return NativeType::config;

    return std::nullopt;
}

bool NativeStreamingClientModule::validateTransportLayerConfig(const PropertyObjectPtr& config)
{
    return config.hasProperty("MonitoringEnabled") &&
           config.hasProperty("HeartbeatPeriod") &&
           config.hasProperty("InactivityTimeout") &&
           config.hasProperty("ConnectionTimeout") &&
           config.hasProperty("StreamingInitTimeout") &&
           config.hasProperty("ReconnectionPeriod") &&
           config.getProperty("MonitoringEnabled").getValueType() == ctBool &&
           config.getProperty("HeartbeatPeriod").getValueType() == ctInt &&
           config.getProperty("InactivityTimeout").getValueType() == ctInt &&
           config.getProperty("ConnectionTimeout").getValueType() == ctInt &&
           config.getProperty("StreamingInitTimeout").getValueType() == ctInt &&
           config.getProperty("ReconnectionPeriod").getValueType() == ctInt;
}

bool NativeStreamingClientModule::validateDeviceConfig(const PropertyObjectPtr& config)
{
    return config.hasProperty("ConfigProtocolRequestTimeout") &&
           config.getProperty("ConfigProtocolRequestTimeout").getValueType() == ctInt &&
           config.hasProperty("RestoreClientConfigOnReconnect") &&
           config.getProperty("RestoreClientConfigOnReconnect").getValueType() == ctBool &&
           validateConnectionConfig(config);
}

bool NativeStreamingClientModule::validateConnectionConfig(const PropertyObjectPtr& config)
{
    return config.hasProperty("TransportLayerConfig") &&
           config.getPropertyValue("TransportLayerConfig").supportsInterface<IPropertyObject>() &&
           validateTransportLayerConfig(config.getPropertyValue("TransportLayerConfig"));
}

bool NativeStreamingClientModule::ValidateConnectionString(const StringPtr& connectionString)
{
    try
    {
        auto host = ConnectionStringUtils::GetHost(connectionString);
        auto port = ConnectionStringUtils::GetPort(connectionString);
        auto path = ConnectionStringUtils::GetPath(connectionString);
        return true;
    }
    catch(...)
    {
        return false;
    }
}

DictPtr<IString, IInteger> NativeStreamingClientModule::SupportedDiscoveredTransports(const MdnsDiscoveredDevice& discoveredDevice) const
{
    auto serverTransports = discovery_common::DiscoveryUtils::populateTransports(
        discoveredDevice.getPropertyOrDefault(discovery_common::DiscoveryUtils::TRANSPORTS_KEY, ""));

    // a server which does not list its transports serves plain on the advertised port
    if (serverTransports.getCount() == 0 && discoveredDevice.servicePort > 0)
        serverTransports.set(CONST_NATIVE_TRANSPORT_PLAIN, static_cast<Int>(discoveredDevice.servicePort));

    auto transports = Dict<IString, IInteger>();
    for (const auto& [name, port] : serverTransports)
    {
        if (availableTransports.hasKey(name))
            transports.set(name, port);
    }

    return transports;
}

DeviceInfoPtr NativeStreamingClientModule::populateDiscoveredConfigurationDevice(const MdnsDiscoveredDevice& discoveredDevice,
                                                                               const DictPtr<IString, IInteger>& transports)
{
    auto cap = ServerCapability(CONST_NATIVE_CONFIG_ID, CONST_NATIVE_CONFIG_ID, ProtocolType::ConfigurationAndStreaming);

    // the connection strings carry the port of the first (default) transport
    SetupProtocolAddresses(discoveredDevice, cap, CONST_NATIVE_CONFIG_PREFIX, transports.getValueList().getItemAt(0));
    cap.setAvailableTransports(transports);
    cap.setCoreEventsEnabled(true);
    cap.setProtocolVersion(discoveredDevice.getPropertyOrDefault("protocolVersion", ""));
    for (const auto& id : discovery_common::DiscoveryUtils::populateAuthenticationMethods(discoveredDevice.getPropertyOrDefault("authMethods", "")))
        cap.addAuthenticationMethod(id);

    return populateDiscoveredDeviceInfo(
        DiscoveryClient::populateDiscoveredInfoProperties, discoveredDevice, cap, createDeviceType());
}

DeviceInfoPtr NativeStreamingClientModule::populateDiscoveredStreamingDevice(const MdnsDiscoveredDevice& discoveredDevice,
                                                                           const DictPtr<IString, IInteger>& transports)
{
    auto cap = ServerCapability(CONST_NATIVE_STREAMING_ID, CONST_NATIVE_STREAMING_ID, ProtocolType::Streaming);

    // the connection strings carry the port of the first (default) transport
    SetupProtocolAddresses(discoveredDevice, cap, CONST_NATIVE_STREAMING_PREFIX, transports.getValueList().getItemAt(0));
    cap.setAvailableTransports(transports);
    for (const auto& id : discovery_common::DiscoveryUtils::populateAuthenticationMethods(discoveredDevice.getPropertyOrDefault("authMethods", "")))
        cap.addAuthenticationMethod(id);

    return populateDiscoveredDeviceInfo(
        DiscoveryClient::populateDiscoveredInfoProperties, discoveredDevice, cap, createPseudoDeviceType());
}

END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_CLIENT_MODULE
