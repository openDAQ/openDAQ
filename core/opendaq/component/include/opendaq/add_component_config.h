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
#include <coretypes/listobject.h>
#include <coretypes/stringobject.h>
#include <coreobjects/property_object.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup objects_utility
 * @addtogroup objects_add_component_config Add component config
 * @{
 */

/*#
 * [templated(defaultAliasName: AddComponentConfigPtr)]
 * [interfaceSmartPtr(IAddComponentConfig, GenericAddComponentConfigPtr)]
 * [interfaceLibrary(IPropertyObject, "coreobjects")]
 * [interfaceSmartPtr(IPropertyObject, GenericPropertyObjectPtr, "<coreobjects/property_object_ptr.h>")]
 */

/*!
 * @brief Selects the transport a component is connected over when it is added.
 *
 * Is itself a Property object - `"Transport"` is a Selection property whose candidates are the names of the
 * transports the component type supports (see `IComponentType::getAvailableTransports`), in the same order.
 * The first one starts out selected. The typed getters/setters below are an equal alternative to reading and
 * writing that property directly.
 *
 * This is a temporary bridge: for now the object is carried as a property of the plain add-device or
 * add-streaming config. It is meant to replace that config entirely later.
 */
DECLARE_OPENDAQ_INTERFACE(IAddComponentConfig, IPropertyObject)
{
    // [templateType(transports, IString)]
    /*!
     * @brief Gets the names of the transports the component can be connected over.
     * @param[out] transports The transport names (e.g. "plain", "tls"). Empty if the component type lists none.
     */
    virtual ErrCode INTERFACE_FUNC getAvailableTransports(IList** transports) = 0;

    /*!
     * @brief Selects the transport to connect over.
     * @param transport The name of one of the available transports.
     * @retval OPENDAQ_ERR_NOTFOUND if `transport` is not one of the available transports.
     */
    virtual ErrCode INTERFACE_FUNC setTransport(IString* transport) = 0;

    /*!
     * @brief Gets the selected transport.
     * @param[out] transport The name of the selected transport, or `nullptr` if no transports are available.
     */
    virtual ErrCode INTERFACE_FUNC getTransport(IString** transport) = 0;
};
/*!@}*/

END_NAMESPACE_OPENDAQ
