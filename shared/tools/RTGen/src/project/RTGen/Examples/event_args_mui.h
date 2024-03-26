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
#include "common.h"
#include <coretypes/stringobject.h>

BEGIN_MUI_NAMESPACE

static const RT::Core::IntfID IEventArgsGuid = { 0x033CE044, 0xC292, 0x5601, { 0xBE, 0x84, 0x82, 0x7A, 0xD8, 0x64, 0xD6, 0xA7 } };

/*
 * templated => EventArgsPtr is a C++ template
 * 
 * By default EventArgsPtr<> is EventArgsPtr<IEventArgs> that inherits ObjectPtr<Interface>
 * 
 * If [templated] is NOT specified it would generate non templated EventArgsPtr
 * that inherits ObjectPtr<IEventArgs>
 */

/*#
 * [templated]
 * [interfaceNamespace(IEventArgs, "Dewesoft::MUI::")]
 */
struct IEventArgs : Dewesoft::RT::Core::IBaseObject
{
    DEFINE_INTFID(IEventArgsGuid)

    /*
     * By default treat types that start with capital I as interface objects.
     * With exceptions e.g. Int.
     */
    virtual RT::Core::ErrCode INTERFACE_FUNC getEventId(RT::Core::Int* id) = 0;
    virtual RT::Core::ErrCode INTERFACE_FUNC getEventName(RT::Core::IString** name) = 0;
};

END_MUI_NAMESPACE
