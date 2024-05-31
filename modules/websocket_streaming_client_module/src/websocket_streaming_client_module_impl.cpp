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
static const char* OldWebsocketDevicePrefix = "daq.ws://";

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
                auto cap = ServerCapability(WebsocketDeviceTypeId, "openDAQ LT Streaming", ProtocolType::Streaming);

                if (!discoveredDevice.ipv4Address.empty())
                {
                    auto connectionStringIpv4 = WebsocketStreamingClientModule::createUrlConnectionString(
                        discoveredDevice.ipv4Address,
                        discoveredDevice.servicePort,
                        discoveredDevice.getPropertyOrDefault("path", "/")
                    );
                    cap.addConnectionString(connectionStringIpv4);
                    cap.addAddress(discoveredDevice.ipv4Address);
                }

                if(!discoveredDevice.ipv6Address.empty())
                {
                    auto connectionStringIpv6 = WebsocketStreamingClientModule::createUrlConnectionString(
                        "[" + discoveredDevice.ipv6Address + "]",
                        discoveredDevice.servicePort,
                        discoveredDevice.getPropertyOrDefault("path", "/")
                    );
                    cap.addConnectionString(connectionStringIpv6);
                    cap.addAddress("[" + discoveredDevice.ipv6Address + "]");
                }

                cap.setConnectionType("TCP/IP");
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
    auto device = WebsocketClientDevice(context, parent, localId, connectionString);

    // Set the connection info for the device
    auto host = String("");
    auto port = -1;
    {
        std::smatch match;
        auto regexpConnectionString = std::regex(R"(^(.*://)?([^:/\s]+)(?::(\d+))?(/.*)?$)");
        std::string connectionStringStr = connectionString;
        if (std::regex_search(connectionStringStr, match, regexpConnectionString))
        {
            host = match[2].str();
            port = 7414;
            if (match[3].matched)
                port = std::stoi(match[3]);
        }
    }

    // Set the connection info for the device
    ServerCapabilityConfigPtr connectionInfo = device.getInfo().getConfigurationConnectionInfo();
    connectionInfo.setProtocolId(WebsocketDeviceTypeId);
    connectionInfo.setProtocolName("openDAQ LT Streaming");
    connectionInfo.setProtocolType(ProtocolType::Streaming);
    connectionInfo.addAddress(host);
    connectionInfo.setPort(port);
    connectionInfo.setPrefix("daq.lt");
    connectionInfo.setConnectionString(connectionString);

    return device;
}

bool WebsocketStreamingClientModule::onAcceptsConnectionParameters(const StringPtr& connectionString, const PropertyObjectPtr& /*config*/)
{
    std::string connStr = connectionString;
    auto found = connStr.find(WebsocketDevicePrefix) == 0 || connStr.find(OldWebsocketDevicePrefix) == 0;
    return found;
}

bool WebsocketStreamingClientModule::onAcceptsStreamingConnectionParameters(const StringPtr& connectionString, const PropertyObjectPtr& config)
{
    if (connectionString.assigned() && connectionString != "")
    {
        return onAcceptsConnectionParameters(connectionString, config);
    }
    return false;
}

StreamingPtr WebsocketStreamingClientModule::onCreateStreaming(const StringPtr& connectionString, const PropertyObjectPtr& config)
{
    if (!connectionString.assigned())
        throw ArgumentNullException();

    if (!onAcceptsStreamingConnectionParameters(connectionString, config))
        throw InvalidParameterException();

    return WebsocketStreaming(connectionString, context);
}

StringPtr WebsocketStreamingClientModule::onCreateConnectionString(const ServerCapabilityPtr& serverCapability)
{
    if (serverCapability.getProtocolId() != "opendaq_lt_streaming")
        return nullptr;

    StringPtr connectionString = serverCapability.getConnectionString();
    if (connectionString.getLength() != 0)
        return connectionString;

    StringPtr address;
    if (ListPtr<IString> addresses = serverCapability.getAddresses(); addresses.getCount() > 0)
    {
        address = addresses[0];
    }
    if (!address.assigned() || address.toStdString().empty())
        throw InvalidParameterException("Address is not set");

    auto port = serverCapability.getPort();
    if (port == -1)
        throw InvalidParameterException("Port is not set");

    return WebsocketStreamingClientModule::createUrlConnectionString(
        address,
        port,
        serverCapability.hasProperty("Path") ? serverCapability.getPropertyValue("Path") : ""
    );
}

StringPtr WebsocketStreamingClientModule::createUrlConnectionString(const StringPtr& host,
                                                                    const IntegerPtr& port,
                                                                    const StringPtr& path)
{
    return String(fmt::format("daq.lt://{}:{}{}", host, port, path));
}

DeviceTypePtr WebsocketStreamingClientModule::createWebsocketDeviceType()
{
    return DeviceType(WebsocketDeviceTypeId,
                      "Websocket enabled device",
                      "Pseudo device, provides only signals of the remote device as flat list");
}

END_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING_CLIENT_MODULE
