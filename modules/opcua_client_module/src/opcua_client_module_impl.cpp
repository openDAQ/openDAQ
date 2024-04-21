#include <opcua_client_module/opcua_client_module_impl.h>
#include <opcua_client_module/version.h>
#include <coretypes/version_info_factory.h>
#include <chrono>
#include <opcuatms_client/tms_client.h>
#include <opendaq/custom_log.h>
#include <coreobjects/property_object_factory.h>
#include <opendaq/device_type_factory.h>
#include <opendaq/mirrored_signal_config_ptr.h>
#include <opendaq/search_filter_factory.h>
#include <regex>

BEGIN_NAMESPACE_OPENDAQ_OPCUA_CLIENT_MODULE

static const char* DaqOpcUaDeviceTypeId = "opendaq_opcua_config";
static const char* DaqOpcUaDevicePrefix = "daq.opcua://";
static const char* OpcUaScheme = "opc.tcp://";

using namespace discovery;
using namespace daq::opcua;

OpcUaClientModule::OpcUaClientModule(ContextPtr context)
    : Module("openDAQ OpcUa client module",
            daq::VersionInfo(OPCUA_CLIENT_MODULE_MAJOR_VERSION, OPCUA_CLIENT_MODULE_MINOR_VERSION, OPCUA_CLIENT_MODULE_PATCH_VERSION),
            std::move(context),
            "OpcUaClient")
    , discoveryClient(
        {
            [context = this->context](const MdnsDiscoveredDevice& discoveredDevice)
            {
                auto connectionStringIpv4 = DaqOpcUaDevicePrefix + discoveredDevice.ipv4Address + "/";
                auto connectionStringIpv6 = fmt::format("{}[{}]/",
                                    DaqOpcUaDevicePrefix,
                                    discoveredDevice.ipv6Address);
                auto cap = ServerCapability("opendaq_opcua_config", "openDAQ OpcUa", ProtocolType::Configuration);
                cap.addConnectionString(connectionStringIpv4);
                cap.addAddress(discoveredDevice.ipv4Address);
                cap.addConnectionString(connectionStringIpv6);
                cap.addAddress("[" + discoveredDevice.ipv6Address + "]");
                cap.setConnectionType("TCP/IP");
                cap.setPrefix("daq.opcua");
                return cap;
            }
        },
        {"OPENDAQ"}
    )
{
    discoveryClient.initMdnsClient(List<IString>("_opcua-tcp._tcp.local."));
}

ListPtr<IDeviceInfo> OpcUaClientModule::onGetAvailableDevices()
{
    auto availableDevices = discoveryClient.discoverDevices();
    for (auto device : availableDevices)
    {
        device.asPtr<IDeviceInfoConfig>().setDeviceType(createDeviceType());
    }
    return availableDevices;
}

DictPtr<IString, IDeviceType> OpcUaClientModule::onGetAvailableDeviceTypes()
{
    auto result = Dict<IString, IDeviceType>();

    auto deviceType = createDeviceType();
    result.set(deviceType.getId(), deviceType);

    return result;
}

DevicePtr OpcUaClientModule::onCreateDevice(const StringPtr& connectionString,
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
        throw InvalidParameterException{"Context is not available."};

    auto parsedConnection = ParseConnectionString(connectionString);
    auto prefix = std::get<0>(parsedConnection);
    auto host = std::get<1>(parsedConnection);
    auto path = std::get<2>(parsedConnection);
    if (prefix != DaqOpcUaDevicePrefix)
        throw InvalidParameterException("OpcUa does not support connection string with prefix");

    FunctionPtr createStreamingCallback = [&](const ServerCapabilityConfigPtr& capability,
                                              bool isRootDevice) -> StreamingPtr
        {
            if (capability.getProtocolType() != ProtocolType::Streaming)
                return nullptr;

            if (isRootDevice)
                capability.addAddress(host);

            const StringPtr streamingHeuristic = deviceConfig.getPropertySelectionValue("StreamingConnectionHeuristic");
            const ListPtr<IString> allowedStreamingProtocols = deviceConfig.getPropertyValue("AllowedStreamingProtocols");

            const StringPtr protocolId = capability.getProtocolId();

            if (protocolId != nullptr)
                if (const auto it = std::find(allowedStreamingProtocols.begin(),
                                            allowedStreamingProtocols.end(),
                                            protocolId);
                    it == allowedStreamingProtocols.end())
                    return nullptr;

            if (streamingHeuristic == "MinHops" ||
                streamingHeuristic == "Fallbacks" ||
                streamingHeuristic == "MinConnections" && isRootDevice)
                return this->createStreamingFromAnotherModule(nullptr, capability);
            else
                return nullptr;
        };

    std::scoped_lock lock(sync);
    TmsClient client(context, parent, OpcUaScheme + host + path, createStreamingCallback);
    auto device = client.connect();
    this->configureStreamingSources(deviceConfig, device);
    return device;
}

std::tuple<std::string, std::string, std::string> OpcUaClientModule::ParseConnectionString(const StringPtr& connectionString)
{
    std::string urlString = connectionString.toStdString();

    auto regexIpv6Hostname = std::regex("^(.*:\\/\\/)(\\[[a-fA-F0-9:]+\\])(.*)");
    auto regexIpv4Hostname = std::regex("^(.*:\\/\\/)([^:\\/\\s]+)(.*)");
    std::smatch match;

    if (std::regex_search(urlString, match, regexIpv6Hostname))
        return {match[1],match[2],match[3]};
    if (std::regex_search(urlString, match, regexIpv4Hostname))
        return {match[1],match[2],match[3]};
    
    throw InvalidParameterException("Host name not found in url: {}", connectionString);
}

bool OpcUaClientModule::onAcceptsConnectionParameters(const StringPtr& connectionString, const PropertyObjectPtr& config)
{
    std::string connStr = connectionString;
    auto found = connStr.find(DaqOpcUaDevicePrefix);
    if (found != 0)
        return false;

    if (config.assigned() && !acceptDeviceProperties(config))
    {
        LOG_W("Connection string \"{}\" is accepted but config is incomplete", connectionString);
        return false;
    }
    else
    {
        return true;
    }
}

bool OpcUaClientModule::acceptDeviceProperties(const PropertyObjectPtr& config)
{
    bool hasRequiredProperties = config.hasProperty("StreamingConnectionHeuristic") &&
                                 config.hasProperty("AllowedStreamingProtocols") &&
                                 config.hasProperty("PrimaryStreamingProtocol");
    if (!hasRequiredProperties)
        return false;

    const StringPtr primaryStreamingProtocol = config.getPropertyValue("PrimaryStreamingProtocol");
    const ListPtr<IString> allowedStreamingProtocols = config.getPropertyValue("AllowedStreamingProtocols");
    if (const auto it = std::find(allowedStreamingProtocols.begin(),
                                  allowedStreamingProtocols.end(),
                                  primaryStreamingProtocol);
        it == allowedStreamingProtocols.end())
        return false;

    return true;
}

PropertyObjectPtr OpcUaClientModule::createDeviceDefaultConfig()
{
    auto defaultConfig = PropertyObject();

    const auto streamingConnectionHeuristicProp =  SelectionProperty("StreamingConnectionHeuristic",
                                                                    List<IString>("MinConnections",
                                                                                  "MinHops",
                                                                                  "Fallbacks",
                                                                                  "NotConnected"),
                                                                    0);
    defaultConfig.addProperty(streamingConnectionHeuristicProp);

    auto allowedStreamingProtocols = List<IString>();
    StringPtr primaryStreamingProtocol = "none";

#if defined(OPENDAQ_ENABLE_NATIVE_STREAMING)
    allowedStreamingProtocols.pushBack("opendaq_native_streaming");
    primaryStreamingProtocol = "opendaq_native_streaming";
// TODO add websocket streaming to default list of allowed protocols
// when it will have subscribe/unsubscribe support
//#if defined(OPENDAQ_ENABLE_WEBSOCKET_STREAMING)
//    allowedStreamingProtocols.pushBack("opendaq_lt_streaming");
#endif
#if defined(OPENDAQ_ENABLE_WEBSOCKET_STREAMING)
    allowedStreamingProtocols.pushBack("opendaq_lt_streaming");
//    primaryStreamingProtocol = "opendaq_lt_streaming";
#endif

    defaultConfig.addProperty(ListProperty("AllowedStreamingProtocols", allowedStreamingProtocols));
    defaultConfig.addProperty(StringProperty("PrimaryStreamingProtocol", primaryStreamingProtocol));

    return defaultConfig;
}

DeviceTypePtr OpcUaClientModule::createDeviceType()
{
    auto configurationCallback = [](IBaseObject* input, IBaseObject** output) -> ErrCode
    {
        PropertyObjectPtr propObjPtr;
        ErrCode errCode = wrapHandlerReturn(&OpcUaClientModule::createDeviceDefaultConfig, propObjPtr);
        *output = propObjPtr.detach();
        return errCode;
    };

    return DeviceType(DaqOpcUaDeviceTypeId,
                      "OpcUa enabled device",
                      "Network device connected over OpcUa protocol",
                      configurationCallback);
}

void OpcUaClientModule::configureStreamingSources(const PropertyObjectPtr& deviceConfig, const DevicePtr& device)
{
    const StringPtr streamingHeuristic = deviceConfig.getPropertySelectionValue("StreamingConnectionHeuristic");
    if (streamingHeuristic == "NotConnected")
        return;

    const StringPtr primaryStreamingProtocol = deviceConfig.getPropertyValue("PrimaryStreamingProtocol");
    const ListPtr<IString> allowedStreamingProtocols = deviceConfig.getPropertyValue("AllowedStreamingProtocols");
    std::unordered_map<std::string, std::string> idPrefixMap;

    {
        auto devices = device.getDevices(search::Recursive(search::Any()));
        devices.pushBack(device);

        for (const auto& dev : devices)
        {
            const auto capabilities = dev.getInfo().getServerCapabilities();
            for (const auto & cap : capabilities)
            {
                idPrefixMap.insert(std::make_pair(cap.getProtocolId(), cap.getPrefix()));
            }
        }
    }

    for (const auto& signal : device.getSignals(search::Recursive(search::Any())))
    {
        MirroredSignalConfigPtr mirroredSignalConfigPtr = signal.template asPtr<IMirroredSignalConfig>();

        auto streamingSources = mirroredSignalConfigPtr.getStreamingSources();

        if (streamingSources.empty())
            continue;

        // TODO this may raise bugs
        // since sub-devices are being created recursively it is expected that
        // the leaf device direct streaming is the first-one of available sources
        // and root streaming is the last-one
        StringPtr leafStreaming;
        StringPtr rootStreaming;
        for (const auto& streamingConnectionString : streamingSources)
        {
            std::string connectionString = streamingConnectionString.toStdString();
            std::string streamingProtocolId = primaryStreamingProtocol.toStdString();

            if (!idPrefixMap.count(streamingProtocolId))
                continue;

            std::string protocolPrefix = idPrefixMap.at(streamingProtocolId);
            if (connectionString.find(protocolPrefix) == 0)
            {
                // save the first streaming source as the leaf streaming
                if (!leafStreaming.assigned())
                    leafStreaming = streamingConnectionString;

                // save the last streaming source as the root streaming
                rootStreaming = streamingConnectionString;
            }
        }

        if (!leafStreaming.assigned() || !rootStreaming.assigned())
        {
            for (const auto& streamingConnectionString : streamingSources)
            {
                std::string connectionString = streamingConnectionString.toStdString();
                for (const auto& streamingProtocolId : allowedStreamingProtocols)
                {
                    if (!idPrefixMap.count(streamingProtocolId))
                        continue;

                    std::string protocolPrefix = idPrefixMap.at(streamingProtocolId);
                    if (connectionString.find(protocolPrefix) == 0)
                    {
                        // save the first streaming source as the leaf streaming
                        if (!leafStreaming.assigned())
                            leafStreaming = streamingConnectionString;

                        // save the last streaming source as the root streaming
                        rootStreaming = streamingConnectionString;
                    }
                }
            }
        }

        if (!leafStreaming.assigned() || !rootStreaming.assigned())
            continue;

        if (streamingHeuristic == "MinConnections" || streamingHeuristic == "Fallbacks")
        {
            mirroredSignalConfigPtr.setActiveStreamingSource(rootStreaming);
        }
        else if (streamingHeuristic == "MinHops")
        {
            mirroredSignalConfigPtr.setActiveStreamingSource(leafStreaming);
        }
    }
}

END_NAMESPACE_OPENDAQ_OPCUA_CLIENT_MODULE
