#include <gtest/gtest.h>
#include <opendaq/opendaq.h>
#include "docs_test_helpers.h"
#include <coretypes/listobject_factory.h>

using ConfigProvider = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

// Corresponding document: Antora/modules/howto_guides/pages/howto_configure_instance_providers.adoc
TEST_F(ConfigProvider, JsonConfigProvider)
{
    const daq::StringPtr configPath = "opendaq-config.json";
    const daq::ConfigProviderPtr configProvider = daq::JsonConfigProvider(configPath);
    const daq::InstanceBuilderPtr instanceBuilder = daq::InstanceBuilder().addConfigProvider(configProvider);
    const daq::InstancePtr instance = daq::InstanceFromBuilder(instanceBuilder);
}

TEST_F(ConfigProvider, EnvConfigProvider)
{
    const daq::ConfigProviderPtr configProvider = daq::EnvConfigProvider();
    const daq::InstanceBuilderPtr instanceBuilder = daq::InstanceBuilder().addConfigProvider(configProvider);
    const daq::InstancePtr instance = daq::InstanceFromBuilder(instanceBuilder);
}

static ConfigProviderPtr CmdLineArgsConfigProvider(int argc, const char* argv[])
{
  daq::ListPtr<IString> args = daq::List<IString>();
  for (int i = 1; i < argc; i++)
    args.pushBack(argv[i]);

  return daq::CmdLineArgsConfigProvider(args);
}

TEST_F(ConfigProvider, CmdLineArgsConfigProvider)
{
    int argc = 2; 
    const char* argv[] = {"program", "-Cmodulemanager_modulespath=\"\""};

    const daq::ConfigProviderPtr configProvider = CmdLineArgsConfigProvider(argc, argv);
    const daq::InstanceBuilderPtr instanceBuilder = daq::InstanceBuilder().addConfigProvider(configProvider);
    const daq::InstancePtr instance = daq::InstanceFromBuilder(instanceBuilder);
}

END_NAMESPACE_OPENDAQ