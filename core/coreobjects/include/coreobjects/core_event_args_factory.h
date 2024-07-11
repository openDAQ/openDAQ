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
#include <coreobjects/core_event_args_ptr.h>
#include <coretypes/dictobject_factory.h>
#include <coreobjects/property_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_core_events
 * @addtogroup opendaq_core_events_factories Factories
 * @{
 */

/*!
 * @brief Creates Core event args with a given ID and custom parameters.
 * @param id The ID of the event. If the ID is not equal to one of the pre-defined IDs, the event name will be "Unknown".
 * @param parameters The parameters of the event.
 */
inline CoreEventArgsPtr CoreEventArgs(CoreEventId id, const StringPtr& name, const DictPtr<IString, IBaseObject>& parameters)
{
    CoreEventArgsPtr obj(CoreEventArgs_Create(id, name,parameters));
    return obj;
}

/*!
 * @brief Creates Core event args that are passed as argument when a property value of a component is changed.
 * @param propOwner The property object that owns the changed property.
 * @param propName The name of the property of which value was changed.
 * @param value The new value of the property.
 * @param path The relative path to the property owner from the sender component. Used for object-type properties. Eg. "child1.child2".
 * Does not include the Component id and property name.
 */
inline CoreEventArgsPtr CoreEventArgsPropertyValueChanged(const PropertyObjectPtr& propOwner, const StringPtr& propName, const BaseObjectPtr& value, const StringPtr& path)
{
    CoreEventArgsPtr obj(CoreEventArgsPropertyValueChanged_Create(propOwner, propName, value, path));
    return obj;
}

/*!
 * @brief Creates Core event args that are passed as argument when a component is finished updating.
 * @param propOwner The property object that was updated.
 * @param updatedProperties The dictionary of updated properties. Contains the name (string) of a property
 * as key, and the new value (base object) as the dictionary value.
 * @param path The relative path to the property owner from the sender component. Used for object-type properties. Eg. "child1.child2".
 * Does not include the Component id.
 *
 * A component finished updating when `endUpdate` is called, or at the end of the `update` call.
 */
inline CoreEventArgsPtr CoreEventArgsPropertyObjectUpdateEnd(const PropertyObjectPtr& propOwner, const DictPtr<IString, IBaseObject>& updatedProperties, const StringPtr& path)
{
    CoreEventArgsPtr obj(CoreEventArgsPropertyObjectUpdateEnd_Create(propOwner, updatedProperties, path));
    return obj;
}

/*!
 * @brief Creates Core event args that are passed as argument when a property is added to a component.
 * @param propOwner The property object that owns the added property.
 * @param prop The property that was added.
 * @param path The relative path to the property owner from the sender component. Used for object-type properties. Eg. "child1.child2".
 * Does not include the Component id and property name.
 */
inline CoreEventArgsPtr CoreEventArgsPropertyAdded(const PropertyObjectPtr& propOwner, const PropertyPtr& prop, const StringPtr& path)
{
    CoreEventArgsPtr obj(CoreEventArgsPropertyAdded_Create(propOwner, prop, path));
    return obj;
}

/*!
 * @brief Creates Core event args that are passed as argument when a property is removed from a component.
 * @param propOwner The property object that owned the removed property.
 * @param propName The name of the property that was removed.
 * @param path The relative path to the property owner from the sender component. Used for object-type properties. Eg. "child1.child2".
 * Does not include the Component id and property name.
 */
inline CoreEventArgsPtr CoreEventArgsPropertyRemoved(const PropertyObjectPtr& propOwner, const StringPtr& propName, const StringPtr& path)
{
    CoreEventArgsPtr obj(CoreEventArgsPropertyRemoved_Create(propOwner, propName, path));
    return obj;
}

/*!
 * @brief Creates Core event args that are passed as argument when a type is added to the type manager.
 * @param type The type that was added.
 *
 * The ID of the event is 130, and the event name is "TypeAdded".
 */
inline CoreEventArgsPtr CoreEventArgsTypeAdded(const TypePtr& type)
{
    CoreEventArgsPtr obj(CoreEventArgsTypeAdded_Create(type));
    return obj;
}

/*!
 * @brief Creates Core event args that are passed as argument when a type is removed from the type manager.
 * @param typeName The name of the removed type
 *
 * The ID of the event is 140, and the event name is "TypeRemoved".
 */
inline CoreEventArgsPtr CoreEventArgsTypeRemoved(const StringPtr& typeName)
{
    CoreEventArgsPtr obj(CoreEventArgsTypeRemoved_Create(typeName));
    return obj;
}

/*!@}*/

END_NAMESPACE_OPENDAQ
