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
#include <coretypes/baseobject_factory.h>
#include <coretypes/objectptr.h>
#include <coretypes/event_wrapper.h>
#include <coretypes/event_factory.h>
#include <coretypes/event_args_factory.h>

BEGIN_NAMESPACE_OPENDAQ

DECLARE_OPENDAQ_INTERFACE(IHasEvent, IBaseObject)
{
};

class HasEvent : public ImplementationOf<IHasEvent>
{
public:

    HasEvent()
        : callCount(0)
        , onEvent(EventObject<BaseObjectPtr, EventArgsPtr<>>())
    {
    }

    void triggerEvent()
    {
        EventEmitter<BaseObjectPtr, EventArgsPtr<>> emitter(onEvent);
        EventArgsPtr<> args = EventArgs(0, "OnEvent");

        auto borrowed = borrowPtr<BaseObjectPtr>();

        emitter(borrowed, args);
    }

    int callCount;
    Event<BaseObjectPtr, EventArgsPtr<>> onEvent;
};

class Base
{
public:
    virtual ~Base()
    {
    }

    virtual void onEvent(BaseObjectPtr& /*sender*/, EventArgsPtr<IEventArgs>& /*args*/)
    {
        ASSERT_FALSE(true) << "Base";
    }
};

class Derived : public Base
{
public:
    void onEvent(BaseObjectPtr& /*sender*/, EventArgsPtr<IEventArgs>& /*args*/) override
    {
        printf("Derived!\n");
        callCount++;
    }

    void onEvent2(BaseObjectPtr& /*sender*/, EventArgsPtr<IEventArgs>& /*args*/) const
    {
    }

    int getCallCount() const
    {
        return callCount;
    }

private:
    int callCount{};
};

END_NAMESPACE_OPENDAQ
