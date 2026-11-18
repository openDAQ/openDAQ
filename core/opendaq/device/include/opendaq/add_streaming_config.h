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
#include <opendaq/transport_option.h>

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [templated(defaultAliasName: AddStreamingConfigPtr)]
 * [interfaceSmartPtr(IAddStreamingConfig, GenericAddStreamingConfigPtr)]
 * [interfaceLibrary(IPropertyObject, "coreobjects")]
 * [interfaceSmartPtr(IStreamingType, StreamingTypePtr, "<opendaq/streaming_type_ptr.h>")]
 * [interfaceSmartPtr(ITransportOption, TransportOptionPtr, "<opendaq/transport_option_ptr.h>")]
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
 *
 * The transport the streaming connects over is one of the config's transport options. For a Streaming type with
 * options, the config's `TransportOption` property holds the id of the selected option, and its selection values are
 * the ids of the options. The `Port` property holds the port set, or -1 while none is set; a port set is used instead
 * of the port of whichever option is selected, even when it is the same, until setPort is called with -1.
 *
 * When the streaming is created, it connects at the first of these ports:
 * - the port set on the config, by setPort or by writing the `Port` property;
 * - the port the connection string of the streaming names, unless it is the port of an option the config lists or the
 *   server announces, as the connection strings of the server capabilities carry;
 * - the port the server of the added device announces for the selected option;
 * - the port of the selected option, as getAvailableTransportOptions lists it.
 *
 * The module gets the selected option and that port as the "TransportOption" and "Port" properties of the type config
 * (see `TransportOptionConfigProperty` and `PortConfigProperty`), in place of a port its own default config declares,
 * and a connection string that names the same port.
 *
 * Creating the streaming fails with OPENDAQ_ERR_INVALIDPARAMETER when a port is set and the connection string names
 * another port that is not the port of an option the config lists or the server announces.
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

    virtual ErrCode INTERFACE_FUNC getAuthenticationConfig(IAuthenticationConfig** config) = 0;

    // [templateType(transportOptions, IString, ITransportOption)]
    /*!
     * @brief Gets the transport options the streaming can connect over.
     * @param[out] transportOptions The options of the device info's server capability for the Streaming type, with
     * the ports the server announces, keyed by their own ids, the server's preferred first in dict iteration order.
     * The options of the Streaming type when the device info has no such capability. Empty for a Streaming type
     * without options.
     */
    virtual ErrCode INTERFACE_FUNC getAvailableTransportOptions(IDict** transportOptions) = 0;

    /*!
     * @brief Selects the transport option the streaming connects over.
     * @param id The id of one of the options of getAvailableTransportOptions.
     * @retval OPENDAQ_ERR_NOT_SUPPORTED for a Streaming type without transport options.
     * @retval OPENDAQ_ERR_NOTFOUND if none of the options has that id.
     *
     * A port set by setPort is kept and overrides the port of the newly selected option.
     */
    virtual ErrCode INTERFACE_FUNC setTransportOption(IString* id) = 0;

    /*!
     * @brief Gets the selected transport option.
     * @param[out] transportOption The selected option, as getAvailableTransportOptions lists it.
     * @retval OPENDAQ_ERR_NOT_SUPPORTED for a Streaming type without transport options.
     * @retval OPENDAQ_ERR_NOTFOUND if the selected option is none of the options.
     */
    virtual ErrCode INTERFACE_FUNC getSelectedTransportOption(ITransportOption** transportOption) = 0;

    /*!
     * @brief Sets the port the streaming connects to.
     * @param port The port, used instead of the one the selected option lists whichever option is selected, even when
     * it is the same; -1 to use the port of the selected option again.
     * @retval OPENDAQ_ERR_NOT_SUPPORTED for a Streaming type without transport options.
     * @retval OPENDAQ_ERR_INVALIDPARAMETER for a negative port other than -1.
     */
    virtual ErrCode INTERFACE_FUNC setPort(Int port) = 0;

    /*!
     * @brief Gets the port the streaming connects to.
     * @param[out] port The port set by setPort. Otherwise the port of the selected transport option: the one the
     * device info's server capability announces for it, or the Streaming type's default port when there is no such
     * capability.
     * @retval OPENDAQ_ERR_NOT_SUPPORTED for a Streaming type without transport options.
     *
     * Unless a port is set, a port the connection string of the streaming names, or else a port the server of the
     * added device announces for the selected option, takes precedence (see the class description).
     */
    virtual ErrCode INTERFACE_FUNC getPort(Int* port) = 0;
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
//[factory(Hide)]
OPENDAQ_DECLARE_CLASS_FACTORY(
    LIBRARY_FACTORY, AddStreamingConfig,
    IStreamingType*, streamingType
)

/*!@}*/

END_NAMESPACE_OPENDAQ
