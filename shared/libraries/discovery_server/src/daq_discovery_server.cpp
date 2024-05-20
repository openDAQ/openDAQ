#include <discovery_server/daq_discovery_server.h>
#include <coreobjects/property_factory.h>
#include <coreobjects/property_object_protected_ptr.h>

BEGIN_NAMESPACE_DISCOVERY_SERVICE


bool DiscoveryServer::registerDevice(const StringPtr& serverId, const PropertyObjectPtr& config, const DeviceInfoPtr& deviceInfo)
{
    if (!config.hasProperty("ServiceDiscoverable") || config.getPropertyValue("ServiceDiscoverable").asPtr<IBoolean>() == false)
    {
        return false;
    }
    if (!config.hasProperty("ServiceName"))
    {
        return false;
    }
    if (!config.hasProperty("Port"))
    {
        return false;
    }
    if (!config.hasProperty("ServiceCap"))
    {
        return false;
    }

    auto serviceName = config.getPropertyValue("ServiceName");
    auto servicePort = config.getPropertyValue("Port");
    auto serviceCap = config.getPropertyValue("ServiceCap");

    std::unordered_map<std::string, std::string> properties;
    properties["caps"] = serviceCap.asPtr<IString>().toStdString();
    if (config.hasProperty("ServicePath"))
        properties["path"] = config.getPropertyValue("ServicePath").asPtr<IString>().toStdString();

    properties["name"] = "";
    properties["manufacturer"] = "";
    properties["model"] = "";
    properties["serialNumber"] = "";

    if (deviceInfo.assigned())
    {
        properties["name"] = deviceInfo.getName().toStdString();
        properties["manufacturer"] = deviceInfo.getManufacturer().toStdString();
        properties["model"] = deviceInfo.getModel().toStdString();
        properties["serialNumber"] = deviceInfo.getSerialNumber().toStdString();
    }
    
    MdnsDiscoveredDevice device(serviceName, servicePort, properties);
    return server.addDevice(serverId, device);
}

bool DiscoveryServer::removeDevice(const StringPtr& serverId)
{
    if (serverId == nullptr)
    {
        return false;
    }
    return server.removeDevice(serverId);
}


END_NAMESPACE_DISCOVERY_SERVICE
