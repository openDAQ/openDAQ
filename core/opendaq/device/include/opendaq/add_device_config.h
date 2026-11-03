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
 * @ingroup opendaq_device_type
 * @addtogroup opendaq_add_device_config Add device config
 * @{
 */

/*!
 * @brief Selects the transport a device is connected over when it is added.
 *
 * A device type that lists transports (see `IComponentType::getAvailableTransports`) puts this object into
 * the config returned by `IComponentType::createDefaultConfig`, as the `"AddDeviceConfig"` object property.
 * The module reads the selected transport from it when the device is created.
 *
 * Only the configuration connection follows this selection. The streaming connections of the device, including
 * the ones attached automatically, use the transport selected in the add-streaming config of their own streaming
 * type, in the `"Streaming"` section of the add-device config. A device connected over "tls" still streams over
 * the default transport unless another one is selected there.
 */
DECLARE_OPENDAQ_INTERFACE(IAddDeviceConfig, IAddComponentConfig)
{
};
/*!@}*/

/*!
 * @ingroup opendaq_add_device_config
 * @addtogroup opendaq_add_device_config_factories Factories
 * @{
 */

/*!
 * @brief Creates an add-device config listing the given transports. The first one starts out selected.
 * @param transports The names of the transports the device can be connected over.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, AddDeviceConfig, IAddDeviceConfig,
    IList*, transports
)

/*!@}*/

END_NAMESPACE_OPENDAQ
