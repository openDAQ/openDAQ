#include "test_helpers/test_helpers.h"

using ModulesDefaultConfigTest = testing::Test;

using namespace daq;

TEST_F(ModulesDefaultConfigTest, Create)
{
    const auto instance = Instance();
    const auto config = instance.createDefaultAddDeviceConfig();
    ASSERT_TRUE(config.hasProperty("general"));
    ASSERT_TRUE(config.hasProperty("device"));
    ASSERT_TRUE(config.hasProperty("streaming"));
}

TEST_F(ModulesDefaultConfigTest, GeneralConfig)
{
    const auto instance = Instance();
    const auto config = instance.createDefaultAddDeviceConfig();
    const PropertyObjectPtr general = config.getPropertyValue("general");
    ASSERT_TRUE(general.hasProperty("PrioritizedStreamingProtocols"));
    ASSERT_TRUE(general.hasProperty("StreamingConnectionHeuristic"));
}

TEST_F(ModulesDefaultConfigTest, NativeConfig)
{
    const auto instance = Instance();
    const auto config = instance.createDefaultAddDeviceConfig();
    const PropertyObjectPtr deviceConfig = config.getPropertyValue("device");
    const PropertyObjectPtr nativeDeviceConfig = deviceConfig.getPropertyValue("opendaq_native_config");
    ASSERT_TRUE(nativeDeviceConfig.hasProperty("Port"));
}

TEST_F(ModulesDefaultConfigTest, NativeConfigConnect)
{
    const auto serverInstance = Instance();
    const auto serverConfig = PropertyObject();
    serverConfig.addProperty(IntProperty("NativeStreamingPort", 7421));
    serverInstance.addServer("openDAQ Native Streaming", serverConfig);

    const auto instance = Instance();
    const auto config = instance.createDefaultAddDeviceConfig();
    const PropertyObjectPtr deviceConfig = config.getPropertyValue("device");
    const PropertyObjectPtr nativeDeviceConfig = deviceConfig.getPropertyValue("opendaq_native_config");
    nativeDeviceConfig.setPropertyValue("Port", 7421);

    ASSERT_TRUE(instance.addDevice("daq.nd://127.0.0.1", config).assigned());
}

TEST_F(ModulesDefaultConfigTest, NativeStreaming)
{
    const auto instance = Instance();
    const auto config = instance.createDefaultAddDeviceConfig();
}

TEST_F(ModulesDefaultConfigTest, NativeStreamingConnect)
{
    const auto instance = Instance();
    const auto config = instance.createDefaultAddDeviceConfig();
}

TEST_F(ModulesDefaultConfigTest, LTStreaming)
{
    const auto instance = Instance();
    const auto config = instance.createDefaultAddDeviceConfig();
}

TEST_F(ModulesDefaultConfigTest, LTStreamingConnect)
{
    const auto instance = Instance();
    const auto config = instance.createDefaultAddDeviceConfig();
}

TEST_F(ModulesDefaultConfigTest, OPCUAConfig)
{
    const auto instance = Instance();
    const auto config = instance.createDefaultAddDeviceConfig();
}

TEST_F(ModulesDefaultConfigTest, OPCUAConfigConnect)
{
    const auto instance = Instance();
    const auto config = instance.createDefaultAddDeviceConfig();
}

TEST_F(ModulesDefaultConfigTest, NativeConfigNativeStreamingConnect)
{
    const auto instance = Instance();
    const auto config = instance.createDefaultAddDeviceConfig();
}

TEST_F(ModulesDefaultConfigTest, NativeConfigLTStreamingConnect)
{
    const auto instance = Instance();
    const auto config = instance.createDefaultAddDeviceConfig();
}

TEST_F(ModulesDefaultConfigTest, OPCUAConfigNativeConnect)
{
    const auto instance = Instance();
    const auto config = instance.createDefaultAddDeviceConfig();
}

