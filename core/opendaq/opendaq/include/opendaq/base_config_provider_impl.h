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
#include <opendaq/config_provider.h>
#include <rapidjson/document.h>
#include <coretypes/baseobject_factory.h>

BEGIN_NAMESPACE_OPENDAQ

class BaseConfigProviderImpl : public ImplementationOf<IConfigProvider>
{
protected:
    static BaseObjectPtr TryConvertToBoolean(const std::string& value);
    static BaseObjectPtr TryConvertToFloat(const std::string& value);
    static BaseObjectPtr TryConvertToInteger(const std::string& value);

    static std::string ToLowerCase(const std::string& input);
    static ListPtr<IString> SplitKey(const std::string& envKey, const std::string& prefix, char delimiter);
    static DictPtr<IString, IString> GetValuesStartingWith(const ListPtr<IString>& cmdLineArgs, const std::string& prefix);

    static BaseObjectPtr HandleUnderfineValue(const std::string& value);
    static bool HandleOptionLeaf(DictPtr<IString, IBaseObject> optionsValue, StringPtr envKey, StringPtr envValue);

    static bool WriteValue(DictPtr<IString,IBaseObject> options, const ListPtr<IString>& tokens, const std::string& value);
};

END_NAMESPACE_OPENDAQ
