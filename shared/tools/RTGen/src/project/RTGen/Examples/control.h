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
#include "component.h"

BEGIN_MUI_NAMESPACE

static const RT::Core::IntfID IControlGuid = { 0x0DBB6A0B, 0xB7C7, 0x57A3,{ 0xB8, 0xFF, 0x1C, 0xF4, 0x55, 0xC6, 0xDE, 0x06 } };

enum Visibility
{
    vVisible,
    vCollapsed,
    vHidden
};

typedef uint32_t Color;

/*#
* [templated(defaultAlias: false)]
* [includeHeader("<coretypes/funcobject_ptr.h>")]
* [interfaceSmartPtr(IFuncObject, FuncPtr)]
* [interfaceSmartPtr(IComponent, ComponentPtr)]
* [interfaceNamespace(IControl, "Dewesoft::MUI::")]
*/
struct IControl : IComponent
{
    DEFINE_INTFID(IControlGuid)

    virtual RT::Core::ErrCode INTERFACE_FUNC getWidth(RT::Core::SizeT* width) = 0;
    virtual RT::Core::ErrCode INTERFACE_FUNC setWidth(RT::Core::SizeT width) = 0;

    virtual RT::Core::ErrCode INTERFACE_FUNC getHeight(RT::Core::SizeT* height) = 0;
    virtual RT::Core::ErrCode INTERFACE_FUNC setHeight(RT::Core::SizeT height) = 0;

    virtual RT::Core::ErrCode INTERFACE_FUNC getMarginTop(RT::Core::SizeT* top) = 0;
    virtual RT::Core::ErrCode INTERFACE_FUNC setMarginTop(RT::Core::SizeT top) = 0;

    virtual RT::Core::ErrCode INTERFACE_FUNC getMarginRight(RT::Core::SizeT* right) = 0;
    virtual RT::Core::ErrCode INTERFACE_FUNC setMarginRight(RT::Core::SizeT right) = 0;

    virtual RT::Core::ErrCode INTERFACE_FUNC getMarginBottom(RT::Core::SizeT* bottom) = 0;
    virtual RT::Core::ErrCode INTERFACE_FUNC setMarginBottom(RT::Core::SizeT bottom) = 0;

    virtual RT::Core::ErrCode INTERFACE_FUNC getMarginLeft(RT::Core::SizeT* left) = 0;
    virtual RT::Core::ErrCode INTERFACE_FUNC setMarginLeft(RT::Core::SizeT left) = 0;

    virtual RT::Core::ErrCode INTERFACE_FUNC getIsEnabled(RT::Core::Bool* enabled) = 0;
    virtual RT::Core::ErrCode INTERFACE_FUNC setIsEnabled(RT::Core::Bool enabled) = 0;

    virtual RT::Core::ErrCode INTERFACE_FUNC getVisibility(Visibility* visibility) = 0;
    virtual RT::Core::ErrCode INTERFACE_FUNC setVisibility(Visibility visibility) = 0;

    virtual RT::Core::ErrCode INTERFACE_FUNC getBackground(Color* color) = 0;
    virtual RT::Core::ErrCode INTERFACE_FUNC setBackground(Color color) = 0;

    virtual RT::Core::ErrCode INTERFACE_FUNC invalidate() = 0;

    virtual RT::Core::ErrCode INTERFACE_FUNC getPaddingTop(RT::Core::SizeT* top) = 0;
    virtual RT::Core::ErrCode INTERFACE_FUNC setPaddingTop(RT::Core::SizeT top) = 0;

    virtual RT::Core::ErrCode INTERFACE_FUNC getPaddingRight(RT::Core::SizeT* right) = 0;
    virtual RT::Core::ErrCode INTERFACE_FUNC setPaddingRight(RT::Core::SizeT right) = 0;

    virtual RT::Core::ErrCode INTERFACE_FUNC getPaddingBottom(RT::Core::SizeT* bottom) = 0;
    virtual RT::Core::ErrCode INTERFACE_FUNC setPaddingBottom(RT::Core::SizeT bottom) = 0;

    virtual RT::Core::ErrCode INTERFACE_FUNC getPaddingLeft(RT::Core::SizeT* left) = 0;
    virtual RT::Core::ErrCode INTERFACE_FUNC setPaddingLeft(RT::Core::SizeT left) = 0;

    virtual RT::Core::ErrCode INTERFACE_FUNC getTooltip(RT::Core::IString** tooltip) = 0;
    virtual RT::Core::ErrCode INTERFACE_FUNC setTooltip(RT::Core::IString* tooltip) = 0;

    virtual RT::Core::ErrCode INTERFACE_FUNC isInvokeRequired(RT::Core::Bool* required) = 0;
    virtual RT::Core::ErrCode INTERFACE_FUNC invokeCallback(RT::Core::IFuncObject* callback) = 0;
    virtual RT::Core::ErrCode INTERFACE_FUNC scheduleCallback(RT::Core::IFuncObject* callback) = 0;
};

END_MUI_NAMESPACE
