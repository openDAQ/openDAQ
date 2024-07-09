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
#include <coretypes/listobject.h>
#include "mui/common.h"
#include "mui/event_handler.h"
#include "control.h"

BEGIN_MUI_NAMESPACE

static const RT::Core::IntfID IComboBoxGuid = { 0x629F8449, 0x3C91, 0x5687, { 0x8F, 0x8C, 0xF7, 0xD2, 0x08, 0x24, 0x9B, 0xFB } };

/*# 
 * [interfaceSmartPtr(IControl, ControlPtr)]
 * [interfaceNamespace(IComboBox, "Dewesoft::MUI::")]
 */
struct IComboBox : IControl
{
    DEFINE_INTFID(IComboBoxGuid);

    //[elementType(name, IString)]
    virtual RT::Core::ErrCode INTERFACE_FUNC getItems(RT::Core::IList** name) = 0;
    virtual RT::Core::ErrCode INTERFACE_FUNC getCount(RT::Core::SizeT* count) = 0;

    virtual RT::Core::ErrCode INTERFACE_FUNC getIndexOf(RT::Core::IString* item, RT::Core::Int* index) = 0;

    virtual RT::Core::ErrCode INTERFACE_FUNC getItemAt(RT::Core::SizeT index, RT::Core::IString** item) = 0;
    virtual RT::Core::ErrCode INTERFACE_FUNC setItemAt(RT::Core::SizeT index, RT::Core::IString* item) = 0;

    virtual RT::Core::ErrCode INTERFACE_FUNC addItem(RT::Core::IString* item) = 0;
    virtual RT::Core::ErrCode INTERFACE_FUNC insertItemAt(RT::Core::SizeT index, RT::Core::IString* item) = 0;
    virtual RT::Core::ErrCode INTERFACE_FUNC deleteItemAt(RT::Core::SizeT index) = 0;
    virtual RT::Core::ErrCode INTERFACE_FUNC clear() = 0;

    virtual RT::Core::ErrCode INTERFACE_FUNC clearSelection() = 0;

    virtual RT::Core::ErrCode INTERFACE_FUNC getSelectedIndex(RT::Core::SizeT* index) = 0;
    virtual RT::Core::ErrCode INTERFACE_FUNC setSelectedIndex(RT::Core::SizeT index) = 0;

    virtual RT::Core::ErrCode INTERFACE_FUNC getSelectedItem(RT::Core::IString** item) = 0;
    virtual RT::Core::ErrCode INTERFACE_FUNC setSelectedItem(RT::Core::IString* item) = 0;

    //[event(Add, OnChange)]
    virtual RT::Core::ErrCode INTERFACE_FUNC addOnChange(IEventHandler* handler) = 0;

    //[event(Remove, OnChange)]
    virtual RT::Core::ErrCode INTERFACE_FUNC removeOnChange(IEventHandler* handler) = 0;
};

END_MUI_NAMESPACE
