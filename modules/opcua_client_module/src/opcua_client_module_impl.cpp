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
#include <coreobjects/property_factory.h>

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
                auto cap = ServerCapability(DaqOpcUaDeviceTypeId, "openDAQ OpcUa", ProtocolType::Configuration);
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
                if (discoveredDevice.servicePort > 0)
                    cap.setPort(discoveredDevice.servicePort);
                return cap;
            }
        },
        {"OPENDAQ"}
    )
{
    loggerComponent = this->context.getLogger().getOrAddComponent("OpcUaClient");
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
                                            const PropertyObjectPtr& aConfig)
{
    if (!connectionString.assigned())
        throw ArgumentNullException();

    PropertyObjectPtr config = aConfig;
    if (!config.assigned())
        config = createDefaultConfig();
    else
        config = populateDefaultConfig(config);

    if (!acceptsConnectionParameters(connectionString, config))
        throw InvalidParameterException();

    if (!context.assigned())
        throw InvalidParameterException{"Context is not available."};

    std::string host;
    int port;
    auto formedConnectionString = formConnectionString(connectionString, config, host, port);

    std::scoped_lock lock(sync);

    auto endpoint = OpcUaEndpoint(formedConnectionString);

    if (config.assigned())
    {
        if (config.hasProperty("Username"))
            endpoint.setUsername(config.getPropertyValue("Username"));
        if (config.hasProperty("Password"))
            endpoint.setPassword(config.getPropertyValue("Password"));
    }

    TmsClient client(context, parent, endpoint);
    auto device = client.connect();
    completeServerCapabilities(device, host);

    // Set the connection info for the device
    ServerCapabilityConfigPtr connectionInfo = device.getInfo().getConfigurationConnectionInfo();

    connectionInfo.setProtocolId(DaqOpcUaDeviceTypeId);
    connectionInfo.setProtocolName("openDAQ OpcUa");
    connectionInfo.setProtocolType(ProtocolType::Configuration);
    connectionInfo.setConnectionType("TCP/IP");
    connectionInfo.addAddress(host);
    connectionInfo.setPort(port);
    connectionInfo.setPrefix("daq.opcua");
    connectionInfo.setConnectionString(connectionString);

    return device;
}

void OpcUaClientModule::completeServerCapabilities(const DevicePtr& device, const StringPtr& deviceAddress)
{
    auto deviceInfo = device.getInfo();
    if (deviceInfo.assigned())
    {
        for (const auto& capability : deviceInfo.getServerCapabilities())
        {
            if (capability.getConnectionType() == "TCP/IP")
                capability.asPtr<IServerCapabilityConfig>().addAddress(deviceAddress);
        }
    }
}

PropertyObjectPtr OpcUaClientModule::populateDefaultConfig(const PropertyObjectPtr& config)
{
    const auto defConfig = createDefaultConfig();
    for (const auto& prop : defConfig.getAllProperties())
    {
        const auto name = prop.getName();
        if (config.hasProperty(name))
            defConfig.setPropertyValue(name, config.getPropertyValue(name));
    }

    return defConfig;
}

StringPtr OpcUaClientModule::formConnectionString(const StringPtr& connectionString, const PropertyObjectPtr& config, std::string& host, int& port)
{
    if (config.assigned() && config.hasProperty("Port"))
    {
        port = config.getPropertyValue("Port");
    }

    std::string urlString = connectionString.toStdString();

    auto regexIpv6Hostname = std::regex(R"(^(.*://)(\[[a-fA-F0-9:]+\])(?::(\d+))?(/.*)?$)");
    auto regexIpv4Hostname = std::regex(R"(^(.*://)?([^:/\s]+)(?::(\d+))?(/.*)?$)");
    std::smatch match;

    std::string target = "/";
    std::string prefix = "";
    std::string path = "";

    bool parsed = false;
    parsed = std::regex_search(urlString, match, regexIpv6Hostname);
    if (!parsed)
    {
        parsed = std::regex_search(urlString, match, regexIpv4Hostname);
    }

    if (parsed)
    {
        prefix = match[1];
        host = match[2];

        if (match[3].matched && port == 4840)
            port = std::stoi(match[3]);

        if (match[4].matched)
            path = match[4];
    }
    else
        throw InvalidParameterException("Host name not found in url: {}", connectionString);

    if (prefix != DaqOpcUaDevicePrefix)
        throw InvalidParameterException("OpcUa does not support connection string with prefix {}", prefix);

    return OpcUaScheme + host + ":" + std::to_string(port) + "/" + path;
}

bool OpcUaClientModule::acceptsConnectionParameters(const StringPtr& connectionString, const PropertyObjectPtr& config)
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
    return DeviceTypeBuilder()
        .setId(DaqOpcUaDeviceTypeId)
        .setName("OpcUa enabled device")
        .setDescription("Network device connected over OpcUa protocol")
        .setConnectionStringPrefix("daq.opcua")
        .setDefaultConfig(createDefaultConfig())
        .build();
}

PropertyObjectPtr OpcUaClientModule::createDefaultConfig()
{
    auto config = PropertyObject();

    config.addProperty(StringProperty("Username", ""));
    config.addProperty(StringProperty("Password", ""));
    config.addProperty(IntProperty("Port", 4840));

    return config;
}

StringPtr OpcUaClientModule::onCreateConnectionString(const ServerCapabilityPtr& serverCapability)
{
    if (serverCapability.getProtocolId() != "opendaq_opcua_config")
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
    {
        port = 4840;
        LOG_W("OPC UA server capability is missing port. Defaulting to 4840.")
    }

    return fmt::format("{}{}:{}", DaqOpcUaDevicePrefix, address, port);
}

END_NAMESPACE_OPENDAQ_OPCUA_CLIENT_MODULE
