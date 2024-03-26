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

﻿#pragma once
#include "mui/common.h"
#include <coretypes/stringobject.h>
#include <coretypes/listobject.h>
#include "control.h"
#include "mui/event_handler.h"
#include "draw_grid/common.h"
#include "draw_grid/draw_grid_column.h"

BEGIN_MUI_NAMESPACE

static const RT::Core::IntfID IDrawGridGuid = { 0xF161449B, 0xE1DD, 0x51F7,{ 0xB8, 0xBE, 0x1C, 0x99, 0x91, 0xC7, 0xC3, 0x08 } };

/*#
* [uiControl]
* [includeHeader("draw_grid/event_args.h")]
* [interfaceNamespace(IDSDrawGrid, "Dewesoft::MUI::")]
* [interfaceSmartPtr(IControl, ControlPtr)]
* [interfaceSmartPtr(IDSDrawGrid, DSDrawGrid)]
* [interfaceSmartPtr(IDrawGridCellPropsArgs, DrawGridCellPropsArgs)]
*/
struct IDSDrawGrid : IControl
{
    DEFINE_INTFID(IDrawGridGuid)

    virtual RT::Core::ErrCode INTERFACE_FUNC getCellAt(RT::Core::Int x, RT::Core::Int y, RT::Core::Int* realRow, RT::Core::Int* realCol) = 0;
};

END_MUI_NAMESPACE
