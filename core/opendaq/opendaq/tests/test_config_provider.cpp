#include <opendaq/config_provider_factory.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <set>
#include <opendaq/instance_factory.h>
#include "test_config_provider.h"

using namespace daq;
using namespace test_config_provider_helpers;

TEST_F(ConfigProviderTest, jsonConfigReadModuleManagerPath)
{
    std::string filename = "jsonConfigReadModuleManagerPath.json";
    std::string json = "{ \"ModuleManager\": { \"ModulesPath\": \"testtest\" } }";
    createConfigFile(filename, json);

    auto options = GetDefaultOptions(); 

    auto expectedOptions = GetDefaultOptions();
    getChildren(expectedOptions, "ModuleManager").set("ModulesPath", "testtest");

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
    getChildren(expectedOptions, "Scheduler").set("WorkersNum", 4);

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
    getChildren(expectedOptions, "Logging").set("GlobalLogLevel", 0);

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
    getChildren(expectedOptions, "RootDevice").set("DefaultLocalId", "localId");

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
    getChildren(expectedOptions, "RootDevice").set("ConnectionString", "dev://connectionString");

    auto provider = JsonConfigProvider(StringPtr(filename));
    provider.populateOptions(options);

    ASSERT_EQ(options, expectedOptions);
}

TEST_F(ConfigProviderTest, jsonConfigReadModules)
{
    std::string filename = "jsonConfigReadModules.json";
    std::string json = "{ \"Modules\": { \"OpenDAQOPCUAClientModule\": { \"Debug\": 1 }, \"RefDevice\": { \"UseGlobalThreadForAcq\": 1 } } }";
    createConfigFile(filename, json);

    auto options = GetDefaultOptions();

    auto expectedModules = Dict<IString, IBaseObject>({
            {"OpenDAQOPCUAClientModule", Dict<IString, IBaseObject>({
                    {"Debug", 1}
                })},
            {"RefDevice", Dict<IString, IBaseObject>({
                    {"UseGlobalThreadForAcq", 1}
                })},
        });
    auto expectedOptions = GetDefaultOptions();
    expectedOptions.set("Modules", expectedModules);    

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
    expectedOptions.set("List", List<IBaseObject>(String("test"), Integer(123), Boolean(true), Dict<IString, IBaseObject>(), BaseObjectPtr()));

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
    expectedOptions.set("NullValue", {});

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
    getChildren(expectedOptions, "RootDevice").set("ConnectionString", "dev://connectionString");

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
    getChildren(expectedOptions, "Logging").set("GlobalLogLevel", 0);

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
    getChildren(expectedOptions, "ModuleManager").set("ModulesPath", "testtest");

    auto provider = EnvConfigProvider();
    provider.populateOptions(options);

    ASSERT_EQ(options, expectedOptions);
}

TEST_F(ConfigProviderTest, envConfigReadSchedulerWorkersNum)
{
    setEnvironmentVariableValue("OPENDAQ_CONFIG_Scheduler_WorkersNum", "4");

    auto options = GetDefaultOptions(); 
    
    auto expectedOptions = GetDefaultOptions();
    getChildren(expectedOptions, "Scheduler").set("WorkersNum", 4);

    auto provider = EnvConfigProvider();
    provider.populateOptions(options);

    ASSERT_EQ(options, expectedOptions);
}

TEST_F(ConfigProviderTest, envConfigReadLoggingGlobalLogLevel)
{
    setEnvironmentVariableValue("OPENDAQ_CONFIG_Logging_GlobalLogLevel", "0");

    auto options = GetDefaultOptions(); 
    
    auto expectedOptions = GetDefaultOptions();
    getChildren(expectedOptions, "Logging").set("GlobalLogLevel", 0);

    auto provider = EnvConfigProvider();
    provider.populateOptions(options);

    ASSERT_EQ(options, expectedOptions);
}

TEST_F(ConfigProviderTest, envConfigReadRootDeviceDefaultLocalId)
{
    setEnvironmentVariableValue("OPENDAQ_CONFIG_RootDevice_DefaultLocalId", "\"localId\"");

    auto options = GetDefaultOptions(); 
    
    auto expectedOptions = GetDefaultOptions();
    getChildren(expectedOptions, "RootDevice").set("DefaultLocalId", "localId");

    auto provider = EnvConfigProvider();
    provider.populateOptions(options);

    ASSERT_EQ(options, expectedOptions);
}

TEST_F(ConfigProviderTest, envConfigReadRootDeviceConnectionString)
{
    setEnvironmentVariableValue("OPENDAQ_CONFIG_RootDevice_ConnectionString", "\"dev://connectionString\"");

    auto options = GetDefaultOptions(); 
    
    auto expectedOptions = GetDefaultOptions();
    getChildren(expectedOptions, "RootDevice").set("ConnectionString", "dev://connectionString");

    auto provider = EnvConfigProvider();
    provider.populateOptions(options);

    ASSERT_EQ(options, expectedOptions);
}

TEST_F(ConfigProviderTest, envConfigReadOutOfReservedName)
{
    setEnvironmentVariableValue("OPENDAQ_CONFIG_Deep1_Deep2", "\"SomeValue\"");

    auto options = GetDefaultOptions(); 
    
    auto expectedOptions = GetDefaultOptions();
    expectedOptions.set("Deep1", Dict<IString, IBaseObject>({{"Deep2", "SomeValue"}}));

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
    getChildren(expectedOptions, "ModuleManager").set("ModulesPath", "testtest");

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
    expectedOptions.set("Deep1", Dict<IString, IBaseObject>({{"Deep2", "SomeValue"}}));

    auto provider = EnvConfigProvider();
    provider.populateOptions(options);

    ASSERT_EQ(options, expectedOptions);
}


TEST_F(ConfigProviderTest, cmdLineArgsConfigReadModuleManagerPath)
{
    auto comandLineArgs = List<IString>("-CModuleManager_ModulesPath=\"testtest\"");

    auto options = GetDefaultOptions(); 

    auto expectedOptions = GetDefaultOptions();
    getChildren(expectedOptions, "ModuleManager").set("ModulesPath", "testtest");

    auto provider = CmdLineArgsConfigProvider(comandLineArgs);
    provider.populateOptions(options);

    ASSERT_EQ(options, expectedOptions);
}

TEST_F(ConfigProviderTest, cmdLineArgsConfigReadSchedulerWorkersNum)
{
    auto comandLineArgs = List<IString>("-CScheduler_WorkersNum=4");

    auto options = GetDefaultOptions(); 
    
    auto expectedOptions = GetDefaultOptions();
    getChildren(expectedOptions, "Scheduler").set("WorkersNum", 4);

    auto provider = CmdLineArgsConfigProvider(comandLineArgs);
    provider.populateOptions(options);

    ASSERT_EQ(options, expectedOptions);
}

TEST_F(ConfigProviderTest, cmdLineArgsConfigReadLoggingGlobalLogLevel)
{
    auto comandLineArgs = List<IString>("-CLogging_GlobalLogLevel=0");

    auto options = GetDefaultOptions(); 
    
    auto expectedOptions = GetDefaultOptions();
    getChildren(expectedOptions, "Logging").set("GlobalLogLevel", 0);

    auto provider = CmdLineArgsConfigProvider(comandLineArgs);
    provider.populateOptions(options);

    ASSERT_EQ(options, expectedOptions);
}

TEST_F(ConfigProviderTest, cmdLineArgsConfigReadRootDeviceDefaultLocalId)
{
    auto comandLineArgs = List<IString>("-CRootDevice_DefaultLocalId=\"localId\"");

    auto options = GetDefaultOptions(); 
    
    auto expectedOptions = GetDefaultOptions();
    getChildren(expectedOptions, "RootDevice").set("DefaultLocalId", "localId");

    auto provider = CmdLineArgsConfigProvider(comandLineArgs);
    provider.populateOptions(options);

    ASSERT_EQ(options, expectedOptions);
}

TEST_F(ConfigProviderTest, cmdLineArgsConfigReadRootDeviceConnectionString)
{
    auto comandLineArgs = List<IString>("-CRootDevice_ConnectionString=\"dev://connectionString\"");

    auto options = GetDefaultOptions(); 
    
    auto expectedOptions = GetDefaultOptions();
    getChildren(expectedOptions, "RootDevice").set("ConnectionString", "dev://connectionString");

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
        "-CModuleManager_ModulesPath=\"testtest\"", 
        "-CScheduler_WorkersNum=\"string\"");

    auto options = GetDefaultOptions(); 
    
    auto expectedOptions = GetDefaultOptions();
    getChildren(expectedOptions, "ModuleManager").set("ModulesPath", "testtest");

    auto provider = CmdLineArgsConfigProvider(comandLineArgs);
    provider.populateOptions(options);

    ASSERT_EQ(options, expectedOptions);
}

TEST_F(ConfigProviderTest, cmdLineArgsConfigReadInvalidArgument2)
{
    auto comandLineArgs = List<IString>(
        "-Cdeep1_deep2=\"SomeValue\"",
        "-CModuleManager_ModulesPath_NotExpectedChild=\"string\"");

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
