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
#include <opendaq/component_type_ptr.h>
#include <opendaq/container_utils.h>
#include <opendaq/server_capability_ptr.h>
#include <opendaq/transport_option_factory.h>
#include <opendaq/transport_option_ptr.h>
#include <optional>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @brief Gets the port set on an add device or add streaming config.
 * @returns The port given to setPort, or -1 if none is set.
 */
template <typename ConfigPtr>
Int GetPortSet(const ConfigPtr& config)
{
    const Int port = config.template asPtr<IPropertyObject>().getPropertyValue(PortConfigProperty);
    return port;
}

/*!
 * @brief Checks whether a server capability announces the transport option an add device or add streaming config selects.
 * @returns True as well when the config selects no option, and when the capability announces no options.
 */
template <typename ConfigPtr>
bool AnnouncesSelectedTransportOption(const ConfigPtr& config, const ServerCapabilityPtr& capability)
{
    const TransportOptionPtr selected = config.getTransportOption();
    if (!selected.assigned())
        return true;

    const auto announced = capability.getAvailableTransportOptions();
    return !announced.assigned() || announced.getCount() == 0 || announced.hasKey(selected.getId());
}

/*!
 * @brief Gets the port to connect at to the server of a capability, with an add device or add streaming config.
 * @param config The config.
 * @param type The Device or Streaming type of the config.
 * @param capability The server capability. It has to announce the selected option, unless a port is set.
 * @returns
 * - the port set on the config;
 * - otherwise the port the capability announces for the selected option;
 * - for a capability that announces no options: none for the first option of the type, so the port of the capability's
 *   connection string stays, and the port of the selected option for any other option;
 * - none when the config selects no option.
 */
template <typename ConfigPtr>
std::optional<Int> GetCapabilityConnectionPort(const ConfigPtr& config, const ComponentTypePtr& type, const ServerCapabilityPtr& capability)
{
    if (const Int portSet = GetPortSet(config); portSet != -1)
        return portSet;

    const TransportOptionPtr selected = config.getTransportOption();
    if (!selected.assigned())
        return std::nullopt;

    const auto announced = capability.getAvailableTransportOptions();
    if (announced.assigned() && announced.getCount() > 0)
    {
        const TransportOptionPtr option = announced.get(selected.getId());
        return option.getPort();
    }

    const TransportOptionPtr firstOption = FirstValue(type.getAvailableTransportOptions());
    if (selected.getId() == firstOption.getId())
        return std::nullopt;

    return selected.getPort();
}

END_NAMESPACE_OPENDAQ
