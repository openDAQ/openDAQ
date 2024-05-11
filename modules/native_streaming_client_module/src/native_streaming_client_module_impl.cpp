#include <native_streaming_client_module/native_streaming_client_module_impl.h>
#include <native_streaming_client_module/version.h>
#include <coretypes/version_info_factory.h>
#include <daq_discovery/daq_discovery_client.h>
#include <opendaq/device_type_factory.h>

#include <native_streaming_client_module/native_streaming_device_impl.h>
#include <native_streaming_client_module/native_streaming_impl.h>
#include <native_streaming_client_module/native_device_impl.h>

#include <regex>
#include <string>

#include <config_protocol/config_protocol_client.h>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_CLIENT_MODULE

using namespace discovery;
using namespace opendaq_native_streaming_protocol;
using namespace config_protocol;

NativeStreamingClientModule::NativeStreamingClientModule(ContextPtr context)
    : Module("openDAQ native streaming client module",
            VersionInfo(NATIVE_STREAM_CL_MODULE_MAJOR_VERSION,
                        NATIVE_STREAM_CL_MODULE_MINOR_VERSION,
                        NATIVE_STREAM_CL_MODULE_PATCH_VERSION),
            std::move(context),
            "NativeStreamingClient")
    , pseudoDeviceIndex(0)
    , discoveryClient(
        {
            [context = this->context](MdnsDiscoveredDevice discoveredDevice)
            {
                auto connectionStringIpv4 = fmt::format("daq.ns://{}:{}{}",
                                   discoveredDevice.ipv4Address,
                                   discoveredDevice.servicePort,
                                   discoveredDevice.getPropertyOrDefault("path", "/"));
                auto connectionStringIpv6 = fmt::format("daq.ns://[{}]:{}{}",
                                   discoveredDevice.ipv6Address,
                                   discoveredDevice.servicePort,
                                   discoveredDevice.getPropertyOrDefault("path", "/"));
                auto cap = ServerCapability("opendaq_native_streaming", "openDAQ Native Streaming", ProtocolType::Streaming);
                cap.addConnectionString(connectionStringIpv4);
                cap.addAddress(discoveredDevice.ipv4Address);
                cap.addConnectionString(connectionStringIpv6);
                cap.addAddress("[" + discoveredDevice.ipv6Address + "]");
                cap.setConnectionType("TCP/IP");
                cap.setPrefix("daq.ns");
                return cap;
            },
            [context = this->context](MdnsDiscoveredDevice discoveredDevice)
            {
                auto connectionStringIpv4 = fmt::format("daq.nd://{}:{}{}",
                                   discoveredDevice.ipv4Address,
                                   discoveredDevice.servicePort,
                                   discoveredDevice.getPropertyOrDefault("path", "/"));
                auto connectionStringIpv6 = fmt::format("daq.nd://[{}]:{}{}",
                                   discoveredDevice.ipv6Address,
                                   discoveredDevice.servicePort,
                                   discoveredDevice.getPropertyOrDefault("path", "/"));
                auto cap = ServerCapability("opendaq_native_config", "openDAQ Native Configuration", ProtocolType::ConfigurationAndStreaming);
                cap.addConnectionString(connectionStringIpv4);
                cap.addAddress(discoveredDevice.ipv4Address);
                cap.addConnectionString(connectionStringIpv6);
                cap.addAddress("[" + discoveredDevice.ipv6Address + "]");
                cap.setConnectionType("TCP/IP");
                cap.setCoreEventsEnabled(true);
                cap.setPrefix("daq.nd");
                return cap;
            }
        },
        {"OPENDAQ_NS"}
    )
{
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

ListPtr<IDeviceInfo> NativeStreamingClientModule::onGetAvailableDevices()
{
    auto availableDevices = discoveryClient.discoverDevices();
    for (const auto& device : availableDevices)
    {
        if (connectionStringHasPrefix(device.getConnectionString(), NativeStreamingDevicePrefix))
            device.asPtr<IDeviceInfoConfig>().setDeviceType(createPseudoDeviceType());
        else if (connectionStringHasPrefix(device.getConnectionString(), NativeConfigurationDevicePrefix))
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

DevicePtr NativeStreamingClientModule::createNativeDevice(const ContextPtr& context,
                                                          const ComponentPtr& parent,
                                                          const StringPtr& connectionString,
                                                          const PropertyObjectPtr& config,
                                                          const StringPtr& host,
                                                          const StringPtr& port,
                                                          const StringPtr& path)
{
    std::string streamingConnectionString = std::regex_replace(connectionString.toStdString(),
                                                               std::regex(NativeConfigurationDevicePrefix),
                                                               NativeStreamingPrefix);

    PropertyObjectPtr transportLayerConfig = config.getPropertyValue("TransportLayerConfig");
    auto transportClient = createAndConnectTransportClient(host, port, path, transportLayerConfig);

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
    auto deviceHelper = std::make_unique<NativeDeviceHelper>(context,
                                                             transportClient,
                                                             processingIOContextPtr,
                                                             reconnectionProcessingIOContextPtr,
                                                             reconnectionProcessingThread.get_id());
    auto device = deviceHelper->connectAndGetDevice(parent);

    Int initTimeout = transportLayerConfig.getPropertyValue("StreamingInitTimeout");
    StreamingPtr nativeStreaming = createNativeStreaming(
        streamingConnectionString,
        transportClient,
        initTimeout
    );
    nativeStreaming.setActive(true);

    deviceHelper->addStreaming(nativeStreaming);
    // TODO add streaming sources via IMirroredDeviceConfig interface

    deviceHelper->subscribeToCoreEvent(context);

    device.asPtr<INativeDevicePrivate>()->attachDeviceHelper(std::move(deviceHelper));
    device.asPtr<INativeDevicePrivate>()->setConnectionString(connectionString);

    processingContextPool.emplace_back("Device " + device.getGlobalId() + " config protocol processing",
                                                    std::move(processingThread),
                                                    processingIOContextPtr);
    processingContextPool.emplace_back("Device " + device.getGlobalId() + " reconnection processing",
                                                    std::move(reconnectionProcessingThread),
                                                    reconnectionProcessingIOContextPtr);

    return device;
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

DevicePtr NativeStreamingClientModule::onCreateDevice(const StringPtr& connectionString,
                                                      const ComponentPtr& parent,
                                                      const PropertyObjectPtr& config)
{
    if (!connectionString.assigned())
        throw ArgumentNullException();

    PropertyObjectPtr deviceConfig = config;
    if (!deviceConfig.assigned())
        deviceConfig = createDeviceDefaultConfig();

    if (!onAcceptsConnectionParameters(connectionString, deviceConfig))
        throw InvalidParameterException();

    if (!context.assigned())
        throw InvalidParameterException("Context is not available.");

    auto host = getHost(connectionString);
    auto port = getPort(connectionString);
    auto path = getPath(connectionString);

    if (connectionStringHasPrefix(connectionString, NativeStreamingDevicePrefix))
    {
        std::scoped_lock lock(sync);

        std::string localId = fmt::format("streaming_pseudo_device{}", pseudoDeviceIndex++);

        PropertyObjectPtr transportLayerConfig = deviceConfig.getPropertyValue("TransportLayerConfig");
        auto transportClient = createAndConnectTransportClient(host, port, path, transportLayerConfig);
        Int initTimeout = transportLayerConfig.getPropertyValue("StreamingInitTimeout");
        return createWithImplementation<IDevice, NativeStreamingDeviceImpl>(
            context,
            parent,
            localId,
            connectionString,
            transportClient,
            addStreamingProcessingContext(connectionString),
            initTimeout
        );
    }
    else if (connectionStringHasPrefix(connectionString, NativeConfigurationDevicePrefix))
    {
        return createNativeDevice(context, parent, connectionString, deviceConfig, host, port, path);
    }
    else
    {
        throw InvalidParameterException();
    }
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

PropertyObjectPtr NativeStreamingClientModule::createDeviceDefaultConfig()
{
    auto defaultConfig = PropertyObject();

    defaultConfig.addProperty(ObjectProperty("TransportLayerConfig", createTransportLayerDefaultConfig()));

    return defaultConfig;
}

bool NativeStreamingClientModule::onAcceptsConnectionParameters(const StringPtr& connectionString,
                                                                const PropertyObjectPtr& config)
{
    auto pseudoDevicePrefixFound = connectionStringHasPrefix(connectionString, NativeStreamingDevicePrefix);
    auto devicePrefixFound = connectionStringHasPrefix(connectionString, NativeConfigurationDevicePrefix);

    if ((!devicePrefixFound && !pseudoDevicePrefixFound) ||
        !validateConnectionString(connectionString))
    {
        return false;
    }

    if ( config.assigned() && !validateDeviceConfig(config))
    {
        LOG_W("Connection string \"{}\" is accepted but config is incomplete", connectionString);
        return false;
    }
    else
    {
        return true;
    }
}

bool NativeStreamingClientModule::onAcceptsStreamingConnectionParameters(const StringPtr& connectionString,
                                                                   const PropertyObjectPtr& config)
{
    if (connectionString.assigned() && connectionString != "")
    {
        return connectionStringHasPrefix(connectionString, NativeStreamingPrefix) &&
               validateConnectionString(connectionString);
    }
    else if (config.assigned())
    {
        StringPtr configConnectionString = config.getPropertyValue("PrimaryConnectionString");
        if (configConnectionString.getLength())
        {
            return connectionStringHasPrefix(configConnectionString, NativeStreamingPrefix) &&
               validateConnectionString(configConnectionString);
        }
        StringPtr protocolId = config.getPropertyValue("protocolId");
        StringPtr primaryAddress = "";
        if (ListPtr<IString> addresses = config.getPropertyValue("Addresses"); addresses.getCount() > 0)
        {
            primaryAddress = addresses[0];
        }
        if (protocolId == NativeStreamingID && primaryAddress != "")
            return config.hasProperty("Port");
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
    const PropertyObjectPtr& transportLayerConfig)
{
    StringPtr modifiedHost = host;
    if (modifiedHost.assigned() && modifiedHost.getLength() > 1 && modifiedHost.getCharPtr()[0] == '[' && modifiedHost.getCharPtr()[modifiedHost.getLength() - 1] == ']')
    {
        modifiedHost = modifiedHost.toStdString().substr(1, modifiedHost.getLength() - 2);
    }
    auto transportClientHandler = std::make_shared<NativeStreamingClientHandler>(context, transportLayerConfig);
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
    if (!onAcceptsStreamingConnectionParameters(connectionString, config))
        throw InvalidParameterException();

    StringPtr host;
    StringPtr port;
    StringPtr path;
    StringPtr streamingConnectionString;

    if (connectionString.assigned() && connectionString != "")
    {
        host = getHost(connectionString);
        port = getPort(connectionString);
        path = getPath(connectionString);
        streamingConnectionString = connectionString;
    }
    else if(config.assigned())
    {
        streamingConnectionString = config.getPropertyValue("PrimaryConnectionString");
        if (streamingConnectionString.getLength() == 0)
        {
            if (ListPtr<IString> addresses = config.getPropertyValue("Addresses"); addresses.getCount() > 0)
            {
                host = addresses[0];
            }
            if (!host.assigned() || host.toStdString().empty()) 
                throw InvalidParameterException("Device address is not set");

            auto portNumber = config.getPropertyValue("Port").template asPtr<IInteger>();
            port = String(fmt::format("{}", portNumber));

            path = String("/");
            streamingConnectionString = String(fmt::format("{}{}:{}", NativeStreamingPrefix, host, portNumber));
        }
    }
    else
    {
        throw ArgumentNullException();
    }

    auto transportLayerConfig = createTransportLayerDefaultConfig();
    Int initTimeout = transportLayerConfig.getPropertyValue("StreamingInitTimeout");

    auto transportClient = createAndConnectTransportClient(host, port, path, transportLayerConfig);
    return createNativeStreaming(streamingConnectionString, transportClient, initTimeout);
}

bool NativeStreamingClientModule::connectionStringHasPrefix(const StringPtr& connectionString,
                                                            const char* prefix)
{
    std::string connStr = connectionString;
    auto found = connStr.find(prefix);
    return (found == 0);
}

DeviceTypePtr NativeStreamingClientModule::createPseudoDeviceType()
{
    return DeviceType(NativeStreamingDeviceTypeId,
                      "PseudoDevice",
                      "Pseudo device, provides only signals of the remote device as flat list",
                      NativeStreamingClientModule::createDeviceDefaultConfig());
}

DeviceTypePtr NativeStreamingClientModule::createDeviceType()
{
    return DeviceType(NativeConfigurationDeviceTypeId,
                      "Device",
                      "Network device connected over Native configuration protocol",
                      NativeStreamingClientModule::createDeviceDefaultConfig());
}

StringPtr NativeStreamingClientModule::getHost(const StringPtr& url)
{
    std::string urlString = url.toStdString();

    auto regexIpv6Hostname = std::regex("^.*:\\/\\/(\\[([a-fA-F0-9:]+)\\])");
    auto regexIpv4Hostname = std::regex("^.*:\\/\\/([^:\\/\\s]+)");
    std::smatch match;

    if (std::regex_search(urlString, match, regexIpv6Hostname))
        return String(match[2]);
    if (std::regex_search(urlString, match, regexIpv4Hostname))
        return String(match[1]);
    throw InvalidParameterException("Host name not found in url: {}", url);
}

StringPtr NativeStreamingClientModule::getPort(const StringPtr& url)
{
    std::string urlString = url.toStdString();

    auto regexPort = std::regex(":(\\d+)");
    std::smatch match;

    std::string host = getHost(url).toStdString();
    std::string suffix = urlString.substr(urlString.find(host) + host.size());

    if (std::regex_search(suffix, match, regexPort))
    {
        return String(match[1]);
    }
    else
    {
        return "7420";
    }
}

StringPtr NativeStreamingClientModule::getPath(const StringPtr& url)
{
    std::string urlString = url.toStdString();

    std::string host = getHost(url).toStdString();
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
    return config.hasProperty("TransportLayerConfig") &&
           config.getPropertyValue("TransportLayerConfig").supportsInterface<IPropertyObject>() &&
           validateTransportLayerConfig(config.getPropertyValue("TransportLayerConfig"));
}

bool NativeStreamingClientModule::validateConnectionString(const StringPtr& connectionString)
{
    try
    {
        auto host = getHost(connectionString);
        auto port = getPort(connectionString);
        auto path = getPath(connectionString);
        return true;
    }
    catch(...)
    {
        return false;
    }
}

END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_CLIENT_MODULE
