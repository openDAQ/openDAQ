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
#include <coretypes/event_handler_ptr.h>
#include <coretypes/event_args_ptr.h>
#include <coretypes/event.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @addtogroup types_event
 * @{
 */

template <typename TSender = BaseObjectPtr, typename TEventArgs = EventArgsPtr<>>
class EventPtr;

template <>
struct InterfaceToSmartPtr<IEvent>
{
    typedef EventPtr<> SmartPtr;
};

template <typename TSender, typename TEventArgs>
class EventPtr : public ObjectPtr<IEvent>
{
public:
    using ObjectPtr<IEvent>::ObjectPtr;

    EventPtr() : ObjectPtr<IEvent>()
    {
    }

    ~EventPtr() override
    {
    }

    EventPtr(ObjectPtr<IEvent>&& ptr) : ObjectPtr<IEvent>(std::move(ptr))
    {
    }

    EventPtr(const ObjectPtr<IEvent>& ptr) : ObjectPtr<IEvent>(ptr)
    {
    }

    EventPtr(const EventPtr& other) : ObjectPtr<IEvent>(other)
    {
    }

    EventPtr(EventPtr&& other) noexcept : ObjectPtr<IEvent>(std::move(other))
    {
    }

    EventPtr& operator=(const EventPtr& other)
    {
        if (this == &other)
            return *this;

        ObjectPtr<IEvent>::operator=(other);
        return *this;
    }

    EventPtr& operator=(EventPtr&& other) noexcept
    {
        if (this == std::addressof(other))
        {
            return *this;
        }

        ObjectPtr<IEvent>::operator=(std::move(other));
        return *this;
    }

    void operator()(TSender& sender, TEventArgs& args) const
    {
        trigger(sender, args);
    }

    void operator()(TSender& sender, TEventArgs&& args) const
    {
        trigger(sender, args);
    }

    void addHandler(const EventHandlerPtr<TSender, TEventArgs>& eventHandler) const
    {
        if (this->object == nullptr)
            throw InvalidParameterException();

        auto errCode = this->object->addHandler(eventHandler);
        checkErrorInfo(errCode);
    }

    void removeHandler(const EventHandlerPtr<TSender, TEventArgs>& eventHandler) const
    {
        if (this->object == nullptr)
            throw InvalidParameterException();

        auto errCode = this->object->removeHandler(eventHandler);
        checkErrorInfo(errCode);
    }

    void trigger(TSender& sender, TEventArgs& args) const
    {
        if (this->object == nullptr)
            throw InvalidParameterException();

        auto errCode = this->object->trigger(sender, args);
        checkErrorInfo(errCode);
    }

    void clear() const
    {
        if (this->object == nullptr)
            throw InvalidParameterException();

        auto errCode = this->object->clear();
        checkErrorInfo(errCode);
    }

    bool hasListeners() const
    {
        return object != nullptr
            ? getListenerCount() > 0u
            : false;
    }

    SizeT getListenerCount() const
    {
        if (this->object == nullptr)
            throw InvalidParameterException();

        SizeT count;
        auto errCode = this->object->getSubscriberCount(&count);
        checkErrorInfo(errCode);

        return count;
    }

    void mute() const
    {
        if (this->object == nullptr)
            throw InvalidParameterException();

        auto errCode = this->object->mute();
        checkErrorInfo(errCode);
    }

    void unmute() const
    {
        if (this->object == nullptr)
            throw InvalidParameterException();

        auto errCode = this->object->unmute();
        checkErrorInfo(errCode);
    }

    void muteListener(const EventHandlerPtr<TSender, TEventArgs>& eventHandler) const
    {
        if (this->object == nullptr)
            throw InvalidParameterException();

        auto errCode = this->object->muteListener(eventHandler);
        checkErrorInfo(errCode);
    }

    void unmuteListener(const EventHandlerPtr<TSender, TEventArgs>& eventHandler) const
    {
        if (this->object == nullptr)
            throw InvalidParameterException();

        auto errCode = this->object->unmuteListener(eventHandler);
        checkErrorInfo(errCode);
    }
};

/*!
 * @}
 */

END_NAMESPACE_OPENDAQ
