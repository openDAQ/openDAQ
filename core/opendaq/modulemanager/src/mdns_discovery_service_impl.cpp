/*
 * Copyright 2022-2024 Blueberry d.o.o.
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

#include <opendaq/mdns_discovery_service_impl.h>
#include <opendaq/custom_log.h>

BEGIN_NAMESPACE_OPENDAQ

MdnsDiscoveryServiceImpl::MdnsDiscoveryServiceImpl(const LoggerPtr& logger)
    : loggerComponent(logger.getOrAddComponent("MdnsDiscoveryServiceImpl"))
{
}

ErrCode MdnsDiscoveryServiceImpl::registerService(IString* id, IPropertyObject* config, IDeviceInfo* deviceInfo)
{
    if (id == nullptr || config == nullptr)
        return OPENDAQ_IGNORED;

    if (discoveryServer.registerDevice(id, config, deviceInfo))
    {
        LOG_I("Server \"{}\" registered in the discovery service", StringPtr::Borrow(id));
        return OPENDAQ_SUCCESS;
    }
    return OPENDAQ_IGNORED;
}

ErrCode MdnsDiscoveryServiceImpl::unregisterService(IString* id)
{
    if (id == nullptr)
        return OPENDAQ_IGNORED;

    if (discoveryServer.removeDevice(id))
    {
        LOG_I("Server \"{}\" removed from the discovery service", StringPtr::Borrow(id));
        return OPENDAQ_SUCCESS;
    }
    return OPENDAQ_IGNORED;
}

ErrCode MdnsDiscoveryServiceImpl::getName(IString** name)
{
    if (name == nullptr)
        return OPENDAQ_IGNORED;

    *name = String("mdns").detach();
    return OPENDAQ_SUCCESS;
}

extern "C" ErrCode PUBLIC_EXPORT createMdnsDiscoveryService(IDiscoveryService** objTmp, ILogger* logger)
{
    return daq::createObject<IDiscoveryService, MdnsDiscoveryServiceImpl>(objTmp, logger);
}

END_NAMESPACE_OPENDAQ
