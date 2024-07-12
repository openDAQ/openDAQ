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
#include "mui/common.h"
#include "mui/controls/control.h"

BEGIN_MUI_NAMESPACE

static const RT::Core::IntfID IParentControlGuid = { 0xD8A17490, 0x5C9C, 0x5563, { 0xB0, 0x8F, 0x6B, 0x44, 0x96, 0x1A, 0xF5, 0x14 } };

/*#
 * [uiControl(ForwardDeclare)]
 * [templated(IControl, IParentControl)]
 * [interfaceNamespace(IParentControl, "Dewesoft::MUI::")]
 * [interfaceSmartPtr(IControl, ControlPtr, "<mui/controls/control_ptr.h>")]
 */
struct IParentControl : IControl
{
    DEFINE_INTFID(IParentControlGuid)

    virtual RT::Core::ErrCode INTERFACE_FUNC findControl(RT::Core::IString* name, IControl** control) = 0;
    virtual RT::Core::ErrCode INTERFACE_FUNC getChildCount(RT::Core::SizeT* count) = 0;
    virtual RT::Core::ErrCode INTERFACE_FUNC setUI(RT::Core::IString* ui) = 0;
};

END_MUI_NAMESPACE
