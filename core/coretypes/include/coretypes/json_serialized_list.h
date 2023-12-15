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
#include <coretypes/deserializer.h>
#include <coretypes/intfs.h>
#include <rapidjson/document.h>
#include <coretypes/listobject.h>

BEGIN_NAMESPACE_OPENDAQ

class JsonSerializedList : public ImplementationOf<ISerializedList>
{
public:
    using JsonList = rapidjson::Value::Array;

    explicit JsonSerializedList(const JsonList& list);

    ErrCode INTERFACE_FUNC readSerializedList(ISerializedList** list) override;
    ErrCode INTERFACE_FUNC readList(IBaseObject* context, IFunction* factoryCallback, IList** list) override;
    ErrCode INTERFACE_FUNC readSerializedObject(ISerializedObject** plainObj) override;
    ErrCode INTERFACE_FUNC readObject(IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj) override;
    ErrCode INTERFACE_FUNC readString(IString** obj) override;
    ErrCode INTERFACE_FUNC readBool(Bool* obj) override;
    ErrCode INTERFACE_FUNC readInt(Int* obj) override;
    ErrCode INTERFACE_FUNC readFloat(Float* obj) override;
    ErrCode INTERFACE_FUNC getCount(SizeT* size) override;
    ErrCode INTERFACE_FUNC getCurrentItemType(CoreType* size) override;

    ErrCode INTERFACE_FUNC toString(CharPtr* str) override;

private:
    rapidjson::SizeType index;
    rapidjson::SizeType length;
    const JsonList array;
};

END_NAMESPACE_OPENDAQ
