/*
 * Copyright 2022-2023 Blueberry d.o.o.
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
#include <coreobjects/core_event_args.h>
#include <opendaq/component.h>
#include <opendaq/signal.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_core_events
 * @addtogroup opendaq_core_events_factories Factories
 * @{
 */

/*!
 * @brief Creates Core event args that are passed as argument when a new component is added as a child.
 * @param component The added component.
 *
 * The sender of the event is always the parent component.
 * The ID of the event is 40, and the event name is "ComponentAdded".
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, CoreEventArgsComponentAdded, ICoreEventArgs,
    IComponent*, component
)

/*!
 * @brief Creates Core event args that are passed as argument when a component is removed from the list of children.
 * @param componentId The local ID of the removed component.
 *
 * The sender of the event is always the parent of the removed component.
 *
 * The ID of the event is 50, and the event name is "ComponentRemoved".
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, CoreEventArgsComponentRemoved, ICoreEventArgs,
    IString*, componentId
)

/*!
 * @brief Creates Core event args that are passed as argument when a signal is connected to an input port.
 * @param signal The connected signal.
 *
 * The sender of the event is always the input port into which the signal was connected.
 *
 * The ID of the event is 60, and the event name is "SignalConnected".
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, CoreEventArgsSignalConnected, ICoreEventArgs,
    ISignal*, signal
)

/*!
 * @brief Creates Core event args that are passed as argument when a signal is connected to an input port.
 *
 * The sender of the event is always the input port from which the signal was disconnected. The parameters
 * of this event are empty.
 *
 * The ID of the event is 70, and the event name is "SignalDisconnected".
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, CoreEventArgsSignalDisconnected, ICoreEventArgs
)

/*!
 * @brief Creates Core event args that are passed as argument when the descriptor of a signal changes.
 * @param descriptor The new descriptor.
 *
 * The sender of the event is always the signal of which descriptor was changed.
 *
 * The ID of the event is 80, and the event name is "DataDescriptorChanged".
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, CoreEventArgsDataDescriptorChanged, ICoreEventArgs,
    IDataDescriptor*, descriptor
)

/*!
 * @brief Creates Core event args that are passed as argument when a component is finished updating.
 *
 * A component finished updating at the end of the `update` call.
 * The ID of the event is 90, and the event name is "ComponentUpdateEnd".
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, CoreEventArgsComponentUpdateEnd, ICoreEventArgs
)

/*!
 * @brief Creates Core event args that are passed as argument when a component's internal fields/parameters
 * are modified.
 * @param modifiedAttributes Dictionary of modified attributes where the string key represents the attribute, and the
 * base object value the new value.
 *
 * An example of such attribute is the "active" state of a component.
 *
 * The ID of the event is 100, and the event name is "ComponentModified".
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, CoreEventArgsComponentModified, ICoreEventArgs,
    IDict*, modifiedAttributes
)

/*!@}*/

END_NAMESPACE_OPENDAQ
