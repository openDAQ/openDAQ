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
#include "mui/common.h"
#include <coretypes/stringobject.h>
#include <coretypes/listobject.h>
#include "control.h"
#include "mui/event_handler.h"
#include "draw_grid/common.h"
#include "draw_grid/draw_grid_column.h"

BEGIN_MUI_NAMESPACE

static const RT::Core::IntfID IDrawGridGuid = { 0xF161449B, 0xE1DD, 0x51F7, { 0xB8, 0xBE, 0x1C, 0x99, 0x91, 0xC7, 0xC3, 0x08 } };

/*#
* [uiControl]
* [smartPtrSuffix("")]
* [includeHeader("draw_grid/event_args.h")]
* [includeHeader("draw_grid/draw_grid_column_ptr.h")]
* [interfaceNamespace(IDSDrawGrid, "Dewesoft::MUI::")]
* [interfaceSmartPtr(IControl, ControlPtr)]
* [interfaceSmartPtr(IDSDrawGrid, DSDrawGrid)]
*/
struct IDSDrawGrid : IControl
{
    DEFINE_INTFID(IDrawGridGuid)

    //[elementType(columns, IDrawGridColumn)]
    virtual RT::Core::ErrCode INTERFACE_FUNC getColumns(RT::Core::IList** columns) = 0;
    virtual RT::Core::ErrCode INTERFACE_FUNC getColumn(RT::Core::Int index, IDrawGridColumn** column) = 0;

    virtual RT::Core::ErrCode INTERFACE_FUNC setColumn(
        RT::Core::Int index,
        RT::Core::IString* name,
        DrawGridCellType colType,
        RT::Core::Bool visible,
        RT::Core::Int width,
        RT::Core::IString* uniqueKey
    ) = 0;

    virtual RT::Core::ErrCode INTERFACE_FUNC applyColumns() = 0;

    virtual RT::Core::ErrCode INTERFACE_FUNC applyRows() = 0;
    virtual RT::Core::ErrCode INTERFACE_FUNC applyFilter(RT::Core::IString* filterStr, RT::Core::Bool force) = 0;

    virtual RT::Core::ErrCode INTERFACE_FUNC refreshGroups() = 0;

    virtual RT::Core::ErrCode INTERFACE_FUNC setRowCount(RT::Core::Int rowCount) = 0;
    virtual RT::Core::ErrCode INTERFACE_FUNC setColumnCount(RT::Core::Int colCount) = 0;
    virtual RT::Core::ErrCode INTERFACE_FUNC setGridSize(RT::Core::Int rowCount, RT::Core::Int columnCount) = 0;

    virtual RT::Core::ErrCode INTERFACE_FUNC selectCell(RT::Core::Int column, RT::Core::Int row, RT::Core::Bool* selected) = 0;

    virtual RT::Core::ErrCode INTERFACE_FUNC invalidateColumn(RT::Core::Int realCol) = 0;
    virtual RT::Core::ErrCode INTERFACE_FUNC invalidateRow(RT::Core::Int realRow) = 0;

    virtual RT::Core::ErrCode INTERFACE_FUNC getCellAt(RT::Core::Int x, RT::Core::Int y, RT::Core::Int* realRow, RT::Core::Int* realCol) = 0;

    virtual RT::Core::ErrCode INTERFACE_FUNC selectRow(RT::Core::Int realRow, RT::Core::Bool moveAnchor) = 0;

    virtual RT::Core::ErrCode INTERFACE_FUNC isCellSelected(RT::Core::Int col, RT::Core::Int row, RT::Core::Bool* selected) = 0;
    virtual RT::Core::ErrCode INTERFACE_FUNC isRowSelected(RT::Core::Int row, RT::Core::Bool* selected) = 0;

    virtual RT::Core::ErrCode INTERFACE_FUNC moveCursorRowAndClearSelection(RT::Core::Int relativeRowOffset) = 0;

    //[elementType(selectedRows, "RT::Core::IInteger")]
    virtual RT::Core::ErrCode INTERFACE_FUNC selectRows(RT::Core::IList* selectedRows) = 0;
    virtual RT::Core::ErrCode INTERFACE_FUNC resort() = 0;
    virtual RT::Core::ErrCode INTERFACE_FUNC sort(RT::Core::Int column, RT::Core::Bool incremental) = 0;

    virtual RT::Core::ErrCode INTERFACE_FUNC getFirstSelectableColumnIndex(RT::Core::Int* index) = 0;

    // { Get / Set }

    virtual RT::Core::ErrCode INTERFACE_FUNC getFilter(RT::Core::IString** value) = 0;
    virtual RT::Core::ErrCode INTERFACE_FUNC setFilter(RT::Core::IString* value) = 0;

    virtual RT::Core::ErrCode INTERFACE_FUNC getSortedColumn(RT::Core::Int realCol, RT::Core::Int* sortedCol) = 0;
    virtual RT::Core::ErrCode INTERFACE_FUNC getSortedRow(RT::Core::Int realRow, RT::Core::Int* sortedRow) = 0;

    virtual RT::Core::ErrCode INTERFACE_FUNC getRealColumn(RT::Core::Int sortedCol, RT::Core::Int* realCol) = 0;
    virtual RT::Core::ErrCode INTERFACE_FUNC getRealRow(RT::Core::Int sortedRow, RT::Core::Int* realRow) = 0;

    virtual RT::Core::ErrCode INTERFACE_FUNC getCursorRow(RT::Core::Int* rowIndex) = 0;
    virtual RT::Core::ErrCode INTERFACE_FUNC getCursorColumn(RT::Core::Int* colIndex) = 0;
    virtual RT::Core::ErrCode INTERFACE_FUNC getIsMultiselect(RT::Core::Bool* multiSelect) = 0;

    virtual RT::Core::ErrCode INTERFACE_FUNC getCell(RT::Core::Int col, RT::Core::Int row, RT::Core::IString** text) = 0;

    virtual RT::Core::ErrCode INTERFACE_FUNC getColumnCount(RT::Core::Int* count) = 0;
    virtual RT::Core::ErrCode INTERFACE_FUNC getRowCount(RT::Core::Int* count) = 0;

    virtual RT::Core::ErrCode INTERFACE_FUNC getStaticColumnCount(RT::Core::Int* count) = 0;

    virtual RT::Core::ErrCode INTERFACE_FUNC getStaticRowCount(RT::Core::Int* count) = 0;
    virtual RT::Core::ErrCode INTERFACE_FUNC getFilteredRowCount(RT::Core::Int* count) = 0;

    virtual RT::Core::ErrCode INTERFACE_FUNC getDefaultRowHeight(RT::Core::Int* height) = 0;
    virtual RT::Core::ErrCode INTERFACE_FUNC setDefaultRowHeight(RT::Core::Int height) = 0;

    virtual RT::Core::ErrCode INTERFACE_FUNC getCaptionRowHeight(RT::Core::Int* height) = 0;
    virtual RT::Core::ErrCode INTERFACE_FUNC setCaptionRowHeight(RT::Core::Int captionHeight) = 0;

    virtual RT::Core::ErrCode INTERFACE_FUNC getSortColumn(RT::Core::Int* sortedIndex) = 0;
    virtual RT::Core::ErrCode INTERFACE_FUNC setSortColumn(RT::Core::Int value) = 0;

    virtual RT::Core::ErrCode INTERFACE_FUNC getSortIncremental(RT::Core::Bool* sortIncremental) = 0;

    virtual RT::Core::ErrCode INTERFACE_FUNC getTimeDisplay(TimeDisplay* timeDisplay) = 0;
    virtual RT::Core::ErrCode INTERFACE_FUNC setTimeDisplay(TimeDisplay display) = 0;

    virtual RT::Core::ErrCode INTERFACE_FUNC getGridVersion(RT::Core::Int* version) = 0;
    virtual RT::Core::ErrCode INTERFACE_FUNC setGridVersion(RT::Core::Int version) = 0;

    //virtual RT::Core::ErrCode INTERFACE_FUNC GetGridGroups() : TDSGridGroups;

    // PUBLISHED
    virtual RT::Core::ErrCode INTERFACE_FUNC getFixedColumns(RT::Core::Int* fixedCols) = 0;
    virtual RT::Core::ErrCode INTERFACE_FUNC setFixedColumns(RT::Core::Int fixedCols) = 0;

    virtual RT::Core::ErrCode INTERFACE_FUNC getFixedRows(RT::Core::Int* fixedRows) = 0;
    virtual RT::Core::ErrCode INTERFACE_FUNC setFixedRows(RT::Core::Int fixedRows) = 0;

    /* [default(dsgColSizing, dsgRowSelect, dsgCellSelect)] */
    virtual RT::Core::ErrCode INTERFACE_FUNC getOptions(DrawGridOption* options) = 0;
    virtual RT::Core::ErrCode INTERFACE_FUNC setOptions(DrawGridOption options) = 0;

    // { Events }

    //[event(Add, OnSortChanged)]
    virtual RT::Core::ErrCode INTERFACE_FUNC addOnSortChanged(IEventHandler* listener) = 0;
    //[event(Remove, OnSortChanged)]
    virtual RT::Core::ErrCode INTERFACE_FUNC removeOnSortChanged(IEventHandler* listener) = 0;

    //[event(Add, OnCellGetProps, IDrawGridCellPropsArgs)]
    virtual RT::Core::ErrCode INTERFACE_FUNC addOnCellGetProps(IEventHandler* listener) = 0;
    //[event(Remove, OnCellGetProps)]
    virtual RT::Core::ErrCode INTERFACE_FUNC removeOnCellGetProps(IEventHandler* listener) = 0;

    //[event(Add, OnCellGetComboItems, IDrawGridComboItemsArgs)]
    virtual RT::Core::ErrCode INTERFACE_FUNC addOnCellGetComboItems(IEventHandler* listener) = 0;
    //[event(Remove, OnCellGetComboItems)]
    virtual RT::Core::ErrCode INTERFACE_FUNC removeOnCellGetComboItems(IEventHandler* listener) = 0;

    //[event(Add, OnCellGetLiveValue, IDrawGridLiveValueArgs)]
    virtual RT::Core::ErrCode INTERFACE_FUNC addOnCellGetLiveValue(IEventHandler* listener) = 0;
    //[event(Remove, OnCellGetLiveValue)]
    virtual RT::Core::ErrCode INTERFACE_FUNC removeOnCellGetLiveValue(IEventHandler* listener) = 0;

    //[event(Add, OnCellGet2DLiveValue, IDrawGrid2DLiveValueArgs)]
    virtual RT::Core::ErrCode INTERFACE_FUNC addOnCellGet2DLiveValue(IEventHandler* listener) = 0;
    //[event(Remove, OnCellGet2DLiveValue)]
    virtual RT::Core::ErrCode INTERFACE_FUNC removeOnCellGet2DLiveValue(IEventHandler* listener) = 0;

    //[event(Add, OnCellAction, IDrawGridCellActionEventArgs)]
    virtual RT::Core::ErrCode INTERFACE_FUNC addOnCellAction(IEventHandler* listener) = 0;
    //[event(Remove, OnCellAction)]
    virtual RT::Core::ErrCode INTERFACE_FUNC removeOnCellAction(IEventHandler* listener) = 0;

    //[event(Add, OnCellActionStartStop, IDrawGridCellActionStartStopEventArgs)]
    virtual RT::Core::ErrCode INTERFACE_FUNC addOnCellActionStartStop(IEventHandler* listener) = 0;
    //[event(Remove, OnCellActionStartStop)]
    virtual RT::Core::ErrCode INTERFACE_FUNC removeOnCellActionStartStop(IEventHandler* listener) = 0;

    //[event(Add, OnCellInput, IDrawGridCellInputArgs)]
    virtual RT::Core::ErrCode INTERFACE_FUNC addOnCellInput(IEventHandler* listener) = 0;
    //[event(Remove, OnCellInput)]
    virtual RT::Core::ErrCode INTERFACE_FUNC removeOnCellInput(IEventHandler* listener) = 0;

    virtual RT::Core::ErrCode INTERFACE_FUNC addOnCellDrawManual(IEventHandler* listener) = 0;
    virtual RT::Core::ErrCode INTERFACE_FUNC removeOnCellDrawManual(IEventHandler* listener) = 0;

    //[event(Add, OnCellSelected, IDrawGridCellSelectedEventArgs)]
    virtual RT::Core::ErrCode INTERFACE_FUNC addOnCellSelected(IEventHandler* listener) = 0;
    //[event(Remove, OnCellSelected)]
    virtual RT::Core::ErrCode INTERFACE_FUNC removeOnCellSelected(IEventHandler* listener) = 0;

    //[event(Add, OnCellMultiSelectStartStop, IDrawGridCellMultiSelectStartStopArgs)]
    virtual RT::Core::ErrCode INTERFACE_FUNC addOnCellMultiSelectStartStop(IEventHandler* listener) = 0;
    //[event(Remove, OnCellMultiSelectStartStop)]
    virtual RT::Core::ErrCode INTERFACE_FUNC removeOnCellMultiSelectStartStop(IEventHandler* listener) = 0;

    //[event(Add, OnCellsDeselect)]
    virtual RT::Core::ErrCode INTERFACE_FUNC addOnCellsDeselect(IEventHandler* listener) = 0;
    //[event(Remove, OnCellsDeselect)]
    virtual RT::Core::ErrCode INTERFACE_FUNC removeOnCellsDeselect(IEventHandler* listener) = 0;

    //[event(Add, OnCellPopupMenu, IDrawGridCellPopupMenuArgs)]
    virtual RT::Core::ErrCode INTERFACE_FUNC addOnCellPopupMenu(IEventHandler* listener) = 0;
    //[event(Remove, OnCellPopupMenu)]
    virtual RT::Core::ErrCode INTERFACE_FUNC removeOnCellPopupMenu(IEventHandler* listener) = 0;

    //[event(Add, OnSetGroups, IDrawGridSetGroupsArgs)]
    virtual RT::Core::ErrCode INTERFACE_FUNC addOnSetGroups(IEventHandler* listener) = 0;
    //[event(Remove, OnSetGroups)]
    virtual RT::Core::ErrCode INTERFACE_FUNC removeOnSetGroups(IEventHandler* listener) = 0;

    // New
    virtual RT::Core::ErrCode INTERFACE_FUNC getIsFixedColumn(RT::Core::Bool* fixed) = 0;
    virtual RT::Core::ErrCode INTERFACE_FUNC setIsFixedColumn(RT::Core::Bool fixed) = 0;

    virtual RT::Core::ErrCode INTERFACE_FUNC isColumnSelected(RT::Core::Int column, RT::Core::Bool* selected) = 0;
    virtual RT::Core::ErrCode INTERFACE_FUNC selectColumn(RT::Core::Int realCol, RT::Core::Bool moveAnchor) = 0;
};

END_MUI_NAMESPACE
