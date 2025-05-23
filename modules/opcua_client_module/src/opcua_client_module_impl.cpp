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
#include <opendaq/address_info_factory.h>
#include <coreobjects/property_factory.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA_CLIENT_MODULE

static const char* DaqOpcUaDeviceTypeId = "OpenDAQOPCUAConfiguration";
static const char* DaqOpcUaDevicePrefix = "daq.opcua";
static const char* OpcUaScheme = "opc.tcp";

static const std::regex RegexIpv6Hostname(R"(^(.+://)?(\[[a-fA-F0-9:]+(?:\%[a-zA-Z0-9_\.-~]+)?\])(?::(\d+))?(/.*)?$)");
static const std::regex RegexIpv4Hostname(R"(^(.+://)?([^:/\s]+)(?::(\d+))?(/.*)?$)");

using namespace discovery;
using namespace daq::opcua;

OpcUaClientModule::OpcUaClientModule(ContextPtr context)
    : Module("OpenDAQOPCUAClientModule",
            daq::VersionInfo(OPCUA_CLIENT_MODULE_MAJOR_VERSION, OPCUA_CLIENT_MODULE_MINOR_VERSION, OPCUA_CLIENT_MODULE_PATCH_VERSION),
            std::move(context),
            "OpenDAQOPCUAClientModule")
    , discoveryClient(
        {"OPENDAQ"}
    )
{
    loggerComponent = this->context.getLogger().getOrAddComponent("OPCUAClient");
    discoveryClient.initMdnsClient(List<IString>("_opcua-tcp._tcp.local."));
}

ListPtr<IDeviceInfo> OpcUaClientModule::onGetAvailableDevices()
{
    auto availableDevices = List<IDeviceInfo>();
    for (const auto& device : discoveryClient.discoverMdnsDevices())
        availableDevices.pushBack(populateDiscoveredDevice(device));
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
        DAQ_THROW_EXCEPTION(ArgumentNullException);

    PropertyObjectPtr configPtr = config;
    if (!configPtr.assigned())
        configPtr = createDefaultConfig();
    else
        configPtr = populateDefaultConfig(configPtr);

    if (!acceptsConnectionParameters(connectionString, configPtr))
        DAQ_THROW_EXCEPTION(InvalidParameterException);

    if (!context.assigned())
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Context is not available.");

    std::string host;
    std::string hostType;
    int port;
    auto formedConnectionString = formConnectionString(connectionString, configPtr, host, port, hostType);

    std::scoped_lock lock(sync);

    auto endpoint = OpcUaEndpoint(formedConnectionString);

    endpoint.setUsername(configPtr.getPropertyValue("Username"));
    endpoint.setPassword(configPtr.getPropertyValue("Password"));

    TmsClient client(context, parent, endpoint);
    auto device = client.connect();

    // Set the connection info for the device
    DeviceInfoPtr deviceInfo = device.getInfo();
    deviceInfo.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue("connectionString", connectionString);
    ServerCapabilityConfigPtr connectionInfo = deviceInfo.getConfigurationConnectionInfo();
    
    const auto addressInfo = AddressInfoBuilder().setAddress(host)
                                                 .setReachabilityStatus(AddressReachabilityStatus::Reachable)
                                                 .setType(hostType)
                                                 .setConnectionString(connectionString)
                                                 .build();

    connectionInfo.setProtocolId(DaqOpcUaDeviceTypeId)
                  .setProtocolName("OpenDAQOPCUA")
                  .setProtocolType(ProtocolType::Configuration)
                  .setConnectionType("TCP/IP")
                  .addAddress(host)
                  .setPort(port)
                  .setPrefix(DaqOpcUaDevicePrefix)
                  .setConnectionString(connectionString)
                  .addAddressInfo(addressInfo)
                  .freeze();

    return device;
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

DeviceInfoPtr OpcUaClientModule::populateDiscoveredDevice(const MdnsDiscoveredDevice& discoveredDevice)
{
    auto cap = ServerCapability(DaqOpcUaDeviceTypeId, "OpenDAQOPCUA", ProtocolType::Configuration);
    if (!discoveredDevice.ipv4Address.empty())
    {
        auto connectionStringIpv4 = fmt::format("{}://{}:{}{}",
                                                DaqOpcUaDevicePrefix,
                                                discoveredDevice.ipv4Address,
                                                discoveredDevice.servicePort,
                                                discoveredDevice.getPropertyOrDefault("path", "/"));
        cap.addConnectionString(connectionStringIpv4);
        cap.addAddress(discoveredDevice.ipv4Address);

        const auto addressInfo = AddressInfoBuilder().setAddress(discoveredDevice.ipv4Address)
                                     .setReachabilityStatus(AddressReachabilityStatus::Unknown)
                                     .setType("IPv4")
                                     .setConnectionString(connectionStringIpv4)
                                     .build();
        cap.addAddressInfo(addressInfo);
    }
    if(!discoveredDevice.ipv6Address.empty())
    {
        auto connectionStringIpv6 = fmt::format("{}://{}:{}{}",
                                                DaqOpcUaDevicePrefix,
                                                discoveredDevice.ipv6Address,
                                                discoveredDevice.servicePort,
                                                discoveredDevice.getPropertyOrDefault("path", "/"));
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
    cap.setPrefix(DaqOpcUaDevicePrefix);
    cap.setProtocolVersion(discoveredDevice.getPropertyOrDefault("protocolVersion", ""));
    if (discoveredDevice.servicePort > 0)
        cap.setPort(discoveredDevice.servicePort);

    return populateDiscoveredDeviceInfo(DiscoveryClient::populateDiscoveredInfoProperties, discoveredDevice, cap, createDeviceType());
}

StringPtr OpcUaClientModule::formConnectionString(const StringPtr& connectionString, const PropertyObjectPtr& config, std::string& host, int& port, std::string& hostType)
{
    std::string urlString = connectionString.toStdString();
    std::smatch match;

    std::string prefix = "";
    std::string path = "/";

    if (config.assigned() )
    {
        if (config.hasProperty("Port"))
            port = config.getPropertyValue("Port");

        if (config.hasProperty("Path"))
            path = String(config.getPropertyValue("Path")).toStdString();
    }

    bool parsed = false;
    parsed = std::regex_search(urlString, match, RegexIpv6Hostname);
    hostType = "IPv6";

    if (!parsed)
    {
        parsed = std::regex_search(urlString, match, RegexIpv4Hostname);
        hostType = "IPv4";
    }

    if (parsed)
    {
        prefix = match[1];
        host = match[2];

        if (match[3].matched)
            port = std::stoi(match[3]);

        if (match[4].matched)
            path = match[4];
    }
    else
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Host name not found in url: {}", connectionString);

    if (prefix != std::string(DaqOpcUaDevicePrefix) + "://")
        DAQ_THROW_EXCEPTION(InvalidParameterException, "OpcUa does not support connection string with prefix {}", prefix);

    return std::string(OpcUaScheme) + "://" + host + ":" + std::to_string(port) + path;
}

bool OpcUaClientModule::acceptsConnectionParameters(const StringPtr& connectionString, const PropertyObjectPtr& config)
{
    std::string connStr = connectionString;
    auto found = connStr.find(std::string(DaqOpcUaDevicePrefix) + "://");
    return found == 0;
}

Bool OpcUaClientModule::onCompleteServerCapability(const ServerCapabilityPtr& source, const ServerCapabilityConfigPtr& target)
{
    if (target.getProtocolId() != "OpenDAQOPCUAConfiguration")
        return false;

    if (source.getConnectionType() != "TCP/IP")
        return false;

    if (!source.getAddresses().assigned() || !source.getAddresses().getCount())
    {
        LOG_W("Source server capability address is not available when filling in missing OPC UA capability information.")
        return false;
    }

    const auto addrInfos = source.getAddressInfo();
    if (!addrInfos.assigned() || !addrInfos.getCount())
    {
        LOG_W("Source server capability addressInfo is not available when filling in missing OPC UA capability information.")
        return false;
    }

    auto port = target.getPort();
    if (port == -1)
    {
        port = 4840;
        target.setPort(port);
        LOG_W("OPC UA server capability is missing port. Defaulting to 4840.")
    }

    const auto path = target.hasProperty("Path") ? target.getPropertyValue("Path") : "";
    const auto targetAddress = target.getAddresses();
    for (const auto& addrInfo : addrInfos)
    {
        const auto address = addrInfo.getAddress();
        if (auto it = std::find(targetAddress.begin(), targetAddress.end(), address); it != targetAddress.end())
            continue;

        const auto prefix = target.getPrefix();
        StringPtr connectionString;
        if (source.getPrefix() == prefix)
            connectionString = addrInfo.getConnectionString();
        else
            connectionString = fmt::format("{}://{}:{}{}", prefix, address, port, path);

        const auto targetAddrInfo = AddressInfoBuilder()
                                        .setAddress(address)
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

DeviceTypePtr OpcUaClientModule::createDeviceType()
{
    return DeviceTypeBuilder()
        .setId(DaqOpcUaDeviceTypeId)
        .setName("OpcUa enabled device")
        .setDescription("Network device connected over OpcUa protocol")
        .setConnectionStringPrefix(DaqOpcUaDevicePrefix)
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

END_NAMESPACE_OPENDAQ_OPCUA_CLIENT_MODULE
