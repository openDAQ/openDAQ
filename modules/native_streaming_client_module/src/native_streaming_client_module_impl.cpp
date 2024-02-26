#include <native_streaming_client_module/native_streaming_client_module_impl.h>
#include <native_streaming_client_module/version.h>
#include <coretypes/version_info_factory.h>
#include <daq_discovery/daq_discovery_client.h>
#include <opendaq/device_type_factory.h>

#include <native_streaming_client_module/native_streaming_device_impl.h>
#include <native_streaming_client_module/native_streaming_impl.h>
#include <native_streaming_client_module/native_device_impl.h>

#include <regex>

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
            std::move(context))
    , deviceIndex(0)
    , discoveryClient(
        {
            [](MdnsDiscoveredDevice discoveredDevice)
            {
                return fmt::format("daq.nsd://{}:{}{}",
                                   discoveredDevice.ipv4Address,
                                   discoveredDevice.servicePort,
                                   discoveredDevice.getPropertyOrDefault("path", "/"));
            },
            [](MdnsDiscoveredDevice discoveredDevice)
            {
                return fmt::format("daq.nd://{}:{}{}",
                                   discoveredDevice.ipv4Address,
                                   discoveredDevice.servicePort,
                                   discoveredDevice.getPropertyOrDefault("path", "/"));
            }
        },
        {"OPENDAQ_NS"}
    )
{
    discoveryClient.initMdnsClient("_opendaq-streaming-native._tcp.local.");
}

ListPtr<IDeviceInfo> NativeStreamingClientModule::onGetAvailableDevices()
{
    auto availableDevices = discoveryClient.discoverDevices();
    for (const auto& device : availableDevices)
    {
        device.asPtr<IDeviceInfoConfig>().setDeviceType(createPseudoDeviceType());
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
    auto transportProtocolClient =
        std::make_shared<NativeStreamingClientHandler>(context, config.getPropertyValue("TransportLayerConfig"));
    StreamingPtr nativeStreaming =
        createWithImplementation<IStreaming, NativeStreamingImpl>(streamingConnectionString,
                                                                  host,
                                                                  port,
                                                                  path,
                                                                  context,
                                                                  transportProtocolClient,
                                                                  nullptr,
                                                                  nullptr,
                                                                  nullptr);
    nativeStreaming.setActive(true);

    auto deviceHelper = std::make_unique<NativeDeviceHelper>(context, transportProtocolClient);
    auto device = deviceHelper->connectAndGetDevice(parent);

    deviceHelper->addStreaming(nativeStreaming);
    // TODO check streaming options recursively and add optional streamings
    deviceHelper->subscribeToCoreEvent(context);

    device.asPtr<INativeDevicePrivate>()->attachDeviceHelper(std::move(deviceHelper));
    device.asPtr<INativeDevicePrivate>()->setConnectionString(connectionString);

    return device;
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

        std::string localId = fmt::format("streaming_pseudo_device{}", deviceIndex++);

        auto clientHandler = std::make_shared<NativeStreamingClientHandler>(context, deviceConfig.getPropertyValue("TransportLayerConfig"));
        return createWithImplementation<IDevice, NativeStreamingDeviceImpl>(context, parent, localId, connectionString, host, port, path, clientHandler);
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

    transportLayerConfig.addProperty(daq::BoolProperty("HeartbeatEnabled", daq::False));
    transportLayerConfig.addProperty(daq::IntProperty("HeartbeatPeriod", 1000));
    transportLayerConfig.addProperty(daq::IntProperty("HeartbeatTimeout", 1500));
    transportLayerConfig.addProperty(daq::IntProperty("ConnectionTimeout", 1000));
    transportLayerConfig.addProperty(daq::IntProperty("StreamingInitTimeout", 1000));
    transportLayerConfig.addProperty(daq::IntProperty("ReconnectionPeriod", 1000));

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
    std::string connStr = connectionString;
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
                                                                   const StreamingInfoPtr& config)
{
    if (connectionString.assigned())
    {
        return connectionStringHasPrefix(connectionString, NativeStreamingPrefix) &&
               validateConnectionString(connectionString);
    }
    else if (config.assigned())
    {
        if (config.getProtocolId() == NativeStreamingID && config.getPrimaryAddress().assigned())
        {
            const auto propertyObj = config.asPtr<IPropertyObject>();
            return propertyObj.assigned() && propertyObj.hasProperty("Port");
        }
    }
    return false;
}

StreamingPtr NativeStreamingClientModule::onCreateStreaming(const StringPtr& connectionString,
                                                      const StreamingInfoPtr& config)
{
    if (!onAcceptsStreamingConnectionParameters(connectionString, config))
        throw InvalidParameterException();

    if (connectionString.assigned())
    {
        auto host = getHost(connectionString);
        auto port = getPort(connectionString);
        auto path = getPath(connectionString);
        return createWithImplementation<IStreaming, NativeStreamingImpl>(
            connectionString,
            host,
            port,
            path,
            context,
            std::make_shared<opendaq_native_streaming_protocol::NativeStreamingClientHandler>(context, createTransportLayerDefaultConfig()),
            nullptr,
            nullptr,
            nullptr);
    }
    else if(config.assigned())
    {
        auto host = config.getPrimaryAddress();
        if (host.toStdString().empty()) throw InvalidParameterException("Device address is not set");

        const auto propertyObj = config.asPtr<IPropertyObject>();
        auto portNumber = propertyObj.getPropertyValue("Port").template asPtr<IInteger>();
        auto port = String(fmt::format("{}", portNumber));

        auto generatedConnectionString = String(fmt::format("{}{}:{}", NativeStreamingPrefix, host, portNumber));
        return createWithImplementation<IStreaming, NativeStreamingImpl>(
            generatedConnectionString,
            host,
            port,
            String("/"),
            context,
            std::make_shared<opendaq_native_streaming_protocol::NativeStreamingClientHandler>(context, createTransportLayerDefaultConfig()),
            nullptr,
            nullptr,
            nullptr);
    }
    else
    {
        throw ArgumentNullException();
    }
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
    auto configurationCallback = [](IBaseObject* input, IBaseObject** output) -> ErrCode
    {
        PropertyObjectPtr propObjPtr;
        ErrCode errCode = wrapHandlerReturn(&NativeStreamingClientModule::createDeviceDefaultConfig, propObjPtr);
        *output = propObjPtr.detach();
        return errCode;
    };

    return DeviceType(NativeStreamingDeviceTypeId,
                      "PseudoDevice",
                      "Pseudo device, provides only signals of the remote device as flat list",
                      configurationCallback);
}

DeviceTypePtr NativeStreamingClientModule::createDeviceType()
{
    auto configurationCallback = [](IBaseObject* input, IBaseObject** output) -> ErrCode
    {
        PropertyObjectPtr propObjPtr;
        ErrCode errCode = wrapHandlerReturn(&NativeStreamingClientModule::createDeviceDefaultConfig, propObjPtr);
        *output = propObjPtr.detach();
        return errCode;
    };

    return DeviceType(NativeConfigurationDeviceTypeId,
                      "Device",
                      "Network device connected over Native configuration protocol",
                      configurationCallback);
}

StringPtr NativeStreamingClientModule::getHost(const StringPtr& url)
{
    std::string urlString = url.toStdString();

    auto regexHostname = std::regex("^.*:\\/\\/([^:\\/\\s]+)");
    std::smatch match;

    if (std::regex_search(urlString, match, regexHostname))
    {
        return String(match[1]);
    }
    else
    {
        throw InvalidParameterException("Host name not found in url: {}", url);
    }
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
    return config.hasProperty("HeartbeatEnabled") &&
           config.hasProperty("HeartbeatPeriod") &&
           config.hasProperty("HeartbeatTimeout") &&
           config.hasProperty("ConnectionTimeout") &&
           config.hasProperty("StreamingInitTimeout") &&
           config.hasProperty("ReconnectionPeriod") &&
           config.getProperty("HeartbeatEnabled").getValueType() == ctBool &&
           config.getProperty("HeartbeatPeriod").getValueType() == ctInt &&
           config.getProperty("HeartbeatTimeout").getValueType() == ctInt &&
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
