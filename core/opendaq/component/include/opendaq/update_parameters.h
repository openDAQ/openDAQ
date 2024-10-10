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

#include <coreobjects/property_object.h>
#include <opendaq/tags.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_update_parameters
 * @addtogroup opendaq_update Paramerters
 * @{
 */

/*#
 * [interfaceLibrary(IPropertyObject, "coreobjects")]
 * [interfaceSmartPtr(IPropertyObject, GenericPropertyObjectPtr, "<coreobjects/property_object_ptr.h>")]
 * [templated(defaultAliasName: UpdateParametersPtr)]
 * [interfaceSmartPtr(IUpdateParameters, GenericUpdateParametersPtrPtr)]
 */

/*!
 * @brief IUpdateParameters interface provides a set of methods to give user flexibility to load instance configuration.
 */
DECLARE_OPENDAQ_INTERFACE(IUpdateParameters, IPropertyObject)
{
    /*!
     * @brief Returns whether the re-add devices is enabled. If enabled, the devices will be re-added in update process.
     * @param[out] enabled The flag indicating whether the re-add devices is enabled.
     *
     * The configuration is set from the property `ReAddDevices` of configuration object.
     */
    virtual ErrCode INTERFACE_FUNC getReAddDevicesEnabled(Bool* enabled) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the re-add devices enabled flag.
     * @param enabled The flag indicating whether the re-add devices is enabled.
     *
     * The configuration is set to the property `ReAddDevices` of configuration object.
     */
    virtual ErrCode INTERFACE_FUNC setReAddDevicesEnabled(Bool enabled) = 0;
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
