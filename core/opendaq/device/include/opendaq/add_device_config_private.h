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
#include <coretypes/serialized_object.h>
#include <opendaq/add_device_config.h>

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [interfaceSmartPtr(IAddDeviceConfig, AddDeviceConfigPtr, "<opendaq/add_device_config_ptr.h>")]
 */

/*!
 * @ingroup opendaq_add_device_config
 * @addtogroup opendaq_add_device_config_private Add device config private
 * @{
 */

/*!
 * @brief Provides access to private methods of the add device config.
 */
DECLARE_OPENDAQ_INTERFACE(IAddDeviceConfigPrivate, IBaseObject)
{
    // [templateType(deviceTypes, IString, IDeviceType)]
    // [templateType(streamingTypes, IString, IStreamingType)]
    /*!
     * @brief Creates a config bound by type ID to the given types, with the values of this config.
     * @param deviceTypes The available Device types, keyed by ID
     * @param streamingTypes The available Streaming types, keyed by ID
     * @param[out] bound The bound config
     * @retval OPENDAQ_ERR_NOTFOUND when the Device type is not among the given types.
     * @retval OPENDAQ_ERR_INVALIDVALUE when the values of this config don't fit the given types.
     *
     * The bound config is created from the default configs of the given types, with the values of this config applied.
     * Unset values take the current defaults of the given types.
     * Streaming types that are not among the given types are not listed, and a selection of one of them is cleared.
     */
    virtual ErrCode INTERFACE_FUNC bind(IDict* deviceTypes, IDict* streamingTypes, IAddDeviceConfig** bound) = 0;
};

/*!@}*/

//[factory(Hide)]
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, BoundAddDeviceConfig, IAddDeviceConfig,
    ISerializedObject*, saved,
    IDict*, deviceTypes,
    IDict*, streamingTypes
)

END_NAMESPACE_OPENDAQ
