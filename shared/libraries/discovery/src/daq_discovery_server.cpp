#include "daq_discovery/daq_discovery_server.h"
#include <opendaq/device_info_factory.h>
#include <coreobjects/property_factory.h>
#include <coreobjects/property_object_protected_ptr.h>
#include <opendaq/device_info_internal_ptr.h>

BEGIN_NAMESPACE_DISCOVERY


void DiscoveryServer::registerDevice(const DeviceInfoPtr& deviceInfo, const PropertyObjectPtr& config)
{
    StringPtr serviceName = config.getPropertyValue("ServiceName");
    IntPtr servicePort = config.getPropertyValue("Port");
    StringPtr serviceCap = config.getPropertyValue("ServiceCap");
    StringPtr servicePath = config.getPropertyValue("ServicePath");

    if (serviceName == nullptr || servicePort == nullptr || serviceCap == nullptr)
    {
        return;
    }

    std::unordered_map<std::string, std::string> properties;
    properties["name"] = deviceInfo.getName().toStdString();
    properties["manufacturer"] = deviceInfo.getManufacturer().toStdString();
    properties["model"] = deviceInfo.getModel().toStdString();
    properties["serialNumber"] = deviceInfo.getSerialNumber().toStdString();
    properties["caps"] = serviceName.toStdString();
    if (servicePath != nullptr)
        properties["path"] = servicePath.toStdString();

    StringPtr id = deviceInfo.getManufacturer() + "_" + deviceInfo.getSerialNumber();
    MdnsDiscoveredDevice device(serviceName, servicePort, properties);
    server.addDevice(id, device);
}

void DiscoveryServer::removeDevice(const DeviceInfoPtr& deviceInfo)
{
    StringPtr id = deviceInfo.getManufacturer() + "_" + deviceInfo.getSerialNumber();
    server.removeDevice(id);
}


END_NAMESPACE_DISCOVERY
