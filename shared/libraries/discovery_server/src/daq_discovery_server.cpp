#include <discovery_server/daq_discovery_server.h>
#include <coreobjects/property_factory.h>
#include <coreobjects/property_object_protected_ptr.h>

BEGIN_NAMESPACE_DISCOVERY_SERVICE


void DiscoveryServer::registerDevice(const StringPtr& serverId, const PropertyObjectPtr& config)
{
    if (!config.hasProperty("ServiceDiscoverable") || config.getPropertyValue("ServiceDiscoverable").asPtr<IBoolean>() == false)
    {
        return;
    }
    if (!config.hasProperty("ServiceName"))
    {
        return;
    }
    if (!config.hasProperty("Port"))
    {
        return;
    }
    if (!config.hasProperty("ServiceCap"))
    {
        return;
    }

    auto serviceName = config.getPropertyValue("ServiceName");
    auto servicePort = config.getPropertyValue("Port");
    auto serviceCap = config.getPropertyValue("ServiceCap");

    std::unordered_map<std::string, std::string> properties;
    properties["caps"] = serviceCap.asPtr<IString>().toStdString();
    if (config.hasProperty("Name"))
        properties["name"] = config.getPropertyValue("Name").asPtr<IString>().toStdString();
    if (config.hasProperty("Manufacturer"))
        properties["manufacturer"] = config.getPropertyValue("Manufacturer").asPtr<IString>().toStdString();
    if (config.hasProperty("Model"))
        properties["model"] = config.getPropertyValue("Model").asPtr<IString>().toStdString();
    if (config.hasProperty("SerialNumber"))
        properties["serialNumber"] = config.getPropertyValue("SerialNumber").asPtr<IString>().toStdString();
    if (config.hasProperty("ServicePath"))
        properties["path"] = config.getPropertyValue("ServicePath").asPtr<IString>().toStdString();

    MdnsDiscoveredDevice device(serviceName, servicePort, properties);
    server.addDevice(serverId, device);
}

void DiscoveryServer::removeDevice(const StringPtr& serverId)
{
    if (serverId == nullptr)
    {
        return;
    }
    server.removeDevice(serverId);
}


END_NAMESPACE_DISCOVERY_SERVICE
