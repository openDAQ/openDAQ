/*
 * Copyright 2022-2024 openDAQ d.o.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <opendaq/mdns_discovery_server_impl.h>
#include <opendaq/custom_log.h>
#include <coreobjects/property_object_ptr.h>
#include <opendaq/device_info_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

MdnsDiscoveryServerImpl::MdnsDiscoveryServerImpl(const LoggerPtr& logger)
    : loggerComponent(logger.getOrAddComponent("MdnsDiscoveryServerImpl"))
{
}

ErrCode MdnsDiscoveryServerImpl::registerService(IString* id, IPropertyObject* config, IDeviceInfo* deviceInfo)
{
    auto serviceId = StringPtr::Borrow(id);
    auto configPtr = PropertyObjectPtr::Borrow(config);
    auto deviceInfoPtr = DeviceInfoPtr::Borrow(deviceInfo);

    if (!serviceId.assigned())
        return OPENDAQ_ERR_ARGUMENT_NULL;
    if (!configPtr.assigned())
        return OPENDAQ_IGNORED;

    if (!configPtr.hasProperty("ServiceName"))
    {
        LOG_I("Service name not provided for server \"{}\"", serviceId);
        return OPENDAQ_IGNORED;
    }
    if (!configPtr.hasProperty("Port"))
    {
        LOG_I("Port not provided for server \"{}\"", serviceId);
        return OPENDAQ_IGNORED;
    }
    if (!configPtr.hasProperty("ServiceCap"))
    {
        LOG_I("Service capability not provided for server \"{}\"", serviceId);
        return OPENDAQ_IGNORED;
    }

    auto serviceName = configPtr.getPropertyValue("ServiceName");
    auto servicePort = configPtr.getPropertyValue("Port");
    auto serviceCap = configPtr.getPropertyValue("ServiceCap");

    std::unordered_map<std::string, std::string> properties;
    properties["caps"] = serviceCap.asPtr<IString>().toStdString();
    if (configPtr.hasProperty("Path"))
        properties["path"] = configPtr.getPropertyValue("Path").asPtr<IString>().toStdString();

    properties["name"] = "";
    properties["manufacturer"] = "";
    properties["model"] = "";
    properties["serialNumber"] = "";

    if (deviceInfoPtr.assigned())
    {
        properties["name"] = deviceInfoPtr.getName().toStdString();
        properties["manufacturer"] = deviceInfoPtr.getManufacturer().toStdString();
        properties["model"] = deviceInfoPtr.getModel().toStdString();
        properties["serialNumber"] = deviceInfoPtr.getSerialNumber().toStdString();
    }

    discovery_server::MdnsDiscoveredDevice device(serviceName, servicePort, properties);
    if (discoveryServer.addDevice(serviceId, device))
    {
        LOG_I("Server \"{}\" registered with the discovery service", serviceId);
        return OPENDAQ_SUCCESS;
    }
    return OPENDAQ_ERR_INVALIDSTATE;
}

ErrCode MdnsDiscoveryServerImpl::unregisterService(IString* id)
{
    if (id == nullptr)
        return OPENDAQ_IGNORED;

    if (discoveryServer.removeDevice(StringPtr::Borrow(id)))
    {
        LOG_I("Server \"{}\" removed from the discovery service", StringPtr::Borrow(id));
        return OPENDAQ_SUCCESS;
    }
    return OPENDAQ_IGNORED;
}

extern "C" ErrCode PUBLIC_EXPORT createMdnsDiscoveryServer(IDiscoveryServer** objTmp, ILogger* logger)
{
    return daq::createObject<IDiscoveryServer, MdnsDiscoveryServerImpl>(objTmp, logger);
}

END_NAMESPACE_OPENDAQ
