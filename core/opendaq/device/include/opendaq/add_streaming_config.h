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
#include <coreobjects/property_object.h>
#include <opendaq/streaming_type.h>

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [templated(defaultAliasName: AddStreamingConfigPtr)]
 * [interfaceSmartPtr(IAddStreamingConfig, GenericAddStreamingConfigPtr)]
 * [interfaceLibrary(IPropertyObject, "coreobjects")]
 * [interfaceSmartPtr(IStreamingType, StreamingTypePtr, "<opendaq/streaming_type_ptr.h>")]
 */

/*!
 * @ingroup opendaq_devices
 * @addtogroup opendaq_add_device_config Add device config
 * @{
 */

/*!
 * @brief The configuration of a streaming connection of a Streaming type.
 *
 * The Streaming type is fixed when the config is created.
 * The type's settings are the `TypeConfig` child object, created from the type's default config.
 */
DECLARE_OPENDAQ_INTERFACE(IAddStreamingConfig, IBaseObject)
{
    /*!
     * @brief Gets the Streaming type the config is created for.
     * @param[out] type The Streaming type
     */
    virtual ErrCode INTERFACE_FUNC getStreamingType(IStreamingType** type) = 0;

    /*!
     * @brief Gets the configuration of the Streaming type.
     * @param[out] config The Streaming type configuration
     */
    virtual ErrCode INTERFACE_FUNC getTypeConfig(IPropertyObject** config) = 0;
};

/*!@}*/

/*!
 * @ingroup opendaq_add_device_config
 * @addtogroup opendaq_add_device_config_factories Factories
 * @{
 */

/*!
 * @brief Creates a streaming config with the default config of the Streaming type.
 */
OPENDAQ_DECLARE_CLASS_FACTORY(
    LIBRARY_FACTORY, AddStreamingConfig,
    IStreamingType*, streamingType
)

/*!@}*/

END_NAMESPACE_OPENDAQ
