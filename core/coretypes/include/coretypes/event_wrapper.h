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
#include <coretypes/delegate.hpp>
#include <coretypes/common.h>
#include <coretypes/event_ptr.h>
#include <coretypes/event_handler_impl.h>

BEGIN_NAMESPACE_OPENDAQ

template <typename TSender, typename TEventArgs>
class Event
{
public:
    using Subscription = delegate<void(TSender&, TEventArgs&)>;
    using HandlerPtr = EventHandlerPtr<TSender, TEventArgs>;
    using EventHandlerImplType = EventHandlerImpl<TSender, TEventArgs>;

    Event(EventPtr<TSender, TEventArgs> ptr)
        : eventPtr(std::move(ptr))
    {
    }

    void operator+=(Subscription&& sub)
    {
        if (!sub)
        {
            throw InvalidParameterException("Must bind to a valid callable.");
        }

        if (!eventPtr.assigned())
        {
            throw InvalidParameterException("Invalid or uninitialized control.");
        }

        HandlerPtr handler;
        ErrCode err = createObjectForwarding<IEventHandler, EventHandlerImplType>(&handler, sub);
        checkErrorInfo(err);

        eventPtr.addHandler(handler);
    }

    void operator+=(const Subscription& sub)
    {
        if (!sub)
        {
            throw InvalidParameterException("Must bind to a valid callable.");
        }

        if (!eventPtr.assigned())
        {
            throw InvalidParameterException("Invalid or uninitialized event.");
        }

        HandlerPtr handler;
        ErrCode err = createObjectForwarding<IEventHandler, EventHandlerImplType>(&handler, sub);
        checkErrorInfo(err);

        eventPtr.addHandler(handler);
    }

    Event& operator =(std::nullptr_t)
    {
        if (!eventPtr.assigned())
        {
            throw InvalidParameterException("Invalid or uninitialized event.");
        }

        eventPtr.clear();
        return *this;
    }

    void operator-=(const Subscription& sub)
    {
        if (!sub)
        {
            throw InvalidParameterException("Must bind to a valid callable.");
        }

        if (!eventPtr.assigned())
        {
            throw InvalidParameterException("Invalid or uninitialized event.");
        }

        HandlerPtr handler;
        createObjectForwarding<IEventHandler, EventHandlerImplType>(&handler, sub);
        eventPtr.removeHandler(handler);
    }

    void operator-=(Subscription&& sub)
    {
        if (!sub)
        {
            throw InvalidParameterException("Must bind to a valid callable.");
        }

        if (!eventPtr.assigned())
        {
            throw InvalidParameterException("Invalid or uninitialized control.");
        }

        HandlerPtr handler;
        ErrCode err = createObjectForwarding<IEventHandler, EventHandlerImplType>(&handler, sub);
        checkErrorInfo(err);

        eventPtr.removeHandler(handler);
    }

    SizeT getListenerCount()
    {
        if (!eventPtr.assigned())
        {
            throw InvalidParameterException("Invalid or uninitialized event.");
        }

        return eventPtr.getListenerCount();
    }

    ListPtr<IEventHandler> getListeners()
    {
        if (!eventPtr.assigned())
        {
            throw InvalidParameterException("Invalid or uninitialized event.");
        }

        return eventPtr.getListeners();
    }

    void mute() const
    {
        if (!eventPtr.assigned())
        {
            throw InvalidParameterException("Invalid or uninitialized event.");
        }

        eventPtr.mute();
    }

    void unmute() const
    {
        if (!eventPtr.assigned())
        {
            throw InvalidParameterException("Invalid or uninitialized event.");
        }

        eventPtr.unmute();
    }

    Event& operator|=(Subscription&& sub)
    {
        muteListener(std::forward<decltype(sub)>(sub));
        return *this;
    }

    void muteListener(Subscription&& sub)
    {
        if (!sub)
        {
            throw InvalidParameterException("Must bind to a valid callable.");
        }

        if (!eventPtr.assigned())
        {
            throw InvalidParameterException("Invalid or uninitialized event.");
        }

        HandlerPtr handler;
        ErrCode err = createObjectForwarding<IEventHandler, EventHandlerImplType>(&handler, sub);
        checkErrorInfo(err);

        eventPtr->muteListener(handler);
    }

    Event& operator&=(Subscription&& sub)
    {
        unmuteListener(std::forward<decltype(sub)>(sub));
        return *this;
    }

    void unmuteListener(Subscription&& sub)
    {
        if (!sub)
        {
            throw InvalidParameterException("Must bind to a valid callable.");
        }

        if (!eventPtr.assigned())
        {
            throw InvalidParameterException("Invalid or uninitialized event.");
        }

        HandlerPtr handler;
        ErrCode err = createObjectForwarding<IEventHandler, EventHandlerImplType>(&handler, sub);
        checkErrorInfo(err);

        eventPtr->unmuteListener(handler);
    }

    explicit operator bool() const
    {
        return eventPtr.assigned();
    }

    operator IEvent*() const
    {
        return eventPtr;
    }

private:
    friend EventEmitter<TSender, TEventArgs>;

    EventPtr<TSender, TEventArgs> eventPtr;
};

END_NAMESPACE_OPENDAQ
