/*
 * Copyright 2022-2024 openDAQ d. o. o.
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
#include <coretypes/event_factory.h>

BEGIN_NAMESPACE_OPENDAQ

template <typename TSender, typename TEventArgs>
class Event;

template <typename TSender, typename TEventArgs>
class EventEmitter : public EventPtr<TSender, TEventArgs>
{
public:
    using EventPtr<TSender, TEventArgs>::EventPtr;

    EventEmitter()
        : EventPtr<TSender, TEventArgs>(Event_Create())
    {
    }

    EventEmitter(Event<TSender, TEventArgs> ev) : EventPtr<TSender, TEventArgs>(ev.eventPtr)
    {
    }

    EventEmitter& operator=(IEvent* other)
    {
        EventPtr<TSender, TEventArgs>::operator =(other);
        return *this;
    }
};

END_NAMESPACE_OPENDAQ
