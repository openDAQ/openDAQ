#include <daq_discovery/daq_discovery_server.h>
#include <opendaq/device_info_factory.h>
#include <coreobjects/property_factory.h>
#include <coreobjects/property_object_protected_ptr.h>
#include <opendaq/device_info_internal_ptr.h>

BEGIN_NAMESPACE_DISCOVERY


void DiscoveryServer::registerDevice(const StringPtr& serverId, const PropertyObjectPtr& config)
{
    StringPtr serviceName = config.getPropertyValue("ServiceName");
    IntPtr servicePort = config.getPropertyValue("Port");
    StringPtr serviceCap = config.getPropertyValue("ServiceCap");

    if (serviceName == nullptr || servicePort == nullptr || serviceCap == nullptr)
    {
        return;
    }

    std::unordered_map<std::string, std::string> properties;
    if (StringPtr name = config.getPropertyValue("Name"); name != nullptr)
        properties["name"] = name.toStdString();
    if (StringPtr manufacturer = config.getPropertyValue("Manufacturer"); manufacturer != nullptr)
        properties["manufacturer"] = manufacturer.toStdString();
    if (StringPtr model = config.getPropertyValue("Model"); model != nullptr)
        properties["model"] = model.toStdString();
    if (StringPtr serialNumber = config.getPropertyValue("SerialNumber"); serialNumber != nullptr)
        properties["serialNumber"] = serialNumber.toStdString();
    properties["caps"] = serviceName.toStdString();
    if (StringPtr servicePath = config.getPropertyValue("ServicePath"); servicePath != nullptr)
        properties["path"] = servicePath.toStdString();

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


END_NAMESPACE_DISCOVERY
