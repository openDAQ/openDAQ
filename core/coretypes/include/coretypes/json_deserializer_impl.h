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

#include <coretypes/intfs.h>
#include <coretypes/deserializer.h>
#include <coretypes/updatable.h>
#include <rapidjson/document.h>

BEGIN_NAMESPACE_OPENDAQ

class JsonDeserializerImpl : public ImplementationOf<IDeserializer>
{
public:
    using JsonDocument = rapidjson::Document;
    using JsonValue = rapidjson::Value;
    using JsonList = rapidjson::GenericArray<false, JsonValue>;

    ErrCode INTERFACE_FUNC deserialize(IString* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** object) override;
    ErrCode INTERFACE_FUNC update(IUpdatable* updatable, IString* serialized) override;

    ErrCode INTERFACE_FUNC toString(CharPtr* str) override;

    static CoreType GetCoreType(const JsonValue& value) noexcept;
    static ErrCode Deserialize(JsonValue& document, IBaseObject* context, IFunction* factoryCallback, IBaseObject** object);

private:
    static ErrCode DeserializeTagged(JsonValue& document, IBaseObject* context, IFunction* factoryCallback, IBaseObject** object);
    static ErrCode DeserializeList(const JsonList& array, IBaseObject* context, IFunction* factoryCallback, IBaseObject** object);
};

END_NAMESPACE_OPENDAQ
