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
#include <opendaq/device_info_config_ptr.h>
#include <opendaq/device_info_factory.h>

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
            [context = this->context](MdnsDiscoveredDevice discoveredDevice)
            {
                auto cap = ServerCapability("opendaq_opcua_config", "openDAQ OpcUa", ProtocolType::Configuration);
                if (!discoveredDevice.ipv4Address.empty())
                {
                    auto connectionStringIpv4 = fmt::format("{}{}:{}{}",
                                    DaqOpcUaDevicePrefix,
                                    discoveredDevice.ipv4Address,
                                    discoveredDevice.servicePort,
                                    discoveredDevice.getPropertyOrDefault("path", "/"));
                    cap.addConnectionString(connectionStringIpv4);
                    cap.addAddress(discoveredDevice.ipv4Address);
                }
                if(!discoveredDevice.ipv6Address.empty())
                {
                    auto connectionStringIpv6 = fmt::format("{}[{}]:{}{}",
                                    DaqOpcUaDevicePrefix,
                                    discoveredDevice.ipv6Address,
                                    discoveredDevice.servicePort,
                                    discoveredDevice.getPropertyOrDefault("path", "/"));
                    cap.addConnectionString(connectionStringIpv6);
                    cap.addAddress("[" + discoveredDevice.ipv6Address + "]");
                }
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

    if (!onAcceptsConnectionParameters(connectionString, config))
        throw InvalidParameterException();

    if (!context.assigned())
        throw InvalidParameterException{"Context is not available."};

    auto parsedConnection = ParseConnectionString(connectionString);
    auto prefix = std::get<0>(parsedConnection);
    auto host = std::get<1>(parsedConnection);
    auto port = std::get<2>(parsedConnection);
    auto path = std::get<3>(parsedConnection);
    if (prefix != DaqOpcUaDevicePrefix)
        throw InvalidParameterException("OpcUa does not support connection string with prefix " + prefix);

    std::scoped_lock lock(sync);
    TmsClient client(context, parent, OpcUaScheme + host + ":" + port + path);
    auto device = client.connect();
    completeDeviceServerCapabilities(device, host);
    return device;
}

void OpcUaClientModule::completeDeviceServerCapabilities(const DevicePtr& device, const StringPtr& deviceAddress)
{
    auto deviceInfo = device.getInfo();
    if (deviceInfo.assigned())
    {
        for (const auto& capability : deviceInfo.getServerCapabilities())
        {
            capability.asPtr<IServerCapabilityConfig>().addAddress(deviceAddress);
        }
    }
}

// {prefix://}{hostname}:{port}{/path}
std::tuple<std::string, std::string, std::string, std::string> OpcUaClientModule::ParseConnectionString(const StringPtr& connectionString)
{
    std::string port = "4840";
    std::string target = "/";
    std::string urlString = connectionString.toStdString();

    auto regexIpv6Hostname = std::regex(R"(^(.*://)?(\[[a-fA-F0-9:]+\])(?::(\d+))?(/.*)?$)");
    auto regexIpv4Hostname = std::regex(R"(^(.*://)?([^:/\s]+)(?::(\d+))?(/.*)?$)");
    std::smatch match;

    bool parsed = false;
    parsed = std::regex_search(urlString, match, regexIpv6Hostname);
    if (!parsed)
        parsed = std::regex_search(urlString, match, regexIpv4Hostname);

    if (!parsed)
        throw InvalidParameterException("Host name not found in url: {}", connectionString);

    if (match[3].matched)
        port = match[3];
    if (match[4].matched)
        target = match[4];

    return {match[1], match[2], port, target};
}

bool OpcUaClientModule::onAcceptsConnectionParameters(const StringPtr& connectionString, const PropertyObjectPtr& config)
{
    std::string connStr = connectionString;
    auto found = connStr.find(DaqOpcUaDevicePrefix);
    if (found == 0)
        return true;
    else
        return false;
}

DeviceTypePtr OpcUaClientModule::createDeviceType()
{
    return DeviceType(DaqOpcUaDeviceTypeId,
                      "OpcUa enabled device",
                      "Network device connected over OpcUa protocol");
}

END_NAMESPACE_OPENDAQ_OPCUA_CLIENT_MODULE
