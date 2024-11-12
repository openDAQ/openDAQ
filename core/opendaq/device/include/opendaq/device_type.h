/*
 * Copyright 2022-2024 openDAQ d.o.o.
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
#include <opendaq/component_type.h>

BEGIN_NAMESPACE_OPENDAQ

/*#
* [templated(defaultAliasName: DeviceTypePtr)]
* [interfaceSmartPtr(IDeviceType, GenericDeviceTypePtr)]
* [interfaceSmartPtr(IComponentType, GenericComponentTypePtr, "<opendaq/component_type_ptr.h>")]
* [interfaceLibrary(IComponentType, "opendaq")]
*/

/*!
 * @ingroup opendaq_devices
 * @addtogroup opendaq_device_type Device type
 * @{
 */

/*!
 * @brief Provides information on what device type can be created by a given module. Can be used
 * to obtain the default configuration used when either adding/creating a new device.
 */
DECLARE_OPENDAQ_INTERFACE(IDeviceType, IComponentType)
{
    /*
     * @brief Gets the prefix found in connection strings used to create a device of the given type.
     * @param[out] prefix The connection string prefix.
     *
     * The prefix is always found at the start of the connection string, before the "://" delimiter.
     */
    virtual ErrCode INTERFACE_FUNC getConnectionStringPrefix(IString** prefix) = 0;
};
/*!@}*/

/*!
 * @ingroup opendaq_device_type
 * @addtogroup opendaq_device_type_factories Factories
 * @{
 */

/*!
 * @brief Creates a Device type object, with the id, name, description and optional defaultConfig.
 * @param id The unique type ID of the device.
 * @param name The name of the device type.
 * @param description A short description of the device type.
 * @param defaultConfig The property object, to be cloned and returned, each time user creates default
 * configuration object. This way each instance of the device has its own configuration object.
 */
OPENDAQ_DECLARE_CLASS_FACTORY(
    LIBRARY_FACTORY, DeviceType,
    IString*, id,
    IString*, name,
    IString*, description,
    IPropertyObject*, defaultConfig,
    IString*, prefix
)

/*!@}*/
END_NAMESPACE_OPENDAQ
