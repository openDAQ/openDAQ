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
//#include <cstdint>
#include "common.h"
#include <coretypes/baseobject.h>
#include <coretypes/listobject.h>

BEGIN_NAMESPACE_DEWESOFT_RT_CORE

static const IntfID DictGuid = { 0x90EAAC02, 0xF875, 0x510A, { 0xA7, 0x30, 0x8B, 0x79, 0x2F, 0xCD, 0x49, 0x63 } };

// OPENDAQ_TODO: dict iterator missing
DECLARE_RT_INTERFACE(IDict, IBaseObject)
{
    DEFINE_INTFID(DictGuid)

    virtual ErrCode INTERFACE_FUNC get(IBaseObject* key, IBaseObject** value) = 0;
    virtual ErrCode INTERFACE_FUNC set(IBaseObject* key, IBaseObject* value) = 0;
    virtual ErrCode INTERFACE_FUNC remove(IBaseObject* key, IBaseObject** value) = 0;
    virtual ErrCode INTERFACE_FUNC deleteItem(IBaseObject* key) = 0;
    virtual ErrCode INTERFACE_FUNC clear() = 0;
    virtual ErrCode INTERFACE_FUNC getCount(SizeT* size) = 0;
    virtual ErrCode INTERFACE_FUNC hasKey(IBaseObject* key, Bool* hasKey) = 0;
    virtual ErrCode INTERFACE_FUNC enumKeys(IList** keys) = 0;
    virtual ErrCode INTERFACE_FUNC enumValues(IList** values) = 0;
};

OPENDAQ_DECLARE_CLASS_FACTORY(LIBRARY_FACTORY, Dict)

END_NAMESPACE_DEWESOFT_RT_CORE
