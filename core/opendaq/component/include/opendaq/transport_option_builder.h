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
#include <opendaq/transport_option.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_component_type
 * @addtogroup opendaq_transport_option Transport option
 * @{
 */

/*!
 * @brief Builds a transport option.
 *
 * The id is given when the builder is created and cannot be changed. The other fields default to port 0,
 * SecurityMode::None and no client certificate required.
 */
DECLARE_OPENDAQ_INTERFACE(ITransportOptionBuilder, IBaseObject)
{
    /*!
     * @brief Builds a transport option from the values set on the builder.
     * @param[out] transportOption The built option.
     * @retval OPENDAQ_ERR_INVALIDPARAMETER when the id is null or empty, or the security mode is not valid.
     */
    virtual ErrCode INTERFACE_FUNC build(ITransportOption** transportOption) = 0;

    /*!
     * @brief Gets the id of the option.
     * @param[out] id The id the builder was created with.
     */
    virtual ErrCode INTERFACE_FUNC getId(IString** id) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the port of the option.
     * @param port The default port on a component type, the served port on a server capability; 0 for a carrier
     * without ports.
     */
    virtual ErrCode INTERFACE_FUNC setPort(Int port) = 0;

    /*!
     * @brief Gets the port of the option.
     * @param[out] port The port of the option.
     */
    virtual ErrCode INTERFACE_FUNC getPort(Int* port) = 0;

    // [returnSelf]
    /*!
     * @brief Sets what the option protects the data it carries against.
     * @param securityMode The security mode of the option.
     */
    virtual ErrCode INTERFACE_FUNC setSecurityMode(SecurityMode securityMode) = 0;

    /*!
     * @brief Gets what the option protects the data it carries against.
     * @param[out] securityMode The security mode of the option.
     */
    virtual ErrCode INTERFACE_FUNC getSecurityMode(SecurityMode* securityMode) = 0;

    // [returnSelf]
    /*!
     * @brief Sets whether the connection over this option needs a client certificate.
     * @param required True when the user has to supply a client certificate to connect over this option.
     */
    virtual ErrCode INTERFACE_FUNC setCertificateRequired(Bool required) = 0;

    /*!
     * @brief Tells whether the connection over this option needs a client certificate.
     * @param[out] required True when the user has to supply a client certificate to connect over this option.
     */
    virtual ErrCode INTERFACE_FUNC getCertificateRequired(Bool* required) = 0;
};
/*!@}*/

/*!
 * @ingroup opendaq_transport_option
 * @addtogroup opendaq_transport_option_factories Factories
 * @{
 */

/*!
 * @brief Creates a transport option builder with the default values of the fields other than the id.
 * @param id The id of the option, unique within a component type (e.g. "plain", "tls"). Must not be empty.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, TransportOptionBuilder, ITransportOptionBuilder,
    IString*, id
)

/*!
 * @brief Creates a transport option builder with the values of an existing option.
 * @param transportOptionToCopy The option whose values the builder takes.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, TransportOptionBuilderFromExisting, ITransportOptionBuilder,
    ITransportOption*, transportOptionToCopy
)

/*!@}*/

END_NAMESPACE_OPENDAQ
