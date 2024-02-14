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
#include <opendaq/config_provider.h>
#include <rapidjson/document.h>
#include <coretypes/baseobject_factory.h>

BEGIN_NAMESPACE_OPENDAQ

class JsonConfigProviderImpl final : public ImplementationOf<IConfigProvider>
{
public:
    explicit JsonConfigProviderImpl(const StringPtr& filename);

    ErrCode INTERFACE_FUNC populateOptions(IDict* options) override;
private:
    static StringPtr GetEnvironmentVariableValue(StringPtr variableName, StringPtr defaultValue);
    static StringPtr GetDataFromFile(const StringPtr& filename);
    static std::string ToLowerCase(const std::string& input);

    static BaseObjectPtr HandleNumber(const rapidjson::Value& value);
    static BaseObjectPtr HandlePrimitive(const rapidjson::Value& value);
    static void HandleArray(const BaseObjectPtr& options, const rapidjson::Value& value);
    static void HandleObject(const BaseObjectPtr& options, const rapidjson::Value& value);

    StringPtr filename;
};

END_NAMESPACE_OPENDAQ
