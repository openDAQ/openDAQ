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

class EnvConfigProviderImpl final : public ImplementationOf<IConfigProvider>
{
public:
    explicit EnvConfigProviderImpl();

    ErrCode INTERFACE_FUNC populateModuleOptions(IDict* options);
    ErrCode INTERFACE_FUNC populateOptions(IDict* options) override;

private:
    bool handleOptionLeaf(DictPtr<IString, IBaseObject> optionsValue, StringPtr envKey, StringPtr envValue);

    static DictPtr<IString, IString> GetEnvValuesStartingWith(const std::string& prefix);
    static ListPtr<IString> SplitEnvKey(const std::string& envKey, const std::string& prefix, char delimiter = '_');
    static std::string ToUpperCase(const std::string &input);
};

END_NAMESPACE_OPENDAQ
