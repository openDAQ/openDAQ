#include "test_helpers/test_helpers.h"

using ModulesDefaultConfigTest = testing::Test;

using namespace daq;

TEST_F(ModulesDefaultConfigTest, Create)
{
    const auto instance = Instance();
    const auto config = instance.createDefaultAddDeviceConfig();
    ASSERT_TRUE(config.hasProperty("General"));
    ASSERT_TRUE(config.hasProperty("Device"));
    ASSERT_TRUE(config.hasProperty("Streaming"));
}

TEST_F(ModulesDefaultConfigTest, GeneralConfig)
{
    const auto instance = Instance();
    const auto config = instance.createDefaultAddDeviceConfig();
    const PropertyObjectPtr general = config.getPropertyValue("General");
    ASSERT_TRUE(general.hasProperty("PrioritizedStreamingProtocols"));
    ASSERT_TRUE(general.hasProperty("StreamingConnectionHeuristic"));
    ASSERT_TRUE(general.hasProperty("AllowedStreamingProtocols"));
    ASSERT_TRUE(general.hasProperty("AutomaticallyConnectStreaming"));
}

TEST_F(ModulesDefaultConfigTest, NativeConfigDevice)
{
    const auto instance = Instance();
    const auto config = instance.createDefaultAddDeviceConfig();
    const PropertyObjectPtr deviceConfig = config.getPropertyValue("Device");
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
    const PropertyObjectPtr deviceConfig = config.getPropertyValue("Device");
    const PropertyObjectPtr nativeDeviceConfig = deviceConfig.getPropertyValue("opendaq_native_config");
    nativeDeviceConfig.setPropertyValue("Port", 7421);

    ASSERT_TRUE(instance.addDevice("daq.nd://127.0.0.1", config).assigned());
}

TEST_F(ModulesDefaultConfigTest, NativeStreamingDevice)
{
    const auto instance = Instance();
    const auto config = instance.createDefaultAddDeviceConfig();
    const PropertyObjectPtr deviceConfig = config.getPropertyValue("Device");
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
    const PropertyObjectPtr deviceConfig = config.getPropertyValue("Device");
    const PropertyObjectPtr nativeDeviceConfig = deviceConfig.getPropertyValue("opendaq_native_config");
    nativeDeviceConfig.setPropertyValue("Port", 7415);

    ASSERT_TRUE(instance.addDevice("daq.nd://127.0.0.1", config).assigned());
}

TEST_F(ModulesDefaultConfigTest, LTStreamingDevice)
{
    const auto instance = Instance();
    const auto config = instance.createDefaultAddDeviceConfig();
    const PropertyObjectPtr deviceConfig = config.getPropertyValue("Device");
    const PropertyObjectPtr ltDeviceConfig = deviceConfig.getPropertyValue("opendaq_lt_streaming");
    ASSERT_TRUE(ltDeviceConfig.hasProperty("Port"));
}

TEST_F(ModulesDefaultConfigTest, LTStreamingDeviceConnect)
{
    const auto serverInstance = Instance();
    const auto serverConfig = PropertyObject();
    serverConfig.addProperty(IntProperty("WebsocketStreamingPort", 7415));
    serverInstance.addServer("openDAQ LT Streaming", serverConfig);

    const auto instance = Instance();
    const auto config = instance.createDefaultAddDeviceConfig();
    const PropertyObjectPtr deviceConfig = config.getPropertyValue("Device");
    const PropertyObjectPtr ltDeviceConfig = deviceConfig.getPropertyValue("opendaq_lt_streaming");
    ltDeviceConfig.setPropertyValue("Port", 7415);

    ASSERT_TRUE(instance.addDevice("daq.lt://127.0.0.1", config).assigned());
}

TEST_F(ModulesDefaultConfigTest, OPCUAConfigDevice)
{
    const auto instance = Instance();
    const auto config = instance.createDefaultAddDeviceConfig();
    const PropertyObjectPtr deviceConfig = config.getPropertyValue("Device");
    const PropertyObjectPtr opcuaDeviceConfig  = deviceConfig.getPropertyValue("opendaq_opcua_config");
    ASSERT_TRUE(opcuaDeviceConfig .hasProperty("Port"));
}

TEST_F(ModulesDefaultConfigTest, OPCUAConfigDeviceConnect)
{
    const auto serverInstance = Instance();
    const auto serverConfig = PropertyObject();
    serverConfig.addProperty(IntProperty("Port", 4841));
    serverInstance.addServer("openDAQ OpcUa", serverConfig);

    const auto instance = Instance();
    const auto config = instance.createDefaultAddDeviceConfig();
    const PropertyObjectPtr deviceConfig = config.getPropertyValue("Device");
    const PropertyObjectPtr opcuaDeviceConfig = deviceConfig.getPropertyValue("opendaq_opcua_config");
    opcuaDeviceConfig .setPropertyValue("Port", 4841);

    ASSERT_TRUE(instance.addDevice("daq.opcua://127.0.0.1", config).assigned());
}

TEST_F(ModulesDefaultConfigTest, NativeConfigDeviceNativeStreamingConnect)
{
    const auto serverInstance = Instance();
    const auto serverConfig = PropertyObject();
    serverConfig.addProperty(IntProperty("NativeStreamingPort", 7415));
    serverInstance.addServer("openDAQ Native Streaming", serverConfig);
    serverInstance.addServer("openDAQ LT Streaming", nullptr);

    const auto instance = Instance();
    const auto config = instance.createDefaultAddDeviceConfig();
    const PropertyObjectPtr deviceConfig = config.getPropertyValue("Device");
    const PropertyObjectPtr nativeDeviceConfig = deviceConfig.getPropertyValue("opendaq_native_config");
    nativeDeviceConfig.setPropertyValue("Port", 7415);

    const PropertyObjectPtr streamingConfig = config.getPropertyValue("Streaming");
    const PropertyObjectPtr nativeStreamingConfig = streamingConfig.getPropertyValue("opendaq_native_streaming");
    nativeStreamingConfig.setPropertyValue("Port", 7415);
    
    const PropertyObjectPtr generalConfig = config.getPropertyValue("General");
    generalConfig.setPropertyValue("AllowedStreamingProtocols", List<IString>("opendaq_native_streaming"));

    const auto device = instance.addDevice("daq.nd://127.0.0.1", config);
    ASSERT_EQ(device.asPtr<IMirroredDevice>().getStreamingSources().getCount(), 1);
}

TEST_F(ModulesDefaultConfigTest, NativeConfigDeviceAnyStreamingConnect)
{
    const auto serverInstance = Instance();
    const auto serverConfig = PropertyObject();
    serverConfig.addProperty(IntProperty("NativeStreamingPort", 7415));
    serverInstance.addServer("openDAQ Native Streaming", serverConfig);
    serverInstance.addServer("openDAQ LT Streaming", nullptr);

    const auto instance = Instance();
    const auto config = instance.createDefaultAddDeviceConfig();
    const PropertyObjectPtr deviceConfig = config.getPropertyValue("Device");
    const PropertyObjectPtr nativeDeviceConfig = deviceConfig.getPropertyValue("opendaq_native_config");
    nativeDeviceConfig.setPropertyValue("Port", 7415);

    const PropertyObjectPtr streamingConfig = config.getPropertyValue("Streaming");
    const PropertyObjectPtr nativeStreamingConfig = streamingConfig.getPropertyValue("opendaq_native_streaming");
    nativeStreamingConfig.setPropertyValue("Port", 7415);
    
    const PropertyObjectPtr generalConfig = config.getPropertyValue("General");
    generalConfig.setPropertyValue("AllowedStreamingProtocols", List<IString>());

    const auto device = instance.addDevice("daq.nd://127.0.0.1", config);
    ASSERT_EQ(device.asPtr<IMirroredDevice>().getStreamingSources().getCount(), 2);
}

TEST_F(ModulesDefaultConfigTest, NativeConfigDeviceNoStreamingConnect)
{
    const auto serverInstance = Instance();
    const auto serverConfig = PropertyObject();
    serverConfig.addProperty(IntProperty("NativeStreamingPort", 7415));
    serverInstance.addServer("openDAQ Native Streaming", serverConfig);

    const auto instance = Instance();
    const auto config = instance.createDefaultAddDeviceConfig();
    const PropertyObjectPtr deviceConfig = config.getPropertyValue("Device");
    const PropertyObjectPtr nativeDeviceConfig = deviceConfig.getPropertyValue("opendaq_native_config");
    nativeDeviceConfig.setPropertyValue("Port", 7415);

    const PropertyObjectPtr generalConfig = config.getPropertyValue("General");
    generalConfig.setPropertyValue("AutomaticallyConnectStreaming", false);

    const auto device = instance.addDevice("daq.nd://127.0.0.1", config);
    ASSERT_EQ(device.asPtr<IMirroredDevice>().getStreamingSources().getCount(), 0);
}

TEST_F(ModulesDefaultConfigTest, OPCUAConfigLTStreamingConnect)
{
    const auto serverInstance = Instance();

    const auto ltServerConfig = PropertyObject();
    ltServerConfig.addProperty(IntProperty("WebsocketStreamingPort", 7415));
    serverInstance.addServer("openDAQ LT Streaming", ltServerConfig);

    serverInstance.addServer("openDAQ Native Streaming", nullptr);

    const auto opcuaServerConfig = PropertyObject();
    opcuaServerConfig.addProperty(IntProperty("Port", 4841));
    serverInstance.addServer("openDAQ OpcUa", opcuaServerConfig);

    const auto instance = Instance();
    const auto config = instance.createDefaultAddDeviceConfig();
    const PropertyObjectPtr deviceConfig = config.getPropertyValue("Device");
    const PropertyObjectPtr opcuaDeviceConfig = deviceConfig.getPropertyValue("opendaq_opcua_config");
    opcuaDeviceConfig.setPropertyValue("Port", 4841);

    const PropertyObjectPtr streamingConfig = config.getPropertyValue("Streaming");
    const PropertyObjectPtr ltStreamingConfig = streamingConfig.getPropertyValue("opendaq_lt_streaming");
    ltStreamingConfig.setPropertyValue("Port", 7415);
    
    const PropertyObjectPtr generalConfig = config.getPropertyValue("General");
    generalConfig.setPropertyValue("AllowedStreamingProtocols", List<IString>("opendaq_lt_streaming"));

    const auto device = instance.addDevice("daq.opcua://127.0.0.1", config);
    ASSERT_EQ(device.asPtr<IMirroredDevice>().getStreamingSources().getCount(), 1);
}

TEST_F(ModulesDefaultConfigTest, OPCUAConfigAnyStreamingConnect)
{
    const auto serverInstance = Instance();

    const auto ltServerConfig = PropertyObject();
    ltServerConfig.addProperty(IntProperty("WebsocketStreamingPort", 7415));
    serverInstance.addServer("openDAQ LT Streaming", ltServerConfig);
    
    const auto nativeServerConfig = PropertyObject();
    nativeServerConfig.addProperty(IntProperty("NativeStreamingPort", 7416));
    serverInstance.addServer("openDAQ Native Streaming", nativeServerConfig);

    const auto opcuaServerConfig = PropertyObject();
    opcuaServerConfig.addProperty(IntProperty("Port", 4841));
    serverInstance.addServer("openDAQ OpcUa", opcuaServerConfig);

    const auto instance = Instance();
    const auto config = instance.createDefaultAddDeviceConfig();
    const PropertyObjectPtr deviceConfig = config.getPropertyValue("Device");
    const PropertyObjectPtr nativeDeviceConfig = deviceConfig.getPropertyValue("opendaq_opcua_config");
    nativeDeviceConfig.setPropertyValue("Port", 4841);
    
    const PropertyObjectPtr streamingConfig = config.getPropertyValue("Streaming");
    const PropertyObjectPtr ltStreamingConfig = streamingConfig.getPropertyValue("opendaq_lt_streaming");
    ltStreamingConfig.setPropertyValue("Port", 7415);

    const PropertyObjectPtr nativeStreamingConfig = streamingConfig.getPropertyValue("opendaq_native_streaming");
    nativeStreamingConfig.setPropertyValue("Port", 7416);

    const PropertyObjectPtr generalConfig = config.getPropertyValue("General");
    generalConfig.setPropertyValue("AllowedStreamingProtocols", List<IString>());

    const auto device = instance.addDevice("daq.opcua://127.0.0.1", config);
    ASSERT_EQ(device.asPtr<IMirroredDevice>().getStreamingSources().getCount(), 2);
}
