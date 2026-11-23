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
#include <opendaq/transport_option.h>
#include <opendaq/address_info.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_add_device_config
 * @{
 */

/*#
 * [templated(defaultAliasName: AddDeviceConfigPtr)]
 * [interfaceSmartPtr(IAddDeviceConfig, GenericAddDeviceConfigPtr)]
 * [interfaceLibrary(IPropertyObject, "coreobjects")]
 * [interfaceSmartPtr(IDeviceType, GenericDeviceTypePtr, "<opendaq/device_type_ptr.h>", true)]
 * [interfaceSmartPtr(ITransportOption, TransportOptionPtr, "<opendaq/transport_option_ptr.h>")]
 */

/*!
 * @brief The configuration used when adding a device of a Device type.
 *
 * The Device type is fixed when the config is created, and its ID is the read-only `DeviceType` property.
 * The nested config objects are created with the config and never replaced.
 * The `Streaming` child object holds the streaming options.
 * It exists only for Mirrored and Unspecified Device types with at least one listed Streaming type.
 *
 * A Device type can list several transport options, each using another transport layer for the connection. The
 * config selects one of them, and its port is the port of the selected option unless a port is set.
 *
 * The device is connected at the first of these ports:
 * 1. the port set on the config;
 * 2. the port named by the connection string the device is added with (e.g. "daq.nd://127.0.0.1:7420"), even when
 *    that string is taken from a device info;
 * 3. the port of the selected option.
 *
 * A port set and another port named by the connection string contradict each other: adding the device fails with
 * OPENDAQ_ERR_INVALIDPARAMETER.
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
     * @param[out] options The streaming options, or null when the config has no `Streaming` child object
     */
    virtual ErrCode INTERFACE_FUNC getStreamingOptions(IAddStreamingOptions** options) = 0;

    virtual ErrCode INTERFACE_FUNC getAuthenticationConfig(IAuthenticationConfig** config) = 0;

    // [templateType(transportOptions, IString, ITransportOption)]
    /*!
     * @brief Gets the transport options the device can be added over.
     * @param[out] transportOptions The options of the device info's server capability for the Device type that the
     * Device type lists as well, keyed by their own ids in the server's order, with the ports the server announces and
     * the security mode and certificate requirement of the Device type. The options of the Device type when the device
     * info has no such capability. Empty for a Device type without options, and when the server announces none of the
     * options of the Device type.
     */
    virtual ErrCode INTERFACE_FUNC getAvailableTransportOptions(IDict** transportOptions) = 0;

    /*!
     * @brief Selects the transport option the device is added over.
     * @param id The id of one of the options of getAvailableTransportOptions.
     * @retval OPENDAQ_ERR_NOTFOUND if none of the options has that id, as for a Device type without options.
     *
     * The selection is the `TransportOption` property, whose selection values are the ids of the options. A port set
     * by setPort is kept and overrides the port of the newly selected option.
     */
    virtual ErrCode INTERFACE_FUNC setTransportOption(IString* id) = 0;

    /*!
     * @brief Gets the selected transport option.
     * @param[out] transportOption The selected option, as getAvailableTransportOptions lists it; null for a Device type
     * without options.
     */
    virtual ErrCode INTERFACE_FUNC getSelectedTransportOption(ITransportOption** transportOption) = 0;

    /*!
     * @brief Sets the port the device is added on.
     * @param port The port, used instead of the one the selected option lists whichever option is selected, even when
     * it is the same; -1 to use the port of the selected option again.
     * @retval OPENDAQ_ERR_INVALIDPARAMETER for a negative port other than -1.
     *
     * The port set is the `Port` property, -1 while none is set. A Device type without options takes a port as well.
     */
    virtual ErrCode INTERFACE_FUNC setPort(Int port) = 0;

    /*!
     * @brief Gets the port the device is added on.
     * @param[out] port The port set by setPort. Otherwise the port of the selected transport option: the one the
     * device info's server capability announces for it, or the Device type's default port when the device info has
     * no such capability. -1 for a Device type without options when no port is set.
     *
     * createAddDeviceConfig does not take the port the connection string of the device info names. Unless a port is
     * set, the port named by the connection string the device is added with takes precedence (see the class
     * description).
     */
    virtual ErrCode INTERFACE_FUNC getPort(Int* port) = 0;
};

/*!@}*/

/*!
 * @ingroup opendaq_add_device_config
 * @addtogroup opendaq_add_device_config_factories Factories
 * @{
 */

// [templateType(streamingTypes, IString, IStreamingType)]
// [templateType(capabilities, IServerCapability)]
/*!
 * @brief Creates an add device config with the default config of the Device type.
 * @retval OPENDAQ_ERR_INVALIDPARAMETER when the Device type is Mirrored or Unspecified and a Streaming type ID is reserved.
 */
//[factory(Hide)]
OPENDAQ_DECLARE_CLASS_FACTORY(
    LIBRARY_FACTORY, AddDeviceConfig,
    IDeviceType*, deviceType,
    IDict*, streamingTypes,
    IList*, capabilities,
    Bool, scanOnAdd
)

/*!@}*/

END_NAMESPACE_OPENDAQ
