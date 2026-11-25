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
#include <opendaq/transport_option_builder_ptr.h>
#include <coretypes/integer_factory.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_transport_option
 * @brief The device or streaming type config property that holds the id of the transport option to connect over.
 *
 * A String property. The module manager adds it to the type config a device or streaming of a type that lists transport
 * options is created with, right before the module creates it: set to the option the add-device or add-streaming config
 * selects, else to the first option. The add fails with OPENDAQ_ERR_NOTFOUND when it holds the id of an option the type
 * does not list.
 */
inline constexpr const char* TransportOptionConfigProperty = "TransportOption";

/*!
 * @ingroup opendaq_transport_option
 * @brief The device or streaming type config property with the port to connect to.
 *
 * An Int property next to TransportOptionConfigProperty, added the same way. A module that already declares a "Port"
 * property in its type config keeps it; otherwise the default is the port of the type's first transport option.
 *
 * When the device or streaming is added with an add-device or add-streaming config, the property holds the port to
 * connect at, and the connection string names the same port (see IAddDeviceConfig and IAddStreamingConfig). Otherwise
 * the module chooses the port: a value that differs from the property's default comes first, then the port the
 * connection string names, then the default.
 */
inline constexpr const char* PortConfigProperty = "Port";

/*!
 * @ingroup opendaq_transport_option
 * @addtogroup opendaq_transport_option_factories Factories
 * @{
 */

/*!
 * @brief Creates a transport option from the values set on a builder.
 * @param builder The transport option builder.
 * @throws InvalidParameterException when the id is null or empty, or the security mode is not valid.
 */
inline TransportOptionPtr TransportOptionFromBuilder(const TransportOptionBuilderPtr& builder)
{
    TransportOptionPtr obj(TransportOptionFromBuilder_Create(builder));
    return obj;
}

/*!
 * @brief Creates a transport option builder with the default values of the fields other than the id.
 * @param id The id of the option, unique within a component type (e.g. "plain", "tls"). Must not be empty.
 */
inline TransportOptionBuilderPtr TransportOptionBuilder(const StringPtr& id)
{
    TransportOptionBuilderPtr obj(TransportOptionBuilder_Create(id));
    return obj;
}

/*!
 * @brief Creates a transport option builder with the values of an existing option, e.g. to build the same option
 * with another port.
 * @param transportOption The option whose values the builder takes.
 */
inline TransportOptionBuilderPtr TransportOptionBuilderCopy(const TransportOptionPtr& transportOption)
{
    TransportOptionBuilderPtr obj(TransportOptionBuilderFromExisting_Create(transportOption));
    return obj;
}

/*!@}*/

END_NAMESPACE_OPENDAQ
