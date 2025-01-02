/*
 * Copyright 2022-2025 openDAQ d.o.o.
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
#include <coreobjects/core_event_args_factory.h>
#include <opendaq/core_opendaq_event_args.h>
#include <opendaq/signal_ptr.h>
#include <opendaq/device_domain_ptr.h>

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
inline CoreEventArgsPtr CoreEventArgsComponentAdded(const ComponentPtr& component)
{
    CoreEventArgsPtr obj(CoreEventArgsComponentAdded_Create(component));
    return obj;
}

/*!
 * @brief Creates Core event args that are passed as argument when a component is removed from the list of children.
 * @param componentId The local ID of the removed component.
 *
 * The sender of the event is always the parent of the removed component.
 *
 * The ID of the event is 50, and the event name is "ComponentRemoved".
 */
inline CoreEventArgsPtr CoreEventArgsComponentRemoved(const StringPtr& componentId)
{
    CoreEventArgsPtr obj(CoreEventArgsComponentRemoved_Create(componentId));
    return obj;
}

/*!
 * @brief Creates Core event args that are passed as argument when a signal is connected to an input port.
 * @param signal The connected signal.
 *
 * The sender of the event is always the input port into which the signal was connected.
 *
 * The ID of the event is 60, and the event name is "SignalConnected".
 */
inline CoreEventArgsPtr CoreEventArgsSignalConnected(const SignalPtr& signal)
{
    CoreEventArgsPtr obj(CoreEventArgsSignalConnected_Create(signal));
    return obj;
}

/*!
 * @brief Creates Core event args that are passed as argument when a signal is connected to an input port.
 *
 * The sender of the event is always the input port from which the signal was disconnected. The parameters
 * of this event are empty.
 *
 * The ID of the event is 70, and the event name is "SignalDisconnected".
 */
inline CoreEventArgsPtr CoreEventArgsSignalDisconnected()
{
    CoreEventArgsPtr obj(CoreEventArgsSignalDisconnected_Create());
    return obj;
}

/*!
 * @brief Creates Core event args that are passed as argument when the descriptor of a signal changes.
 * @param descriptor The new descriptor.
 *
 * The sender of the event is always the signal of which descriptor was changed.
 *
 * The ID of the event is 80, and the event name is "DataDescriptorChanged".
 */
inline CoreEventArgsPtr CoreEventArgsDataDescriptorChanged(const DataDescriptorPtr& descriptor)
{
    CoreEventArgsPtr obj(CoreEventArgsDataDescriptorChanged_Create(descriptor));
    return obj;
}

/*!
 * @brief Creates Core event args that are passed as argument when a component is finished updating.
 *
 * A component finished updating at the end of the `update` call.
 * The ID of the event is 90, and the event name is "ComponentUpdateEnd".
 */
inline CoreEventArgsPtr CoreEventArgsComponentUpdateEnd()
{
    CoreEventArgsPtr obj(CoreEventArgsComponentUpdateEnd_Create());
    return obj;
}

/*!
 * @brief Creates Core event args that are passed as argument when a component's internal attribute
 * is modified.
 * @param attributeName The name of the changed attribute.
 * @param attributeValue The new value of the attribute.
 *
 * An example of such attribute are the "Active" and "Visible" states of a component.
 *
 * The ID of the event is 100, and the event name is "AttributeChanged".
 */
inline CoreEventArgsPtr CoreEventArgsAttributeChanged(const StringPtr& attributeName, const BaseObjectPtr& attributeValue)
{
    CoreEventArgsPtr obj(CoreEventArgsAttributeChanged_Create(attributeName, attributeValue));
    return obj;
}

/*!
 * @brief Creates Core event args that are passed as argument when a tag is added/removed from a component.
 * @param tags The list of tags (as strings).
 *
 * The ID of the event is 110, and the event name is "TagsChanged".
 */
inline CoreEventArgsPtr CoreEventArgsTagsChanged(const ListPtr<IString>& tags)
{
    CoreEventArgsPtr obj(CoreEventArgsTagsChanged_Create(tags));
    return obj;
}

/*!
 * @brief Creates Core event args that are passed as argument when the domain of a device changes.
 * @param deviceDomain The new device domain
 *
 * The ID of the event is 150, and the event name is "DeviceDomainChanged".
 */
inline CoreEventArgsPtr CoreEventArgsDeviceDomainChanged(const DeviceDomainPtr& deviceDomain)
{
    CoreEventArgsPtr obj(CoreEventArgsDeviceDomainChanged_Create(deviceDomain));
    return obj;
}

/*!
 * @brief Creates Core event args that are passed as argument when the device is locked or unlocked.
 * @param isLocked New lock state of the device.
 *
 * The ID of the event is 160, and the event name is "DeviceLockStateChanged".
 */
inline CoreEventArgsPtr CoreEventArgsDeviceLockStateChanged(Bool isLocked)
{
    CoreEventArgsPtr obj(CoreEventArgsDeviceLockStateChanged_Create(isLocked));
    return obj;
}


/*!@}*/

END_NAMESPACE_OPENDAQ
