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
#include <coretypes/common.h>
#include <coretypes/baseobject.h>
#include <coretypes/event_handler.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup types_events
 * @defgroup types_event Event
 * @{
 */

typedef std::size_t HandlerId;

DECLARE_OPENDAQ_INTERFACE(IEvent, IBaseObject)
{
    virtual ErrCode INTERFACE_FUNC addHandler(IEventHandler* eventHandler) = 0;
    virtual ErrCode INTERFACE_FUNC removeHandler(IEventHandler* eventHandler) = 0;

    virtual ErrCode INTERFACE_FUNC trigger(IBaseObject* sender, IEventArgs* args) = 0;

    virtual ErrCode INTERFACE_FUNC clear() = 0;
    virtual ErrCode INTERFACE_FUNC getSubscriberCount(SizeT* count) = 0;

    virtual ErrCode INTERFACE_FUNC mute() = 0;
    virtual ErrCode INTERFACE_FUNC unmute() = 0;

    virtual ErrCode INTERFACE_FUNC muteListener(IEventHandler* eventHandler) = 0;
    virtual ErrCode INTERFACE_FUNC unmuteListener(IEventHandler* eventHandler) = 0;
};

/*!
 * @}
 */

OPENDAQ_DECLARE_CLASS_FACTORY(LIBRARY_FACTORY, Event)

END_NAMESPACE_OPENDAQ
