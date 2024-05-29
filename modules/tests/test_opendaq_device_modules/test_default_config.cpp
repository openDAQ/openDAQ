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

TEST_F(ModulesDefaultConfigTest, NativeConfigDevice)
{
    const auto instance = Instance();
    const auto config = instance.createDefaultAddDeviceConfig();
    const PropertyObjectPtr deviceConfig = config.getPropertyValue("device");
    const PropertyObjectPtr nativeDeviceConfig = deviceConfig.getPropertyValue("opendaq_native_config");
    ASSERT_TRUE(nativeDeviceConfig.hasProperty("Port"));
}

TEST_F(ModulesDefaultConfigTest, NativeConfigDeviceConnect)
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

TEST_F(ModulesDefaultConfigTest, NativeStreamingDevice)
{
    const auto instance = Instance();
    const auto config = instance.createDefaultAddDeviceConfig();
    const PropertyObjectPtr deviceConfig = config.getPropertyValue("device");
    const PropertyObjectPtr nativeDeviceConfig = deviceConfig.getPropertyValue("opendaq_native_streaming");
    ASSERT_TRUE(nativeDeviceConfig.hasProperty("Port"));
}

TEST_F(ModulesDefaultConfigTest, NativeStreamingDeviceConnect)
{
    const auto serverInstance = Instance();
    const auto serverConfig = PropertyObject();
    serverConfig.addProperty(IntProperty("NativeStreamingPort", 7415));
    serverInstance.addServer("openDAQ Native Streaming", serverConfig);

    const auto instance = Instance();
    const auto config = instance.createDefaultAddDeviceConfig();
    const PropertyObjectPtr deviceConfig = config.getPropertyValue("device");
    const PropertyObjectPtr nativeDeviceConfig = deviceConfig.getPropertyValue("opendaq_native_config");
    nativeDeviceConfig.setPropertyValue("Port", 7415);

    ASSERT_TRUE(instance.addDevice("daq.nd://127.0.0.1", config).assigned());
}

TEST_F(ModulesDefaultConfigTest, LTStreamingDevice)
{
    const auto instance = Instance();
    const auto config = instance.createDefaultAddDeviceConfig();
    const PropertyObjectPtr deviceConfig = config.getPropertyValue("device");
    const PropertyObjectPtr nativeDeviceConfig = deviceConfig.getPropertyValue("opendaq_lt_streaming");
    ASSERT_TRUE(nativeDeviceConfig.hasProperty("Port"));
}

TEST_F(ModulesDefaultConfigTest, LTStreamingDeviceConnect)
{
    const auto serverInstance = Instance();
    const auto serverConfig = PropertyObject();
    serverConfig.addProperty(IntProperty("WebsocketStreamingPort", 7415));
    serverInstance.addServer("openDAQ LT Streaming", serverConfig);

    const auto instance = Instance();
    const auto config = instance.createDefaultAddDeviceConfig();
    const PropertyObjectPtr deviceConfig = config.getPropertyValue("device");
    const PropertyObjectPtr nativeDeviceConfig = deviceConfig.getPropertyValue("opendaq_lt_streaming");
    nativeDeviceConfig.setPropertyValue("Port", 7415);

    ASSERT_TRUE(instance.addDevice("daq.lt://127.0.0.1", config).assigned());
}

TEST_F(ModulesDefaultConfigTest, OPCUAConfigDevice)
{
    const auto instance = Instance();
    const auto config = instance.createDefaultAddDeviceConfig();
    const PropertyObjectPtr deviceConfig = config.getPropertyValue("device");
    const PropertyObjectPtr nativeDeviceConfig = deviceConfig.getPropertyValue("opendaq_opcua_config");
    ASSERT_TRUE(nativeDeviceConfig.hasProperty("Port"));
}

TEST_F(ModulesDefaultConfigTest, OPCUAConfigDeviceConnect)
{
    const auto serverInstance = Instance();
    const auto serverConfig = PropertyObject();
    serverConfig.addProperty(IntProperty("Port", 4841));
    serverInstance.addServer("openDAQ OpcUa", serverConfig);

    const auto instance = Instance();
    const auto config = instance.createDefaultAddDeviceConfig();
    const PropertyObjectPtr deviceConfig = config.getPropertyValue("device");
    const PropertyObjectPtr nativeDeviceConfig = deviceConfig.getPropertyValue("opendaq_opcua_config");
    nativeDeviceConfig.setPropertyValue("Port", 4841);

    ASSERT_TRUE(instance.addDevice("daq.opcua://127.0.0.1", config).assigned());
}

TEST_F(ModulesDefaultConfigTest, NativeConfigDeviceNativeStreamingConnect)
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

