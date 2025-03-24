//------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
//
//     RTGen (CGenerator v0.1.0) on 25.03.2025 01:13:32.
// </auto-generated>
//------------------------------------------------------------------------------

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

#ifdef __cplusplus
extern "C"
{
#endif

#include "ccommon.h"

    typedef struct SerializedList SerializedList;
    typedef struct SerializedObject SerializedObject;
    typedef struct Function Function;
    typedef struct List List;
    typedef struct String String;

    EXPORTED extern const IntfID SERIALIZED_LIST_INTF_ID;

    ErrCode EXPORTED SerializedList_readSerializedObject(SerializedList* self, SerializedObject** plainObj);
    ErrCode EXPORTED SerializedList_readSerializedList(SerializedList* self, SerializedList** list);
    ErrCode EXPORTED SerializedList_readList(SerializedList* self, BaseObject* context, Function* factoryCallback, List** list);
    ErrCode EXPORTED SerializedList_readObject(SerializedList* self, BaseObject* context, Function* factoryCallback, BaseObject** obj);
    ErrCode EXPORTED SerializedList_readString(SerializedList* self, String** string);
    ErrCode EXPORTED SerializedList_readBool(SerializedList* self, Bool* boolean);
    ErrCode EXPORTED SerializedList_readFloat(SerializedList* self, Float* real);
    ErrCode EXPORTED SerializedList_readInt(SerializedList* self, Int* integer);
    ErrCode EXPORTED SerializedList_getCount(SerializedList* self, SizeT* size);
    ErrCode EXPORTED SerializedList_getCurrentItemType(SerializedList* self, CoreType* size);

#ifdef __cplusplus
}
#endif
