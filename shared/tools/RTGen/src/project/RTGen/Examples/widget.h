/*
 * Copyright 2022-2025 openDAQ d.o.o.
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
#include "coretypes/coretypes.h"

BEGIN_NAMESPACE_DEWESOFT_RT_CORE

static const IntfID WidgetGuid = { 0x315116a2, 0xbe7e, 0x5a68, { 0xaf, 0x3d, 0xba, 0xe0, 0x29, 0x6a, 0xeb, 0xa6 } };

DECLARE_RT_INTERFACE(IWidget, IBaseObject)
{
    DEFINE_INTFID(WidgetGuid)

    virtual ErrCode INTERFACE_FUNC setId(IString* value) = 0;
    virtual ErrCode INTERFACE_FUNC getId(IString** value) = 0;
    virtual ErrCode INTERFACE_FUNC getId2(IString** value, IString** value2) = 0;
};

OPENDAQ_DECLARE_CLASS_FACTORY(LIBRARY_FACTORY, Widget)

END_NAMESPACE_DEWESOFT_RT_CORE
