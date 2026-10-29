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
#include <coretypes/dictobject.h>
#include <opendaq/device_type.h>
#include <opendaq/add_streaming_options.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_add_device_config
 * @{
 */

/*!
 * @brief The type of IP address preferred when connecting to a device.
 */
enum class AddressType : EnumType
{
    IPv4 = 0,
    IPv6
};

/*#
 * [templated(defaultAliasName: AddDeviceConfigPtr)]
 * [interfaceSmartPtr(IAddDeviceConfig, GenericAddDeviceConfigPtr)]
 * [interfaceLibrary(IPropertyObject, "coreobjects")]
 * [interfaceSmartPtr(IDeviceType, GenericDeviceTypePtr, "<opendaq/device_type_ptr.h>", true)]
 */

/*!
 * @brief The configuration used when adding a device of a Device type.
 *
 * The Device type is fixed when the config is created, and its ID is the read-only `DeviceType` property.
 * The nested config objects are created with the config and never replaced.
 * The `Streaming` child object holds the streaming options, and exists only for Mirrored Device types.
 */
DECLARE_OPENDAQ_INTERFACE(IAddDeviceConfig, IBaseObject)
{
    /*!
     * @brief Gets the Device type the config is created for.
     * @param[out] type The Device type
     */
    virtual ErrCode INTERFACE_FUNC getDeviceType(IDeviceType** type) = 0;

    /*!
     * @brief Gets the configuration of the Device type.
     * @param[out] config The Device type configuration
     */
    virtual ErrCode INTERFACE_FUNC getTypeConfig(IPropertyObject** config) = 0;

    /*!
     * @brief Gets the preferred type of address used to connect to the device.
     * @param[out] type The address type
     *
     * IPv4 by default.
     * Addresses of the preferred type are tried first.
     */
    virtual ErrCode INTERFACE_FUNC getPrimaryAddressType(AddressType* type) = 0;

    /*!
     * @brief Sets the preferred type of address used to connect to the device.
     * @param type The address type
     */
    virtual ErrCode INTERFACE_FUNC setPrimaryAddressType(AddressType type) = 0;

    /*!
     * @brief Gets whether adding the device scans the network when the device list is missing or outdated.
     * @param[out] scanOnAdd True when the add scans the network
     */
    virtual ErrCode INTERFACE_FUNC getScanOnAdd(Bool* scanOnAdd) = 0;

    /*!
     * @brief Sets whether adding the device scans the network when the device list is missing or outdated.
     * @param scanOnAdd True when the add scans the network
     */
    virtual ErrCode INTERFACE_FUNC setScanOnAdd(Bool scanOnAdd) = 0;

    /*!
     * @brief Gets the streaming options.
     * @param[out] options The streaming options, or null when the Device type is not Mirrored
     */
    virtual ErrCode INTERFACE_FUNC getStreamingOptions(IAddStreamingOptions** options) = 0;
};

/*!@}*/

/*!
 * @ingroup opendaq_add_device_config
 * @addtogroup opendaq_add_device_config_factories Factories
 * @{
 */

// [templateType(streamingTypes, IString, IStreamingType)]
/*!
 * @brief Creates an add device config with the default config of the Device type.
 */
OPENDAQ_DECLARE_CLASS_FACTORY(
    LIBRARY_FACTORY, AddDeviceConfig,
    IDeviceType*, deviceType,
    IDict*, streamingTypes,
    Bool, scanOnAdd
)

/*!@}*/

END_NAMESPACE_OPENDAQ
