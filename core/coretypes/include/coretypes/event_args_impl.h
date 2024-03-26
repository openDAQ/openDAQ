/*
 * Copyright 2022-2024 Blueberry d.o.o.
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
#include <utility>
#include <coretypes/coretypes.h>

BEGIN_NAMESPACE_OPENDAQ

template <typename... TInterfaces>
class EventArgsImplTemplate;

using EventArgsImpl = EventArgsImplTemplate<IEventArgs>;

template <typename... TInterfaces>
using EventArgsBase = EventArgsImplTemplate<TInterfaces...>;

template <typename... TInterfaces>
class EventArgsImplTemplate : public ImplementationOf<TInterfaces...>
{
public:
    EventArgsImplTemplate(Int id, const StringPtr& name)
        : eventId(id)
        , eventName(name)
    {
    }

    virtual ErrCode INTERFACE_FUNC getEventId(Int* id) override;
    virtual ErrCode INTERFACE_FUNC getEventName(IString** name) override;

protected:
    Int eventId;
    StringPtr eventName;
};

template <typename... TInterfaces>
ErrCode EventArgsImplTemplate<TInterfaces...>::getEventId(Int* id)
{
    *id = eventId;

    return OPENDAQ_SUCCESS;
}

template <typename... TInterfaces>
ErrCode EventArgsImplTemplate<TInterfaces...>::getEventName(IString** name)
{
    *name = eventName.addRefAndReturn();

    return OPENDAQ_SUCCESS;
}

END_NAMESPACE_OPENDAQ
