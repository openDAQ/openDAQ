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
#include <coretypes/common.h>
#include <coretypes/coretype.h>
#include <coretypes/stringobject.h>
#include <coretypes/function.h>

BEGIN_NAMESPACE_OPENDAQ

struct IList;
struct ISerializedObject;

/*!
 * @ingroup types_serialization
 * @defgroup types_serialized_list SerializedList
 * @{
 */

DECLARE_OPENDAQ_INTERFACE(ISerializedList, IBaseObject)
{
    virtual ErrCode INTERFACE_FUNC readSerializedObject(ISerializedObject** plainObj) = 0;
    virtual ErrCode INTERFACE_FUNC readSerializedList(ISerializedList** list) = 0;
    virtual ErrCode INTERFACE_FUNC readList(IBaseObject* context, IFunction* factoryCallback, IList** list) = 0;
    virtual ErrCode INTERFACE_FUNC readObject(IBaseObject* context, IFunction* factoryCallback, IBaseObject * *obj) = 0;
    virtual ErrCode INTERFACE_FUNC readString(IString** string) = 0;
    virtual ErrCode INTERFACE_FUNC readBool(Bool* boolean) = 0;
    virtual ErrCode INTERFACE_FUNC readFloat(Float* real) = 0;
    virtual ErrCode INTERFACE_FUNC readInt(Int* integer) = 0;
    virtual ErrCode INTERFACE_FUNC getCount(SizeT* size) = 0;
    virtual ErrCode INTERFACE_FUNC getCurrentItemType(CoreType* size) = 0;
};

/*!
 * @}
 */

END_NAMESPACE_OPENDAQ
