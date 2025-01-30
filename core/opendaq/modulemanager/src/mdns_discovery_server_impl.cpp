/*
 * Copyright 2022-2025 openDAQ d.o.o.
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
#include <coreobjects/property_object_factory.h>
#include <opendaq/device_info_ptr.h>
#include <coreobjects/property_factory.h>
#include <opendaq/device_network_config_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

MdnsDiscoveryServerImpl::MdnsDiscoveryServerImpl(const LoggerPtr& logger)
    : loggerComponent(logger.getOrAddComponent("MdnsDiscoveryServerImpl"))
{
}

std::string GetPropertyValueOrDefault(const daq::PropertyObjectPtr& propObj, const daq::StringPtr& key, const std::string& defaultValue)
{
    daq::BaseObjectPtr value;
    auto errCode = propObj->getPropertyValue(key, &value);
    if (OPENDAQ_FAILED(errCode))
        return defaultValue;
    return std::string(value);
}

ErrCode MdnsDiscoveryServerImpl::registerService(IString* id, IPropertyObject* config, IDeviceInfo* deviceInfo)
{
    using namespace discovery_common;
    using namespace discovery_server;

    if (!id)
        return OPENDAQ_ERR_ARGUMENT_NULL;
    if (!config)
        return OPENDAQ_IGNORED;
    if (!deviceInfo)
        return OPENDAQ_IGNORED;

    auto serviceId = StringPtr::Borrow(id);
    auto configPtr = PropertyObjectPtr::Borrow(config);

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

    TxtProperties properties;
    properties["caps"] = std::string(configPtr.getPropertyValue("ServiceCap"));
    properties["path"] = GetPropertyValueOrDefault(configPtr, "Path", "/");
    properties["protocolVersion"] = GetPropertyValueOrDefault(configPtr, "ProtocolVersion", "");

    MdnsDiscoveredService service(serviceName, servicePort, properties, PropertyObjectPtr::Borrow(deviceInfo));
    if (discoveryServer.registerService(serviceId, service))
    {
        LOG_I("Service \"{}\" registered with the discovery server", serviceId);
        return OPENDAQ_SUCCESS;
    }
    return OPENDAQ_ERR_INVALIDSTATE;
}

ErrCode MdnsDiscoveryServerImpl::unregisterService(IString* id)
{
    if (id == nullptr)
        return OPENDAQ_IGNORED;

    if (discoveryServer.unregisterService(StringPtr::Borrow(id)))
    {
        LOG_I("Service \"{}\" removed from the discovery server", StringPtr::Borrow(id));
        return OPENDAQ_SUCCESS;
    }
    return OPENDAQ_IGNORED;
}

ErrCode MdnsDiscoveryServerImpl::setRootDevice(IDevice* device)
{
    DevicePtr devicePtr = DevicePtr::Borrow(device);

    return daqTry([&]()
        {
            using namespace discovery_common;
            if (discoveryServer.isServiceRegistered(IpModificationUtils::DAQ_IP_MODIFICATION_SERVICE_ID))
                discoveryServer.unregisterIpModificationService();

            if (devicePtr.assigned() && devicePtr.asPtr<IDeviceNetworkConfig>().getNetworkConfigurationEnabled())
                registerIpModificationService(devicePtr);
            return OPENDAQ_SUCCESS;
        });
}

void MdnsDiscoveryServerImpl::registerIpModificationService(const DevicePtr& rootDevice)
{
    using namespace discovery_common;
    using namespace discovery_server;

    TxtProperties properties;
    properties["caps"] = "OPENDAQ_IPC";
    properties["path"] = "/";
    properties["protocolVersion"] = IpModificationUtils::DAQ_IP_MODIFICATION_SERVICE_VERSION;

    std::string interfaces;
    for(const auto& ifaceName : rootDevice.asPtr<IDeviceNetworkConfig>().getNetworkInterfaceNames())
        interfaces += ifaceName.getLength() == 0 ? "" : (ifaceName.toStdString() + ";");
    if (interfaces.empty())
    {
        LOG_W("Cannot register IP modification service without at least one non-empty interface name specified");
        return;
    }
    properties["interfaces"] = interfaces;

    if (const auto deviceInfo = rootDevice.getInfo(); deviceInfo.assigned())
    {
        if (deviceInfo.getManufacturer().getLength() == 0 || deviceInfo.getSerialNumber().getLength() == 0)
        {
            LOG_W("Cannot register IP modification service without \"manufacturer\" and \"serialNumber\" "
                  "specified within device info");
            return;
        }
        properties["name"] = deviceInfo.getName().toStdString();
        properties["manufacturer"] = deviceInfo.getManufacturer().toStdString();
        properties["model"] = deviceInfo.getModel().toStdString();
        properties["serialNumber"] = deviceInfo.getSerialNumber().toStdString();
    }
    else
    {
        LOG_W("Cannot register IP modification service without device info specified");
        return;
    }

    rootDeviceRef = rootDevice;

    ModifyIpConfigCallback modifyIpConfigCb = [this](const std::string& ifaceName, const TxtProperties& reqProps)
    {
        DevicePtr rootDevice = rootDeviceRef.assigned() ? rootDeviceRef.getRef() : nullptr;

        TxtProperties resProps;
        if (rootDevice.assigned() && rootDevice.asPtr<IDeviceNetworkConfig>().getNetworkConfigurationEnabled())
        {
            DeviceNetworkConfigPtr deviceNetworkConfig = rootDevice.asPtr<IDeviceNetworkConfig>();
            try
            {
                auto config = IpModificationUtils::populateIpConfigProperties(reqProps);
                deviceNetworkConfig.submitNetworkConfiguration(ifaceName, config);
                resProps[IpModificationUtils::ERROR_CODE_KEY] = std::to_string(OPENDAQ_SUCCESS);
                resProps[IpModificationUtils::ERROR_MESSAGE_KEY] = "";
            }
            catch (const DaqException& e)
            {
                resProps[IpModificationUtils::ERROR_CODE_KEY] = std::to_string(e.getErrCode());
                resProps[IpModificationUtils::ERROR_MESSAGE_KEY] = DiscoveryUtils::toTxtValue(e.what(), IpModificationUtils::ERROR_MESSAGE_LENGTH);
            }
            catch (const std::exception& e)
            {
                resProps[IpModificationUtils::ERROR_CODE_KEY] = std::to_string(OPENDAQ_ERR_GENERALERROR);
                resProps[IpModificationUtils::ERROR_MESSAGE_KEY] = DiscoveryUtils::toTxtValue(e.what(), IpModificationUtils::ERROR_MESSAGE_LENGTH);
            }
        }
        else
        {
            resProps[IpModificationUtils::ERROR_CODE_KEY] = std::to_string(OPENDAQ_ERR_NOTIMPLEMENTED);
            resProps[IpModificationUtils::ERROR_MESSAGE_KEY] = "";
        }
        return resProps;
    };
    RetrieveIpConfigCallback retrieveIpConfigCb = [this](const std::string& ifaceName)
    {
        DevicePtr rootDevice = rootDeviceRef.assigned() ? rootDeviceRef.getRef() : nullptr;

        TxtProperties resProps;
        if (rootDevice.assigned() && rootDevice.asPtr<IDeviceNetworkConfig>().getNetworkConfigurationEnabled())
        {
            DeviceNetworkConfigPtr deviceNetworkConfig = rootDevice.asPtr<IDeviceNetworkConfig>();
            try
            {
                PropertyObjectPtr config = deviceNetworkConfig.retrieveNetworkConfiguration(ifaceName);
                IpModificationUtils::encodeIpConfiguration(config, resProps);

                resProps[IpModificationUtils::ERROR_CODE_KEY] = std::to_string(OPENDAQ_SUCCESS);
                resProps[IpModificationUtils::ERROR_MESSAGE_KEY] = "";
            }
            catch (const DaqException& e)
            {
                resProps[IpModificationUtils::ERROR_CODE_KEY] = std::to_string(e.getErrCode());
                resProps[IpModificationUtils::ERROR_MESSAGE_KEY] = DiscoveryUtils::toTxtValue(e.what(), IpModificationUtils::ERROR_MESSAGE_LENGTH);
            }
            catch (const std::exception& e)
            {
                resProps[IpModificationUtils::ERROR_CODE_KEY] = std::to_string(OPENDAQ_ERR_GENERALERROR);
                resProps[IpModificationUtils::ERROR_MESSAGE_KEY] = DiscoveryUtils::toTxtValue(e.what(), IpModificationUtils::ERROR_MESSAGE_LENGTH);
            }
        }
        else
        {
            resProps[IpModificationUtils::ERROR_CODE_KEY] = std::to_string(OPENDAQ_ERR_NOTIMPLEMENTED);
            resProps[IpModificationUtils::ERROR_MESSAGE_KEY] = "";
        }
        return resProps;
    };

    MdnsDiscoveredService service(IpModificationUtils::DAQ_IP_MODIFICATION_SERVICE_NAME, MDNS_PORT, properties, rootDevice.getInfo());
    if (discoveryServer.registerIpModificationService(service, modifyIpConfigCb, retrieveIpConfigCb))
    {
        LOG_I("IP modification service registered with the discovery server");
    }
    else
    {
        LOG_E("Failed to register IP modification service with the discovery server");
    }
}

#if !defined(BUILDING_STATIC_LIBRARY)

extern "C" ErrCode PUBLIC_EXPORT createMdnsDiscoveryServer(IDiscoveryServer** objTmp, ILogger* logger)
{
    return daq::createObject<IDiscoveryServer, MdnsDiscoveryServerImpl>(objTmp, logger);
}

#endif

END_NAMESPACE_OPENDAQ
