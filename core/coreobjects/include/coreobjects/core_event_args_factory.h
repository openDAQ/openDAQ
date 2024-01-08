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
inline CoreEventArgsPtr CoreEventArgs(Int id, const DictPtr<IString, IBaseObject>& parameters)
{
    CoreEventArgsPtr obj(CoreEventArgs_Create(id, parameters));
    return obj;
}

/*!
 * @brief Creates Core event args that are passed as argument when a property value of a component is changed.
 * @param propOwner The property object that owns the changed property.
 * @param propName The name of the property of which value was changed.
 * @param value The new value of the property.
 */
inline CoreEventArgsPtr CoreEventArgsPropertyValueChanged(const PropertyObjectPtr& propOwner, const StringPtr& propName, const BaseObjectPtr& value)
{
    CoreEventArgsPtr obj(CoreEventArgsPropertyValueChanged_Create(propOwner, propName, value));
    return obj;
}

/*!
 * @brief Creates Core event args that are passed as argument when a component is finished updating.
 * @param propOwner The property object that was updated.
 * @param updatedProperties The dictionary of updated properties. Contains the name (string) of a property
 * as key, and the new value (base object) as the dictionary value.
 *
 * A component finished updating when `endUpdate` is called, or at the end of the `update` call.
 */
inline CoreEventArgsPtr CoreEventArgsPropertyObjectUpdateEnd(const PropertyObjectPtr& propOwner, const DictPtr<IString, IBaseObject>& updatedProperties)
{
    CoreEventArgsPtr obj(CoreEventArgsPropertyObjectUpdateEnd_Create(propOwner, updatedProperties));
    return obj;
}

/*!
 * @brief Creates Core event args that are passed as argument when a property is added to a component.
 * @param propOwner The property object that owns the added property.
 * @param prop The property that was added.
 */
inline CoreEventArgsPtr CoreEventArgsPropertyAdded(const PropertyObjectPtr& propOwner, const PropertyPtr& prop)
{
    CoreEventArgsPtr obj(CoreEventArgsPropertyAdded_Create(propOwner, prop));
    return obj;
}

/*!
 * @brief Creates Core event args that are passed as argument when a property is removed from a component.
 * @param propOwner The property object that owned the removed property.
 * @param propName The name of the property that was removed.
 */
inline CoreEventArgsPtr CoreEventArgsPropertyRemoved(const PropertyObjectPtr& propOwner, const StringPtr& propName)
{
    CoreEventArgsPtr obj(CoreEventArgsPropertyRemoved_Create(propOwner, propName));
    return obj;
}

/*!@}*/

END_NAMESPACE_OPENDAQ
