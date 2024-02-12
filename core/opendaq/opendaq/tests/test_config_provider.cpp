#include <opendaq/config_provider_factory.h>
#include <gtest/gtest.h>
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <set>
#include <opendaq/log_level.h>
#include <opendaq/instance_factory.h>
#include <coretypes/coretypes.h>

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

    void createConfigFile(const std::string & filename, const std::string & data)
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
        for (const auto & filename : filenames)
            remove(filename.c_str());
        filenames.clear();

        for (const auto& [envKey, envVal] : oldEnvValues)
            setEnv(envKey, envVal);
        oldEnvValues.clear();
    }

    static DictPtr<IString, IBaseObject> GetDefaultOptions()
    {
        return Dict<IString, IBaseObject>({
            {"modulemanager", Dict<IString, IBaseObject>({
                    {"modulespath", ""}
                })},
            {"scheduler", Dict<IString, IBaseObject>({
                    {"workersnum", 0}
                })},
            {"logging", Dict<IString, IBaseObject>({
                    {"globalloglevel", OPENDAQ_LOG_LEVEL_DEFAULT}
                })},
            {"rootdevice", Dict<IString, IBaseObject>({
                    {"defaultlocalid", ""},
                    {"connectionstring", ""}
                })},
            {"modules", Dict<IString, IBaseObject>()}
        });
    }

    std::set<std::string> filenames;
    std::map<std::string, std::string> oldEnvValues;
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

    auto expectedOptions = GetDefaultOptions();
    getChildren(expectedOptions, "modulemanager").set("modulespath", "testtest");

    auto provider = JsonConfigProvider(StringPtr(filename));
    provider.populateOptions(options);

    ASSERT_EQ(options, expectedOptions);
}

TEST_F(ConfigProviderTest, jsonConfigReadSchedulerWorkersNum)
{
    std::string filename = "jsonConfigReadSchedulerWorkersNum.json";
    std::string json = "{ \"Scheduler\": { \"WorkersNum\": 4 } }";
    createConfigFile(filename, json);

    auto options = GetDefaultOptions(); 
    
    auto expectedOptions = GetDefaultOptions();
    getChildren(expectedOptions, "scheduler").set("workersnum", 4);

    auto provider = JsonConfigProvider(StringPtr(filename));
    provider.populateOptions(options);

    ASSERT_EQ(options, expectedOptions);
}

TEST_F(ConfigProviderTest, jsonConfigReadLoggingGlobalLogLevel)
{
    std::string filename = "jsonConfigReadLoggingGlobalLogLevel.json";
    std::string json = "{ \"Logging\": { \"GlobalLogLevel\": 0 } }";
    createConfigFile(filename, json);

    auto options = GetDefaultOptions(); 
    
    auto expectedOptions = GetDefaultOptions();
    getChildren(expectedOptions, "logging").set("globalloglevel", 0);

    auto provider = JsonConfigProvider(StringPtr(filename));
    provider.populateOptions(options);

    ASSERT_EQ(options, expectedOptions);
}

TEST_F(ConfigProviderTest, jsonConfigReadRootDeviceDefaultLocalId)
{
    std::string filename = "jsonConfigReadLoggingGlobalLogLevel.json";
    std::string json = "{ \"RootDevice\": { \"DefaultLocalId\": \"localId\" } }";
    createConfigFile(filename, json);

    auto options = GetDefaultOptions(); 
    
    auto expectedOptions = GetDefaultOptions();
    getChildren(expectedOptions, "rootdevice").set("defaultlocalid", "localId");

    auto provider = JsonConfigProvider(StringPtr(filename));
    provider.populateOptions(options);

    ASSERT_EQ(options, expectedOptions);
}

TEST_F(ConfigProviderTest, jsonConfigReadRootDeviceConnectionString)
{
    std::string filename = "jsonConfigReadLoggingGlobalLogLevel.json";
    std::string json = "{ \"RootDevice\": { \"ConnectionString\": \"dev://connectionString\" } }";
    createConfigFile(filename, json);

    auto options = GetDefaultOptions(); 
    
    auto expectedOptions = GetDefaultOptions();
    getChildren(expectedOptions, "rootdevice").set("connectionstring", "dev://connectionString");

    auto provider = JsonConfigProvider(StringPtr(filename));
    provider.populateOptions(options);

    ASSERT_EQ(options, expectedOptions);
}

TEST_F(ConfigProviderTest, jsonConfigReadModules)
{
    std::string filename = "jsonConfigReadModules.json";
    std::string json = "{ \"Modules\": { \"OpcUAClient\": { \"Debug\": 1 }, \"RefDevice\": { \"UseGlobalThreadForAcq\": 1 } } }";
    createConfigFile(filename, json);

    auto options = GetDefaultOptions();

    auto expectedModules = Dict<IString, IBaseObject>({
            {"opcuaclient", Dict<IString, IBaseObject>({
                    {"debug", 1}
                })},
            {"refdevice", Dict<IString, IBaseObject>({
                    {"useglobalthreadforacq", 1}
                })},
        });
    auto expectedOptions = GetDefaultOptions();
    expectedOptions.set("modules", expectedModules);    

    auto provider = JsonConfigProvider(StringPtr(filename));
    provider.populateOptions(options);

    ASSERT_EQ(options, expectedOptions);
}

TEST_F(ConfigProviderTest, jsonConfigReadLists)
{
    std::string filename = "jsonConfigReadModules.json";
    std::string json = "{ \"List\": [\"test\", 123, true, {}, null] }";
    createConfigFile(filename, json);

    auto options = GetDefaultOptions();
    auto expectedOptions = GetDefaultOptions();
    expectedOptions.set("list", List<IBaseObject>(String("test"), Integer(123), Boolean(true), Dict<IString, IBaseObject>(), BaseObjectPtr()));

    auto provider = JsonConfigProvider(StringPtr(filename));
    provider.populateOptions(options);

    ASSERT_EQ(options, expectedOptions);
}

TEST_F(ConfigProviderTest, jsonConfigReadNull)
{
    std::string filename = "jsonConfigReadNull.json";
    std::string json = "{ \"NullValue\": null }";
    createConfigFile(filename, json);

    auto options = GetDefaultOptions();
    auto expectedOptions = GetDefaultOptions();
    expectedOptions.set("nullvalue", {});

    auto provider = JsonConfigProvider(StringPtr(filename));
    provider.populateOptions(options);

    ASSERT_EQ(options, expectedOptions);
}

TEST_F(ConfigProviderTest, jsonConfigReadNull2)
{
    std::string filename = "jsonConfigReadNull2.json";
    std::string json = "{ \"Modules\": null }";
    createConfigFile(filename, json);

    auto options = GetDefaultOptions();
    auto expectedOptions = GetDefaultOptions();

    auto provider = JsonConfigProvider(StringPtr(filename));
    provider.populateOptions(options);

    ASSERT_EQ(options, expectedOptions);
}

TEST_F(ConfigProviderTest, jsonConfigIncorrectType)
{
    std::string filename = "jsonConfigIncorrectType.json";
    std::string json = "{ \"ModuleManager\": { \"ModulesPath\": 123 }, \"RootDevice\": { \"ConnectionString\": \"dev://connectionString\" } }";
    createConfigFile(filename, json);

    auto options = GetDefaultOptions();

    auto expectedOptions = GetDefaultOptions();
    getChildren(expectedOptions, "rootdevice").set("connectionstring", "dev://connectionString");

    auto provider = JsonConfigProvider(StringPtr(filename));
    ASSERT_NO_THROW(provider.populateOptions(options));
    ASSERT_EQ(options, expectedOptions);
}

TEST_F(ConfigProviderTest, jsonConfigIncorrectType2)
{
    std::string filename = "jsonConfigIncorrectType2.json";
    std::string json = "{ \"ModuleManager\": true, \"Logging\": { \"GlobalLogLevel\": 0 } }";
    createConfigFile(filename, json);

    auto options = GetDefaultOptions();
    
    auto expectedOptions = GetDefaultOptions();
    getChildren(expectedOptions, "logging").set("globalloglevel", 0);

    auto provider = JsonConfigProvider(StringPtr(filename));
    ASSERT_NO_THROW(provider.populateOptions(options));
    ASSERT_EQ(options, expectedOptions);
}

TEST_F(ConfigProviderTest, jsonConfigDamagaed)
{
    std::string filename = "jsonConfigIncorrectType2.json";
    std::string json = "{ \"ModuleManager\" : { ModulesPath : \"testtest\"} }";
    createConfigFile(filename, json);

    auto options = GetDefaultOptions();
    auto provider = JsonConfigProvider(StringPtr(filename));
    ASSERT_ANY_THROW(provider.populateOptions(options));

    ASSERT_EQ(options, GetDefaultOptions());
}

TEST_F(ConfigProviderTest, InstanceBuilderFromJson)
{
    std::string filename = "InstanceBuilderFromJson.json";
    std::string json =  R"(
        {
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
    instanceBuilder.addConfigProvider(JsonConfigProvider());
    instanceBuilder.build();

    ASSERT_EQ(instanceBuilder.getSchedulerWorkerNum(), 8u);
    ASSERT_EQ(int(instanceBuilder.getGlobalLogLevel()), 6);
}

TEST_F(ConfigProviderTest, envConfigReadModuleManagerPath)
{
    setEnvironmentVariableValue("OPENDAQ_CONFIG_ModuleManager_ModulesPath", "\"testtest\"");

    auto options = GetDefaultOptions(); 

    auto expectedOptions = GetDefaultOptions();
    getChildren(expectedOptions, "modulemanager").set("modulespath", "testtest");

    auto provider = EnvConfigProvider();
    provider.populateOptions(options);

    ASSERT_EQ(options, expectedOptions);
}

TEST_F(ConfigProviderTest, envConfigReadSchedulerWorkersNum)
{
    setEnvironmentVariableValue("OPENDAQ_CONFIG_Scheduler_WorkersNum", "4");

    auto options = GetDefaultOptions(); 
    
    auto expectedOptions = GetDefaultOptions();
    getChildren(expectedOptions, "scheduler").set("workersnum", 4);

    auto provider = EnvConfigProvider();
    provider.populateOptions(options);

    ASSERT_EQ(options, expectedOptions);
}

TEST_F(ConfigProviderTest, envConfigReadLoggingGlobalLogLevel)
{
    setEnvironmentVariableValue("OPENDAQ_CONFIG_Logging_GlobalLogLevel", "0");

    auto options = GetDefaultOptions(); 
    
    auto expectedOptions = GetDefaultOptions();
    getChildren(expectedOptions, "logging").set("globalloglevel", 0);

    auto provider = EnvConfigProvider();
    provider.populateOptions(options);

    ASSERT_EQ(options, expectedOptions);
}

TEST_F(ConfigProviderTest, envConfigReadRootDeviceDefaultLocalId)
{
    setEnvironmentVariableValue("OPENDAQ_CONFIG_RootDevice_DefaultLocalId", "\"localId\"");

    auto options = GetDefaultOptions(); 
    
    auto expectedOptions = GetDefaultOptions();
    getChildren(expectedOptions, "rootdevice").set("defaultlocalid", "localId");

    auto provider = EnvConfigProvider();
    provider.populateOptions(options);

    ASSERT_EQ(options, expectedOptions);
}

TEST_F(ConfigProviderTest, envConfigReadRootDeviceConnectionString)
{
    setEnvironmentVariableValue("OPENDAQ_CONFIG_RootDevice_ConnectionString", "\"dev://connectionString\"");

    auto options = GetDefaultOptions(); 
    
    auto expectedOptions = GetDefaultOptions();
    getChildren(expectedOptions, "rootdevice").set("connectionstring", "dev://connectionString");

    auto provider = EnvConfigProvider();
    provider.populateOptions(options);

    ASSERT_EQ(options, expectedOptions);
}

TEST_F(ConfigProviderTest, envConfigReadOutOfReservedName)
{
    setEnvironmentVariableValue("OPENDAQ_CONFIG_Deep1_Deep2", "\"SomeValue\"");

    auto options = GetDefaultOptions(); 
    
    auto expectedOptions = GetDefaultOptions();
    expectedOptions.set("deep1", Dict<IString, IBaseObject>({{"deep2", "SomeValue"}}));

    auto provider = EnvConfigProvider();
    provider.populateOptions(options);

    ASSERT_EQ(options, expectedOptions);
}

TEST_F(ConfigProviderTest, envConfigReadInvalidArgument1)
{
    // correct field
    setEnvironmentVariableValue("OPENDAQ_CONFIG_ModuleManager_ModulesPath", "\"testtest\"");
    // broken field (can not convert to integer)
    setEnvironmentVariableValue("OPENDAQ_CONFIG_Scheduler_WorkersNum", "\"string\"");

    auto options = GetDefaultOptions(); 
    
    auto expectedOptions = GetDefaultOptions();
    getChildren(expectedOptions, "modulemanager").set("modulespath", "testtest");

    auto provider = EnvConfigProvider();
    provider.populateOptions(options);

    ASSERT_EQ(options, expectedOptions);
}

TEST_F(ConfigProviderTest, envConfigReadInvalidArgument2)
{
    // correct field
    setEnvironmentVariableValue("OPENDAQ_CONFIG_Deep1_Deep2", "\"SomeValue\"");
    // broken field (out of depth)
    setEnvironmentVariableValue("OPENDAQ_CONFIG_ModuleManager_ModulesPath_NotExpectedChild", "\"string\"");

    auto options = GetDefaultOptions(); 
    
    auto expectedOptions = GetDefaultOptions();
    expectedOptions.set("deep1", Dict<IString, IBaseObject>({{"deep2", "SomeValue"}}));

    auto provider = EnvConfigProvider();
    provider.populateOptions(options);

    ASSERT_EQ(options, expectedOptions);
}


TEST_F(ConfigProviderTest, cmdLineArgsConfigReadModuleManagerPath)
{
    auto comandLineArgs = List<IString>("-CmoduleManager_ModulesPath=\"testtest\"");

    auto options = GetDefaultOptions(); 

    auto expectedOptions = GetDefaultOptions();
    getChildren(expectedOptions, "modulemanager").set("modulespath", "testtest");

    auto provider = CmdLineArgsConfigProvider(comandLineArgs);
    provider.populateOptions(options);

    ASSERT_EQ(options, expectedOptions);
}

TEST_F(ConfigProviderTest, cmdLineArgsConfigReadSchedulerWorkersNum)
{
    auto comandLineArgs = List<IString>("-Cscheduler_WorkersNum=4");

    auto options = GetDefaultOptions(); 
    
    auto expectedOptions = GetDefaultOptions();
    getChildren(expectedOptions, "scheduler").set("workersnum", 4);

    auto provider = CmdLineArgsConfigProvider(comandLineArgs);
    provider.populateOptions(options);

    ASSERT_EQ(options, expectedOptions);
}

TEST_F(ConfigProviderTest, cmdLineArgsConfigReadLoggingGlobalLogLevel)
{
    auto comandLineArgs = List<IString>("-Clogging_GlobalLogLevel=0");

    auto options = GetDefaultOptions(); 
    
    auto expectedOptions = GetDefaultOptions();
    getChildren(expectedOptions, "logging").set("globalloglevel", 0);

    auto provider = CmdLineArgsConfigProvider(comandLineArgs);
    provider.populateOptions(options);

    ASSERT_EQ(options, expectedOptions);
}

TEST_F(ConfigProviderTest, cmdLineArgsConfigReadRootDeviceDefaultLocalId)
{
    auto comandLineArgs = List<IString>("-CrootDevice_DefaultLocalId=\"localId\"");

    auto options = GetDefaultOptions(); 
    
    auto expectedOptions = GetDefaultOptions();
    getChildren(expectedOptions, "rootdevice").set("defaultlocalid", "localId");

    auto provider = CmdLineArgsConfigProvider(comandLineArgs);
    provider.populateOptions(options);

    ASSERT_EQ(options, expectedOptions);
}

TEST_F(ConfigProviderTest, cmdLineArgsConfigReadRootDeviceConnectionString)
{
    auto comandLineArgs = List<IString>("-CrootDevice_ConnectionString=\"dev://connectionString\"");

    auto options = GetDefaultOptions(); 
    
    auto expectedOptions = GetDefaultOptions();
    getChildren(expectedOptions, "rootdevice").set("connectionstring", "dev://connectionString");

    auto provider = CmdLineArgsConfigProvider(comandLineArgs);
    provider.populateOptions(options);

    ASSERT_EQ(options, expectedOptions);
}

TEST_F(ConfigProviderTest, cmdLineArgsConfigReadOutOfReservedName)
{
    auto comandLineArgs = List<IString>("-Cdeep1_deep2=\"SomeValue\"");

    auto options = GetDefaultOptions(); 
    
    auto expectedOptions = GetDefaultOptions();
    expectedOptions.set("deep1", Dict<IString, IBaseObject>({{"deep2", "SomeValue"}}));

    auto provider = CmdLineArgsConfigProvider(comandLineArgs);
    provider.populateOptions(options);

    ASSERT_EQ(options, expectedOptions);
}

TEST_F(ConfigProviderTest, cmdLineArgsConfigReadInvalidArgument1)
{
    auto comandLineArgs = List<IString>(
        "-CmoduleManager_ModulesPath=\"testtest\"", 
        "-Cscheduler_WorkersNum=\"string\"");

    auto options = GetDefaultOptions(); 
    
    auto expectedOptions = GetDefaultOptions();
    getChildren(expectedOptions, "modulemanager").set("modulespath", "testtest");

    auto provider = CmdLineArgsConfigProvider(comandLineArgs);
    provider.populateOptions(options);

    ASSERT_EQ(options, expectedOptions);
}

TEST_F(ConfigProviderTest, cmdLineArgsConfigReadInvalidArgument2)
{
    auto comandLineArgs = List<IString>(
        "-Cdeep1_deep2=\"SomeValue\"",
        "-CmoduleManager_ModulesPath_NotExpectedChild=\"string\"");

    auto options = GetDefaultOptions(); 
    
    auto expectedOptions = GetDefaultOptions();
    expectedOptions.set("deep1", Dict<IString, IBaseObject>({{"deep2", "SomeValue"}}));

    auto provider = CmdLineArgsConfigProvider(comandLineArgs);
    provider.populateOptions(options);

    ASSERT_EQ(options, expectedOptions);
}

TEST_F(ConfigProviderTest, cmdLineArgsConfigReadBrokenCommand)
{
    auto comandLineArgs = List<IString>("-Cdeep1_deep2=", "\"SomeValue\"");

    auto options = GetDefaultOptions(); 
    
    auto expectedOptions = GetDefaultOptions();
    expectedOptions.set("deep1", Dict<IString, IBaseObject>({{"deep2", ""}}));

    auto provider = CmdLineArgsConfigProvider(comandLineArgs);
    provider.populateOptions(options);

    ASSERT_EQ(options, expectedOptions);
}

END_NAMESPACE_OPENDAQ
