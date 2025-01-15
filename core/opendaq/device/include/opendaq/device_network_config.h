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

/*!
 * @brief Interface for managing device network configuration information and settings.
 *
 * The interface is typically used by the discovery server, if present, to obtain the necessary
 * information for advertising the device's network configuration capabilities and handling related requests.
 *
 * The device must be designated as the root device to enable the use of this interface's methods.
 */

DECLARE_OPENDAQ_INTERFACE(IDeviceNetworkConfig, IBaseObject)
{
    /*!
     * @brief Submits a new configuration parameters to a specified network interface.
     * @param ifaceName The name of the network interface adapter as registered in the operating system.
     * Typically, this is a short symbolic identifier for the adapter, e.g. "eth0".
     * @param config The property object with new configuration parameters to submit.
     * The format of the properties matches that used in INetworkInterface.
     * @retval OPENDAQ_ERR_NOTIMPLEMENTED if the device does not support network configuration management.
     */
    virtual ErrCode INTERFACE_FUNC submitNetworkConfiguration(IString* ifaceName, IPropertyObject* config) = 0;

    /*!
     * @brief Retrieves the currently active configuration of a specified network interface.
     * @param ifaceName The name of the network interface adapter as registered in the operating system.
     * Typically, this is a short symbolic identifier for the adapter, e.g. "eth0".
     * @param[out] config The property object containing the active configuration of the network interface.
     * @retval OPENDAQ_ERR_NOTIMPLEMENTED if the device does not support retrieving network configurations.
     */
    virtual ErrCode INTERFACE_FUNC retrieveNetworkConfiguration(IString* ifaceName, IPropertyObject** config) = 0;

    /*!
     * @brief Checks if the device supports network configuration management.
     * @param[out] enabled A flag indicating whether the device supports managing network configurations.
     */
    virtual ErrCode INTERFACE_FUNC getNetworkConfigurationEnabled(Bool* enabled) = 0;

    // [templateType(ifaceNames, IString)]
    /*!
     * @brief Gets the names of all configurable network interfaces on the device.
     * @param[out] ifaceNames A list containing the names of network interface adapters available for configuration.
     * @retval OPENDAQ_ERR_NOTIMPLEMENTED if the device does not support network configuration management.
     */
    virtual ErrCode INTERFACE_FUNC getNetworkInterfaceNames(IList** ifaceNames) = 0;
};
/*!@}*/

END_NAMESPACE_OPENDAQ
