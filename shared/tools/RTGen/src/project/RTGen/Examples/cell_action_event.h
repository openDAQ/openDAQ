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

﻿#pragma once
#include "mui/common.h"
#include "draw_grid_event_args.h"

BEGIN_MUI_NAMESPACE

static const RT::Core::IntfID IDSCellActionEventGuid = { 0x6B8D4191, 0x7852, 0x544D,{ 0x93, 0xBD, 0x09, 0x7D, 0x8C, 0xE6, 0xBD, 0xC1 } };

/*
 * [interfaceSmartPtr]
 * 
 * If defined for a base interface (IDrawGridEventArgs) the implementing interface SmartPtr (IDSCellActionEventArgs)
 * will inherit from the specified SmartPtr (DrawGridEventArgsPtr) otherwise it will be ObjectPtr<>.
 * 
 * The third optional parameter defines the include name to use for the base SmartPtr (DrawGridEventArgsPtr) 
 * if it does not conform to the convention (e.g. is not "draw_grid_event_args_ptr").
 */

/*#
 * [valueType(Itoa)]
 * [interfaceSmartPtr(IDrawGridEventArgs, DrawGridEventArgsPtr)]
 * [interfaceNamespace(IDSCellActionEventArgs, "Dewesoft::MUI::")]
 */
struct IDSCellActionEventArgs : public IDrawGridEventArgs
{
    DEFINE_INTFID(IDSCellActionEventGuid);

    virtual RT::Core::ErrCode INTERFACE_FUNC getOverrideAction(RT::Core::Bool* overrideAction) = 0;
    virtual RT::Core::ErrCode INTERFACE_FUNC setOverrideAction(RT::Core::Bool overrideAction) = 0;

    virtual RT::Core::ErrCode INTERFACE_FUNC getAbortMultiAction(RT::Core::Bool* abortAction) = 0;
    virtual RT::Core::ErrCode INTERFACE_FUNC setAbortMultiAction(RT::Core::Bool abortAction) = 0;

    /*
     * By default treat type names that don't start with capital I as value types
     */
    virtual RT::Core::ErrCode INTERFACE_FUNC getActionType(ActionType* actionType) = 0;
    virtual RT::Core::ErrCode INTERFACE_FUNC getModifierKeys(ModifiersState* modifiers) = 0;

    virtual RT::Core::ErrCode INTERFACE_FUNC getButtonDownState(RT::Core::Bool* state) = 0;
    virtual RT::Core::ErrCode INTERFACE_FUNC getIsPasting(RT::Core::Bool* pasting) = 0;

    /* 
     * Type that starts with capital I is expected to be an Interface
     * use the attribute to mark it as a value type with [valueType(TypeName)]
     */
    virtual RT::Core::ErrCode INTERFACE_FUNC getTest(Itoa* pasting) = 0;
    virtual RT::Core::ErrCode INTERFACE_FUNC setTest(Itoa pasting) = 0;
};

END_MUI_NAMESPACE
