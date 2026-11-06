/*
 * Copyright 2022-2026 openDAQ d.o.o.
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
#include <opendaq/add_device_config_ptr.h>
#include <opendaq/add_device_config_private.h>
#include <opendaq/add_streaming_config_ptr.h>
#include <opendaq/device_type_ptr.h>
#include <opendaq/streaming_type_ptr.h>
#include <coretypes/dict_ptr.h>
#include <coretypes/serialized_object_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_add_device_config
 * @addtogroup opendaq_add_device_config_factories Factories
 * @{
 */

/*!
 * @brief Creates an add device config with the default config of the Device type.
 * @param deviceType The Device type
 * @param streamingTypes The Streaming types listed by the streaming options, keyed by Streaming type ID
 * @param scanOnAdd The initial ScanOnAdd value
 *
 * The streaming types are ignored unless the Device type is Mirrored.
 * No Streaming type is selected.
 */
inline AddDeviceConfigPtr AddDeviceConfig(const DeviceTypePtr& deviceType,
                                          const DictPtr<IString, IStreamingType>& streamingTypes,
                                          Bool scanOnAdd)
{
    AddDeviceConfigPtr obj(AddDeviceConfig_Create(deviceType, streamingTypes, scanOnAdd));
    return obj;
}

/*!
 * @brief Creates an add device config from its saved form, bound by type ID to the given types.
 * @param saved The config as written by `serializeForUpdate`
 * @param deviceTypes The available Device types, keyed by ID
 * @param streamingTypes The available Streaming types, keyed by ID
 *
 * Binds as IAddDeviceConfigPrivate::bind does.
 */
inline AddDeviceConfigPtr BoundAddDeviceConfig(const SerializedObjectPtr& saved,
                                               const DictPtr<IString, IDeviceType>& deviceTypes,
                                               const DictPtr<IString, IStreamingType>& streamingTypes)
{
    AddDeviceConfigPtr obj(BoundAddDeviceConfig_Create(saved, deviceTypes, streamingTypes));
    return obj;
}

/*!
 * @brief Creates a streaming config with the default config of the Streaming type.
 * @param streamingType The Streaming type
 */
inline AddStreamingConfigPtr AddStreamingConfig(const StreamingTypePtr& streamingType)
{
    AddStreamingConfigPtr obj(AddStreamingConfig_Create(streamingType));
    return obj;
}

/*!@}*/

END_NAMESPACE_OPENDAQ
