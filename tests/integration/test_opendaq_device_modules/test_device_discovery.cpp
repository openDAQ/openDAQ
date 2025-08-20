#include "opendaq/mock/mock_device_module.h"
#include "test_helpers/test_helpers.h"

using ModulesDeviceDiscoveryTest = testing::Test;

using namespace daq;

TEST_F(ModulesDeviceDiscoveryTest, ChangeIpConfig)
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
    mockDeviceConfig.setPropertyValue("netConfigEnabled", True);
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

TEST_F(ModulesDeviceDiscoveryTest, ChangeIpConfigError)
{
    const auto serverInstance = InstanceBuilder().addDiscoveryServer("mdns").build();
    const ModulePtr deviceModule(MockDeviceModule_Create(serverInstance.getContext()));
    serverInstance.getModuleManager().addModule(deviceModule);
    auto deviceTypes = serverInstance.getAvailableDeviceTypes();
    auto mockDeviceConfig = deviceTypes.get("mock_phys_device").createDefaultConfig();
    mockDeviceConfig.setPropertyValue("netConfigEnabled", True);
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

TEST_F(ModulesDeviceDiscoveryTest, RetrieveIpConfig)
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
    mockDeviceConfig.setPropertyValue("netConfigEnabled", True);
    mockDeviceConfig.setPropertyValue("ifaceNames", List<IString>("eth0", "eth1"));
    mockDeviceConfig.setPropertyValue("onSubmitConfig", Procedure([](StringPtr, PropertyObjectPtr) {}));
    mockDeviceConfig.setPropertyValue("onRetrieveConfig", retrieveIpConfigCallback);
    serverInstance.setRootDevice("daqmock://phys_device", mockDeviceConfig);
    serverInstance.addServer("OpenDAQNativeStreaming", nullptr).enableDiscovery();

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

class ConnectedClientsDiscoveryTest : public ModulesDeviceDiscoveryTest
{
public:
    void SetUp() override
    {
        serverInstance = InstanceBuilder().addDiscoveryServer("mdns").build();
        const ModulePtr deviceModule(MockDeviceModule_Create(serverInstance.getContext()));
        serverInstance.getModuleManager().addModule(deviceModule);
        serverInstance.setRootDevice("daqmock://phys_device");

        clientInstance = Instance();
    }

protected:
    ListPtr<IConnectedClientInfo> getConnectedClients()
    {
        ListPtr<IConnectedClientInfo> connectedClients = List<IConnectedClientInfo>();
        for (const auto& deviceInfo : clientInstance.getAvailableDevices())
            if (deviceInfo.getConnectionString() == "daq://manufacturer_serial_number")
                connectedClients = deviceInfo.getConnectedClientsInfo();
        return connectedClients;
    }

    InstancePtr serverInstance;
    InstancePtr clientInstance;
};

TEST_F(ConnectedClientsDiscoveryTest, NativeConnectedClients)
{
    serverInstance.addServer("OpenDAQNativeStreaming", nullptr).enableDiscovery();

    ASSERT_EQ(getConnectedClients().getCount(), 0u);
    {
        // native streaming client
        auto device = clientInstance.addDevice("daq.ns://127.0.0.1");
        auto connectedClientsInfo = getConnectedClients();
        ASSERT_EQ(connectedClientsInfo.getCount(), 1u);

        ASSERT_EQ(connectedClientsInfo[0].getProtocolType(), ProtocolType::Streaming);
        ASSERT_EQ(connectedClientsInfo[0].getProtocolName(), "OpenDAQNativeStreaming");
        ASSERT_EQ(connectedClientsInfo[0].getClientTypeName(), "");
        ASSERT_TRUE(connectedClientsInfo[0].getAddress().toStdString().find("127.0.0.1") != std::string::npos);

        clientInstance.removeDevice(device);
        ASSERT_EQ(getConnectedClients().getCount(), 0u);
    }
    {
        // native configuration & streaming client
        auto device = clientInstance.addDevice("daq.nd://127.0.0.1");
        auto connectedClientsInfo = getConnectedClients();
        ASSERT_EQ(connectedClientsInfo.getCount(), 2u);

        ASSERT_EQ(connectedClientsInfo[0].getProtocolType(), ProtocolType::Configuration);
        ASSERT_EQ(connectedClientsInfo[0].getProtocolName(), "OpenDAQNativeConfiguration");
        ASSERT_EQ(connectedClientsInfo[0].getClientTypeName(), "Control");
        ASSERT_TRUE(connectedClientsInfo[0].getAddress().toStdString().find("127.0.0.1") != std::string::npos);

        ASSERT_EQ(connectedClientsInfo[1].getProtocolType(), ProtocolType::Streaming);
        ASSERT_EQ(connectedClientsInfo[1].getProtocolName(), "OpenDAQNativeStreaming");
        ASSERT_EQ(connectedClientsInfo[1].getClientTypeName(), "");
        ASSERT_TRUE(connectedClientsInfo[1].getAddress().toStdString().find("127.0.0.1") != std::string::npos);

        ASSERT_EQ(connectedClientsInfo[0].getHostName(), connectedClientsInfo[1].getHostName());

        clientInstance.removeDevice(device);
        ASSERT_EQ(getConnectedClients().getCount(), 0u);
    }
    {
        // native configuration exclusive control client
        clientInstance = test_helpers::connectInstanceWithClientType("daq.nd://127.0.0.1", ClientType::ExclusiveControl);
        auto device = clientInstance.getDevices()[0];
        auto connectedClientsInfo = getConnectedClients();
        ASSERT_EQ(connectedClientsInfo.getCount(), 2u);

        ASSERT_EQ(connectedClientsInfo[0].getProtocolType(), ProtocolType::Configuration);
        ASSERT_EQ(connectedClientsInfo[0].getProtocolName(), "OpenDAQNativeConfiguration");
        ASSERT_EQ(connectedClientsInfo[0].getClientTypeName(), "ExclusiveControl");
        ASSERT_TRUE(connectedClientsInfo[0].getAddress().toStdString().find("127.0.0.1") != std::string::npos);
    }
}

TEST_F(ConnectedClientsDiscoveryTest, LtConnectedClients)
{
    serverInstance.addServer("OpenDAQLTStreaming", nullptr).enableDiscovery();

    ASSERT_EQ(getConnectedClients().getCount(), 0u);
    {
        auto device = clientInstance.addDevice("daq.lt://127.0.0.1");
        auto connectedClientsInfo = getConnectedClients();
        ASSERT_EQ(connectedClientsInfo.getCount(), 1u);

        ASSERT_EQ(connectedClientsInfo[0].getProtocolType(), ProtocolType::Streaming);
        ASSERT_EQ(connectedClientsInfo[0].getProtocolName(), "OpenDAQLTStreaming");
        ASSERT_EQ(connectedClientsInfo[0].getClientTypeName(), "");
        ASSERT_TRUE(connectedClientsInfo[0].getAddress().toStdString().find("127.0.0.1") != std::string::npos);

        clientInstance.removeDevice(device);
        ASSERT_EQ(getConnectedClients().getCount(), 0u);
    }
}

TEST_F(ConnectedClientsDiscoveryTest, OpcuaConnectedClients)
{
    serverInstance.addServer("OpenDAQOPCUA", nullptr).enableDiscovery();

    ASSERT_EQ(getConnectedClients().getCount(), 0u);
    {
        auto device = clientInstance.addDevice("daq.opcua://127.0.0.1");
        auto connectedClientsInfo = getConnectedClients();
        ASSERT_EQ(connectedClientsInfo.getCount(), 1u);

        ASSERT_EQ(connectedClientsInfo[0].getProtocolType(), ProtocolType::Configuration);
        ASSERT_EQ(connectedClientsInfo[0].getProtocolName(), "OpenDAQOPCUA");

        clientInstance.removeDevice(device);
        ASSERT_EQ(getConnectedClients().getCount(), 0u);
    }
}
