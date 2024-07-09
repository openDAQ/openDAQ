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

#pragma once
#include "mui/common.h"
#include "mui/controls/control.h"
#include <coretypes/stringobject.h>

BEGIN_MUI_NAMESPACE

static const RT::Core::IntfID IControlFactoryGuid = { 0xBF370521, 0x8C76, 0x56E9, { 0xB3, 0xE7, 0x15, 0x0C, 0x29, 0x1C, 0xD8, 0x21 } };

/*#
 * [decorated]
 * [smartPtrSuffix("")]
 * [includeHeader("controls/control_ptr.h")]
 * [interfaceNamespace(IControlFactory, "Dewesoft::MUI::")]
 */
struct IControlFactory : RT::Core::IBaseObject
{
    DEFINE_INTFID(IControlFactoryGuid);

    virtual RT::Core::ErrCode INTERFACE_FUNC createControl(RT::Core::IString* tagName, RT::Core::IString* controlName, IControl** control) = 0;
};

END_MUI_NAMESPACE
