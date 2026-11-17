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
#include <coretypes/stringobject.h>
#include <coretypes/integer.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_component_type
 * @addtogroup opendaq_transport_option Transport option
 * @{
 */

/*!
 * @brief What a transport protects the data it carries against.
 */
enum class TransportProtection : EnumType
{
    None = 0,  ///< Nothing is protected.
    Signed,    ///< The data cannot be altered on the way, but can be read.
    Encrypted  ///< The data cannot be altered and cannot be read. Encrypted or fails, never a fallback to plain.
};

/*#
 * [interfaceSmartPtr(IInteger, IntegerPtr, "<coretypes/integer.h>")]
 */

/*!
 * @brief Describes one way to connect to a component: a transport, its port and the protection it provides.
 *
 * Created by the module and immutable. A component type lists the options its module supports, with their
 * default ports; a server capability lists the ones the server serves, with the ports it serves them on.
 *
 * A server capability keeps each option as a plain Property object with the `"Id"`, `"Port"`, `"Protection"` and
 * `"CertificateRequired"` properties, so that a peer that does not know this interface still reads the capability.
 */
DECLARE_OPENDAQ_INTERFACE(ITransportOption, IBaseObject)
{
    /*!
     * @brief Gets the id of the option, unique within a component type (e.g. "plain", "tls").
     * @param[out] id The id of the option.
     */
    virtual ErrCode INTERFACE_FUNC getId(IString** id) = 0;

    /*!
     * @brief Gets the port of the option.
     * @param[out] port The default port on a component type, the served port on a server capability;
     * 0 for a carrier without ports.
     */
    virtual ErrCode INTERFACE_FUNC getPort(IInteger** port) = 0;

    /*!
     * @brief Gets what the option protects the data it carries against.
     * @param[out] protection The protection of the option.
     */
    virtual ErrCode INTERFACE_FUNC getProtection(TransportProtection* protection) = 0;

    /*!
     * @brief Tells whether the connection over this option needs a client certificate.
     * @param[out] required True when the user has to supply a client certificate (through the authentication
     * config) to connect over this option.
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
 * @brief Creates a frozen transport option.
 * @param id The id of the option, unique within a component type. Must not be empty.
 * @param port The port of the option, 0 for a carrier without ports. Must not be negative.
 * @param protection What the option protects the data it carries against.
 * @param certificateRequired True when the connection over this option needs a client certificate.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, TransportOption, ITransportOption,
    IString*, id,
    IInteger*, port,
    TransportProtection, protection,
    Bool, certificateRequired
)

/*!@}*/

END_NAMESPACE_OPENDAQ
