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
#include <coretypes/objectptr.h>
#include <coretypes/string_ptr.h>
#include <coretypes/event_args.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @addtogroup types_event_args
 * @{
 */

template <typename Interface = IEventArgs>
class EventArgsPtr;

template <>
struct InterfaceToSmartPtr<IEventArgs>
{
    typedef EventArgsPtr<IEventArgs> SmartPtr;
};

template <typename Interface>
class EventArgsPtr : public ObjectPtr<Interface>
{
public:
    using ObjectPtr<Interface>::ObjectPtr;

    EventArgsPtr()
        : ObjectPtr<Interface>()
    {
    }

    EventArgsPtr(ObjectPtr<Interface>&& ptr)
        : ObjectPtr<Interface>(std::move(ptr))

    {
    }

    EventArgsPtr(const ObjectPtr<Interface>& ptr)
        : ObjectPtr<Interface>(ptr)
    {
    }

    EventArgsPtr(const EventArgsPtr& other)
        : ObjectPtr<Interface>(other)
    {
    }

    EventArgsPtr(EventArgsPtr&& other) noexcept
        : ObjectPtr<Interface>(std::move(other))
    {
    }

    EventArgsPtr& operator=(const EventArgsPtr& other)
    {
        if (this == &other)
            return *this;

        ObjectPtr<Interface>::operator=(other);
        return *this;
    }

    EventArgsPtr& operator=(EventArgsPtr&& other) noexcept
    {
        if (this == std::addressof(other))
        {
            return *this;
        }

        ObjectPtr<Interface>::operator=(std::move(other));
        return *this;
    }

    Int getEventId() const
    {
        if (this->object == nullptr)
            throw InvalidParameterException();

        Int id;
        auto errCode = this->object->getEventId(&id);
        checkErrorInfo(errCode);

        return id;
    }

    StringPtr getEventName() const
    {
        if (this->object == nullptr)
            throw InvalidParameterException();

        StringPtr name;
        auto errCode = this->object->getEventName(&name);
        checkErrorInfo(errCode);

        return name;
    }
};

/*!
 * @}
 */

END_NAMESPACE_OPENDAQ
