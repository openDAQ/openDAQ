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
#include "mui/common.h"
#include "control.h"
#include "mui/event_handler.h"
#include <coretypes/stringobject.h>

BEGIN_MUI_NAMESPACE

static const RT::Core::IntfID IButtonGuid = { 0x961AB5B3, 0xF178, 0x505E, { 0x83, 0x46, 0xF5, 0xE8, 0x43, 0x22, 0xC6, 0xEF } };

/*#
 * [uiControl]
 * [smartPtrSuffix("")]
 * [interfaceSmartPtr(IControl, ControlPtr)]
 * [includeHeader("mui/event_args_ptr.h")]
 * [includeHeader("mui/event_handler_ptr.h")]
 * [interfaceNamespace(IButton, "Dewesoft::MUI::")]
 */
DECLARE_RT_INTERFACE(IButton, IControl)
{
    DEFINE_INTFID("IButtonGuid")

    virtual RT::Core::ErrCode INTERFACE_FUNC setCaption(RT::Core::IString* name) = 0;
    virtual RT::Core::ErrCode INTERFACE_FUNC getCaption(RT::Core::IString** name) = 0;

    //[event(Add, OnClick)]
    virtual RT::Core::ErrCode INTERFACE_FUNC addOnClick(IEventHandler* handler) = 0;

    //[event(Remove, OnClick)]
    virtual RT::Core::ErrCode INTERFACE_FUNC removeOnClick(IEventHandler* handler) = 0;
};

END_MUI_NAMESPACE
