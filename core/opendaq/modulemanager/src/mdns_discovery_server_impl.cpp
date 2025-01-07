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

BEGIN_NAMESPACE_OPENDAQ

MdnsDiscoveryServerImpl::MdnsDiscoveryServerImpl(const LoggerPtr& logger,
                                                 const ListPtr<IString>& netInterfaceNames,
                                                 const ProcedurePtr& modifyIpConfigCallback,
                                                 const FunctionPtr& retrieveIpConfigCallback)
    : loggerComponent(logger.getOrAddComponent("MdnsDiscoveryServerImpl"))
    , netInterfaceNames(netInterfaceNames)
    , modifyIpConfigCallback(modifyIpConfigCallback)
    , retrieveIpConfigCallback(retrieveIpConfigCallback)
{
    this->ipModificationEnabled =
        verifyIpModificationServiceParameters(this->netInterfaceNames, this->modifyIpConfigCallback, this->retrieveIpConfigCallback);
}

ErrCode MdnsDiscoveryServerImpl::registerService(IString* id, IPropertyObject* config, IDeviceInfo* deviceInfo)
{
    using namespace discovery_common;
    using namespace discovery_server;

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

    TxtProperties properties;
    properties["caps"] = serviceCap.asPtr<IString>(true).toStdString();

    properties["name"] = "";
    properties["manufacturer"] = "";
    properties["model"] = "";
    properties["serialNumber"] = "";
    properties["path"] = "/";
    properties["protocolVersion"] = "";

    if (deviceInfoPtr.assigned())
    {
        properties["name"] = deviceInfoPtr.getName().toStdString();
        properties["manufacturer"] = deviceInfoPtr.getManufacturer().toStdString();
        properties["model"] = deviceInfoPtr.getModel().toStdString();
        properties["serialNumber"] = deviceInfoPtr.getSerialNumber().toStdString();

        if (ipModificationEnabled && !discoveryServer.isServiceRegistered(IpModificationUtils::DAQ_IP_MODIFICATION_SERVICE_ID))
            registerIpModificationService(deviceInfoPtr);
    }

    if (configPtr.hasProperty("Path"))
        properties["path"] = configPtr.getPropertyValue("Path").asPtr<IString>().toStdString();
    
    if (configPtr.hasProperty("ProtocolVersion"))
        properties["protocolVersion"] = configPtr.getPropertyValue("ProtocolVersion").asPtr<IString>().toStdString();

    MdnsDiscoveredService service(serviceName, servicePort, properties);
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

void MdnsDiscoveryServerImpl::registerIpModificationService(const DeviceInfoPtr& deviceInfo)
{
    using namespace discovery_common;
    using namespace discovery_server;

    TxtProperties properties;
    properties["caps"] = "OPENDAQ_IPC";
    properties["path"] = "/";
    properties["protocolVersion"] = IpModificationUtils::DAQ_IP_MODIFICATION_SERVICE_VERSION;

    std::string interfaces;
    for(const auto& ifaceName : netInterfaceNames)
        interfaces += ifaceName.toStdString();
    properties["interfaces"] = interfaces;

    if (deviceInfo.assigned())
    {
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

    ModifyIpConfigCallback modifyIpConfigCb = [this](const std::string& ifaceName, const TxtProperties& reqProps)
    {
        TxtProperties resProps;
        try
        {
            auto config = IpModificationUtils::populateIpConfigProperties(reqProps);
            modifyIpConfigCallback(ifaceName, config);
            resProps["ErrorCode"] = std::to_string(OPENDAQ_SUCCESS);
            resProps["ErrorMessage"] = "";
        }
        catch (const DaqException& e)
        {
            resProps["ErrorCode"] = std::to_string(e.getErrCode());
            resProps["ErrorMessage"] = e.what();
        }
        catch (const std::exception& e)
        {
            resProps["ErrorCode"] = std::to_string(OPENDAQ_ERR_GENERALERROR);
            resProps["ErrorMessage"] = e.what();
        }
        return resProps;
    };
    RetrieveIpConfigCallback retrieveIpConfigCb = [this](const std::string& ifaceName)
    {
        TxtProperties resProps;
        try
        {
            PropertyObjectPtr config = retrieveIpConfigCallback(ifaceName);
            IpModificationUtils::encodeIpConfiguration(config, resProps);

            resProps["ErrorCode"] = std::to_string(OPENDAQ_SUCCESS);
            resProps["ErrorMessage"] = "";
        }
        catch (const DaqException& e)
        {
            resProps["ErrorCode"] = std::to_string(e.getErrCode());
            resProps["ErrorMessage"] = e.what();
        }
        catch (const std::exception& e)
        {
            resProps["ErrorCode"] = std::to_string(OPENDAQ_ERR_GENERALERROR);
            resProps["ErrorMessage"] = e.what();
        }
        return resProps;
    };

    MdnsDiscoveredService service(IpModificationUtils::DAQ_IP_MODIFICATION_SERVICE_NAME, MDNS_PORT, properties);
    if (discoveryServer.registerIpModificationService(service, modifyIpConfigCb, retrieveIpConfigCb))
    {
        LOG_I("IP modification service registered with the discovery server");
    }
    else
    {
        LOG_E("Failed to register IP modification service with the discovery server");
    }
}

bool MdnsDiscoveryServerImpl::verifyIpModificationServiceParameters(const ListPtr<IString>& netInterfaceNames,
                                                                    const ProcedurePtr& modifyIpConfigCallback,
                                                                    const FunctionPtr& retrieveIpConfigCallback)
{
    if (netInterfaceNames.assigned() || modifyIpConfigCallback.assigned() || retrieveIpConfigCallback.assigned())
    {
        if (!modifyIpConfigCallback.assigned() || !netInterfaceNames.assigned() || netInterfaceNames.empty())
        {
            throw InvalidParameterException("Incomplete parameters for IP modification service");
        }
        else
        {
            for (const auto& ifaceName : netInterfaceNames)
            {
                if (!ifaceName.assigned() || ifaceName == "")
                    throw InvalidParameterException("Incomplete parameters for IP modification service");
            }
            return true;
        }
    }
    else
    {
        return false;
    }
}

#if !defined(BUILDING_STATIC_LIBRARY)

extern "C" ErrCode PUBLIC_EXPORT createMdnsDiscoveryServer(IDiscoveryServer** objTmp,
                                                           ILogger* logger,
                                                           IList* netInterfaceNames,
                                                           IProcedure* modifyIpConfigCallback,
                                                           IFunction* retrieveIpConfigCallback)
{
    return daq::createObject<IDiscoveryServer, MdnsDiscoveryServerImpl>(objTmp,
                                                                        logger,
                                                                        netInterfaceNames,
                                                                        modifyIpConfigCallback,
                                                                        retrieveIpConfigCallback);
}

#endif

END_NAMESPACE_OPENDAQ
