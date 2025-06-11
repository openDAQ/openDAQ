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

#include <coretypes/coretypes.h>
#include <gtest/gtest.h>
#include <opendaq/log_level.h>
#include <fstream>

namespace test_config_provider_helpers
{
using namespace daq;

[[maybe_unused]]
static DictPtr<IString, IBaseObject> getChildren(const DictPtr<IString, IBaseObject>& dict, const StringPtr& name)
{
    return dict.get(name);
}

class ConfigProviderTest : public testing::Test
{
protected:
    static std::string GetEnvironmentVariableValue(const std::string& variableName, const std::string& defaultValue = "")
    {
        if (variableName.empty())
            return defaultValue;

        const char* value = std::getenv(variableName.c_str());

        if (value)
            return value;
        else
            return defaultValue;
    }

    static inline int setEnv(const std::string& name, const std::string& value)
    {
#ifdef _WIN32
        return _putenv_s(name.c_str(), value.c_str());
#else
        if (value.empty())
            return unsetenv(name.c_str());
        return setenv(name.c_str(), value.c_str(), 1);
#endif
    }

    void setEnvironmentVariableValue(const std::string& variableName, const std::string& defaultValue)
    {
        if (variableName.empty())
            return;

        oldEnvValues.try_emplace(variableName, GetEnvironmentVariableValue(variableName));

        if (setEnv(variableName, defaultValue) != 0)
            throw std::runtime_error("Failed to set env variable");
    }

    void createConfigFile(const std::string& filename, const std::string& data)
    {
        std::ofstream file;
        file.open(filename);
        if (!file.is_open())
            throw std::runtime_error("can not open file for writing");

        file << data;
        file.close();
        filenames.insert(filename);

        setEnvironmentVariableValue("OPENDAQ_CONFIG_PATH", filename);
    }

    void TearDown() override
    {
        for (const auto& filename : filenames)
            remove(filename.c_str());
        filenames.clear();

        for (const auto& [envKey, envVal] : oldEnvValues)
            setEnv(envKey, envVal);
        oldEnvValues.clear();
    }

    static DictPtr<IString, IBaseObject> GetDefaultOptions()
    {
        return Dict<IString, IBaseObject>({{"ModuleManager", Dict<IString, IBaseObject>({{"ModulesPath", ""}})},
                                           {"Scheduler", Dict<IString, IBaseObject>({{"WorkersNum", 0}})},
                                           {"Logging", Dict<IString, IBaseObject>({{"GlobalLogLevel", OPENDAQ_LOG_LEVEL_DEFAULT}})},
                                           {"RootDevice", Dict<IString, IBaseObject>({{"DefaultLocalId", ""}, {"ConnectionString", ""}})},
                                           {"Modules", Dict<IString, IBaseObject>()}});
    }

    static DictPtr<IString, IBaseObject> GetOptionsWithReferenceDevice(const StringPtr& refDeviceId = "ReferenceDevice")
    {
        auto referenceDeviceOptions = Dict<IString, IBaseObject>({{"LocalId", ""}});
        return Dict<IString, IBaseObject>({{"ModuleManager", Dict<IString, IBaseObject>({{"ModulesPath", ""}})},
                                           {"Scheduler", Dict<IString, IBaseObject>({{"WorkersNum", 0}})},
                                           {"Logging", Dict<IString, IBaseObject>({{"GlobalLogLevel", OPENDAQ_LOG_LEVEL_DEFAULT}})},
                                           {"RootDevice", Dict<IString, IBaseObject>({{"DefaultLocalId", ""}, {"ConnectionString", ""}})},
                                           {"Modules", Dict<IString, IBaseObject>({{refDeviceId, referenceDeviceOptions}})}});
    }

    std::set<std::string> filenames;
    std::map<std::string, std::string> oldEnvValues;
};

}
