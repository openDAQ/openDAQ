/*
 * Copyright 2022-2024 openDAQ d.o.o.
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
 * Notably, core events trigger only on components that are reachable from the root of the openDAQ tree. Actions such as
 * adding properties during the creation of a function block will not trigger the event - only attaching the function block
 * to the tree will. Subsequent modification of the function block's properties, however, will trigger events, as the
 * function block is then reachable from the root.
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
 * The "owner" parameter is used to determine whether the change was triggered from within a nested object-type property of the
 * sender component.
 *
 * The parameters dictionary contains:
 *  - The Property object owner of the property under the key "Owner"
 *  - The name of the property as a string under the key "Name"
 *  - The new value of the property under the key "Value"
 *  - The relative path to the property owner from the sender component under the key "Path".
 *
 * The "Path" parameter is used for object-type properties where it represents the path to the property through child Property objects.
 * Eg. the path to the "MyInt" property of child object-type property named "Child1" on a component would be "Child1". In the case of
 * deeper nesting of object-type properties (if "Child1" had another object-type property named "Child2") the path would be as follows:
 * "Child1.Child2.MyInt".
 *
 * The ID of the event is 0, and the event name is "PropertyValueChanged".
 *
 * @subsubsection opendaq_core_event_types_update_end Property object update end
 *
 * Event triggered whenever a property object finishes updating - at the end of the `update` call, or when `endUpdate` is called.
 *
 * The "owner" parameter is used to determine whether the update end event was triggered from within a nested object-type property of the
 * sender component.
 *
 * The parameters dictionary contains:
 *  - The Property object owner of the property under the key "Owner"
 *  - The dictionary of updated properties under the key "UpdatedProperties". The dictionary has the string names
 *  of properties as key, and base object values as values.
 *  - The relative path to the property owner from the sender component under the key "Path".
 *
 * The "Path" parameter is used for object-type properties where it represents the path to the property through child Property objects.
 * Eg. the path to the "MyInt" property of child object-type property named "Child1" on a component would be "Child1". In the case of
 * deeper nesting of object-type properties (if "Child1" had another object-type property named "Child2") the path would be as follows:
 * "Child1.Child2.MyInt".
 *
 * The ID of the event is 10, and the event name is "PropertyObjectUpdateEnd".
 *
 * @subsubsection opendaq_core_event_types_property_added_removed Property added/removed
 *
 * The Property added and Property removed events are triggered whenever a property is added/removed from a component.
 *
 * The "owner" parameter is used to determine whether the addition/removal was triggered from within a nested object-type
 * property of the sender component.
 *
 * The "added" event contains the following parameters:
 *  - The Property object owner of the property under the key "Owner"
 *  - The added property as a Property object under the key "Property"
 *  - The relative path to the property owner from the sender component under the key "Path".
 *
 * The "removed" event contains the following parameters:
 *  - The Property object owner of the property under the key "Owner"
 *  - The name of the property as a string under the key "Name"
 *  - The relative path to the property owner from the sender component under the key "Path".
 *  
 * The "Path" parameter is used for object-type properties where it represents the path to the property through child Property objects.
 * Eg. the path to the "MyInt" property of child object-type property named "Child1" on a component would be "Child1". In the case of
 * deeper nesting of object-type properties (if "Child1" had another object-type property named "Child2") the path would be as follows:
 * "Child1.Child2.MyInt".
 *
 * The ID of the Property added event is 20, and the event name is "PropertyAdded".
 * The ID of the Property removed event is 30, and the event name is "PropertyRemoved".
 *
 * @subsubsection opendaq_core_event_types_component_added_removed Component added/removed
 *
 * The Component added/removed events are triggered whenever a new component is attached or detached from
 * the openDAQ component tree. The event is only triggered when the added/removed component can be
 * reached from the root of the tree. Eg. when creating a new function block, no event will be triggered
 * except for when the entire function block subtree is attached to the main tree.
 *
 * The sender of the event is always the parent component of the added/removed child.
 *
 * The "added" event contains the following parameters:
 *  - The added component under the key "Component"
 *
 * The "removed" event contains the following parameters:
 *  - The local ID of the property as a string under the key "Id"
 *  
 * The ID of the Property added event is 40, and the event name is "ComponentAdded".
 * The ID of the Property removed event is 50, and the event name is "ComponentRemoved".
 *
 * @subsubsection opendaq_core_event_types_signal_conn_disc Signal connected/disconnected
 *
 * Triggered whenever a signal is connected to- or disconnected from an input port.
 *
 * The sender of the event is the input port.
 *
 * The "connected" event contains the following parameters:
 *  - The connected signal under the key "Signal"
 *
 * The "disconnected" event has no parameters.
 *  
 * The ID of the connected event is 60, and the event name is "SignalConnected".
 * The ID of the disconnected event is 70, and the event name is "SignalDisconnected".
 *
 * @subsubsection opendaq_core_event_types_data_desc Data descriptor changed
 *
 * Triggered whenever the data descriptor of a signal changes.
 *
 * The sender of the event is the signal.
 *
 * The event contains the following parameters:
 *  - The new data descriptor under the key "DataDescriptor"
 *
 * The ID of the connected event is 80, and the event name is "DataDescriptorChanged".
 * 
 * @subsubsection opendaq_core_event_types_component_update_end Component updated end
 *
 * Event triggered whenever a component finishes updating - at the end of the `update` call.
 *
 * The event has no arguments. When called, the component should be checked for changes.
 *
 * The ID of the event is 90, and the event name is "ComponentUpdateEnd".
 *
 * @subsubsection opendaq_core_event_types_component_modified Attribute changed
 *
 * Event triggered when an internal attribute of a component has been changed. Eg. when the "Active" state of the
 * component is modified.
 *
 * The event has no preset parameters, but instead contains dictionary key-value pairs where the key corresponds to
 * the name of the modified attribute, and the value to its new value.
 *
 * The Attribute changed event payload always has a "AttributeName" key entry in the dictionary, containing the
 * name of the changed attribute.
 *
 * The payload then contains another entry using the "AttributeName" value as the key. That entry contains
 * the new value of the attribute.
 *
 * Currently the following attribute names can be present in the Attribute changed event payload:
 *
 *  - Signal: "DomainSignal", "RelatedSignals", "Public"
 *  - Component: "Active", "Name", "Description", "Visible"
 *
 * The ID of the event is 100, and the event name is "AttributeChanged".
 *
 * @subsubsection opendaq_core_event_types_tags_changed Tags changed
 *
 * Triggered when a tag of the sender component was added or removed.
 *
 * The event contains the following parameters:
 *  - The list of tags (list of string) under the key "Tags"
 *
 * The ID of the connected event is 110, and the event name is "TagsChanged".
 *
 * @subsubsection opendaq_core_event_types_component_status Component status changed
 *
 * Triggered whenever a component's status changes (excluding device's connection statuses).
 *
 * The sender of the event is the component.
 *
 * The event contains the following parameters:
 *  - The new status value encapsulated within an Enumeration object as a value and the status name as a key
 *
 * The ID of the event is 120, and the event name is "StatusChanged".
 *
 * @subsubsection opendaq_core_event_types_type_added Type added/removed
 *
 * Triggered whenever a new "Type" is added to- or removed from the Type manager. Eg. when a new Struct or Enumeration type is
 * created and added.
 *
 * The sender of the above event types is always an empty Component pointer.
 *
 * The "TypeAdded" event contains the following parameters:
 *  - The newly added type under the key "Type"
 *
 * The ID of the event is 130, and the event name is "TypeAdded".
 *
  * The "TypeRemoved" event contains the following parameters:
 *  - The name of the removed type under the key "TypeName"
 *
 * The ID of the event is 140, and the event name is "TypeRemoved".
 *
 * @subsubsection opendaq_core_event_types_domain_changed Device domain changed
 *
 * Triggered whenever the "Domain" of a device changes.
 *
 * The sender of the above event types is the device of which domain changed.
 *
 * The "DeviceDomainChanged" event contains the following parameters:
 *  - The device domain under the key "DeviceDomain"
 *
 * The ID of the event is 150, and the event name is "DeviceDomainChanged".
 *
 * @subsubsection opendaq_core_event_types_device_connection_status Device connection status changed
 *
 * Triggered whenever connection status of a device changes.
 *
 * The sender of the event is the device of which connection status changed.
 *
 * The event contains the following parameters:
 *  - The changed status name string under the key "StatusName"
 *  - The new status value encapsulated within an Enumeration object under the key "StatusValue". The possible values:
 *  "Connected" - connection just established or restored, "Reconnecting" - connection lost, "Unrecoverable" - reconnection
 *  is not possible, "NotAvailable" - connection removed (typically when corresponding streaming source is removed)
 *  - The connection string associated with connection under the key "ConnectionString"
 *  - The integer value specifying the type of particular connection which status changed (0 - Unknown, 1 - Configuration, 2 - Streaming,
 *  3 - ConfigurationAndStreaming) under the key "ProtocolType"
 *  - The Streaming object associated with connection under the key "StreamingObject", the parameter is nullptr if connection
 *  is of configuration type or the corresponding streaming connection has been removed
 *
 * The ID of the event is 170, and the event name is "ConnectionStatusChanged".
 *
 * @subsection opendaq_core_event_muting Muting core events
 *
 * Components, as previously mentioned, do not trigger core events until they are connected to the root of the
 * openDAQ tree. This is achieved through internal methods that enable/disable the triggers recursively. By default,
 * each component will not trigger any events - the triggers must be manually enabled. The SDK does this whenever
 * a component is added as a child of a parent, if the parent's core triggers are enabled. When done so, the triggers
 * are enabled recursively for the entirety of the subtree. A converse function can be used to instead recursively
 * mute the events.
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
    CoreEventId, eventId,
    IString*, eventName,
    IDict*, parameters
)

/*!
 * @brief Creates Core event args that are passed as argument when a property value of a component is changed.
 * @param propOwner The property object that owns the changed property.
 * @param propName The name of the property of which value was changed.
 * @param value The new value of the property.
 * @param path The relative path to the property owner from the sender component. Used for object-type properties. Eg. "child1.child2".
 * Does not include the Component id and property name.
 *
 * The ID of the event is 0, and the event name is "PropertyValueChanged".
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, CoreEventArgsPropertyValueChanged, ICoreEventArgs,
    IPropertyObject*, propOwner,
    IString*, propName,
    IBaseObject*, value,
    IString*, path
)

/*!
 * @brief Creates Core event args that are passed as argument when a property object is finished updating.
 * @param propOwner The property object that was updated.
 * @param updatedProperties The dictionary of updated properties. Contains the name (string) of a property
 * as key, and the new value (base object) as the dictionary value.
 * @param path The relative path to the property owner from the sender component. Used for object-type properties. Eg. "child1.child2".
 * Does not include the Component id.
 *
 * A property object finished updating when `endUpdate` is called, or at the end of the `update` call.
 * The ID of the event is 10, and the event name is "PropertyObjectUpdateEnd".
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, CoreEventArgsPropertyObjectUpdateEnd, ICoreEventArgs,
    IPropertyObject*, propOwner,
    IDict*, updatedProperties,
    IString*, path
)

/*!
 * @brief Creates Core event args that are passed as argument when a property is added to a component.
 * @param propOwner The property object that owns the added property.
 * @param prop The property that was added.
 * @param path The relative path to the property owner from the sender component. Used for object-type properties. Eg. "child1.child2".
 * Does not include the Component id and property name.
 *
 * The ID of the event is 20, and the event name is "PropertyAdded".
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, CoreEventArgsPropertyAdded, ICoreEventArgs,
    IPropertyObject*, propOwner,
    IProperty*, prop,
    IString*, path
)

/*!
 * @brief Creates Core event args that are passed as argument when a property is removed from a component.
 * @param propOwner The property object that owned the removed property.
 * @param propName The name of the property that was removed.
 * @param path The relative path to the property owner from the sender component. Used for object-type properties. Eg. "child1.child2".
 * Does not include the Component id and property name.
 *
 * The ID of the event is 30, and the event name is "PropertyRemoved".
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, CoreEventArgsPropertyRemoved, ICoreEventArgs,
    IPropertyObject*, propOwner,
    IString*, propName,
    IString*, path
)

/*!
 * @brief Creates Core event args that are passed as argument when a type is added to the type manager.
 * @param type The type that was added.
 *
 * The ID of the event is 130, and the event name is "TypeAdded".
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, CoreEventArgsTypeAdded, ICoreEventArgs,
    IType*, type
)

/*!
 * @brief Creates Core event args that are passed as argument when a type is removed from the type manager.
 * @param typeName The name of the removed type
 *
 * The ID of the event is 140, and the event name is "TypeRemoved".
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, CoreEventArgsTypeRemoved, ICoreEventArgs,
    IString*, typeName
)
/*!@}*/

END_NAMESPACE_OPENDAQ
