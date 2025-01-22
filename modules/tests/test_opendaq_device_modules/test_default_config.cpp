#include "opendaq/mock/mock_device_module.h"
#include "test_helpers/test_helpers.h"
#include <opendaq/module_manager_utils_ptr.h>

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
    ASSERT_TRUE(general.hasProperty("PrimaryAddressType"));
}

TEST_F(ModulesDefaultConfigTest, NativeConfigDevice)
{
    const auto instance = Instance();
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
    const auto serverInstance = Instance();
    const auto serverConfig = PropertyObject();
    serverConfig.addProperty(IntProperty("NativeStreamingPort", 7421));
    serverInstance.addServer("OpenDAQNativeStreaming", serverConfig);

    const auto instance = Instance();
    const auto config = instance.createDefaultAddDeviceConfig();
    const PropertyObjectPtr deviceConfig = config.getPropertyValue("Device");
    const PropertyObjectPtr nativeDeviceConfig = deviceConfig.getPropertyValue("OpenDAQNativeConfiguration");
    nativeDeviceConfig.setPropertyValue("Port", 7421);

    ASSERT_TRUE(instance.addDevice("daq.nd://127.0.0.1", config).assigned());
}

TEST_F(ModulesDefaultConfigTest, NativeStreaming)
{
    const auto instance = Instance();
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
    const auto instance = Instance();
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
    const auto serverInstance = Instance();
    const auto serverConfig = PropertyObject();
    serverConfig.addProperty(IntProperty("NativeStreamingPort", 7415));
    serverInstance.addServer("OpenDAQNativeStreaming", serverConfig);

    const auto instance = Instance();
    const auto config = instance.createDefaultAddDeviceConfig();
    const PropertyObjectPtr deviceConfig = config.getPropertyValue("Device");
    const PropertyObjectPtr nativeDeviceConfig = deviceConfig.getPropertyValue("OpenDAQNativeConfiguration");
    nativeDeviceConfig.setPropertyValue("Port", 7415);

    ASSERT_TRUE(instance.addDevice("daq.nd://127.0.0.1", config).assigned());
}

TEST_F(ModulesDefaultConfigTest, LTStreamingDevice)
{
    const auto instance = Instance();
    const auto config = instance.createDefaultAddDeviceConfig();
    const PropertyObjectPtr deviceConfig = config.getPropertyValue("Device");
    const PropertyObjectPtr ltDeviceConfig = deviceConfig.getPropertyValue("OpenDAQLTStreaming");
    ASSERT_TRUE(ltDeviceConfig.hasProperty("Port"));
}

TEST_F(ModulesDefaultConfigTest, LTStreamingDeviceConnect)
{
    const auto serverInstance = Instance();
    const auto serverConfig = PropertyObject();
    serverConfig.addProperty(IntProperty("WebsocketStreamingPort", 7415));
    serverInstance.addServer("OpenDAQLTStreaming", serverConfig);

    const auto instance = Instance();
    const auto config = instance.createDefaultAddDeviceConfig();
    const PropertyObjectPtr deviceConfig = config.getPropertyValue("Device");
    const PropertyObjectPtr ltDeviceConfig = deviceConfig.getPropertyValue("OpenDAQLTStreaming");
    ltDeviceConfig.setPropertyValue("Port", 7415);

    ASSERT_TRUE(instance.addDevice("daq.lt://127.0.0.1", config).assigned());
}

TEST_F(ModulesDefaultConfigTest, OPCUAConfigDevice)
{
    const auto instance = Instance();
    const auto config = instance.createDefaultAddDeviceConfig();
    const PropertyObjectPtr deviceConfig = config.getPropertyValue("Device");
    const PropertyObjectPtr opcuaDeviceConfig  = deviceConfig.getPropertyValue("OpenDAQOPCUAConfiguration");
    ASSERT_TRUE(opcuaDeviceConfig .hasProperty("Port"));
}

TEST_F(ModulesDefaultConfigTest, OPCUAConfigDeviceConnect)
{
    const auto serverInstance = Instance();
    const auto serverConfig = PropertyObject();
    serverConfig.addProperty(IntProperty("Port", 4841));
    serverInstance.addServer("OpenDAQOPCUA", serverConfig);

    const auto instance = Instance();
    const auto config = instance.createDefaultAddDeviceConfig();
    const PropertyObjectPtr deviceConfig = config.getPropertyValue("Device");
    const PropertyObjectPtr opcuaDeviceConfig = deviceConfig.getPropertyValue("OpenDAQOPCUAConfiguration");
    opcuaDeviceConfig .setPropertyValue("Port", 4841);

    ASSERT_TRUE(instance.addDevice("daq.opcua://127.0.0.1", config).assigned());
}

TEST_F(ModulesDefaultConfigTest, NativeConfigDeviceNativeStreamingConnect)
{
    const auto serverInstance = Instance();
    const auto serverConfig = PropertyObject();
    serverConfig.addProperty(IntProperty("NativeStreamingPort", 7415));
    serverInstance.addServer("OpenDAQNativeStreaming", serverConfig);
    serverInstance.addServer("OpenDAQLTStreaming", nullptr);

    const auto instance = Instance();
    const auto config = instance.createDefaultAddDeviceConfig();
    const PropertyObjectPtr deviceConfig = config.getPropertyValue("Device");
    const PropertyObjectPtr nativeDeviceConfig = deviceConfig.getPropertyValue("OpenDAQNativeConfiguration");
    nativeDeviceConfig.setPropertyValue("Port", 7415);

    const PropertyObjectPtr streamingConfig = config.getPropertyValue("Streaming");
    const PropertyObjectPtr nativeStreamingConfig = streamingConfig.getPropertyValue("OpenDAQNativeStreaming");
    nativeStreamingConfig.setPropertyValue("Port", 7415);
    
    const PropertyObjectPtr generalConfig = config.getPropertyValue("General");
    generalConfig.setPropertyValue("AllowedStreamingProtocols", List<IString>("OpenDAQNativeStreaming"));

    const auto device = instance.addDevice("daq.nd://127.0.0.1", config);
    ASSERT_EQ(device.asPtr<IMirroredDevice>().getStreamingSources().getCount(), 1u);
}

TEST_F(ModulesDefaultConfigTest, NativeConfigDeviceAnyStreamingConnect)
{
    const auto serverInstance = Instance();
    const auto serverConfig = PropertyObject();
    serverConfig.addProperty(IntProperty("NativeStreamingPort", 7415));
    serverInstance.addServer("OpenDAQNativeStreaming", serverConfig);
    serverInstance.addServer("OpenDAQLTStreaming", nullptr);

    const auto instance = Instance();
    const auto config = instance.createDefaultAddDeviceConfig();
    const PropertyObjectPtr deviceConfig = config.getPropertyValue("Device");
    const PropertyObjectPtr nativeDeviceConfig = deviceConfig.getPropertyValue("OpenDAQNativeConfiguration");
    nativeDeviceConfig.setPropertyValue("Port", 7415);

    const PropertyObjectPtr streamingConfig = config.getPropertyValue("Streaming");
    const PropertyObjectPtr nativeStreamingConfig = streamingConfig.getPropertyValue("OpenDAQNativeStreaming");
    nativeStreamingConfig.setPropertyValue("Port", 7415);
    
    const PropertyObjectPtr generalConfig = config.getPropertyValue("General");
    generalConfig.setPropertyValue("AllowedStreamingProtocols", List<IString>());

    const auto device = instance.addDevice("daq.nd://127.0.0.1", config);
    ASSERT_EQ(device.asPtr<IMirroredDevice>().getStreamingSources().getCount(), 2u);
}

TEST_F(ModulesDefaultConfigTest, NativeConfigDeviceNoStreamingConnect)
{
    const auto serverInstance = Instance();
    const auto serverConfig = PropertyObject();
    serverConfig.addProperty(IntProperty("NativeStreamingPort", 7415));
    serverInstance.addServer("OpenDAQNativeStreaming", serverConfig);

    const auto instance = Instance();
    const auto config = instance.createDefaultAddDeviceConfig();
    const PropertyObjectPtr deviceConfig = config.getPropertyValue("Device");
    const PropertyObjectPtr nativeDeviceConfig = deviceConfig.getPropertyValue("OpenDAQNativeConfiguration");
    nativeDeviceConfig.setPropertyValue("Port", 7415);

    const PropertyObjectPtr generalConfig = config.getPropertyValue("General");
    generalConfig.setPropertyValue("AutomaticallyConnectStreaming", false);

    const auto device = instance.addDevice("daq.nd://127.0.0.1", config);
    ASSERT_EQ(device.asPtr<IMirroredDevice>().getStreamingSources().getCount(), 0u);
}

TEST_F(ModulesDefaultConfigTest, OPCUAConfigLTStreamingConnect)
{
    const auto serverInstance = Instance();

    const auto ltServerConfig = PropertyObject();
    ltServerConfig.addProperty(IntProperty("WebsocketStreamingPort", 7415));
    serverInstance.addServer("OpenDAQLTStreaming", ltServerConfig);

    serverInstance.addServer("OpenDAQNativeStreaming", nullptr);

    const auto opcuaServerConfig = PropertyObject();
    opcuaServerConfig.addProperty(IntProperty("Port", 4841));
    serverInstance.addServer("OpenDAQOPCUA", opcuaServerConfig);

    const auto instance = Instance();
    const auto config = instance.createDefaultAddDeviceConfig();
    const PropertyObjectPtr deviceConfig = config.getPropertyValue("Device");
    const PropertyObjectPtr opcuaDeviceConfig = deviceConfig.getPropertyValue("OpenDAQOPCUAConfiguration");
    opcuaDeviceConfig.setPropertyValue("Port", 4841);

    const PropertyObjectPtr streamingConfig = config.getPropertyValue("Streaming");
    const PropertyObjectPtr ltStreamingConfig = streamingConfig.getPropertyValue("OpenDAQLTStreaming");
    ltStreamingConfig.setPropertyValue("Port", 7415);
    
    const PropertyObjectPtr generalConfig = config.getPropertyValue("General");
    generalConfig.setPropertyValue("AllowedStreamingProtocols", List<IString>("OpenDAQLTStreaming"));

    const auto device = instance.addDevice("daq.opcua://127.0.0.1", config);
    ASSERT_EQ(device.asPtr<IMirroredDevice>().getStreamingSources().getCount(), 1u);
}

TEST_F(ModulesDefaultConfigTest, OPCUAConfigAnyStreamingConnect)
{
    const auto serverInstance = Instance();

    const auto ltServerConfig = PropertyObject();
    ltServerConfig.addProperty(IntProperty("WebsocketStreamingPort", 7415));
    serverInstance.addServer("OpenDAQLTStreaming", ltServerConfig);
    
    const auto nativeServerConfig = PropertyObject();
    nativeServerConfig.addProperty(IntProperty("NativeStreamingPort", 7416));
    serverInstance.addServer("OpenDAQNativeStreaming", nativeServerConfig);

    const auto opcuaServerConfig = PropertyObject();
    opcuaServerConfig.addProperty(IntProperty("Port", 4841));
    serverInstance.addServer("OpenDAQOPCUA", opcuaServerConfig);

    const auto instance = Instance();
    const auto config = instance.createDefaultAddDeviceConfig();
    const PropertyObjectPtr deviceConfig = config.getPropertyValue("Device");
    const PropertyObjectPtr nativeDeviceConfig = deviceConfig.getPropertyValue("OpenDAQOPCUAConfiguration");
    nativeDeviceConfig.setPropertyValue("Port", 4841);
    
    const PropertyObjectPtr streamingConfig = config.getPropertyValue("Streaming");
    const PropertyObjectPtr ltStreamingConfig = streamingConfig.getPropertyValue("OpenDAQLTStreaming");
    ltStreamingConfig.setPropertyValue("Port", 7415);

    const PropertyObjectPtr nativeStreamingConfig = streamingConfig.getPropertyValue("OpenDAQNativeStreaming");
    nativeStreamingConfig.setPropertyValue("Port", 7416);

    const PropertyObjectPtr generalConfig = config.getPropertyValue("General");
    generalConfig.setPropertyValue("AllowedStreamingProtocols", List<IString>());

    const auto device = instance.addDevice("daq.opcua://127.0.0.1", config);
    ASSERT_EQ(device.asPtr<IMirroredDevice>().getStreamingSources().getCount(), 2u);
}

TEST_F(ModulesDefaultConfigTest, SmartConnectWithIpVerNative)
{
    PropertyObjectPtr refDevConfig = PropertyObject();
    refDevConfig.addProperty(StringProperty("Name", "Reference device simulator"));
    refDevConfig.addProperty(StringProperty("LocalId", "RefDevSimulator"));
    refDevConfig.addProperty(StringProperty("SerialNumber", "sim01"));

    const auto serverInstance = InstanceBuilder()
                                    .addDiscoveryServer("mdns")
                                    .setRootDevice("daqref://device1", refDevConfig)
                                    .build();

    serverInstance.addServer("OpenDAQLTStreaming", nullptr);
    serverInstance.addServer("OpenDAQNativeStreaming", nullptr);

    for (const auto& server : serverInstance.getServers())
        server.enableDiscovery();

    const auto instance = Instance();
    const auto config = instance.createDefaultAddDeviceConfig();
    const PropertyObjectPtr generalConfig = config.getPropertyValue("General");

    generalConfig.setPropertyValue("PrimaryAddressType", "IPv4");
    {
        const auto device = instance.addDevice("daq://openDAQ_sim01", config);
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
        const auto device = instance.addDevice("daq://openDAQ_sim01", config);
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
    PropertyObjectPtr refDevConfig = PropertyObject();
    refDevConfig.addProperty(StringProperty("Name", "Reference device simulator"));
    refDevConfig.addProperty(StringProperty("LocalId", "RefDevSimulator"));
    refDevConfig.addProperty(StringProperty("SerialNumber", "sim01"));

    const auto serverInstance = InstanceBuilder()
                                    .addDiscoveryServer("mdns")
                                    .setRootDevice("daqref://device1", refDevConfig)
                                    .build();

    serverInstance.addServer("OpenDAQLTStreaming", nullptr);
    serverInstance.addServer("OpenDAQOPCUA", nullptr);

    for (const auto& server : serverInstance.getServers())
        server.enableDiscovery();

    const auto instance = Instance();
    const auto config = instance.createDefaultAddDeviceConfig();
    const PropertyObjectPtr generalConfig = config.getPropertyValue("General");

    generalConfig.setPropertyValue("PrimaryAddressType", "IPv4");
    {
        const auto device = instance.addDevice("daq://openDAQ_sim01", config);
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
        const auto device = instance.addDevice("daq://openDAQ_sim01", config);
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
    PropertyObjectPtr refDevConfig = PropertyObject();
    refDevConfig.addProperty(StringProperty("Name", "Reference device simulator"));
    refDevConfig.addProperty(StringProperty("LocalId", "RefDevSimulator"));
    refDevConfig.addProperty(StringProperty("SerialNumber", "sim01"));

    const auto serverInstance = InstanceBuilder()
                                    .addDiscoveryServer("mdns")
                                    .setRootDevice("daqref://device1", refDevConfig)
                                    .build();

    serverInstance.addServer("OpenDAQLTStreaming", nullptr);

    for (const auto& server : serverInstance.getServers())
        server.enableDiscovery();

    const auto instance = Instance();
    const auto config = instance.createDefaultAddDeviceConfig();
    const PropertyObjectPtr generalConfig = config.getPropertyValue("General");

    generalConfig.setPropertyValue("PrimaryAddressType", "IPv4");
    {
        const auto device = instance.addDevice("daq://openDAQ_sim01", config);
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
        const auto device = instance.addDevice("daq://openDAQ_sim01", config);
        auto devConnStr = device.getInfo().getConfigurationConnectionInfo().getConnectionString();
        EXPECT_TRUE(test_helpers::isIpv6ConnectionString(devConnStr)) << devConnStr;
        devConnStr = device.getInfo().getConnectionString();
        EXPECT_TRUE(test_helpers::isIpv6ConnectionString(devConnStr)) << devConnStr;
        instance.removeDevice(device);
    }
}

TEST_F(ModulesDefaultConfigTest, ChangeIpConfig)
{
    const auto dhcp4 = False;
    const auto address4 = String("192.168.2.100/24");
    const auto gateway4 = String("192.168.2.1");
    const auto dhcp6 = True;
    const auto address6 = String("");
    const auto gateway6 = String("");

    SizeT modifyCallCount = 0;
    ProcedurePtr modifyIpConfigCallback = Procedure([&](StringPtr ifaceName, PropertyObjectPtr config)
    {
        ++modifyCallCount;
        EXPECT_EQ(ifaceName, "eth0");
        EXPECT_EQ(config.getPropertyValue("dhcp4"), dhcp4);
        EXPECT_EQ(config.getPropertyValue("address4"), address4);
        EXPECT_EQ(config.getPropertyValue("gateway4"), gateway4);
        EXPECT_EQ(config.getPropertyValue("dhcp6"), dhcp6);
        EXPECT_EQ(config.getPropertyValue("address6"), address6);
        EXPECT_EQ(config.getPropertyValue("gateway6"), gateway6);
    });

    const auto serverInstance = InstanceBuilder().addDiscoveryServer("mdns").build();
    const ModulePtr deviceModule(MockDeviceModule_Create(serverInstance.getContext()));
    serverInstance.getModuleManager().addModule(deviceModule);
    auto deviceTypes = serverInstance.getAvailableDeviceTypes();
    auto mockDeviceConfig = deviceTypes.get("mock_phys_device").createDefaultConfig();
    mockDeviceConfig.setPropertyValue("ifaceNames", List<IString>("eth0", "eth1"));
    mockDeviceConfig.setPropertyValue("onSubmitConfig", modifyIpConfigCallback);
    serverInstance.setRootDevice("daqmock://phys_device", mockDeviceConfig);

    serverInstance.addServer("OpenDAQNativeStreaming", nullptr);

    for (const auto& server : serverInstance.getServers())
        server.enableDiscovery();

    const auto instance = Instance();
    auto availableDevices = instance.getAvailableDevices();

    for (const auto& devInfo : availableDevices)
    {
        if (devInfo.getConnectionString() == "daq://manufacturer_serial_number")
        {
            EXPECT_TRUE(devInfo.getNetworkInterfaces().hasKey("eth0"));
            EXPECT_TRUE(devInfo.getNetworkInterfaces().hasKey("eth1"));
            auto ipConfig = devInfo.getNetworkInterface("eth0").createDefaultConfiguration();
            ipConfig.setPropertyValue("dhcp4", dhcp4);
            ipConfig.setPropertyValue("address4", address4);
            ipConfig.setPropertyValue("gateway4", gateway4);
            ipConfig.setPropertyValue("dhcp6", dhcp6);
            ipConfig.setPropertyValue("address6", address6);
            ipConfig.setPropertyValue("gateway6", gateway6);
            EXPECT_NO_THROW(devInfo.getNetworkInterface("eth0").submitConfiguration(ipConfig));
        }
    }

    EXPECT_EQ(modifyCallCount, 1u);
}

TEST_F(ModulesDefaultConfigTest, ChangeIpConfigError)
{
    const auto serverInstance = InstanceBuilder().addDiscoveryServer("mdns").build();
    const ModulePtr deviceModule(MockDeviceModule_Create(serverInstance.getContext()));
    serverInstance.getModuleManager().addModule(deviceModule);
    auto deviceTypes = serverInstance.getAvailableDeviceTypes();
    auto mockDeviceConfig = deviceTypes.get("mock_phys_device").createDefaultConfig();
    mockDeviceConfig.setPropertyValue("ifaceNames", List<IString>("eth0"));
    serverInstance.setRootDevice("daqmock://phys_device", mockDeviceConfig);

    serverInstance.addServer("OpenDAQNativeStreaming", nullptr);

    // Truncated error message with disallowed symbols replaced.
    auto retrievedErrorMessage = "This Is An Extremely Long Test String With Invalid Characters Like  Tabs, NewLines , "
                                 "and equals signs                                                                               "
                                 "?!.,:;-+*/|&^~_\\@#$%\"'`()<>[]             Truncated after this";

    for (const auto& server : serverInstance.getServers())
        server.enableDiscovery();

    const auto instance = Instance();
    auto availableDevices = instance.getAvailableDevices();

    for (const auto& devInfo : availableDevices)
    {
        if (devInfo.getConnectionString() == "daq://manufacturer_serial_number")
        {
            EXPECT_TRUE(devInfo.getNetworkInterfaces().hasKey("eth0"));
            EXPECT_FALSE(devInfo.getNetworkInterfaces().hasKey("eth1"));
            auto networInterface = devInfo.getNetworkInterface("eth0");
            auto ipConfig = devInfo.getNetworkInterface("eth0").createDefaultConfiguration();
            ASSERT_THROW_MSG(networInterface.submitConfiguration(ipConfig), NotImplementedException, retrievedErrorMessage);
            ASSERT_THROW_MSG(networInterface.requestCurrentConfiguration(), NotImplementedException, retrievedErrorMessage);
        }
    }
}

TEST_F(ModulesDefaultConfigTest, RetrieveIpConfig)
{
    const auto dhcp4 = False;
    const auto address4 = String("192.168.2.100/24");
    const auto gateway4 = String("192.168.2.1");
    const auto dhcp6 = False;
    const auto address6 = String("2001:db8:1:0::100/64");
    const auto gateway6 = String("2001:db8:1:0::1");

    SizeT retrieveCallCount = 0;
    FunctionPtr retrieveIpConfigCallback = Function([&](StringPtr ifaceName) -> PropertyObjectPtr
    {
        ++retrieveCallCount;
        EXPECT_EQ(ifaceName, "eth0");

        auto config = PropertyObject();
        config.addProperty(BoolProperty("dhcp4", dhcp4));
        config.addProperty(StringProperty("address4", address4));
        config.addProperty(StringProperty("gateway4", gateway4));
        config.addProperty(BoolProperty("dhcp6", dhcp6));
        config.addProperty(StringProperty("address6", address6));
        config.addProperty(StringProperty("gateway6", gateway6));

        return config;
    });

    const auto serverInstance = InstanceBuilder().addDiscoveryServer("mdns").build();
    const ModulePtr deviceModule(MockDeviceModule_Create(serverInstance.getContext()));
    serverInstance.getModuleManager().addModule(deviceModule);
    auto deviceTypes = serverInstance.getAvailableDeviceTypes();
    auto mockDeviceConfig = deviceTypes.get("mock_phys_device").createDefaultConfig();
    mockDeviceConfig.setPropertyValue("ifaceNames", List<IString>("eth0", "eth1"));
    mockDeviceConfig.setPropertyValue("onSubmitConfig", Procedure([](StringPtr, PropertyObjectPtr) {}));
    mockDeviceConfig.setPropertyValue("onRetrieveConfig", retrieveIpConfigCallback);
    serverInstance.setRootDevice("daqmock://phys_device", mockDeviceConfig);

    serverInstance.addServer("OpenDAQNativeStreaming", nullptr);

    for (const auto& server : serverInstance.getServers())
        server.enableDiscovery();

    const auto instance = Instance();
    auto availableDevices = instance.getAvailableDevices();

    for (const auto& devInfo : availableDevices)
    {
        if (devInfo.getConnectionString() == "daq://manufacturer_serial_number")
        {
            EXPECT_TRUE(devInfo.getNetworkInterfaces().hasKey("eth0"));
            EXPECT_TRUE(devInfo.getNetworkInterfaces().hasKey("eth1"));
            PropertyObjectPtr config;
            ASSERT_NO_THROW(config = devInfo.getNetworkInterface("eth0").requestCurrentConfiguration());
            EXPECT_EQ(config.getPropertyValue("dhcp4"), dhcp4);
            EXPECT_EQ(config.getPropertyValue("address4"), address4);
            EXPECT_EQ(config.getPropertyValue("gateway4"), gateway4);
            EXPECT_EQ(config.getPropertyValue("dhcp6"), dhcp6);
            EXPECT_EQ(config.getPropertyValue("address6"), address6);
            EXPECT_EQ(config.getPropertyValue("gateway6"), gateway6);
        }
    }

    EXPECT_EQ(retrieveCallCount, 1u);
}
