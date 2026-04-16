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
#include <opendaq/device_update_options.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_update_parameters
 * @addtogroup opendaq_update Parameters
 * @{
 */

/*#
 * [interfaceLibrary(IPropertyObject, "coreobjects")]
 * [interfaceSmartPtr(IPropertyObject, GenericPropertyObjectPtr, "<coreobjects/property_object_ptr.h>")]
 * [templated(defaultAliasName: UpdateParametersPtr)]
 * [interfaceSmartPtr(IUpdateParameters, GenericUpdateParametersPtrPtr)]
 */

 /*!
 * @brief Defines how configuration should be applied to existing opendaq tree.
 */
enum class ConfigurationLoadMode : EnumType
{
    Exact = 0,    ///< Replicate configuration exactly - if a connected device is not mentioned in the configuration it will be removed.
    Merge,  ///< Apply the configuration on top of existing state. Devices won't be removed unless configured so explicitly.
};

/*!
 * @brief IUpdateParameters interface provides a set of methods to give user flexibility to load instance configuration.
 */
DECLARE_OPENDAQ_INTERFACE(IUpdateParameters, IPropertyObject)
{
    /*!
     * @brief Gets the device update options object that allows for specifying how a device and its subdevices are to be updated.
     * @param options The device update options object.
     */
    virtual ErrCode INTERFACE_FUNC getDeviceUpdateOptions(IDeviceUpdateOptions** options) = 0;

    /*!
     * @brief Sets the device update options object that allows for specifying how a device and its subdevices are to be updated.
     * @param options The device update options object.
     */
    virtual ErrCode INTERFACE_FUNC setDeviceUpdateOptions(IDeviceUpdateOptions* options) = 0;

    /*!
     * @brief Gets the strategy with which the configuration will be loaded.
     * @param mode The configuration load mode.
     */
    virtual ErrCode INTERFACE_FUNC getConfigurationLoadMode(ConfigurationLoadMode* mode) = 0;

    /*!
     * @brief Sets the strategy with which the configuration will be loaded.
     * @param mode The configuration load mode.
     */
    virtual ErrCode INTERFACE_FUNC setConfigurationLoadMode(ConfigurationLoadMode mode) = 0;
};
/*!@}*/

/*!
 * @ingroup opendaq_update_parameters
 * @addtogroup opendaq_update_parameters Factories
 * @{
 */


OPENDAQ_DECLARE_CLASS_FACTORY(
    LIBRARY_FACTORY, UpdateParameters
)

/*!@}*/

END_NAMESPACE_OPENDAQ
