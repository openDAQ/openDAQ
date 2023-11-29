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
#include <coretypes/common.h>
#include <coretypes/baseobject.h>
#include <coretypes/stringobject.h>

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [templated]
 */

/*!
 * @ingroup types_events
 * @defgroup types_event_args EventArgs
 * @{
 */

DECLARE_OPENDAQ_INTERFACE_EX(IEventArgs, IBaseObject)
{
    DEFINE_INTFID("IEventArgs")

    virtual ErrCode INTERFACE_FUNC getEventId(Int* id) = 0;
    virtual ErrCode INTERFACE_FUNC getEventName(IString** name) = 0;
};

/*!
 * @}
 */

OPENDAQ_DECLARE_CLASS_FACTORY(LIBRARY_FACTORY, EventArgs, Int, eventId, IString*, eventName)

END_NAMESPACE_OPENDAQ
