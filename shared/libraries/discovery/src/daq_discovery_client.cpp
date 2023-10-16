#include "daq_discovery/daq_discovery_client.h"
#include "opendaq/device_info_factory.h"
#include "coreobjects/property_factory.h"

BEGIN_NAMESPACE_DISCOVERY

DiscoveryClient::DiscoveryClient(std::function<std::string(MdnsDiscoveredDevice)> connectionStringFormatCb,
                                     std::unordered_set<std::string> requiredCaps)
    : discoveredDevices(List<IDeviceInfo>())
    , requiredCaps(std::move(requiredCaps))
    , connectionStringFormatCb(std::move(connectionStringFormatCb))
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
        try
        {
            discoveredDevices.pushBack(createDeviceInfo(device));
        }
        catch (...)
        {
            // not a valid openDAQ device
        }
    }
}

DeviceInfoPtr DiscoveryClient::createDeviceInfo(MdnsDiscoveredDevice discoveredDevice) const
{
    auto exceptionMessage = "Failed to parse discovery data. Not a openDAQ device.";

    if (discoveredDevice.ipv4Address.empty())
        throw DaqException(OPENDAQ_ERR_INVALIDPARAMETER, exceptionMessage);
    
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
        throw DaqException(OPENDAQ_ERR_INVALIDPARAMETER, exceptionMessage);

    DeviceInfoConfigPtr deviceInfo = DeviceInfo(connectionStringFormatCb(discoveredDevice));

    for (const auto& prop : discoveredDevice.properties)
    {
        if (deviceInfo.hasProperty(prop.first))
            deviceInfo.setPropertyValue(prop.first, prop.second);
        else
            deviceInfo.addProperty(StringProperty(prop.first, prop.second));
    }

    return deviceInfo;
}

END_NAMESPACE_DISCOVERY
