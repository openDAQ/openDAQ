#include <opendaq/config_provider_factory.h>
#include <gtest/gtest.h>
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <set>
#include <opendaq/log_level.h>
#include <opendaq/instance_context_impl.h>
#include <opendaq/instance_factory.h>

BEGIN_NAMESPACE_OPENDAQ

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

    static inline int setEnv(const std::string&  name, const std::string&  value)
    {
    #ifdef _WIN32
        return _putenv_s(name.c_str(), value.c_str());
    #else
        return setenv(name.c_str(), value.c_str(), 1);
    #endif
    }

    static void SetEnvironmentVariableValue(const std::string& variableName, const std::string& defaultValue)
    {
        if (variableName.empty())
            return;

        if (setEnv(variableName.c_str(), defaultValue.c_str()) != 0)
            throw std::runtime_error("Failed to set env variable");
    }

    void createConfigFile(const std::string & filename, const std::string & data)
    {
        std::ofstream file;
        file.open(filename);
        if (!file.is_open()) 
            throw std::runtime_error("can not open file for writting");

        file << data;
        file.close();
        filenames.insert(filename);

        oldConfigPath = GetEnvironmentVariableValue("OPENDAQ_CONFIG_PATH");
        SetEnvironmentVariableValue("OPENDAQ_CONFIG_PATH", filename);
    }

    void TearDown() override
    {
        for (const auto & filename : filenames)
            remove(filename.c_str());
        filenames.clear();
        SetEnvironmentVariableValue("OPENDAQ_CONFIG_PATH", oldConfigPath);
    }

    static DictPtr<IString, IBaseObject> GetDefaultOptions()
    {
        return Dict<IString, IBaseObject>({
            {"ModuleManager", Dict<IString, IBaseObject>({
                    {"ModulesPath", ""}
                })},
            {"Scheduler", Dict<IString, IBaseObject>({
                    {"WorkersNum", 0}
                })},
            {"Logging", Dict<IString, IBaseObject>({
                    {"GlobalLogLevel", OPENDAQ_LOG_LEVEL_DEFAULT}
                })},
            {"Modules", Dict<IString, IBaseObject>()}
        });
    }

    std::string oldConfigPath;
    std::set<std::string> filenames;
};

DictPtr<IString, IBaseObject> getChildren(const DictPtr<IString, IBaseObject>& dict, const StringPtr& name)
{
    return dict.get(name);
}

TEST_F(ConfigProviderTest, jsonConfigReadModuleManagerPath)
{
    std::string filename = "jsonConfigReadModuleManagerPath.json";
    std::string json = "{ \"ModuleManager\": { \"ModulesPath\": \"testtest\" } }";
    createConfigFile(filename, json);

    auto options = GetDefaultOptions(); 
    SizeT optionsKeys = options.getCount();

    auto provider = JsonConfigProvider(StringPtr(filename));
    provider.populateOptions(options);

    // amount of option keys have to be unchanged
    ASSERT_EQ(options.getCount(), optionsKeys);

    // check that options have new value
    StringPtr modulePath;
    ASSERT_NO_THROW(modulePath = getChildren(options, "ModuleManager").get("ModulesPath"));
    ASSERT_EQ(modulePath.toStdString(), "testtest");
}

TEST_F(ConfigProviderTest, jsonConfigReadSchedulerWorkersNum)
{
    std::string filename = "jsonConfigReadSchedulerWorkersNum.json";
    std::string json = "{ \"Scheduler\": { \"WorkersNum\": 4 } }";
    createConfigFile(filename, json);

    auto options = GetDefaultOptions(); 
    SizeT optionsKeys = options.getCount();

    auto provider = JsonConfigProvider(StringPtr(filename));
    provider.populateOptions(options);

    // amount of option keys have to be unchanged
    ASSERT_EQ(options.getCount(), optionsKeys);

    // check that options have new value
    SizeT workersNum;
    ASSERT_NO_THROW(workersNum = getChildren(options, "Scheduler").get("WorkersNum"));
    ASSERT_EQ(workersNum, 4);
}

TEST_F(ConfigProviderTest, jsonConfigReadLoggingGlobalLogLevel)
{
    std::string filename = "jsonConfigReadLoggingGlobalLogLevel.json";
    std::string json = "{ \"Logging\": { \"GlobalLogLevel\": 0 } }";
    createConfigFile(filename, json);

    auto options = GetDefaultOptions(); 
    SizeT optionsKeys = options.getCount();

    auto provider = JsonConfigProvider(StringPtr(filename));
    provider.populateOptions(options);

    // amount of option keys have to be unchanged
    ASSERT_EQ(options.getCount(), optionsKeys);

    // check that options have new value
    SizeT globalLogLevel;
    ASSERT_NO_THROW(globalLogLevel = getChildren(options, "Logging").get("GlobalLogLevel"));
    ASSERT_EQ(globalLogLevel, 0);
}

TEST_F(ConfigProviderTest, jsonConfigReadModules)
{
    std::string filename = "jsonConfigReadModules.json";
    std::string json = "{ \"Modules\": { \"OpcUAClient\": { \"Debug\": 1 }, \"RefDevice\": { \"UseGlobalThreadForAcq\": 1 } } }";
    createConfigFile(filename, json);

    auto options = GetDefaultOptions(); 
    SizeT optionsKeys = options.getCount();

    auto provider = JsonConfigProvider(StringPtr(filename));
    provider.populateOptions(options);

    // amount of option keys have to be unchanged
    ASSERT_EQ(options.getCount(), optionsKeys);

    // check that options have new value
    DictPtr<IString, IBaseObject> modules;
    ASSERT_NO_THROW(modules = getChildren(options, "Modules"));
    ASSERT_EQ(modules.getCount(), 2);

    DictPtr<IString, IBaseObject> opcuaClient;
    ASSERT_NO_THROW(opcuaClient = getChildren(modules, "OpcUAClient"));
    SizeT debug;
    ASSERT_NO_THROW(debug = opcuaClient.get("Debug"));
    ASSERT_EQ(debug, 1);

    DictPtr<IString, IBaseObject> refDevice;
    ASSERT_NO_THROW(refDevice = getChildren(modules, "RefDevice"));
    SizeT useGlobalThreadForAcq;
    ASSERT_NO_THROW(useGlobalThreadForAcq = refDevice.get("UseGlobalThreadForAcq"));
    ASSERT_EQ(useGlobalThreadForAcq, 1);
}

TEST_F(ConfigProviderTest, jsonConfigReadLists)
{
    std::string filename = "jsonConfigReadModules.json";
    std::string json = "{ \"List\": [\"test\", 123, true, {}, null] }";
    createConfigFile(filename, json);

    auto options = GetDefaultOptions(); 
    SizeT optionsKeys = options.getCount();

    auto provider = JsonConfigProvider(StringPtr(filename));
    provider.populateOptions(options);

    // amount of option keys have increased by one
    ASSERT_EQ(options.getCount(), optionsKeys + 1);

    // check that options have new value
    ListPtr<IBaseObject> items;
    ASSERT_NO_THROW(items = options.get("List"));
    ASSERT_EQ(items.getCount(), 5);

    ASSERT_EQ(items[0], "test");
    ASSERT_EQ(items[1], 123);
    ASSERT_EQ(items[2], true);
    auto emptyDict = Dict<IString, IBaseObject>();
    ASSERT_EQ(items[3], emptyDict);
    ASSERT_FALSE(items[4].assigned());
}

TEST_F(ConfigProviderTest, jsonConfigIncorrectType)
{
    std::string filename = "jsonConfigIncorrectType.json";
    std::string json = "{ \"ModuleManager\": { \"ModulesPath\": 123 } }";
    createConfigFile(filename, json);

    auto options = GetDefaultOptions(); 
    SizeT optionsKeys = options.getCount();

    auto provider = JsonConfigProvider(StringPtr(filename));
    ASSERT_ANY_THROW(provider.populateOptions(options));
    ASSERT_EQ(options, GetDefaultOptions());
}

TEST_F(ConfigProviderTest, jsonConfigIncorrectType2)
{
    std::string filename = "jsonConfigIncorrectType2.json";
    std::string json = "{ \"ModuleManager\": true }";
    createConfigFile(filename, json);

    auto options = GetDefaultOptions(); 
    SizeT optionsKeys = options.getCount();

    auto provider = JsonConfigProvider(StringPtr(filename));
    ASSERT_ANY_THROW(provider.populateOptions(options));
    ASSERT_EQ(options, GetDefaultOptions());
}

TEST_F(ConfigProviderTest, jsonConfigDamagaed)
{
    std::string filename = "jsonConfigIncorrectType2.json";
    std::string json = "{ \"ModuleManager\": true }";
    createConfigFile(filename, json);

    auto options = GetDefaultOptions(); 
    SizeT optionsKeys = options.getCount();

    auto provider = JsonConfigProvider(StringPtr(filename));
    ASSERT_ANY_THROW(provider.populateOptions(options));
    ASSERT_EQ(options, GetDefaultOptions());
}

TEST_F(ConfigProviderTest, InstanceBuilderFromJson)
{
    std::string filename = "InstanceBuilderFromJson.json";
    std::string json =  R"(
        {
            "ModuleManager": {
                "ModulesPath": "testtest"
            },
            "Scheduler": {
                "WorkersNum": 8
            },
            "Logging": {
                "GlobalLogLevel": 6
            }
        }
    )";

    createConfigFile(filename, json);

    const auto instanceBuilder = InstanceBuilder();

    ASSERT_EQ(instanceBuilder.getModulePath(), "testtest");
    ASSERT_EQ(instanceBuilder.getSchedulerWorkerNum(), 8);
    ASSERT_EQ(int(instanceBuilder.getGlobalLogLevel()), 6);
}


END_NAMESPACE_OPENDAQ