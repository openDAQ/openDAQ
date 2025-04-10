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

#pragma once
#include <coretypes/stringobject.h>
#include <opendaq/device_info.h>
#include <opendaq/logger.h>
#include <coretypes/listobject.h>
#include <coretypes/function.h>
#include <opendaq/device.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_utility
 * @addtogroup opendaq_discovery_service Discovery service
 * @{
 */

/*#
 * [interfaceLibrary(IPropertyObject, "coreobjects")]
 */

DECLARE_OPENDAQ_INTERFACE(IDiscoveryServer, IBaseObject)
{
    virtual ErrCode INTERFACE_FUNC registerService(IString* id, IPropertyObject* config, IDeviceInfo* deviceInfo) = 0;
    virtual ErrCode INTERFACE_FUNC unregisterService(IString* id) = 0;
    virtual ErrCode INTERFACE_FUNC setRootDevice(IDevice* device) = 0;
};
/*!@}*/

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, 
    MdnsDiscoveryServer, IDiscoveryServer,
    ILogger*, logger)

END_NAMESPACE_OPENDAQ
