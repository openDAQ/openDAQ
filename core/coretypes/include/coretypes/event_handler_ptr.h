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
#include <coretypes/event_handler.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @addtogroup types_event_handler
 * @{
 */

template <typename TSender = IBaseObject, typename TEventArgs = IEventArgs>
class EventHandlerPtr;

using EventHandler = EventHandlerPtr<>;

template <>
struct InterfaceToSmartPtr<IEventHandler>
{
    typedef EventHandlerPtr<> SmartPtr;
};

template <typename TSender, typename TEventArgs>
class EventHandlerPtr : public ObjectPtr<IEventHandler>
{
public:
    using SenderPtr = typename InterfaceToSmartPtr<TSender>::SmartPtr;
    using EventArgsPtr = typename InterfaceToSmartPtr<TEventArgs>::SmartPtr;

    using ObjectPtr<IEventHandler>::ObjectPtr;

    EventHandlerPtr()
        : ObjectPtr<IEventHandler>()
    {
    }

    EventHandlerPtr(ObjectPtr<IEventHandler>&& ptr)
        : ObjectPtr<IEventHandler>(ptr)
    {
    }

    EventHandlerPtr(const ObjectPtr<IEventHandler>& ptr)
        : ObjectPtr<IEventHandler>(ptr)
    {
    }

    EventHandlerPtr(const EventHandlerPtr& other)
        : ObjectPtr<IEventHandler>(other)
    {
    }

    EventHandlerPtr(EventHandlerPtr&& other) noexcept
        : ObjectPtr<IEventHandler>(std::move(other))
    {
    }

    EventHandlerPtr& operator=(const EventHandlerPtr& other)
    {
        if (this == &other)
            return *this;

        ObjectPtr<IEventHandler>::operator =(other);
        return *this;
    }

    EventHandlerPtr& operator=(EventHandlerPtr&& other) noexcept
    {
        if (this == std::addressof(other))
            return *this;
        ObjectPtr<IEventHandler>::operator =(std::move(other));
        return *this;
    }

    void handleEvent(const SenderPtr& sender, const EventArgsPtr& eventArgs) const
    {
        if (this->object == nullptr)
            throw InvalidParameterException();

        auto errCode = this->object->handleEvent(sender, eventArgs);
        checkErrorInfo(errCode);
    }
};

/*!
 * @}
 */

END_NAMESPACE_OPENDAQ
