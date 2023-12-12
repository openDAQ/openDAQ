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
#include <coretypes/coretypes.h>
#include <coretypes/event_args.h>
#include <coreobjects/core_event_args_ids.h>
#include <coreobjects/property.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_utility
 * @addtogroup opendaq_core_events Core Event Args
 * @{
 */
    

/*#
 * [interfaceSmartPtr(IEventArgs, EventArgsPtr, "<coretypes/event_args_ptr.h>")]
 */

/*!
 * @brief Arguments object that defines a Core event type, and provides a set of parameters specific to a given
 * event type.
 *
 * Core events are triggered whenever a change in the openDAQ core structure happens. This includes changes to property values,
 * addition/removal of child components, connecting signals to input ports and others. The event type can be identified
 * via the event ID available within the CoreEventArgs object. Each event type has a set of predetermined parameters
 * available in the `parameters` field of the arguments. These can be used by any openDAQ server, or other listener to
 * react to changes within the core structure.
 *
 * The Core Event object can be obtained from the Context object created by the Instance that is available to any component
 * within the openDAQ tree structure.
 *
 * @subsection opendaq_core_events_types Core event types
 *
 * This section provides a list of core event types, when they are triggered, as well as their parameter content.
 *
 * @subsubsection opendaq_core_event_types_value_changed Property value changed
 *
 * Event triggered whenever a value of a component's property is changed. It triggers after the PropertyObject's and Property's
 * `onValueWriteEvent`, providing the value of the property after said events are finished processing.
 *
 * The Property value changed core event does not trigger when updating a component via `update`, and when `beginUpdate` has been called.
 *
 * The parameters dictionary contains:
 *  - The name of the property as a string under the key "Name"
 *  - The new value of the property under the key "Value"
 *
 * The ID of the event is 0, and the event name is "PropertyValueChanged".
 *
 * @subsubsection opendaq_core_event_types_update_end Update end
 *
 * Event triggered whenever a component finishes updating - at the end of the `update` call, or when `endUpdate` is called.
 *
 * The parameters dictionary contains:
 *  - The dictionary of updated properties under the key "UpdatedProperties". The dictionary has the string names
 *  of properties as key, and base object values as values.
 *
 * The ID of the event is 10, and the event name is "UpdateEnd".
 *
 * @subsubsection opendaq_core_event_types_property_added_removed Property added/removed
 *
 * The Property added and Property removed events are triggered whenever a property is added/removed from a component.
 *
 * The "added" event contains the following parameters:
 *  - The added property as a Property object under the key "Property"
 *
 * The "removed" event contains the following parameters:
 *  - The name of the property as a string under the key "Name"
 *  
 * The ID of the Property added event is 20, and the event name is "PropertyAdded".
 * The ID of the Property removed event is 30, and the event name is "PropertyRemoved".
 */
DECLARE_OPENDAQ_INTERFACE(ICoreEventArgs, IEventArgs)
{
    // [templateType(parameters, IString, IBaseObject)]
    /*!
     * @brief Gets the parameters of the core event.
     * @param[out] parameters The parameters of the core event.
     */
    virtual ErrCode INTERFACE_FUNC getParameters(IDict** parameters) = 0;
};

/*!@}*/

/*!
 * @ingroup opendaq_core_events
 * @addtogroup opendaq_core_events_factories Factories
 * @{
 */

/*!
 * @brief Creates Core event args with a given ID and custom parameters.
 * @param eventId The ID of the event. If the ID is not equal to one of the pre-defined IDs, the event name will be "Unknown".
 * @param parameters The parameters of the event.
 */
OPENDAQ_DECLARE_CLASS_FACTORY(
    LIBRARY_FACTORY, CoreEventArgs,
    Int, eventId,
    IDict*, parameters
)

/*!
 * @brief Creates Core event args that are passed as argument when a property value of a component is changed.
 * @param propName The name of the property of which value was changed.
 * @param value The new value of the property.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, CoreEventArgsPropertyValueChanged, ICoreEventArgs,
    IString*, propName,
    IBaseObject*, value
)

/*!
 * @brief Creates Core event args that are passed as argument when a component is finished updating.
 * @param updatedProperties The dictionary of updated properties. Contains the name (string) of a property
 * as key, and the new value (base object) as the dictionary value.
 *
 * A component finished updating when `endUpdate` is called, or at the end of the `update` call.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, CoreEventArgsUpdateEnd, ICoreEventArgs,
    IDict*, updatedProperties
)

/*!
 * @brief Creates Core event args that are passed as argument when a property is added to a component.
 * @param prop The property that was added.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, CoreEventArgsPropertyAdded, ICoreEventArgs,
    IProperty*, prop
)

/*!
 * @brief Creates Core event args that are passed as argument when a property is removed from a component.
 * @param propName The name of the property that was removed.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, CoreEventArgsPropertyRemoved, ICoreEventArgs,
    IString*, propName
)

/*!@}*/

END_NAMESPACE_OPENDAQ
