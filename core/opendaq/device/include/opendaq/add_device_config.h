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
 * The transport the device is added over is one of the config's transport options. For a Device type with options, the
 * config's `TransportOption` property holds the id of the selected option, and its selection values are the ids of the
 * options. The `Port` property holds the port set, or -1 while none is set; a port set is used instead of the port of
 * whichever option is selected, even when it is the same, until setPort is called with -1.
 *
 * When the device is added, it is connected at the first of these ports:
 * - the port set on the config, by setPort or by writing the `Port` property. For a device info without a server
 *   capability for the Device type, createAddDeviceConfig sets the port its connection string names, unless it is the
 *   port of one of the options;
 * - the port the connection string names, unless it is the port of an option the config lists or the discovered server
 *   announces: such a port is the one a discovered connection string carries, not a port to connect at;
 * - the port the server announces for the selected option, when discovery reports the device;
 * - the port of the selected option, as getAvailableTransportOptions lists it.
 *
 * The module gets the selected option and that port as the "TransportOption" and "Port" properties of the type config
 * (see `TransportOptionConfigProperty` and `PortConfigProperty`), in place of a port its own default config declares,
 * and a connection string that names the same port.
 *
 * Adding the device with the config fails with OPENDAQ_ERR_INVALIDPARAMETER when a port is set and the connection
 * string names another port that is not the port of an option the config lists or the discovered server announces, and
 * when the prefix of the connection string belongs to another Device type. A `daq://` connection string stands for the
 * discovered server capability of the Device type.
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
     * @param[out] transportOptions The options of the device info's server capability for the Device type, with the
     * ports the server announces, keyed by their own ids, the server's preferred first in dict iteration order. The
     * options of the Device type when the device info has no such capability. Empty for a Device type without
     * options.
     */
    virtual ErrCode INTERFACE_FUNC getAvailableTransportOptions(IDict** transportOptions) = 0;

    /*!
     * @brief Selects the transport option the device is added over.
     * @param id The id of one of the options of getAvailableTransportOptions.
     * @retval OPENDAQ_ERR_NOT_SUPPORTED for a Device type without transport options.
     * @retval OPENDAQ_ERR_NOTFOUND if none of the options has that id.
     *
     * A port set by setPort is kept and overrides the port of the newly selected option.
     */
    virtual ErrCode INTERFACE_FUNC setTransportOption(IString* id) = 0;

    /*!
     * @brief Gets the selected transport option.
     * @param[out] transportOption The selected option, as getAvailableTransportOptions lists it.
     * @retval OPENDAQ_ERR_NOT_SUPPORTED for a Device type without transport options.
     * @retval OPENDAQ_ERR_NOTFOUND if the selected option is none of the options.
     */
    virtual ErrCode INTERFACE_FUNC getSelectedTransportOption(ITransportOption** transportOption) = 0;

    /*!
     * @brief Sets the port the device is added on.
     * @param port The port, used instead of the one the selected option lists whichever option is selected, even when
     * it is the same; -1 to use the port of the selected option again.
     * @retval OPENDAQ_ERR_NOT_SUPPORTED for a Device type without transport options.
     * @retval OPENDAQ_ERR_INVALIDPARAMETER for a negative port other than -1.
     */
    virtual ErrCode INTERFACE_FUNC setPort(Int port) = 0;

    /*!
     * @brief Gets the port the device is added on.
     * @param[out] port The port set by setPort. Otherwise the port of the selected transport option: the one the
     * device info's server capability announces for it, or the Device type's default port when the device info has
     * no such capability.
     * @retval OPENDAQ_ERR_NOT_SUPPORTED for a Device type without transport options.
     *
     * For a device info without a server capability for the Device type, createAddDeviceConfig sets the port its
     * connection string names, unless it is the port of one of the options. Unless a port is set, a port the
     * connection string names, or else a port discovery reports for the selected option, takes precedence when the
     * device is added (see the class description).
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
/*!
 * @brief Creates an add device config with the default config of the Device type.
 * @retval OPENDAQ_ERR_INVALIDPARAMETER when the Device type is Mirrored or Unspecified and a Streaming type ID is reserved.
 */
//[factory(Hide)]
OPENDAQ_DECLARE_CLASS_FACTORY(
    LIBRARY_FACTORY, AddDeviceConfig,
    IDeviceType*, deviceType,
    IDict*, streamingTypes,
    Bool, scanOnAdd
)

/*!@}*/

END_NAMESPACE_OPENDAQ
