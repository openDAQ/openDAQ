#include <native_streaming_client_module/native_streaming_client_module_impl.h>
#include <native_streaming_client_module/version.h>
#include <coretypes/version_info_factory.h>
#include <daq_discovery/daq_discovery_client.h>
#include <opendaq/device_type_factory.h>
#include <opendaq/mirrored_device_config_ptr.h>
#include <opendaq/streaming_type_factory.h>
#include <native_streaming_client_module/native_streaming_device_impl.h>
#include <native_streaming_client_module/native_streaming_impl.h>
#include <native_streaming_client_module/native_device_impl.h>
#include <regex>
#include <string>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <config_protocol/config_protocol_client.h>
#include <opendaq/address_info_factory.h>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_CLIENT_MODULE
using namespace discovery;
using namespace opendaq_native_streaming_protocol;
using namespace config_protocol;

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
        {
            [context = this->context](MdnsDiscoveredDevice discoveredDevice)
            {
                auto cap = ServerCapability(NativeStreamingDeviceTypeId, "OpenDAQNativeStreaming", ProtocolType::Streaming);

                SetupProtocolAddresses(discoveredDevice, cap, "daq.ns");
                if (discoveredDevice.servicePort > 0)
                    cap.setPort(discoveredDevice.servicePort);
                return cap;
            },
            [context = this->context](MdnsDiscoveredDevice discoveredDevice)
            {
                auto cap = ServerCapability(NativeConfigurationDeviceTypeId, "OpenDAQNativeConfiguration", ProtocolType::ConfigurationAndStreaming);

                SetupProtocolAddresses(discoveredDevice, cap, "daq.nd");
                cap.setCoreEventsEnabled(true);
                return cap;
            }
        },
        {"OPENDAQ_NS"}
    )
{
    loggerComponent = this->context.getLogger().getOrAddComponent("NativeClient");

    boost::uuids::random_generator gen;
    const auto uuidBoost = gen();
    transportClientUuidBase = boost::uuids::to_string(uuidBoost);

    discoveryClient.initMdnsClient(List<IString>("_opendaq-streaming-native._tcp.local."));
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

void NativeStreamingClientModule::SetupProtocolAddresses(const MdnsDiscoveredDevice& discoveredDevice, ServerCapabilityConfigPtr& cap, std::string protocolPrefix)
{
    if (!discoveredDevice.ipv4Address.empty())
    {
        auto connectionStringIpv4 = NativeStreamingClientModule::CreateUrlConnectionString(
            protocolPrefix,
            discoveredDevice.ipv4Address,
            discoveredDevice.servicePort,
            discoveredDevice.getPropertyOrDefault("path", "/")
            );
        cap.addConnectionString(connectionStringIpv4);
        cap.addAddress(discoveredDevice.ipv4Address);

        const auto addressInfo = AddressInfoBuilder().setAddress(discoveredDevice.ipv4Address)
                                                     .setReachabilityStatus(AddressReachabilityStatus::Unknown)
                                                     .setType("IPv4")
                                                     .setConnectionString(connectionStringIpv4)
                                                     .build();
        cap.addAddressInfo(addressInfo);
    }
    
    if (!discoveredDevice.ipv6Address.empty())
    {
        auto connectionStringIpv6 = NativeStreamingClientModule::CreateUrlConnectionString(
            protocolPrefix,
            discoveredDevice.ipv6Address,
            discoveredDevice.servicePort,
            discoveredDevice.getPropertyOrDefault("path", "/")
        );
        cap.addConnectionString(connectionStringIpv6);
        cap.addAddress(discoveredDevice.ipv6Address);

        const auto addressInfo = AddressInfoBuilder().setAddress(discoveredDevice.ipv6Address)
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
    auto availableDevices = discoveryClient.discoverDevices();
    for (const auto& device : availableDevices)
    {
        if (ConnectionStringHasPrefix(device.getConnectionString(), NativeStreamingDevicePrefix))
            device.asPtr<IDeviceInfoConfig>().setDeviceType(createPseudoDeviceType());
        else if (ConnectionStringHasPrefix(device.getConnectionString(), NativeConfigurationDevicePrefix))
            device.asPtr<IDeviceInfoConfig>().setDeviceType(createDeviceType());
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
                                                          const PropertyObjectPtr& config,
                                                          const StringPtr& host,
                                                          const StringPtr& port,
                                                          const StringPtr& path)
{
    auto transportClient = createAndConnectTransportClient(host, port, path, config);

    auto processingIOContextPtr = std::make_shared<boost::asio::io_context>();
    auto processingThread = std::thread(
        [this, processingIOContextPtr]()
        {
            using namespace boost::asio;
            executor_work_guard<io_context::executor_type> workGuard(processingIOContextPtr->get_executor());
            processingIOContextPtr->run();
            LOG_I("Native device config processing thread finished");
        }
    );
    auto reconnectionProcessingIOContextPtr = std::make_shared<boost::asio::io_context>();
    auto reconnectionProcessingThread = std::thread(
        [this, reconnectionProcessingIOContextPtr]()
        {
            using namespace boost::asio;
            executor_work_guard<io_context::executor_type> workGuard(reconnectionProcessingIOContextPtr->get_executor());
            reconnectionProcessingIOContextPtr->run();
            LOG_I("Native device reconnection processing thread finished");
        }
    );

    try
    {
        auto deviceHelper = std::make_unique<NativeDeviceHelper>(context,
                                                                 transportClient,
                                                                 config.getPropertyValue("ConfigProtocolRequestTimeout"),
                                                                 processingIOContextPtr,
                                                                 reconnectionProcessingIOContextPtr,
                                                                 reconnectionProcessingThread.get_id());
        auto device = deviceHelper->connectAndGetDevice(parent);

        deviceHelper->subscribeToCoreEvent(context);

        device.asPtr<INativeDevicePrivate>(true)->initializeStatus();
        device.asPtr<INativeDevicePrivate>(true)->attachDeviceHelper(std::move(deviceHelper));
        device.asPtr<INativeDevicePrivate>(true)->setConnectionString(connectionString);

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
    auto options = context.getModuleOptions(id);
    if (options.getCount() == 0)
        return;

    if (options.hasKey("ConfigProtocolRequestTimeout"))
    {
        auto value = options.get("ConfigProtocolRequestTimeout");
        if (value.getCoreType() == CoreType::ctInt)
            deviceConfig.setPropertyValue("ConfigProtocolRequestTimeout", value);
    }
}

void NativeStreamingClientModule::populateTransportLayerConfigFromContext(PropertyObjectPtr transportLayerConfig)
{
    auto options = context.getModuleOptions(id);
    if (options.getCount() == 0)
        return;

    if (options.hasKey("MonitoringEnabled"))
    {
        auto value = options.get("MonitoringEnabled");
        if (value.getCoreType() == CoreType::ctBool)
            transportLayerConfig.setPropertyValue("MonitoringEnabled", value);
    }

    if (options.hasKey("HeartbeatPeriod"))
    {
        auto value = options.get("HeartbeatPeriod");
        if (value.getCoreType() == CoreType::ctInt)
            transportLayerConfig.setPropertyValue("HeartbeatPeriod", value);
    }

    if (options.hasKey("InactivityTimeout"))
    {
        auto value = options.get("InactivityTimeout");
        if (value.getCoreType() == CoreType::ctInt)
            transportLayerConfig.setPropertyValue("InactivityTimeout", value);
    }

    if (options.hasKey("ConnectionTimeout"))
    {
        auto value = options.get("ConnectionTimeout");
        if (value.getCoreType() == CoreType::ctInt)
            transportLayerConfig.setPropertyValue("ConnectionTimeout", value);
    }

    if (options.hasKey("StreamingInitTimeout"))
    {
        auto value = options.get("StreamingInitTimeout");
        if (value.getCoreType() == CoreType::ctInt)
            transportLayerConfig.setPropertyValue("StreamingInitTimeout", value);
    }

    if (options.hasKey("ReconnectionPeriod"))
    {
        auto value = options.get("ReconnectionPeriod");
        if (value.getCoreType() == CoreType::ctInt)
            transportLayerConfig.setPropertyValue("ReconnectionPeriod", value);
    }
}

PropertyObjectPtr NativeStreamingClientModule::populateDefaultConfig(const PropertyObjectPtr& config)
{
    auto defConfig = createConnectionDefaultConfig();
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
        throw ArgumentNullException();

    PropertyObjectPtr deviceConfig;
    if (!config.assigned())
        deviceConfig = createConnectionDefaultConfig();
    else
        deviceConfig = populateDefaultConfig(config);

    if (!acceptsConnectionParameters(connectionString, deviceConfig))
        throw InvalidParameterException();

    if (!context.assigned())
        throw InvalidParameterException("Context is not available.");

    auto host = GetHost(connectionString);
    auto port = GetPort(connectionString, deviceConfig);
    auto path = GetPath(connectionString);

    DevicePtr device;
    StringPtr protocolId;
    StringPtr protocolName;
    StringPtr protocolPrefix;
    ProtocolType protocolType = ProtocolType::Unknown;
    if (ConnectionStringHasPrefix(connectionString, NativeStreamingDevicePrefix))
    {
        std::string localId;
        {
            std::scoped_lock lock(sync);
            localId = fmt::format("streaming_pseudo_device{}", pseudoDeviceIndex++);
        }

        auto transportClient = createAndConnectTransportClient(host, port, path, deviceConfig);

        PropertyObjectPtr transportLayerConfig = deviceConfig.getPropertyValue("TransportLayerConfig");
        Int initTimeout = transportLayerConfig.getPropertyValue("StreamingInitTimeout");

        device = createWithImplementation<IDevice, NativeStreamingDeviceImpl>(
            context,
            parent,
            localId,
            connectionString,
            transportClient,
            addStreamingProcessingContext(connectionString),
            initTimeout
        );
        protocolId = NativeStreamingDeviceTypeId;
        protocolName = "OpenDAQNativeStreaming";
        protocolPrefix = "daq.ns";
        protocolType = ProtocolType::Streaming;
    }
    else if (ConnectionStringHasPrefix(connectionString, NativeConfigurationDevicePrefix))
    {
        device = createNativeDevice(context, parent, connectionString, deviceConfig, host, port, path);
        protocolId = NativeConfigurationDeviceTypeId;
        protocolName = "OpenDAQNativeConfiguration";
        protocolPrefix = "daq.nd";
        protocolType = ProtocolType::ConfigurationAndStreaming;
    }
    
    if (!device.assigned())
    {
        throw InvalidParameterException();
    }

    // Set the connection info for the device
    ServerCapabilityConfigPtr connectionInfo = device.getInfo().getConfigurationConnectionInfo();
    
    const auto addressInfo = AddressInfoBuilder().setAddress(host)
                                                 .setReachabilityStatus(AddressReachabilityStatus::Reachable)
                                                 .setType(GetHostType(connectionString))
                                                 .setConnectionString(connectionString)
                                                 .build();

    connectionInfo.setProtocolId(protocolId)
                  .setProtocolName(protocolName)
                  .setProtocolType(protocolType)
                  .setConnectionType("TCP/IP")
                  .addAddress(host)
                  .setPort(std::stoi(port.toStdString()))
                  .setPrefix(protocolPrefix)
                  .setConnectionString(connectionString)
                  .addAddressInfo(addressInfo)
                  .freeze();

    return device;
}

PropertyObjectPtr NativeStreamingClientModule::createTransportLayerDefaultConfig()
{
    auto transportLayerConfig = daq::PropertyObject();

    transportLayerConfig.addProperty(daq::BoolProperty("MonitoringEnabled", daq::False));
    transportLayerConfig.addProperty(daq::IntProperty("HeartbeatPeriod", 1000));
    transportLayerConfig.addProperty(daq::IntProperty("InactivityTimeout", 1500));
    transportLayerConfig.addProperty(daq::IntProperty("ConnectionTimeout", 1000));
    transportLayerConfig.addProperty(daq::IntProperty("StreamingInitTimeout", 1000));
    transportLayerConfig.addProperty(daq::IntProperty("ReconnectionPeriod", 1000));

    populateTransportLayerConfigFromContext(transportLayerConfig);

    return transportLayerConfig;
}

PropertyObjectPtr NativeStreamingClientModule::createConnectionDefaultConfig()
{
    auto defaultConfig = PropertyObject();

    defaultConfig.addProperty(ObjectProperty("TransportLayerConfig", createTransportLayerDefaultConfig()));
    defaultConfig.addProperty(IntProperty("Port", 7420));
    defaultConfig.addProperty(StringProperty("Username", ""));
    defaultConfig.addProperty(StringProperty("Password", ""));

    defaultConfig.addProperty(IntProperty("ConfigProtocolRequestTimeout", 10000));
    populateDeviceConfigFromContext(defaultConfig);

    return defaultConfig;
}

bool NativeStreamingClientModule::acceptsConnectionParameters(const StringPtr& connectionString,
                                                              const PropertyObjectPtr& config)
{
    auto pseudoDevicePrefixFound = ConnectionStringHasPrefix(connectionString, NativeStreamingDevicePrefix);
    auto devicePrefixFound = ConnectionStringHasPrefix(connectionString, NativeConfigurationDevicePrefix);

    if ((!devicePrefixFound && !pseudoDevicePrefixFound) || !ValidateConnectionString(connectionString))
    {
        return false;
    }

    if (config.assigned() &&
        (pseudoDevicePrefixFound && !validateConnectionConfig(config) || devicePrefixFound && !validateDeviceConfig(config)))
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
        return ConnectionStringHasPrefix(connectionString, NativeStreamingPrefix) && ValidateConnectionString(connectionString);
    }
    return false;
}

std::shared_ptr<boost::asio::io_context> NativeStreamingClientModule::addStreamingProcessingContext(const StringPtr& connectionString)
{
    auto processingIOContextPtr = std::make_shared<boost::asio::io_context>();
    auto processingThread = std::thread(
        [this, processingIOContextPtr, connectionString]()
        {
            using namespace boost::asio;
            executor_work_guard<io_context::executor_type> workGuard(processingIOContextPtr->get_executor());
            processingIOContextPtr->run();
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
    const PropertyObjectPtr& config)
{
    const PropertyObjectPtr transportLayerConfig = config.getPropertyValue("TransportLayerConfig");
    const PropertyObjectPtr authenticationConfig = config;  // root config also has a flat lsit of authentication properties

    StringPtr modifiedHost = host;
    if (modifiedHost.assigned() && modifiedHost.getLength() > 1 && modifiedHost.getCharPtr()[0] == '[' && modifiedHost.getCharPtr()[modifiedHost.getLength() - 1] == ']')
    {
        modifiedHost = modifiedHost.toStdString().substr(1, modifiedHost.getLength() - 2);
    }

    {
        std::scoped_lock lock(sync);
        transportLayerConfig.addProperty(StringProperty("ClientId", fmt::format("{}/{}", transportClientUuidBase, transportClientIndex++)));
    }

    auto transportClientHandler = std::make_shared<NativeStreamingClientHandler>(context, transportLayerConfig, authenticationConfig);
    if (!transportClientHandler->connect(modifiedHost.toStdString(), port.toStdString(), path.toStdString()))
    {
        auto message = fmt::format("Failed to connect to native streaming server - host {} port {} path {}", modifiedHost, port, path);
        LOG_E("{}", message);
        throw NotFoundException(message);
    }

    return transportClientHandler;
}

StreamingPtr NativeStreamingClientModule::createNativeStreaming(const StringPtr& connectionString,
                                                                NativeStreamingClientHandlerPtr transportClientHandler,
                                                                Int streamingInitTimeout)
{
    return createWithImplementation<IStreaming, NativeStreamingImpl>(
        connectionString,
        context,
        transportClientHandler,
        addStreamingProcessingContext(connectionString),
        streamingInitTimeout,
        nullptr,
        nullptr,
        nullptr
    );
}

StreamingPtr NativeStreamingClientModule::onCreateStreaming(const StringPtr& connectionString,
                                                            const PropertyObjectPtr& config)
{
    if (!acceptsStreamingConnectionParameters(connectionString, config))
        throw InvalidParameterException();

    PropertyObjectPtr parsedConfig = config.assigned() ? populateDefaultConfig(config) : createConnectionDefaultConfig();

    StringPtr host = GetHost(connectionString);
    StringPtr port = GetPort(connectionString, parsedConfig);
    StringPtr path = GetPath(connectionString);

    PropertyObjectPtr transportLayerConfig = parsedConfig.getPropertyValue("TransportLayerConfig");
    Int initTimeout = transportLayerConfig.getPropertyValue("StreamingInitTimeout");

    auto transportClient = createAndConnectTransportClient(host, port, path, parsedConfig);
    return createNativeStreaming(connectionString, transportClient, initTimeout);
}

Bool NativeStreamingClientModule::onCompleteServerCapability(const ServerCapabilityPtr& source, const ServerCapabilityConfigPtr& target)
{
    if (target.getProtocolId() != "OpenDAQNativeStreaming" &&
        target.getProtocolId() != "OpenDAQNativeConfiguration")
        return false;
    
    if (target.getConnectionString().getLength() != 0)
        return true;

    if (source.getConnectionType() != "TCP/IP")
        return false;

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

    auto port = target.getPort();
    if (port == -1)
    {
        port = 7420;
        target.setPort(port);
        LOG_W("Native server capability is missing port. Defaulting to 7420.")
    }
    
    const auto path = target.hasProperty("Path") ? target.getPropertyValue("Path") : "";
    for (const auto& addrInfo : addrInfos)
    {
        const auto address = addrInfo.getAddress();
        const auto prefix = target.getProtocolId() == "OpenDAQNativeStreaming" ? NativeStreamingPrefix : NativeConfigurationDevicePrefix;
        
        StringPtr connectionString = CreateUrlConnectionString(prefix, address, port,path);
        const auto targetAddrInfo = AddressInfoBuilder()
                                        .setAddress(addrInfo.getAddress())
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
        .setId(NativeStreamingDeviceTypeId)
        .setName("PseudoDevice")
        .setDescription("Pseudo device, provides only signals of the remote device as flat list")
        .setConnectionStringPrefix("daq.ns")
        .setDefaultConfig(NativeStreamingClientModule::createConnectionDefaultConfig())
        .build();
}

DeviceTypePtr NativeStreamingClientModule::createDeviceType()
{
    return DeviceTypeBuilder()
        .setId(NativeConfigurationDeviceTypeId)
        .setName("Device")
        .setDescription("Network device connected over Native configuration protocol")
        .setConnectionStringPrefix("daq.nd")
        .setDefaultConfig(NativeStreamingClientModule::createConnectionDefaultConfig())
        .build();
}

StreamingTypePtr NativeStreamingClientModule::createStreamingType()
{
    return StreamingTypeBuilder()
        .setId(NativeStreamingTypeId)
        .setName("NativeStreaming")
        .setDescription("openDAQ native streaming protocol client")
        .setConnectionStringPrefix("daq.ns")
        .setDefaultConfig(NativeStreamingClientModule::createConnectionDefaultConfig())
        .build();
}

StringPtr NativeStreamingClientModule::GetHostType(const StringPtr& url)
{
	std::string urlString = url.toStdString();

    auto regexIpv6Hostname = std::regex(R"(^(.*://)?(\[[a-fA-F0-9:]+(?:\%[a-zA-Z0-9]+)?\])(?::(\d+))?(/.*)?$)");
    auto regexIpv4Hostname = std::regex(R"(^(.*://)([^:/\s]+))");
	std::smatch match;

	if (std::regex_search(urlString, match, regexIpv6Hostname))
		return String("IPv6");
	if (std::regex_search(urlString, match, regexIpv4Hostname))
		return String("IPv4");
	throw InvalidParameterException("Host type not found in url: {}", url);
}

StringPtr NativeStreamingClientModule::GetHost(const StringPtr& url)
{
    std::string urlString = url.toStdString();

    auto regexIpv6Hostname = std::regex(R"(^(.*://)?(\[[a-fA-F0-9:]+(?:\%[a-zA-Z0-9]+)?\])(?::(\d+))?(/.*)?$)");
    auto regexIpv4Hostname = std::regex(R"(^(.*://)([^:/\s]+))");
    std::smatch match;

    if (std::regex_search(urlString, match, regexIpv6Hostname))
        return String(match[2]);
    if (std::regex_search(urlString, match, regexIpv4Hostname))
        return String(match[2]);
    throw InvalidParameterException("Host name not found in url: {}", url);
}

StringPtr NativeStreamingClientModule::GetPort(const StringPtr& url, const PropertyObjectPtr& config)
{
    std::string outPort;
    std::string urlString = url.toStdString();

    auto regexPort = std::regex(":(\\d+)");
    std::smatch match;

    std::string host = GetHost(url).toStdString();
    std::string suffix = urlString.substr(urlString.find(host) + host.size());

    if (std::regex_search(suffix, match, regexPort))
        outPort = match[1];
    else
        outPort = "7420";

    if (config.assigned())
    {
        std::string ctxPort = config.getPropertyValue("Port");
        if (ctxPort != "7420")
            outPort = ctxPort;
    }

    return outPort;
}

StringPtr NativeStreamingClientModule::GetPath(const StringPtr& url)
{
    std::string urlString = url.toStdString();

    std::string host = GetHost(url).toStdString();
    std::string suffix = urlString.substr(urlString.find(host) + host.size());
    auto pos = suffix.find("/");

    if (pos != std::string::npos)
    {
        return String(suffix.substr(pos));
    }
    else
    {
        return String("/");
    }
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
        auto host = GetHost(connectionString);
        auto port = GetPort(connectionString);
        auto path = GetPath(connectionString);
        return true;
    }
    catch(...)
    {
        return false;
    }
}

END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_CLIENT_MODULE
