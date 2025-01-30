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
#include <opendaq/network_interface_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_device_info
 * @addtogroup opendaq_device_info_factories Factories
 * @{
 */

/*!
 * @brief Creates a Network interface object with the specified name and additional parameters.
 * @param name The name of the interface (e.g. "eth0").
 * @param ownerDeviceManufacturerName The manufacturer name of the device which owns the interface.
 * @param ownerDeviceSerialNumber The serial number of the device which owns the interface.
 * @param moduleManager The module manager.
 */
inline NetworkInterfacePtr NetworkInterface(const StringPtr& name,
                                            const StringPtr& ownerDeviceManufacturerName,
                                            const StringPtr& ownerDeviceSerialNumber,
                                            const BaseObjectPtr& moduleManager)
{
    NetworkInterfacePtr obj(NetworkInterface_Create(name, ownerDeviceManufacturerName, ownerDeviceSerialNumber, moduleManager));
    return obj;
}

/*!@}*/

END_NAMESPACE_OPENDAQ
