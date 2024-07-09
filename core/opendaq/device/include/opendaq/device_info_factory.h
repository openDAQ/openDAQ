/*
 * Copyright 2022-2024 openDAQ d. o. o.
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
#include <opendaq/context_ptr.h>
#include <opendaq/device_info_config_ptr.h>
#include <opendaq/server_capability_config_ptr.h>

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
 * @param customSdkVersion Used when the device uses a custom SDK version. Optional parameter.
 */
inline DeviceInfoConfigPtr DeviceInfo(const StringPtr& connectionString, const StringPtr& name = "", const StringPtr& customSdkVersion = nullptr)
{
    DeviceInfoConfigPtr obj(DeviceInfoConfigWithCustomSdkVersion_Create(name, connectionString, customSdkVersion));
    return obj;
}

inline ServerCapabilityConfigPtr ServerCapability(const StringPtr& protocolId, const StringPtr& protocolName, ProtocolType protocolType)
{
    ServerCapabilityConfigPtr obj(ServerCapability_Create(protocolId, protocolName, protocolType));
    return obj;
}

/*!@}*/
END_NAMESPACE_OPENDAQ
