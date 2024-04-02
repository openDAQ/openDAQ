#include <websocket_streaming_client_module/websocket_streaming_client_module_impl.h>
#include <websocket_streaming_client_module/version.h>
#include <coretypes/version_info_factory.h>
#include <chrono>
#include <websocket_streaming/websocket_client_device_factory.h>
#include <websocket_streaming/websocket_streaming_factory.h>
#include <opendaq/device_type_factory.h>
#include <regex>

BEGIN_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING_CLIENT_MODULE

static const char* WebsocketDeviceTypeId = "daq.ws";
static const char* TcpsocketDeviceTypeId = "daq.tcp";
static const char* WebsocketDevicePrefix = "daq.ws://";
static const char* TcpsocketDevicePrefix = "daq.tcp://";
static const char* WebsocketStreamingPrefix = "daq.wss://";
static const char* WebsocketStreamingID = "daq.wss";

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
                auto connectionString = fmt::format("daq.ws://{}:{}{}",
                                   discoveredDevice.ipv4Address,
                                   discoveredDevice.servicePort,
                                   discoveredDevice.getPropertyOrDefault("path", "/"));
                return ServerCapability(context, "openDAQ WebsocketTcp Streaming", "Streaming").setConnectionString(connectionString).setConnectionType("Ipv4");
            }
        },
        {"WS"}
    )
{
    discoveryClient.initMdnsClient("_streaming-ws._tcp.local.");
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
    auto tcpsocketDeviceType = createTcpsocketDeviceType();

    result.set(websocketDeviceType.getId(), websocketDeviceType);
    result.set(tcpsocketDeviceType.getId(), tcpsocketDeviceType);

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
    if (found != 0)
        found = connStr.find(TcpsocketDevicePrefix);
    return (found == 0);
}

bool WebsocketStreamingClientModule::onAcceptsStreamingConnectionParameters(const StringPtr& connectionString, const ServerCapabilityPtr& capability)
{
    if (connectionString.assigned())
    {
        std::string connStr = connectionString;
        auto found = connStr.find(WebsocketStreamingPrefix);
        return (found == 0);
    }
    else if (capability.assigned())
    {
        if (capability.getPropertyValue("protocolId") == WebsocketStreamingID)
        {
            try
            {
                auto generatedConnectionString = tryCreateWebsocketConnectionString(capability);
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

StreamingPtr WebsocketStreamingClientModule::onCreateStreaming(const StringPtr& connectionString, const ServerCapabilityPtr& capability)
{
    StringPtr streamingConnectionString = connectionString;

    if (!streamingConnectionString.assigned() && !capability.assigned())
        throw ArgumentNullException();

    if (!onAcceptsStreamingConnectionParameters(streamingConnectionString, capability))
        throw InvalidParameterException();

    if (!streamingConnectionString.assigned())
        streamingConnectionString = tryCreateWebsocketConnectionString(capability);

    return WebsocketStreaming(streamingConnectionString, context);
}

StringPtr WebsocketStreamingClientModule::tryCreateWebsocketConnectionString(const ServerCapabilityPtr& capability)
{
    if (capability == nullptr)
        throw InvalidParameterException("Capability is not set");

    StringPtr address = capability.getPropertyValue("address");
    if (!address.assigned() || address.toStdString().empty())
        throw InvalidParameterException("Device address is not set");

    auto port = capability.getPropertyValue("Port").template asPtr<IInteger>();
    auto connectionString = String(fmt::format("daq.wss://{}:{}", address, port));

    return connectionString;
}

DeviceTypePtr WebsocketStreamingClientModule::createWebsocketDeviceType()
{
    return DeviceType(WebsocketDeviceTypeId,
                      "Websocket enabled device",
                      "Pseudo device, provides only signals of the remote device as flat list");
}

DeviceTypePtr WebsocketStreamingClientModule::createTcpsocketDeviceType()
{
    return DeviceType(TcpsocketDeviceTypeId,
                      "Tcpsocket enabled device",
                      "Pseudo device, provides only signals of the remote device as flat list");
}

END_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING_CLIENT_MODULE
