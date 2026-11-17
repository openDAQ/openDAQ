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
#include <opendaq/transport_option_ptr.h>
#include <coretypes/integer_factory.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_transport_option
 * @brief The device or streaming type config property that names the transport option to connect over.
 *
 * A String property. The module manager adds it to the type config a device or streaming of a type that lists
 * transport options is created with, right before the module creates it: set to the option the add-device or
 * add-streaming config selects, else to the first option. The add fails with OPENDAQ_ERR_NOTFOUND when it names an
 * option the type does not list.
 */
inline constexpr const char* TransportOptionConfigProperty = "TransportOption";

/*!
 * @ingroup opendaq_transport_option
 * @brief The device or streaming type config property with the port to connect to.
 *
 * An Int property next to TransportOptionConfigProperty, added the same way. A module that already declares a "Port"
 * property in its type config keeps it; otherwise the default is the port of the type's first transport option.
 */
inline constexpr const char* PortConfigProperty = "Port";

/*!
 * @ingroup opendaq_transport_option
 * @addtogroup opendaq_transport_option_factories Factories
 * @{
 */

/*!
 * @brief Creates a frozen transport option.
 * @param name The name of the option, unique within a component type (e.g. "plain", "tls"). Must not be empty.
 * @param port The port of the option, 0 for a carrier without ports. Must not be negative.
 * @param protection What the option protects the data it carries against.
 * @param certificateRequired True when the connection over this option needs a client certificate.
 */
inline TransportOptionPtr TransportOption(const StringPtr& name,
                                          Int port,
                                          TransportProtection protection = TransportProtection::None,
                                          Bool certificateRequired = False)
{
    TransportOptionPtr obj(TransportOption_Create(name, Integer(port), protection, certificateRequired));
    return obj;
}

/*!@}*/

END_NAMESPACE_OPENDAQ
