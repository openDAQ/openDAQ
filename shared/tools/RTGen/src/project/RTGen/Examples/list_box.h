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

BEGIN_MUI_NAMESPACE

static const RT::Core::IntfID IListBoxGuid = { 0x4535FE35, 0x4CA0, 0x54C6,{ 0x85, 0x29, 0x10, 0xBB, 0x53, 0xCD, 0xC2, 0x01 } };

enum ListBoxSelectionMode : RT::Core::EnumType
{
    smSingle,
    smExtended,
    smMultiple
};

/*#
 * [uiControl]
 * [smartPtrSuffix("")]
 * [interfaceSmartPtr(IControl, ControlPtr)]
 * [interfaceNamespace(IListBox, "Dewesoft::MUI::")]
 */
struct IListBox : IControl
{
    DEFINE_INTFID(IListBoxGuid)

    // [elementType(items, "RT::Core::IString")]
    virtual RT::Core::ErrCode INTERFACE_FUNC getItems(RT::Core::IList** items) = 0;
    virtual RT::Core::ErrCode INTERFACE_FUNC getCount(RT::Core::SizeT* count) = 0;

    virtual RT::Core::ErrCode INTERFACE_FUNC getIndexOf(RT::Core::IString* item, RT::Core::Int* index) = 0;

    virtual RT::Core::ErrCode INTERFACE_FUNC getItemAt(RT::Core::SizeT index, RT::Core::IString** item) = 0;
    virtual RT::Core::ErrCode INTERFACE_FUNC setItemAt(RT::Core::SizeT index, RT::Core::IString* item) = 0;

    virtual RT::Core::ErrCode INTERFACE_FUNC addItem(RT::Core::IString* text)  = 0;
    virtual RT::Core::ErrCode INTERFACE_FUNC insertItemAt(RT::Core::SizeT index, RT::Core::IString* item) = 0;
    virtual RT::Core::ErrCode INTERFACE_FUNC deleteItemAt(RT::Core::SizeT index) = 0;

    virtual RT::Core::ErrCode INTERFACE_FUNC clear() = 0;

    virtual RT::Core::ErrCode INTERFACE_FUNC clearSelection() = 0;
    virtual RT::Core::ErrCode INTERFACE_FUNC selectAll() = 0;
    virtual RT::Core::ErrCode INTERFACE_FUNC deleteSelected() = 0;

    // meant to be used when SelectionMode == Single
    virtual RT::Core::ErrCode INTERFACE_FUNC getSelectedIndex(RT::Core::Int* selectedIndex) = 0;
    virtual RT::Core::ErrCode INTERFACE_FUNC setSelectedIndex(RT::Core::Int selectedIndex) = 0;
    virtual RT::Core::ErrCode INTERFACE_FUNC getSelectedItem(RT::Core::IString** selectedItem) = 0;

    virtual RT::Core::ErrCode INTERFACE_FUNC selectItemAt(RT::Core::Int index) = 0;
    virtual RT::Core::ErrCode INTERFACE_FUNC deselectItemAt(RT::Core::Int index) = 0;
    virtual RT::Core::ErrCode INTERFACE_FUNC selectItem(RT::Core::IString* item) = 0;
    virtual RT::Core::ErrCode INTERFACE_FUNC deselectItem(RT::Core::IString* item) = 0;
    virtual RT::Core::ErrCode INTERFACE_FUNC getIsSelected(RT::Core::IString* item, RT::Core::Bool* isSelected) = 0;
    virtual RT::Core::ErrCode INTERFACE_FUNC getSelectedCount(RT::Core::SizeT* count) = 0;

    // [elementType(selectedItems, "RT::Core::IString")]
    virtual RT::Core::ErrCode INTERFACE_FUNC getSelectedItems(RT::Core::IList** selectedItems) = 0;

    virtual RT::Core::ErrCode INTERFACE_FUNC getSelectionMode(ListBoxSelectionMode* selectedMode) = 0;
    virtual RT::Core::ErrCode INTERFACE_FUNC setSelectionMode(ListBoxSelectionMode selectionMode) = 0;

    // [event(Add, OnClick)]
    virtual RT::Core::ErrCode INTERFACE_FUNC addOnClick(IEventHandler* handler) = 0;

    // [event(Remove, OnClick)]
    virtual RT::Core::ErrCode INTERFACE_FUNC removeOnClick(IEventHandler* handler) = 0;

    // [event(Add, OnSelectedIndexChanged)]
    virtual RT::Core::ErrCode INTERFACE_FUNC addOnSelectedIndexChange(IEventHandler* handler) = 0;

    // [event(Remove, OnSelectedIndexChanged)]
    virtual RT::Core::ErrCode INTERFACE_FUNC removeOnSelectedIndexChange(IEventHandler* handler) = 0;
};

END_MUI_NAMESPACE
