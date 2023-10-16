/*
 * Copyright 2022-2023 Blueberry d.o.o.
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
#include <opendaq/device_info_config_ptr.h>

BEGIN_NAMESPACE_OPENDAQ
/*!
 * @ingroup opendaq_device_info
 * @addtogroup opendaq_device_info_factories Factories
 * @{
 */

/*!
 * @brief Creates a DeviceInfoConfig with a connection string and optional name.
 * @param connectionString String used to connect to the device.
 * @param name The name of the device. Optional parameter.
 */
inline DeviceInfoConfigPtr DeviceInfo(const StringPtr& connectionString, const StringPtr& name = "")
{
    DeviceInfoConfigPtr obj(DeviceInfoConfig_Create(name, connectionString));
    return obj;
}
/*!@}*/
END_NAMESPACE_OPENDAQ
