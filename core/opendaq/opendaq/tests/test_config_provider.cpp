#include <opendaq/config_provider_factory.h>
#include <gtest/gtest.h>
#include <fstream>
#include <iostream>
#include <cstdio>

BEGIN_NAMESPACE_OPENDAQ

class ConfigProviderTest : public testing::Test
{
protected:
    void SetUp() override
    {
        std::ofstream file;
        file.open(filename.toStdString());
        if (!file.is_open()) 
            throw std::runtime_error("can not open file for writting");

        file << R"(
            {
                "ModuleManager": {
                    "ModulesPath": "testtest"
                },
                "Scheduler": {
                    "WorkersNum": 123
                },
                "Logging": {
                    "GlobalLogLevel": 1
                },
                "Modules": {},
                "FloatValue": 1.001,
                "ListValue": [
                    123,
                    "string",
                    {
                        "val1": 123
                    },
                    [
                        1,
                        2,
                        3
                    ]
                ]
            }
        )";
        file.close();

    }

    void TearDown() override
    {
        remove(filename.toStdString().c_str());
    }

    StringPtr filename = "instance_config.json";
};

TEST_F(ConfigProviderTest, jsonConfigRead)
{
    auto provider = JsonConfigProvider(StringPtr(filename));
    auto options = Dict<IString, IBaseObject>({
        {"ModuleManager", Dict<IString, IBaseObject>({
                {"ModulesPath", ""}
            })},
        {"Scheduler", Dict<IString, IBaseObject>({
                {"WorkersNum", 0}
            })},
        {"Logging", Dict<IString, IBaseObject>({
                {"GlobalLogLevel", 2}
            })},
        {"Modules", Dict<IString, IBaseObject>()},
        {"FloatValue",123.123}
    });
    provider.populateOptions(options);
    std::cout << "done" << std::endl;
}


END_NAMESPACE_OPENDAQ