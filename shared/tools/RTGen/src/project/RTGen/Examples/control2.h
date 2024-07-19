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
#include <coretypes/stringobject.h>

BEGIN_MUI_NAMESPACE

static const RT::Core::IntfID IControlGuid = { 0x0DBB6A0B, 0xB7C7, 0x57A3,{ 0xB8, 0xFF, 0x1C, 0xF4, 0x55, 0xC6, 0xDE, 0x06 } };

//[interfaceNamespace(IControl, "Dewesoft::MUI::")]
struct IControl : RT::Core::IBaseObject
{
    DEFINE_INTFID(IControlGuid)

    virtual RT::Core::ErrCode INTERFACE_FUNC getName(RT::Core::IString** name) = 0;
    virtual RT::Core::ErrCode INTERFACE_FUNC getType(RT::Core::IString** name) = 0;
};

END_MUI_NAMESPACE
