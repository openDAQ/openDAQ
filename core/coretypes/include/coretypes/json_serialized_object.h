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

BEGIN_NAMESPACE_OPENDAQ

class JsonSerializedObject : public ImplementationOf<ISerializedObject>
{
public:
    using JsonObject = rapidjson::GenericObject<false, rapidjson::Value>;

    explicit JsonSerializedObject(const JsonObject& obj);

    ErrCode INTERFACE_FUNC readSerializedObject(IString* key, ISerializedObject** plainObj) override;
    ErrCode INTERFACE_FUNC readSerializedList(IString* key, ISerializedList** list) override;
    ErrCode INTERFACE_FUNC readList(IString* key, IBaseObject* context, IFunction* factoryCallback, IList** list) override;
    ErrCode INTERFACE_FUNC readObject(IString* key, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj) override;
    ErrCode INTERFACE_FUNC readString(IString* key, IString** string) override;
    ErrCode INTERFACE_FUNC readBool(IString* key, Bool* boolean) override;
    ErrCode INTERFACE_FUNC readInt(IString* key, Int* integer) override;
    ErrCode INTERFACE_FUNC readFloat(IString* key, Float* real) override;
    ErrCode INTERFACE_FUNC hasKey(IString* key, Bool* hasKey) override;

    ErrCode INTERFACE_FUNC getKeys(IList** list) override;
    ErrCode INTERFACE_FUNC getType(IString* key, CoreType* type) override;

    ErrCode INTERFACE_FUNC toString(CharPtr* str) override;
private:
    const JsonObject object;
};

END_NAMESPACE_OPENDAQ
