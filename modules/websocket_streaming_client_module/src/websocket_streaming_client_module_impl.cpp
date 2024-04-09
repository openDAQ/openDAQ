#include <websocket_streaming_client_module/websocket_streaming_client_module_impl.h>
#include <websocket_streaming_client_module/version.h>
#include <coretypes/version_info_factory.h>
#include <chrono>
#include <websocket_streaming/websocket_client_device_factory.h>
#include <websocket_streaming/websocket_streaming_factory.h>
#include <opendaq/device_type_factory.h>
#include <regex>

BEGIN_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING_CLIENT_MODULE

static const char* WebsocketDeviceTypeId = "opendaq_lt_streaming";
static const char* WebsocketDevicePrefix = "daq.lt://";

using namespace discovery;
using namespace daq::websocket_streaming;

WebsocketStreamingClientModule::WebsocketStreamingClientModule(ContextPtr context)
    : Module("openDAQ websocket client module",
            daq::VersionInfo(WS_STREAM_CL_MODULE_MAJOR_VERSION, WS_STREAM_CL_MODULE_MINOR_VERSION, WS_STREAM_CL_MODULE_PATCH_VERSION),
            std::move(context),
            "WebsocketStreamingClient")
    , deviceIndex(0)
    , discoveryClient(
        {
            [context = this->context](MdnsDiscoveredDevice discoveredDevice)
            {
                auto connectionString = fmt::format("{}{}:{}{}",
                                    WebsocketDevicePrefix,
                                    discoveredDevice.ipv4Address,
                                    discoveredDevice.servicePort,
                                    discoveredDevice.getPropertyOrDefault("path", "/"));
                auto cap = ServerCapability("opendaq_lt_streaming", "openDAQ LT Streaming", ProtocolType::Streaming);
                cap.addConnectionString(connectionString);
                cap.setConnectionType("Ipv4");
                cap.setPrefix("daq.lt");
                return cap;
            }
        }, 
        {"LT"}
    )
{
    discoveryClient.initMdnsClient(List<IString>("_streaming-lt._tcp.local.", "_streaming-ws._tcp.local."));
}

ListPtr<IDeviceInfo> WebsocketStreamingClientModule::onGetAvailableDevices()
{
    auto availableDevices = discoveryClient.discoverDevices();
    for (auto device : availableDevices)
    {
        device.asPtr<IDeviceInfoConfig>().setDeviceType(createWebsocketDeviceType());
    }
    return availableDevices;
}

DictPtr<IString, IDeviceType> WebsocketStreamingClientModule::onGetAvailableDeviceTypes()
{
    auto result = Dict<IString, IDeviceType>();

    auto websocketDeviceType = createWebsocketDeviceType();

    result.set(websocketDeviceType.getId(), websocketDeviceType);

    return result;
}

DevicePtr WebsocketStreamingClientModule::onCreateDevice(const StringPtr& connectionString,
                                                const ComponentPtr& parent,
                                                const PropertyObjectPtr& config)
{
    if (!connectionString.assigned())
        throw ArgumentNullException();

    if (!onAcceptsConnectionParameters(connectionString, config))
        throw InvalidParameterException();

    if (!context.assigned())
        throw InvalidParameterException{"Context is not available."};

    // We don't create any streaming objects here since the
    // internal streaming object is always created within the device

    std::scoped_lock lock(sync);

    std::string localId = fmt::format("websocket_pseudo_device{}", deviceIndex++);
    return WebsocketClientDevice(context, parent, localId, connectionString);
}

bool WebsocketStreamingClientModule::onAcceptsConnectionParameters(const StringPtr& connectionString, const PropertyObjectPtr& /*config*/)
{
    std::string connStr = connectionString;
    auto found = connStr.find(WebsocketDevicePrefix);
    return (found == 0);
}

bool WebsocketStreamingClientModule::onAcceptsStreamingConnectionParameters(const StringPtr& connectionString, const PropertyObjectPtr& config)
{
    if (connectionString.assigned())
    {
        std::string connStr = connectionString;
        auto found = connStr.find(WebsocketDevicePrefix);
        return (found == 0);
    }
    else if (config.assigned())
    {
        if (config.getPropertyValue("protocolId") == WebsocketDeviceTypeId)
        {
            try
            {
                auto generatedConnectionString = tryCreateWebsocketConnectionString(config);
                return true;
            }
            catch (const std::exception& e)
            {
                LOG_W("Failed to interpret streaming info config: {}", e.what())
            }
        }
    }
    return false;
}

StreamingPtr WebsocketStreamingClientModule::onCreateStreaming(const StringPtr& connectionString, const PropertyObjectPtr& config)
{
    StringPtr streamingConnectionString = connectionString;

    if (!streamingConnectionString.assigned() && !config.assigned())
        throw ArgumentNullException();

    if (!onAcceptsStreamingConnectionParameters(streamingConnectionString, config))
        throw InvalidParameterException();

    if (!streamingConnectionString.assigned())
        streamingConnectionString = tryCreateWebsocketConnectionString(config);

    return WebsocketStreaming(streamingConnectionString, context);
}

StringPtr WebsocketStreamingClientModule::tryCreateWebsocketConnectionString(const ServerCapabilityPtr& capability)
{
    if (capability == nullptr)
        throw InvalidParameterException("Capability is not set");

    StringPtr connectionString = capability.getPropertyValue("PrimaryConnectionString");
    if (connectionString.getLength() != 0)
        return connectionString;

    StringPtr address = capability.getPropertyValue("address");
    if (!address.assigned() || address.toStdString().empty())
        throw InvalidParameterException("Device address is not set");

    auto port = capability.getPropertyValue("Port").template asPtr<IInteger>();
    connectionString = String(fmt::format("{}{}:{}", WebsocketDevicePrefix, address, port));

    return connectionString;
}

DeviceTypePtr WebsocketStreamingClientModule::createWebsocketDeviceType()
{
    return DeviceType(WebsocketDeviceTypeId,
                      "Websocket enabled device",
                      "Pseudo device, provides only signals of the remote device as flat list");
}

END_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING_CLIENT_MODULE
