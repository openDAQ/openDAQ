#include "opendaq/mock/mock_device_module.h"
#include "test_helpers/test_helpers.h"
#include "test_helpers/test_ports.h"
#include "test_helpers/device_modules.h"

using ModulesDefaultConfigTest = testing::Test;

using namespace daq;

TEST_F(ModulesDefaultConfigTest, Create)
{
    const auto instance = test_helpers::createInstance("[[none]]");
    const auto config = instance.createDefaultAddDeviceConfig();
    ASSERT_TRUE(config.hasProperty("General"));
    ASSERT_TRUE(config.hasProperty("Device"));
    ASSERT_TRUE(config.hasProperty("Streaming"));
}

TEST_F(ModulesDefaultConfigTest, GeneralConfig)
{
    const auto instance = test_helpers::createInstance("[[none]]");
    const auto config = instance.createDefaultAddDeviceConfig();
    const PropertyObjectPtr general = config.getPropertyValue("General");
    ASSERT_TRUE(general.hasProperty("PrioritizedStreamingProtocols"));
    ASSERT_TRUE(general.hasProperty("StreamingConnectionHeuristic"));
    ASSERT_TRUE(general.hasProperty("AllowedStreamingProtocols"));
    ASSERT_TRUE(general.hasProperty("AutomaticallyConnectStreaming"));
    ASSERT_TRUE(general.hasProperty("PrimaryAddressType"));
}

TEST_F(ModulesDefaultConfigTest, AddDeviceWithoutConfig)
{
    const auto instance = test_helpers::createInstance("[[none]]");
    const ModulePtr deviceModule(MockDeviceModule_Create(instance.getContext()));
    instance.getModuleManager().addModule(deviceModule);

    auto device = instance.addDevice("daqmock://phys_device");
    auto addedComponentConfig = device.asPtr<IComponentPrivate>(true).getComponentConfig();
    ASSERT_TRUE(addedComponentConfig.assigned());

    const auto defaultConfig = instance.createDefaultAddDeviceConfig();
    test_helpers::testPropObjsEquality(defaultConfig, addedComponentConfig);
}

TEST_F(ModulesDefaultConfigTest, AddDeviceWithDefConfigFromDeviceType)
{
    const auto instance = test_helpers::createInstance("[[none]]");
    const ModulePtr deviceModule(MockDeviceModule_Create(instance.getContext()));
    instance.getModuleManager().addModule(deviceModule);

    auto deviceTypes = instance.getAvailableDeviceTypes();
    auto mockDeviceConfig = deviceTypes.get("mock_phys_device").createDefaultConfig();

    auto device = instance.addDevice("daqmock://phys_device", mockDeviceConfig);
    auto addedComponentConfig = device.asPtr<IComponentPrivate>(true).getComponentConfig();
    ASSERT_TRUE(addedComponentConfig.assigned());

    const auto defaultConfig = instance.createDefaultAddDeviceConfig();
    test_helpers::testPropObjsEquality(defaultConfig, addedComponentConfig);
}

TEST_F(ModulesDefaultConfigTest, NativeConfigDevice)
{
    const auto instance = test_helpers::createInstance("[[none]]");
    addNativeClientModule(instance);

    const auto config = instance.createDefaultAddDeviceConfig();
    const PropertyObjectPtr deviceConfig = config.getPropertyValue("Device");
    const PropertyObjectPtr nativeDeviceConfig = deviceConfig.getPropertyValue("OpenDAQNativeConfiguration");
    ASSERT_TRUE(nativeDeviceConfig.hasProperty("Port"));
    ASSERT_TRUE(nativeDeviceConfig.hasProperty("TransportLayerConfig"));
    ASSERT_TRUE(nativeDeviceConfig.hasProperty("ProtocolVersion"));
    ASSERT_TRUE(nativeDeviceConfig.hasProperty("ConfigProtocolRequestTimeout"));
    ASSERT_TRUE(nativeDeviceConfig.hasProperty("RestoreClientConfigOnReconnect"));
}

TEST_F(ModulesDefaultConfigTest, NativeConfigDeviceConnect)
{
    const auto serverInstance = test_helpers::createInstance("[[none]]");
    addNativeServerModule(serverInstance);

    const auto serverConfig = PropertyObject();
    serverConfig.addProperty(IntProperty("NativeStreamingPort", 7421));
    test_helpers::addServer(serverInstance, "OpenDAQNativeStreaming", serverConfig);

    const auto instance = test_helpers::createInstance("[[none]]");
    addNativeClientModule(instance);

    const auto config = instance.createDefaultAddDeviceConfig();
    const PropertyObjectPtr deviceConfig = config.getPropertyValue("Device");
    const PropertyObjectPtr nativeDeviceConfig = deviceConfig.getPropertyValue("OpenDAQNativeConfiguration");
    nativeDeviceConfig.setPropertyValue("Port", test_helpers::testPort(7421));

    auto device = instance.addDevice("daq.nd://127.0.0.1", config);
    ASSERT_TRUE(device.assigned());

    auto addedComponentConfig = device.asPtr<IComponentPrivate>(true).getComponentConfig();
    ASSERT_TRUE(addedComponentConfig.assigned());
    test_helpers::testPropObjsEquality(config, addedComponentConfig);
}

TEST_F(ModulesDefaultConfigTest, NativeStreaming)
{
    const auto instance = test_helpers::createInstance("[[none]]");
    addNativeClientModule(instance);

    const auto config = instance.createDefaultAddDeviceConfig();

    const PropertyObjectPtr deviceConfig = config.getPropertyValue("Streaming");
    const PropertyObjectPtr nativeDeviceConfig = deviceConfig.getPropertyValue("OpenDAQNativeStreaming");
    ASSERT_TRUE(nativeDeviceConfig.hasProperty("Port"));
    ASSERT_TRUE(nativeDeviceConfig.hasProperty("TransportLayerConfig"));
    ASSERT_FALSE(nativeDeviceConfig.hasProperty("ProtocolVersion"));
    ASSERT_FALSE(nativeDeviceConfig.hasProperty("ConfigProtocolRequestTimeout"));
    ASSERT_FALSE(nativeDeviceConfig.hasProperty("RestoreClientConfigOnReconnect"));
}

TEST_F(ModulesDefaultConfigTest, NativeStreamingDevice)
{
    const auto instance = test_helpers::createInstance("[[none]]");
    addNativeClientModule(instance);

    const auto config = instance.createDefaultAddDeviceConfig();

    const PropertyObjectPtr deviceConfig = config.getPropertyValue("Device");
    const PropertyObjectPtr nativeDeviceConfig = deviceConfig.getPropertyValue("OpenDAQNativeStreaming");
    ASSERT_TRUE(nativeDeviceConfig.hasProperty("Port"));
    ASSERT_TRUE(nativeDeviceConfig.hasProperty("TransportLayerConfig"));
    ASSERT_FALSE(nativeDeviceConfig.hasProperty("ProtocolVersion"));
    ASSERT_FALSE(nativeDeviceConfig.hasProperty("ConfigProtocolRequestTimeout"));
    ASSERT_FALSE(nativeDeviceConfig.hasProperty("RestoreClientConfigOnReconnect"));
}

TEST_F(ModulesDefaultConfigTest, NativeStreamingDeviceConnect)
{
    const auto serverInstance = test_helpers::createInstance("[[none]]");
    addNativeServerModule(serverInstance);

    const auto serverConfig = PropertyObject();
    serverConfig.addProperty(IntProperty("NativeStreamingPort", 7415));
    test_helpers::addServer(serverInstance, "OpenDAQNativeStreaming", serverConfig);

    const auto instance = test_helpers::createInstance("[[none]]");
    addNativeClientModule(instance);

    const auto config = instance.createDefaultAddDeviceConfig();
    const PropertyObjectPtr deviceConfig = config.getPropertyValue("Device");
    const PropertyObjectPtr nativeDeviceConfig = deviceConfig.getPropertyValue("OpenDAQNativeConfiguration");
    nativeDeviceConfig.setPropertyValue("Port", test_helpers::testPort(7415));

    auto device = instance.addDevice("daq.nd://127.0.0.1", config);
    ASSERT_TRUE(device.assigned());

    auto addedComponentConfig = device.asPtr<IComponentPrivate>(true).getComponentConfig();
    ASSERT_TRUE(addedComponentConfig.assigned());
    test_helpers::testPropObjsEquality(config, addedComponentConfig);
}

TEST_F(ModulesDefaultConfigTest, LTStreamingDevice)
{
    const auto instance = test_helpers::createInstance("[[none]]");
    addLtClientModule(instance);

    const auto config = instance.createDefaultAddDeviceConfig();
    const PropertyObjectPtr deviceConfig = config.getPropertyValue("Device");
    const PropertyObjectPtr ltDeviceConfig = deviceConfig.getPropertyValue("OpenDAQLTStreaming");
    ASSERT_TRUE(ltDeviceConfig.hasProperty("Port"));
}

TEST_F(ModulesDefaultConfigTest, LTStreamingDeviceConnect)
{
    const auto serverInstance = test_helpers::createInstance("[[none]]");
    addLtServerModule(serverInstance);

    const auto serverConfig = PropertyObject();
    serverConfig.addProperty(IntProperty("WebsocketStreamingPort", 7415));
    test_helpers::addServer(serverInstance, "OpenDAQLTStreaming", serverConfig);

    const auto instance = test_helpers::createInstance("[[none]]");
    addLtClientModule(instance);

    const auto config = instance.createDefaultAddDeviceConfig();
    const PropertyObjectPtr deviceConfig = config.getPropertyValue("Device");
    const PropertyObjectPtr ltDeviceConfig = deviceConfig.getPropertyValue("OpenDAQLTStreaming");
    ltDeviceConfig.setPropertyValue("Port", test_helpers::testPort(7415));

    auto device = instance.addDevice("daq.lt://127.0.0.1", config);
    ASSERT_TRUE(device.assigned());

    auto addedComponentConfig = device.asPtr<IComponentPrivate>(true).getComponentConfig();
    ASSERT_TRUE(addedComponentConfig.assigned());
    test_helpers::testPropObjsEquality(config, addedComponentConfig);
}

TEST_F(ModulesDefaultConfigTest, OPCUAConfigDevice)
{
    const auto instance = test_helpers::createInstance("[[none]]");
    addOpcuaClientModule(instance);

    const auto config = instance.createDefaultAddDeviceConfig();
    const PropertyObjectPtr deviceConfig = config.getPropertyValue("Device");
    const PropertyObjectPtr opcuaDeviceConfig  = deviceConfig.getPropertyValue("OpenDAQOPCUAConfiguration");
    ASSERT_TRUE(opcuaDeviceConfig .hasProperty("Port"));
}

TEST_F(ModulesDefaultConfigTest, OPCUAConfigDeviceConnect)
{
    const auto serverInstance = test_helpers::createInstance("[[none]]");
    addOpcuaServerModule(serverInstance);

    const auto serverConfig = PropertyObject();
    serverConfig.addProperty(IntProperty("Port", 4841));
    test_helpers::addServer(serverInstance, "OpenDAQOPCUA", serverConfig);

    const auto instance = test_helpers::createInstance("[[none]]");
    addOpcuaClientModule(instance);

    const auto config = instance.createDefaultAddDeviceConfig();
    const PropertyObjectPtr deviceConfig = config.getPropertyValue("Device");
    const PropertyObjectPtr opcuaDeviceConfig = deviceConfig.getPropertyValue("OpenDAQOPCUAConfiguration");
    opcuaDeviceConfig .setPropertyValue("Port", test_helpers::testPort(4841));

    auto device = instance.addDevice("daq.opcua://127.0.0.1", config);
    ASSERT_TRUE(device.assigned());

    auto addedComponentConfig = device.asPtr<IComponentPrivate>(true).getComponentConfig();
    ASSERT_TRUE(addedComponentConfig.assigned());
    test_helpers::testPropObjsEquality(config, addedComponentConfig);
}

TEST_F(ModulesDefaultConfigTest, NativeConfigDeviceNativeStreamingConnect)
{
    const auto serverInstance = test_helpers::createInstance("[[none]]");
    addNativeServerModule(serverInstance);
    addLtServerModule(serverInstance);

    const auto serverConfig = PropertyObject();
    serverConfig.addProperty(IntProperty("NativeStreamingPort", 7415));
    test_helpers::addServer(serverInstance, "OpenDAQNativeStreaming", serverConfig);
    test_helpers::addServer(serverInstance, "OpenDAQLTStreaming", nullptr);

    const auto instance = test_helpers::createInstance("[[none]]");
    addNativeClientModule(instance);

    const auto config = instance.createDefaultAddDeviceConfig();

    const PropertyObjectPtr deviceConfig = config.getPropertyValue("Device");
    const PropertyObjectPtr nativeDeviceConfig = deviceConfig.getPropertyValue("OpenDAQNativeConfiguration");
    nativeDeviceConfig.setPropertyValue("Port", test_helpers::testPort(7415));

    const PropertyObjectPtr streamingConfig = config.getPropertyValue("Streaming");
    const PropertyObjectPtr nativeStreamingConfig = streamingConfig.getPropertyValue("OpenDAQNativeStreaming");
    nativeStreamingConfig.setPropertyValue("Port", test_helpers::testPort(7415));
    
    const PropertyObjectPtr generalConfig = config.getPropertyValue("General");
    generalConfig.setPropertyValue("AllowedStreamingProtocols", List<IString>("OpenDAQNativeStreaming"));

    const auto device = instance.addDevice("daq.nd://127.0.0.1", config);
    ASSERT_EQ(device.asPtr<IMirroredDevice>().getStreamingSources().getCount(), 1u);

    auto addedComponentConfig = device.asPtr<IComponentPrivate>(true).getComponentConfig();
    ASSERT_TRUE(addedComponentConfig.assigned());
    test_helpers::testPropObjsEquality(config, addedComponentConfig);
}

TEST_F(ModulesDefaultConfigTest, NativeConfigDeviceAnyStreamingConnect)
{
    const auto serverInstance = test_helpers::createInstance("[[none]]");

    const auto serverConfig = PropertyObject();
    serverConfig.addProperty(IntProperty("NativeStreamingPort", 7415));

    addNativeServerModule(serverInstance);
    test_helpers::addServer(serverInstance, "OpenDAQNativeStreaming", serverConfig);

    addLtServerModule(serverInstance);
    test_helpers::addServer(serverInstance, "OpenDAQLTStreaming", nullptr);

    const auto instance = test_helpers::createInstance("[[none]]");
    addNativeClientModule(instance);
    addLtClientModule(instance);

    const auto config = instance.createDefaultAddDeviceConfig();
    const PropertyObjectPtr deviceConfig = config.getPropertyValue("Device");
    const PropertyObjectPtr nativeDeviceConfig = deviceConfig.getPropertyValue("OpenDAQNativeConfiguration");
    nativeDeviceConfig.setPropertyValue("Port", test_helpers::testPort(7415));

    const PropertyObjectPtr streamingConfig = config.getPropertyValue("Streaming");
    const PropertyObjectPtr nativeStreamingConfig = streamingConfig.getPropertyValue("OpenDAQNativeStreaming");
    nativeStreamingConfig.setPropertyValue("Port", test_helpers::testPort(7415));
    
    const PropertyObjectPtr generalConfig = config.getPropertyValue("General");
    generalConfig.setPropertyValue("AllowedStreamingProtocols", List<IString>());

    const auto device = instance.addDevice("daq.nd://127.0.0.1", config);

    auto availableStreamingSources = device.asPtr<IMirroredDevice>().getStreamingSources();
    ASSERT_EQ(availableStreamingSources.getCount(), 2u);

    auto addedComponentConfig = device.asPtr<IComponentPrivate>(true).getComponentConfig();
    ASSERT_TRUE(addedComponentConfig.assigned());
    test_helpers::testPropObjsEquality(config, addedComponentConfig);
}

TEST_F(ModulesDefaultConfigTest, NativeConfigDeviceNoStreamingConnect)
{
    const auto serverInstance = test_helpers::createInstance("[[none]]");
    addNativeServerModule(serverInstance);

    const auto serverConfig = PropertyObject();
    serverConfig.addProperty(IntProperty("NativeStreamingPort", 7415));
    test_helpers::addServer(serverInstance, "OpenDAQNativeStreaming", serverConfig);

    const auto instance = test_helpers::createInstance("[[none]]");
    addNativeClientModule(instance);

    const auto config = instance.createDefaultAddDeviceConfig();
    const PropertyObjectPtr deviceConfig = config.getPropertyValue("Device");
    const PropertyObjectPtr nativeDeviceConfig = deviceConfig.getPropertyValue("OpenDAQNativeConfiguration");
    nativeDeviceConfig.setPropertyValue("Port", test_helpers::testPort(7415));

    const PropertyObjectPtr generalConfig = config.getPropertyValue("General");
    generalConfig.setPropertyValue("AutomaticallyConnectStreaming", false);

    const auto device = instance.addDevice("daq.nd://127.0.0.1", config);
    ASSERT_EQ(device.asPtr<IMirroredDevice>().getStreamingSources().getCount(), 0u);

    auto addedComponentConfig = device.asPtr<IComponentPrivate>(true).getComponentConfig();
    ASSERT_TRUE(addedComponentConfig.assigned());
    test_helpers::testPropObjsEquality(config, addedComponentConfig);
}

TEST_F(ModulesDefaultConfigTest, OPCUAConfigLTStreamingConnect)
{
    const auto serverInstance = test_helpers::createInstance("[[none]]");

    addLtServerModule(serverInstance);
    const auto ltServerConfig = PropertyObject();
    ltServerConfig.addProperty(IntProperty("WebsocketStreamingPort", 7415));
    test_helpers::addServer(serverInstance, "OpenDAQLTStreaming", ltServerConfig);

    addNativeServerModule(serverInstance);
    test_helpers::addServer(serverInstance, "OpenDAQNativeStreaming", nullptr);

    addOpcuaServerModule(serverInstance);
    const auto opcuaServerConfig = PropertyObject();
    opcuaServerConfig.addProperty(IntProperty("Port", 4841));
    test_helpers::addServer(serverInstance, "OpenDAQOPCUA", opcuaServerConfig);

    const auto instance = test_helpers::createInstance("[[none]]");
    addOpcuaClientModule(instance);
    addLtClientModule(instance);

    const auto config = instance.createDefaultAddDeviceConfig();

    const PropertyObjectPtr deviceConfig = config.getPropertyValue("Device");
    const PropertyObjectPtr opcuaDeviceConfig = deviceConfig.getPropertyValue("OpenDAQOPCUAConfiguration");
    opcuaDeviceConfig.setPropertyValue("Port", test_helpers::testPort(4841));

    const PropertyObjectPtr streamingConfig = config.getPropertyValue("Streaming");
    const PropertyObjectPtr ltStreamingConfig = streamingConfig.getPropertyValue("OpenDAQLTStreaming");
    ltStreamingConfig.setPropertyValue("Port", test_helpers::testPort(7415));
    
    const PropertyObjectPtr generalConfig = config.getPropertyValue("General");
    generalConfig.setPropertyValue("AllowedStreamingProtocols", List<IString>("OpenDAQLTStreaming"));

    const auto device = instance.addDevice("daq.opcua://127.0.0.1", config);
    ASSERT_EQ(device.asPtr<IMirroredDevice>().getStreamingSources().getCount(), 1u);

    auto addedComponentConfig = device.asPtr<IComponentPrivate>(true).getComponentConfig();
    ASSERT_TRUE(addedComponentConfig.assigned());
    test_helpers::testPropObjsEquality(config, addedComponentConfig);
}

TEST_F(ModulesDefaultConfigTest, OPCUAConfigAnyStreamingConnect)
{
    const auto serverInstance = test_helpers::createInstance("[[none]]");
    addLtServerModule(serverInstance);
    addNativeServerModule(serverInstance);
    addOpcuaServerModule(serverInstance);

    const auto ltServerConfig = PropertyObject();
    ltServerConfig.addProperty(IntProperty("WebsocketStreamingPort", 7415));
    test_helpers::addServer(serverInstance, "OpenDAQLTStreaming", ltServerConfig);
    
    const auto nativeServerConfig = PropertyObject();
    nativeServerConfig.addProperty(IntProperty("NativeStreamingPort", 7416));
    test_helpers::addServer(serverInstance, "OpenDAQNativeStreaming", nativeServerConfig);

    const auto opcuaServerConfig = PropertyObject();
    opcuaServerConfig.addProperty(IntProperty("Port", 4841));
    test_helpers::addServer(serverInstance, "OpenDAQOPCUA", opcuaServerConfig);

    const auto instance = test_helpers::createInstance("[[none]]");
    addOpcuaClientModule(instance);
    addLtClientModule(instance);
    addNativeClientModule(instance);

    const auto config = instance.createDefaultAddDeviceConfig();

    const PropertyObjectPtr deviceConfig = config.getPropertyValue("Device");
    const PropertyObjectPtr nativeDeviceConfig = deviceConfig.getPropertyValue("OpenDAQOPCUAConfiguration");
    nativeDeviceConfig.setPropertyValue("Port", test_helpers::testPort(4841));
    
    const PropertyObjectPtr streamingConfig = config.getPropertyValue("Streaming");
    const PropertyObjectPtr ltStreamingConfig = streamingConfig.getPropertyValue("OpenDAQLTStreaming");
    ltStreamingConfig.setPropertyValue("Port", test_helpers::testPort(7415));

    const PropertyObjectPtr nativeStreamingConfig = streamingConfig.getPropertyValue("OpenDAQNativeStreaming");
    nativeStreamingConfig.setPropertyValue("Port", test_helpers::testPort(7416));

    const PropertyObjectPtr generalConfig = config.getPropertyValue("General");
    generalConfig.setPropertyValue("AllowedStreamingProtocols", List<IString>());

    const auto device = instance.addDevice("daq.opcua://127.0.0.1", config);
    ASSERT_EQ(device.asPtr<IMirroredDevice>().getStreamingSources().getCount(), 2u);

    auto addedComponentConfig = device.asPtr<IComponentPrivate>(true).getComponentConfig();
    ASSERT_TRUE(addedComponentConfig.assigned());
    test_helpers::testPropObjsEquality(config, addedComponentConfig);
}

TEST_F(ModulesDefaultConfigTest, SmartConnectWithIpVerNative)
{
    const auto serverInstance = test_helpers::instanceBuilder()
                                    .setModulePath("[[none]]")
                                    .addDiscoveryServer("mdns")
                                    .build();

    PropertyObjectPtr refDevConfig = PropertyObject();
    refDevConfig.addProperty(StringProperty("Name", "Reference device simulator"));
    refDevConfig.addProperty(StringProperty("LocalId", "RefDevSimulator"));
    refDevConfig.addProperty(StringProperty("SerialNumber", String(test_helpers::testSerial("sim01_native"))));

    addRefDeviceModule(serverInstance);
    serverInstance.setRootDevice("daqref://device1", refDevConfig);

    addLtServerModule(serverInstance);
    test_helpers::addServer(serverInstance, "OpenDAQLTStreaming", nullptr);

    addNativeServerModule(serverInstance);
    test_helpers::addServer(serverInstance, "OpenDAQNativeStreaming", nullptr);

    for (const auto& server : serverInstance.getServers())
        server.enableDiscovery();

    const auto instance = test_helpers::createInstance("[[none]]");
    addNativeClientModule(instance);

    const auto config = instance.createDefaultAddDeviceConfig();
    const PropertyObjectPtr generalConfig = config.getPropertyValue("General");

    generalConfig.setPropertyValue("PrimaryAddressType", "IPv4");
    {
        const auto device = instance.addDevice(test_helpers::smartConnectionString("openDAQ", "sim01_native"), config);
        auto devConnStr = device.getInfo().getConfigurationConnectionInfo().getConnectionString();
        EXPECT_TRUE(test_helpers::isIpv4ConnectionString(devConnStr)) << devConnStr;
        devConnStr = device.getInfo().getConnectionString();
        EXPECT_TRUE(test_helpers::isIpv4ConnectionString(devConnStr)) << devConnStr;
        for (const auto& streamingSource : device.asPtr<IMirroredDevice>().getStreamingSources())
        {
            EXPECT_TRUE(test_helpers::isIpv4ConnectionString(streamingSource.getConnectionString()))
                << streamingSource.getConnectionString();
        }
        instance.removeDevice(device);
    }

     if (test_helpers::Ipv6IsDisabled(true))
     {
         GTEST_SKIP() << "Ipv6 is disabled - skip rest of the test";
     }

    generalConfig.setPropertyValue("PrimaryAddressType", "IPv6");
    {
        const auto device = instance.addDevice(test_helpers::smartConnectionString("openDAQ", "sim01_native"), config);
        auto devConnStr = device.getInfo().getConfigurationConnectionInfo().getConnectionString();
        EXPECT_TRUE(test_helpers::isIpv6ConnectionString(devConnStr)) << devConnStr;
        devConnStr = device.getInfo().getConnectionString();
        EXPECT_TRUE(test_helpers::isIpv6ConnectionString(devConnStr)) << devConnStr;
        for (const auto& streamingSource : device.asPtr<IMirroredDevice>().getStreamingSources())
        {
            EXPECT_TRUE(test_helpers::isIpv6ConnectionString(streamingSource.getConnectionString()))
                << streamingSource.getConnectionString();
        }
        instance.removeDevice(device);
    }
}

TEST_F(ModulesDefaultConfigTest, SmartConnectWithIpVerOpcUa)
{
    const auto serverInstance = test_helpers::instanceBuilder()
                                    .setModulePath("[[none]]")
                                    .addDiscoveryServer("mdns")
                                    .build();

    PropertyObjectPtr refDevConfig = PropertyObject();
    refDevConfig.addProperty(StringProperty("Name", "Reference device simulator"));
    refDevConfig.addProperty(StringProperty("LocalId", "RefDevSimulator"));
    refDevConfig.addProperty(StringProperty("SerialNumber", String(test_helpers::testSerial("sim01_opcua"))));

    addRefDeviceModule(serverInstance);
    serverInstance.setRootDevice("daqref://device1", refDevConfig);

    addLtServerModule(serverInstance);
    test_helpers::addServer(serverInstance, "OpenDAQLTStreaming", nullptr);

    addOpcuaServerModule(serverInstance);
    test_helpers::addServer(serverInstance, "OpenDAQOPCUA", nullptr);

    for (const auto& server : serverInstance.getServers())
        server.enableDiscovery();

    const auto instance = test_helpers::createInstance("[[none]]");
    addOpcuaClientModule(instance);

    const auto config = instance.createDefaultAddDeviceConfig();
    const PropertyObjectPtr generalConfig = config.getPropertyValue("General");

    generalConfig.setPropertyValue("PrimaryAddressType", "IPv4");
    {
        const auto device = instance.addDevice(test_helpers::smartConnectionString("openDAQ", "sim01_opcua"), config);
        auto devConnStr = device.getInfo().getConfigurationConnectionInfo().getConnectionString();
        EXPECT_TRUE(test_helpers::isIpv4ConnectionString(devConnStr)) << devConnStr;
        devConnStr = device.getInfo().getConnectionString();
        // FIXME connection string is not set within DeviceInfo for OpcUa devices
        // EXPECT_TRUE(test_helpers::isIpv4ConnectionString(devConnStr)) << devConnStr;
        for (const auto& streamingSource : device.asPtr<IMirroredDevice>().getStreamingSources())
        {
            EXPECT_TRUE(test_helpers::isIpv4ConnectionString(streamingSource.getConnectionString()))
                << streamingSource.getConnectionString();
        }
        instance.removeDevice(device);
    }

     if (test_helpers::Ipv6IsDisabled(true))
     {
         GTEST_SKIP() << "Ipv6 is disabled - skip rest of the test";
     }

    generalConfig.setPropertyValue("PrimaryAddressType", "IPv6");
    {
        const auto device = instance.addDevice(test_helpers::smartConnectionString("openDAQ", "sim01_opcua"), config);
        auto devConnStr = device.getInfo().getConfigurationConnectionInfo().getConnectionString();
        EXPECT_TRUE(test_helpers::isIpv6ConnectionString(devConnStr)) << devConnStr;
        devConnStr = device.getInfo().getConnectionString();
        // FIXME connection string is not set within DeviceInfo for OpcUa devices
        // EXPECT_TRUE(test_helpers::isIpv6ConnectionString(devConnStr)) << devConnStr;
        for (const auto& streamingSource : device.asPtr<IMirroredDevice>().getStreamingSources())
        {
            EXPECT_TRUE(test_helpers::isIpv6ConnectionString(streamingSource.getConnectionString()))
                << streamingSource.getConnectionString();
        }
        instance.removeDevice(device);
    }
}

TEST_F(ModulesDefaultConfigTest, SmartConnectWithIpVerLt)
{
    const auto serverInstance = test_helpers::instanceBuilder()
                                    .setModulePath("[[none]]")
                                    .addDiscoveryServer("mdns")
                                    .build();

    PropertyObjectPtr refDevConfig = PropertyObject();
    refDevConfig.addProperty(StringProperty("Name", "Reference device simulator"));
    refDevConfig.addProperty(StringProperty("LocalId", "RefDevSimulator"));
    refDevConfig.addProperty(StringProperty("SerialNumber", String(test_helpers::testSerial("sim01_lt"))));

    addRefDeviceModule(serverInstance);
    serverInstance.setRootDevice("daqref://device1", refDevConfig);

    addLtServerModule(serverInstance);
    test_helpers::addServer(serverInstance, "OpenDAQLTStreaming", nullptr);

    for (const auto& server : serverInstance.getServers())
        server.enableDiscovery();

    const auto instance = test_helpers::createInstance("[[none]]");
    addLtClientModule(instance);

    const auto config = instance.createDefaultAddDeviceConfig();
    const PropertyObjectPtr generalConfig = config.getPropertyValue("General");

    generalConfig.setPropertyValue("PrimaryAddressType", "IPv4");
    {
        const auto device = instance.addDevice(test_helpers::smartConnectionString("openDAQ", "sim01_lt"), config);
        auto devConnStr = device.getInfo().getConfigurationConnectionInfo().getConnectionString();
        EXPECT_TRUE(test_helpers::isIpv4ConnectionString(devConnStr)) << devConnStr;
        devConnStr = device.getInfo().getConnectionString();
        EXPECT_TRUE(test_helpers::isIpv4ConnectionString(devConnStr)) << devConnStr;
        instance.removeDevice(device);
    }

     if (test_helpers::Ipv6IsDisabled(true))
     {
         GTEST_SKIP() << "Ipv6 is disabled - skip rest of the test";
     }

    generalConfig.setPropertyValue("PrimaryAddressType", "IPv6");
    {
        const auto device = instance.addDevice(test_helpers::smartConnectionString("openDAQ", "sim01_lt"), config);
        auto devConnStr = device.getInfo().getConfigurationConnectionInfo().getConnectionString();
        EXPECT_TRUE(test_helpers::isIpv6ConnectionString(devConnStr)) << devConnStr;
        devConnStr = device.getInfo().getConnectionString();
        EXPECT_TRUE(test_helpers::isIpv6ConnectionString(devConnStr)) << devConnStr;
        instance.removeDevice(device);
    }
}
