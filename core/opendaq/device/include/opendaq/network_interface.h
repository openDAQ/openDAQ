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
#include <coretypes/common.h>
#include <coretypes/stringobject.h>
#include <coreobjects/property_object.h>

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [interfaceLibrary(IPropertyObject, "coreobjects")]
 */

/*!
 * @ingroup opendaq_devices
 * @addtogroup opendaq_device_info Device info
 * @{
 */

/*!
 * @brief Provides an interface to manipulate the configuration of a device's (server's) network interface.
 * Offers methods to update the IP configuration and retrieve the currently active one, if the corresponding feature
 * supported by the device. Additionally, includes a helper method to create a prebuilt property object with valid default configuration.
 *
 * The configuration property object, passed as a parameter to said methods, should include the following properties:
 *
 * - **"dhcp4"**: A boolean property indicating whether DHCP is enabled for IPv4. Defaults to `True` (DHCP enabled).
 *   If set to `False` (DHCP disabled), non-empty static configuration properties are required.
 * - **"address4"**: A string property specifying the statically assigned IPv4 address in the format `address/netmask` (e.g. 192.168.1.2/24).
 *   This property is ignored when DHCP is enabled for IPv4. However, if DHCP is disabled, the list must include at least one address. Defaults to an empty string.
 * - **"gateway4"**: A string property specifying the IPv4 gateway address. This is required if DHCP is disabled and ignored otherwise. Defaults to an empty string.
 * - **"dhcp6"**, **"address6"**, **"gateway6"**: These properties follow the same format and rules as their IPv4 counterparts but apply to IPv6 configuration.
 */

DECLARE_OPENDAQ_INTERFACE(INetworkInterface, IBaseObject)
{
    /*!
     * @brief Requests the currently active configuration for the network interface.
     * @param[out] config The property object containing the currently active configuration.
     * @retval OPENDAQ_ERR_NOTIMPLEMENTED if the device doesn't support retrieving the active configuration.
     */
    virtual ErrCode INTERFACE_FUNC requestCurrentConfiguration(IPropertyObject** config) = 0;

    /*!
     * @brief Submits a new configuration for the network interface.
     * @param config The new configuration to apply.
     *
     * The provided configuration must adhere to the required properties, including "dhcp4", "address4", "gateway4", and their IPv6 equivalents,
     * as described in the class-level documentation.
     */
    virtual ErrCode INTERFACE_FUNC submitConfiguration(IPropertyObject* config) = 0;

    /*!
     * @brief Creates a property object containing default configuration values for a network interface.
     * @param[out] defaultConfig The configuration object containing default settings for configuring device's network interface.
     *
     * The created object can be modified or directly submitted using the `submitConfiguration` method.
     */
    virtual ErrCode INTERFACE_FUNC createDefaultConfiguration(IPropertyObject** defaultConfig) = 0;
};

/*!@}*/

OPENDAQ_DECLARE_CLASS_FACTORY(
    LIBRARY_FACTORY, NetworkInterface,
    IString*, name,
    IString*, ownerDeviceManufacturerName,
    IString*, ownerDeviceSerialNumber,
    IBaseObject*, moduleManager
)

END_NAMESPACE_OPENDAQ
