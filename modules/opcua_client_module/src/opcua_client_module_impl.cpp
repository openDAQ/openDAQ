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

static const char* DaqOpcUaDeviceTypeId = "daq.opcua";
static const char* DaqOpcUaDevicePrefix = "daq.opcua://";
static const char* OpcUaScheme = "opc.tcp://";

using namespace discovery;
using namespace daq::opcua;

OpcUaClientModule::OpcUaClientModule(ContextPtr context)
    : Module("openDAQ OpcUa client module",
            daq::VersionInfo(OPCUA_CLIENT_MODULE_MAJOR_VERSION, OPCUA_CLIENT_MODULE_MINOR_VERSION, OPCUA_CLIENT_MODULE_PATCH_VERSION),
            std::move(context))
    , discoveryClient([](const MdnsDiscoveredDevice& discoveredDevice)
        {
            return DaqOpcUaDevicePrefix + discoveredDevice.ipv4Address + "/";
        },
     {"OPENDAQ"})
{
    discoveryClient.initMdnsClient("_opcua-tcp._tcp.local.");
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

    const auto deviceUrl = GetUrlFromConnectionString(connectionString);
    StringPtr rootDeviceAddress;
    std::smatch match;
    auto regexHostname = std::regex("^(.*:\\/\\/)?([^:\\/\\s]+)");
    if (std::regex_search(deviceUrl, match, regexHostname))
        rootDeviceAddress = String(match[2]);

    FunctionPtr createStreamingCallback = [&](const StreamingInfoPtr& streamingInfo,
                                              bool isRootDevice) -> StreamingPtr
        {
            if (isRootDevice)
                streamingInfo.template asPtr<IStreamingInfoConfig>().setPrimaryAddress(rootDeviceAddress);

            const StringPtr streamingHeuristic = deviceConfig.getPropertySelectionValue("StreamingConnectionHeuristic");
            const ListPtr<IString> allowedStreamingProtocols = deviceConfig.getPropertyValue("AllowedStreamingProtocols");

            if (const auto it = std::find(allowedStreamingProtocols.begin(),
                                          allowedStreamingProtocols.end(),
                                          streamingInfo.getProtocolId());
                it == allowedStreamingProtocols.end())
                return nullptr;

            if (streamingHeuristic == "MinHops" ||
                streamingHeuristic == "Fallbacks" ||
                streamingHeuristic == "MinConnections" && isRootDevice)
                return this->createStreamingFromAnotherModule(nullptr, streamingInfo);
            else
                return nullptr;
        };

    std::scoped_lock lock(sync);
    TmsClient client(context, parent, OpcUaScheme + deviceUrl, createStreamingCallback);
    auto device = client.connect();
    this->configureStreamingSources(deviceConfig, device);
    return device;
}

std::string OpcUaClientModule::GetUrlFromConnectionString(const StringPtr& connectionString)
{
    std::string connStr = connectionString;
    std::string prefixWithDeviceStr = DaqOpcUaDevicePrefix;
    auto found = connStr.find(prefixWithDeviceStr);
    if (found != 0)
        throw InvalidParameterException();

    return connStr.substr(prefixWithDeviceStr.size(), std::string::npos);
}

bool OpcUaClientModule::onAcceptsConnectionParameters(const StringPtr& connectionString, const PropertyObjectPtr& config)
{
    std::string connStr = connectionString;
    auto found = connStr.find(DaqOpcUaDevicePrefix);
    if (found != 0)
        return false;

    if ( config.assigned() && !acceptDeviceProperties(config))
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
    allowedStreamingProtocols.pushBack("daq.ns");
    primaryStreamingProtocol = "daq.ns";
// TODO add websocket streaming to default list of allowed protocols
// when it will have subscribe/unsubscribe support
//#if defined(OPENDAQ_ENABLE_WEBSOCKET_STREAMING)
//    allowedStreamingProtocols.pushBack("daq.wss");
#endif
#if defined(OPENDAQ_ENABLE_WEBSOCKET_STREAMING)
    allowedStreamingProtocols.pushBack("daq.wss");
//    primaryStreamingProtocol = "daq.wss";
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
            std::string protocolPrefix = primaryStreamingProtocol.toStdString();
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
                for (const auto& protocol : allowedStreamingProtocols)
                {
                    std::string protocolPrefix = protocol.toStdString();
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
