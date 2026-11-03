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
#include <opendaq/add_component_config.h>

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [interfaceSmartPtr(IAddComponentConfig, GenericAddComponentConfigPtr, "<opendaq/add_component_config_ptr.h>")]
 * [interfaceLibrary(IAddComponentConfig, "opendaq")]
 */

/*!
 * @ingroup opendaq_streaming_type
 * @addtogroup opendaq_add_streaming_config Add streaming config
 * @{
 */

/*!
 * @brief Selects the transport a streaming connection is established over.
 *
 * A streaming type that lists transports (see `IComponentType::getAvailableTransports`) puts this object into
 * the config returned by `IComponentType::createDefaultConfig`, as the `"AddStreamingConfig"` object property.
 * The module reads the selected transport from it when the streaming connection is created.
 *
 * The selection is independent of the transport selected for the device in its add-device config, and so is
 * the port. A streaming connection attached to a device automatically connects at the port the server announces
 * for the selected transport, unless the streaming type config sets a port itself. When the server does not
 * offer the selected transport, no streaming connection is attached.
 */
DECLARE_OPENDAQ_INTERFACE(IAddStreamingConfig, IAddComponentConfig)
{
};
/*!@}*/

/*!
 * @ingroup opendaq_add_streaming_config
 * @addtogroup opendaq_add_streaming_config_factories Factories
 * @{
 */

/*!
 * @brief Creates an add-streaming config listing the given transports. The first one starts out selected.
 * @param transports The names of the transports the streaming connection can be established over.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, AddStreamingConfig, IAddStreamingConfig,
    IList*, transports
)

/*!@}*/

END_NAMESPACE_OPENDAQ
