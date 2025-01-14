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

#include <coreobjects/property_object.h>

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [interfaceSmartPtr(IPropertyObject, PropertyObjectPtr, "<coreobjects/property_object_ptr.h>")]
 */

/*!
 * @ingroup opendaq_devices
 * @addtogroup opendaq_device Device network config
 * @{
 */

DECLARE_OPENDAQ_INTERFACE(IDeviceNetworkConfig, IBaseObject)
{
    virtual ErrCode INTERFACE_FUNC submitNetworkConfiguration(IString* ifaceName, IPropertyObject* config) = 0;
    virtual ErrCode INTERFACE_FUNC retrieveNetworkConfiguration(IString* ifaceName, IPropertyObject** config) = 0;
    virtual ErrCode INTERFACE_FUNC getNetworkConfigurationEnabled(Bool* enabled) = 0;

    // [templateType(ifaceNames, IString)]
    virtual ErrCode INTERFACE_FUNC getNetworkInterfaceNames(IList** ifaceNames) = 0;
};
/*!@}*/

END_NAMESPACE_OPENDAQ
