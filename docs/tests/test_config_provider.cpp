#include <coretypes/listobject_factory.h>
#include <gtest/gtest.h>
#include <opendaq/opendaq.h>
#include "docs_test_helpers.h"

using namespace daq;

using ConfigProvider = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

// Corresponding document: Antora/modules/howto_guides/pages/howto_configure_instance_providers.adoc
TEST_F(ConfigProvider, JsonConfigProvider)
{
    const StringPtr configPath = "opendaq-config.json";
    const ConfigProviderPtr configProvider = JsonConfigProvider(configPath);
    const InstanceBuilderPtr instanceBuilder = InstanceBuilder().addConfigProvider(configProvider);
    const InstancePtr instance = InstanceFromBuilder(instanceBuilder);
}

TEST_F(ConfigProvider, EnvConfigProvider)
{
    const ConfigProviderPtr configProvider = EnvConfigProvider();
    const InstanceBuilderPtr instanceBuilder = InstanceBuilder().addConfigProvider(configProvider);
    const InstancePtr instance = InstanceFromBuilder(instanceBuilder);
}

static ConfigProviderPtr CmdLineArgsConfigProvider(int argc, const char* argv[])
{
    ListPtr<IString> args = List<IString>();
    for (int i = 1; i < argc; i++)
        args.pushBack(argv[i]);

    return CmdLineArgsConfigProvider(args);
}

TEST_F(ConfigProvider, CmdLineArgsConfigProvider)
{
    int argc = 2;
    const char* argv[] = {"program", "-Cmodulemanager_modulespath=\"\""};

    const ConfigProviderPtr configProvider = CmdLineArgsConfigProvider(argc, argv);
    const InstanceBuilderPtr instanceBuilder = InstanceBuilder().addConfigProvider(configProvider);
    const InstancePtr instance = InstanceFromBuilder(instanceBuilder);
}

END_NAMESPACE_OPENDAQ
