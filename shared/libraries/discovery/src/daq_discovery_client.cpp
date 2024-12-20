#include <daq_discovery/daq_discovery_client.h>
#include <coreobjects/property_factory.h>
#include <coreobjects/property_object_protected_ptr.h>

BEGIN_NAMESPACE_DISCOVERY

DiscoveryClient::DiscoveryClient(std::unordered_set<std::string> requiredCaps)
    : requiredCaps(std::move(requiredCaps))
{
}

void DiscoveryClient::initMdnsClient(const ListPtr<IString>& serviceNames, std::chrono::milliseconds discoveryDuration)
{
    mdnsClient = std::make_shared<MDNSDiscoveryClient>(serviceNames);
    mdnsClient->setDiscoveryDuration(discoveryDuration);
}

std::vector<MdnsDiscoveredDevice> DiscoveryClient::discoverMdnsDevices() const
{
    std::vector<MdnsDiscoveredDevice> discovered;
    if (mdnsClient == nullptr)
        return discovered;

    auto mdnsDevices = mdnsClient->getAvailableDevices();

    for (const auto& device : mdnsDevices)
    {
        if (verifyDiscoveredDevice(device))
            discovered.push_back(device);
    }

    return discovered;
}

template <typename T>
void addInfoProperty(PropertyObjectPtr& info, std::string propName, T propValue)
{
    if (info.hasProperty(propName))
    {
        if (auto protectedObj = info.asPtrOrNull<IPropertyObjectProtected>(); protectedObj.assigned())
        {
            protectedObj.setProtectedPropertyValue(propName, propValue);
        }
        else
        {
            // Ignore errors
            info->setPropertyValue(String(propName), static_cast<daq::BaseObjectPtr>(propValue));
        }
    }
    else
    {
        PropertyBuilderPtr builder;

        if constexpr (std::is_same_v<T, std::string>)
        {
            builder = StringPropertyBuilder(propName, propValue);
        }
        else if constexpr (std::is_integral_v<T>)
        {
            builder = IntPropertyBuilder(propName, propValue);
        }
        else if constexpr (std::is_floating_point_v<T>)
        {
            builder = DoublePropertyBuilder(propName, propValue);
        }
        else if constexpr (std::is_same_v<T, bool>)
        {
            builder = BoolProperty(propName, propValue);
        }

        if (builder.assigned())
        {
            builder.setReadOnly(true);
            info.addProperty(builder.build());
        }
    }
}

void DiscoveryClient::populateDiscoveredInfoProperties(PropertyObjectPtr& info, const MdnsDiscoveredDevice& device)
{
    for (const auto& prop : device.properties)
    {
        addInfoProperty(info, prop.first, prop.second);
    }
}

ErrCode DiscoveryClient::applyIpConfiguration(const StringPtr& manufacturer,
                                              const StringPtr& serialNumber,
                                              const StringPtr& ifaceName,
                                              const PropertyObjectPtr& config)
{
    TxtProperties requestProperties;

    requestProperties["manufacturer"] = manufacturer.toStdString();
    requestProperties["serialNumber"] = serialNumber.toStdString();
    requestProperties["ifaceName"] = ifaceName.toStdString();

    const bool dhcpMode = config.getPropertyValue("dhcp");
    requestProperties["dhcp"] = dhcpMode ? "1" : "0";
    ListPtr<IString> addressesList = config.getPropertyValue("addresses");
    std::string addressesString;
    for (const auto& addr : addressesList)
        addressesString += addr.toStdString() + ";";
    requestProperties["addresses"] = addressesString;
    StringPtr gateway = config.getPropertyValue("gateway");
    requestProperties["gateway"] = gateway.toStdString();

    return mdnsClient->requestIpConfigModification(MDNSDiscoveryClient::DAQ_IP_MODIFICATION_SERVICE_NAME, requestProperties);
}

bool DiscoveryClient::verifyDiscoveredDevice(const MdnsDiscoveredDevice& discoveredDevice) const
{
    if (discoveredDevice.ipv4Address.empty() && discoveredDevice.ipv6Address.empty())
        return false;

    std::unordered_set<std::string> requiredCapsCopy = requiredCaps;
    auto caps = discoveredDevice.getPropertyOrDefault("caps");
    std::stringstream ss(caps);
    std::string segment;

    while (std::getline(ss, segment, ','))
    {
        // Replace legacy caps tag "TMS" with "OPENDAQ"
        // Replace legacy caps tag "WS" with "LT"
        if (segment == "TMS")
            segment = "OPENDAQ";
        else if (segment == "WS")
            segment = "LT";

        if (requiredCapsCopy.count(segment))
            requiredCapsCopy.erase(segment);
    }

    return requiredCapsCopy.empty();
}

END_NAMESPACE_DISCOVERY
