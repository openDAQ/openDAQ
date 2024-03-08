#include "daq_discovery/daq_discovery_client.h"
#include <opendaq/device_info_factory.h>
#include <coreobjects/property_factory.h>
#include <coreobjects/property_object_protected_ptr.h>

BEGIN_NAMESPACE_DISCOVERY

DiscoveryClient::DiscoveryClient(std::vector<ServerCapabilityCb> serverCapabilityCbs,
                                 std::unordered_set<std::string> requiredCaps)
    : discoveredDevices(List<IDeviceInfo>())
    , requiredCaps(std::move(requiredCaps))
    , serverCapabilityCbs(std::move(serverCapabilityCbs))
{
}

void DiscoveryClient::initMdnsClient(const std::string& serviceName, std::chrono::milliseconds discoveryDuration)
{
    mdnsClient = std::make_shared<MDNSDiscoveryClient>(serviceName);
    mdnsClient->setDiscoveryDuration(discoveryDuration);
}

daq::ListPtr<daq::IDeviceInfo> daq::discovery::DiscoveryClient::discoverDevices()
{
    discoveredDevices.clear();

    runInThread([this]() { discoverMdnsDevices(); });
    joinThreads();

    return discoveredDevices;
}

void DiscoveryClient::runInThread(std::function<void()> func)
{
    threadPool.emplace_back(func);
}

void DiscoveryClient::joinThreads()
{
    for (auto& thread : threadPool)
    {
        if (thread.joinable())
            thread.join();
    }
}

void DiscoveryClient::discoverMdnsDevices()
{
    if (mdnsClient == nullptr)
        return;

    auto mdnsDevices = mdnsClient->getAvailableDevices();

    for (const auto& device : mdnsDevices)
    {
        DeviceInfoPtr deviceInfo = createDeviceInfo(device);
        if (deviceInfo.assigned())
        {
            for (const auto& connectionStringFormatCb : serverCapabilityCbs)
            {
                deviceInfo.asPtr<IDeviceInfoConfig>().addServerCapability(connectionStringFormatCb(device));
            }
            discoveredDevices.pushBack(deviceInfo);
        }
    }
}

template <typename T>
void addInfoProperty(DeviceInfoConfigPtr& info, std::string propName, T propValue)
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
            info->setPropertyValue(String(propName), (daq::BaseObjectPtr) propValue);
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

DeviceInfoPtr DiscoveryClient::createDeviceInfo(MdnsDiscoveredDevice discoveredDevice) const
{
    if (discoveredDevice.ipv4Address.empty())
        return nullptr;

    std::unordered_set<std::string> requiredCapsCopy = requiredCaps;
    auto caps = discoveredDevice.getPropertyOrDefault("caps");
    std::stringstream ss(caps);
    std::string segment;

    while (std::getline(ss, segment, ','))
    {
        // Replace legacy caps tag "TMS" with "OPENDAQ"
        if (segment == "TMS")
            segment = "OPENDAQ";

        if (requiredCapsCopy.count(segment))
            requiredCapsCopy.erase(segment);
    }

    if (!requiredCapsCopy.empty())
        return nullptr;

    DeviceInfoConfigPtr deviceInfo = DeviceInfo("daq://device");

    addInfoProperty(deviceInfo, "canonicalName", discoveredDevice.canonicalName);
    addInfoProperty(deviceInfo, "serviceWeight", discoveredDevice.serviceWeight);
    addInfoProperty(deviceInfo, "servicePort", discoveredDevice.servicePort);
    addInfoProperty(deviceInfo, "ipv4Address", discoveredDevice.ipv4Address);
    addInfoProperty(deviceInfo, "ipv6Address", discoveredDevice.ipv6Address);

    for (const auto& prop : discoveredDevice.properties)
    {
        addInfoProperty(deviceInfo, prop.first, prop.second);
    }

    StringPtr connectionString = "daq://" + deviceInfo.getManufacturer() + "_" + deviceInfo.getSerialNumber();
    deviceInfo.asPtr<IDeviceInfoConfig>().setConnectionString(connectionString);
    return deviceInfo;
}

END_NAMESPACE_DISCOVERY
