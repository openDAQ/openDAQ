#include <opendaq/component_exceptions.h>
#include <opendaq/exceptions.h>
#include "test_helpers/test_helpers.h"
#include <coreobjects/authentication_provider_factory.h>
#include "opendaq/mock/mock_device_module.h"
#include <opendaq/device_info_internal_ptr.h>
#include <opendaq/discovery_server_factory.h>
#include <coretypes/json_serializer_factory.h>
#include <coreobjects/user_factory.h>
#include <coreobjects/permissions_builder_factory.h>
#include <coreobjects/permission_mask_builder_factory.h>
#include <opendaq/module_impl.h>
#include <chrono>
#include <coretypes/filesystem.h>
#include <opendaq/client_type.h>

using NativeDeviceModulesTest = testing::Test;

using namespace daq;

static InstancePtr CreateCustomServerInstance(AuthenticationProviderPtr authenticationProvider)
{
    auto logger = Logger();
    auto scheduler = Scheduler(logger);
    auto moduleManager = ModuleManager("");
    auto typeManager = TypeManager();
    auto context = Context(scheduler, logger, typeManager, moduleManager, authenticationProvider);

    auto instance = InstanceCustom(context, "serverLocal");

    const auto statistics = instance.addFunctionBlock("RefFBModuleStatistics");
    const auto refDevice = instance.addDevice("daqref://device0");
    refDevice.addProperty(IntProperty("CustomProp", 0));
    statistics.getInputPorts()[0].connect(refDevice.getSignals(search::Recursive(search::Visible()))[0]);
    statistics.getInputPorts()[0].connect(Signal(context, nullptr, "foo"));

    const auto statusType = EnumerationType("StatusType", List<IString>("Off", "On"));
    typeManager.addType(statusType);
    const auto statusValue = Enumeration("StatusType", "On", typeManager);

    instance.getStatusContainer().asPtr<IComponentStatusContainerPrivate>().addStatus("TestStatus", statusValue);

    return instance;
}

static InstancePtr CreateDefaultServerInstance()
{
    auto authenticationProvider = AuthenticationProvider();
    return CreateCustomServerInstance(authenticationProvider);
}

static InstancePtr CreateUpdatedServerInstance()
{
    auto logger = Logger();
    auto scheduler = Scheduler(logger);
    auto moduleManager = ModuleManager("");
    auto typeManager = TypeManager();
    auto authenticationProvider = AuthenticationProvider();
    auto context = Context(scheduler, logger, typeManager, moduleManager, authenticationProvider);

    auto instance = InstanceCustom(context, "serverLocal");

    const auto statistics = instance.addFunctionBlock("RefFBModuleScaling");
    const auto refDevice = instance.addDevice("daqref://device0");
    refDevice.setPropertyValue("NumberOfChannels", 3);

    const auto testType = EnumerationType("TestEnumType", List<IString>("TestValue1", "TestValue2"));
    instance.getContext().getTypeManager().addType(testType);

    return instance;
}

static InstancePtr CreateServerInstance(InstancePtr instance = CreateDefaultServerInstance())
{
    instance.addServer("OpenDAQNativeStreaming", nullptr);

    return instance;
}

static InstancePtr CreateClientInstance(uint16_t nativeConfigProtocolVersion = std::numeric_limits<uint16_t>::max(),
                                        Bool restoreClientConfigOnReconnect = False)
{
    auto logger = Logger();
    auto scheduler = Scheduler(logger);
    auto moduleManager = ModuleManager("");
    auto typeManager = TypeManager();
    auto authenticationProvider = AuthenticationProvider();
    auto context = Context(scheduler, logger, typeManager, moduleManager, authenticationProvider);

    const ModulePtr deviceModule(MockDeviceModule_Create(context));
    moduleManager.addModule(deviceModule);

    auto instance = InstanceCustom(context, "clientLocal");

    auto config = instance.createDefaultAddDeviceConfig();

    PropertyObjectPtr deviceConfig = config.getPropertyValue("Device");
    PropertyObjectPtr nativeDeviceConfig = deviceConfig.getPropertyValue("OpenDAQNativeConfiguration");
    if (nativeConfigProtocolVersion != std::numeric_limits<uint16_t>::max())
        nativeDeviceConfig.setPropertyValue("ProtocolVersion", nativeConfigProtocolVersion);

    nativeDeviceConfig.setPropertyValue("RestoreClientConfigOnReconnect", restoreClientConfigOnReconnect);

    PropertyObjectPtr general = config.getPropertyValue("General");
    general.setPropertyValue("PrioritizedStreamingProtocols", List<IString>("OpenDAQNativeStreaming"));

    auto refDevice = instance.addDevice("daq.nd://127.0.0.1", config);
    return instance;
}

TEST_F(NativeDeviceModulesTest, ConnectAndDisconnect)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    client->releaseRef();
    server->releaseRef();
    client.detach();
    server.detach();
}

TEST_F(NativeDeviceModulesTest, CheckProtocolVersion)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    const auto info = client.getDevices()[0].getInfo();
    ASSERT_TRUE(info.hasProperty("NativeConfigProtocolVersion"));
    ASSERT_EQ(static_cast<uint16_t>(info.getPropertyValue("NativeConfigProtocolVersion")), 7);

    client->releaseRef();
    server->releaseRef();
    client.detach();
    server.detach();
}

TEST_F(NativeDeviceModulesTest, UseOldProtocolVersion)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance(0);

    const auto info = client.getDevices()[0].getInfo();
    ASSERT_TRUE(info.hasProperty("NativeConfigProtocolVersion"));
    ASSERT_EQ(static_cast<uint16_t>(info.getPropertyValue("NativeConfigProtocolVersion")), 0);

    client->releaseRef();
    server->releaseRef();
    client.detach();
    server.detach();
}

TEST_F(NativeDeviceModulesTest, ServerVersionTooLow)
{
    SKIP_TEST_MAC_CI;

    // connect to server with protocol version 2

    const uint16_t negotiateVersion = 2;

    auto server = CreateServerInstance();
    auto client = CreateClientInstance(negotiateVersion);

    const auto info = client.getDevices()[0].getInfo();
    ASSERT_TRUE(info.hasProperty("NativeConfigProtocolVersion"));
    ASSERT_EQ(static_cast<uint16_t>(info.getPropertyValue("NativeConfigProtocolVersion")), negotiateVersion);

    // call some methods which require protocol version 3 or higher

    ASSERT_THROW(client.lock(), ServerVersionTooLowException);
    ASSERT_THROW(client.unlock(), ServerVersionTooLowException);
    ASSERT_THROW(client.getDevices()[0].getAvailableDevices(), ServerVersionTooLowException);
    ASSERT_THROW(client.getDevices()[0].getAvailableDeviceTypes(), ServerVersionTooLowException);
}


TEST_F(NativeDeviceModulesTest, ConnectViaIpv6)
{
    if (test_helpers::Ipv6IsDisabled())
        return;

    auto server = CreateServerInstance();
    auto client = Instance();
    ASSERT_NO_THROW(client.addDevice("daq.nd://[::1]", nullptr));

    client->releaseRef();
    server->releaseRef();
    client.detach();
    server.detach();
}

TEST_F(NativeDeviceModulesTest, ConnectUsername)
{
    auto users = List<IUser>();
    users.pushBack(User("jure", "jure123"));
    users.pushBack(User("tomaz", "tomaz123"));

    auto authProvider = StaticAuthenticationProvider(false, users);

    auto serverInstance = InstanceBuilder().setAuthenticationProvider(authProvider).build();
    serverInstance.addServer("OpenDAQNativeStreaming", nullptr);

    auto clientInstance = Instance();

    ASSERT_ANY_THROW(clientInstance.addDevice("daq.nd://127.0.0.1"));

    auto config = clientInstance.createDefaultAddDeviceConfig();
    PropertyObjectPtr generalConfig = config.getPropertyValue("General");

    generalConfig.setPropertyValue("Username", "jure");
    generalConfig.setPropertyValue("Password", "wrongPass");
    ASSERT_ANY_THROW(clientInstance.addDevice("daq.nd://127.0.0.1", config));

    generalConfig.setPropertyValue("Username", "jure");
    generalConfig.setPropertyValue("Password", "jure123");
    auto deviceJure = clientInstance.addDevice("daq.nd://127.0.0.1", config);
    ASSERT_TRUE(deviceJure.assigned());
    clientInstance.removeDevice(deviceJure);

    generalConfig.setPropertyValue("Username", "tomaz");
    generalConfig.setPropertyValue("Password", "tomaz123");
    auto deviceTomaz = clientInstance.addDevice("daq.nd://127.0.0.1", config);
    ASSERT_TRUE(deviceTomaz.assigned());
}

TEST_F(NativeDeviceModulesTest, ConnectAllowAnonymous)
{
    auto users = List<IUser>();
    users.pushBack(User("jure", "jure123"));

    auto authProvider = StaticAuthenticationProvider(true, users);

    auto serverInstance = InstanceBuilder().setAuthenticationProvider(authProvider).build();
    serverInstance.addServer("OpenDAQNativeStreaming", nullptr);

    auto clientInstance = Instance();

    auto deviceAnonymous = clientInstance.addDevice("daq.nd://127.0.0.1");
    ASSERT_TRUE(deviceAnonymous.assigned());
    clientInstance.removeDevice(deviceAnonymous);

    auto config = clientInstance.createDefaultAddDeviceConfig();
    PropertyObjectPtr generalConfig = config.getPropertyValue("General");

    generalConfig.setPropertyValue("Username", "jure");
    generalConfig.setPropertyValue("Password", "wrongPass");
    ASSERT_ANY_THROW(clientInstance.addDevice("daq.nd://127.0.0.1", config));

    generalConfig.setPropertyValue("Username", "jure");
    generalConfig.setPropertyValue("Password", "jure123");
    auto deviceJure = clientInstance.addDevice("daq.nd://127.0.0.1", config);
    ASSERT_TRUE(deviceJure.assigned());
}

TEST_F(NativeDeviceModulesTest, ConnectUsernameDeviceConfig)
{
    auto users = List<IUser>();
    users.pushBack(User("jure", "jure123"));

    auto authProvider = StaticAuthenticationProvider(false, users);

    auto serverInstance = InstanceBuilder().setAuthenticationProvider(authProvider).build();
    serverInstance.addServer("OpenDAQNativeStreaming", nullptr);

    auto clientInstance = Instance();

    ASSERT_ANY_THROW(clientInstance.addDevice("daq.nd://127.0.0.1"));

    auto config = clientInstance.createDefaultAddDeviceConfig();
    PropertyObjectPtr deviceConfig = config.getPropertyValue("Device");
    PropertyObjectPtr nativeDeviceConfig = deviceConfig.getPropertyValue("OpenDAQNativeConfiguration");

    nativeDeviceConfig.setPropertyValue("Username", "jure");
    nativeDeviceConfig.setPropertyValue("Password", "wrongPass");
    ASSERT_ANY_THROW(clientInstance.addDevice("daq.nd://127.0.0.1", config));

    nativeDeviceConfig.setPropertyValue("Username", "jure");
    nativeDeviceConfig.setPropertyValue("Password", "jure123");
    auto device = clientInstance.addDevice("daq.nd://127.0.0.1", config);
    ASSERT_TRUE(device.assigned());
}

TEST_F(NativeDeviceModulesTest, ConnectUsernameDeviceAndStreamingConfig)
{
    auto users = List<IUser>();
    users.pushBack(User("jure", "jure123"));
    users.pushBack(User("tomaz", "tomaz123"));

    auto authProvider = StaticAuthenticationProvider(false, users);

    auto serverInstance = InstanceBuilder().setAuthenticationProvider(authProvider).build();
    serverInstance.addServer("OpenDAQNativeStreaming", nullptr);

    auto clientInstance = Instance();

    auto config = clientInstance.createDefaultAddDeviceConfig();
    PropertyObjectPtr deviceConfig = config.getPropertyValue("Device");
    PropertyObjectPtr nativeDeviceConfig = deviceConfig.getPropertyValue("OpenDAQNativeConfiguration");
    PropertyObjectPtr streamingConfig = config.getPropertyValue("Streaming");
    PropertyObjectPtr nativeStreamingConfig = streamingConfig.getPropertyValue("OpenDAQNativeStreaming");

    nativeDeviceConfig.setPropertyValue("Username", "jure");
    nativeDeviceConfig.setPropertyValue("Password", "jure123");
    nativeStreamingConfig.setPropertyValue("Username", "tomaz");
    nativeStreamingConfig.setPropertyValue("Password", "tomaz123");
    auto device = clientInstance.addDevice("daq.nd://127.0.0.1", config);
    ASSERT_TRUE(device.assigned());
}

TEST_F(NativeDeviceModulesTest, ClientTypeExclusiveControlTwice)
{
    const std::string url = "daq.nd://127.0.0.1";

    auto serverInstance = InstanceBuilder().build();
    serverInstance.addServer("OpenDAQNativeStreaming", nullptr);

    auto clientInstance = test_helpers::connectInstanceWithClientType(url, ClientType::ExclusiveControl);
    ASSERT_EQ(clientInstance.getDevices().getCount(), 1u);

    ASSERT_THROW(test_helpers::connectInstanceWithClientType(url, ClientType::ExclusiveControl), ControlClientRejectedException);

    clientInstance = nullptr; // disconnect
    clientInstance = test_helpers::connectInstanceWithClientType(url, ClientType::ExclusiveControl);
    ASSERT_EQ(clientInstance.getDevices().getCount(), 1u);
}

TEST_F(NativeDeviceModulesTest, ClientTypeExclusiveControlAndControl)
{
    const std::string url = "daq.nd://127.0.0.1";

    auto serverInstance = InstanceBuilder().build();
    serverInstance.addServer("OpenDAQNativeStreaming", nullptr);

    auto clientInstance = test_helpers::connectInstanceWithClientType(url, ClientType::ExclusiveControl);
    ASSERT_EQ(clientInstance.getDevices().getCount(), 1u);

    ASSERT_THROW(test_helpers::connectInstanceWithClientType(url, ClientType::Control), ControlClientRejectedException);

    clientInstance = nullptr; // disconnect
    clientInstance = test_helpers::connectInstanceWithClientType(url, ClientType::ExclusiveControl);
    ASSERT_EQ(clientInstance.getDevices().getCount(), 1u);
}

TEST_F(NativeDeviceModulesTest, ClientTypeControlAndExclusiveControl)
{
    const std::string url = "daq.nd://127.0.0.1";

    auto serverInstance = InstanceBuilder().build();
    serverInstance.addServer("OpenDAQNativeStreaming", nullptr);

    auto clientInstance = test_helpers::connectInstanceWithClientType(url, ClientType::Control);
    ASSERT_EQ(clientInstance.getDevices().getCount(), 1u);

    ASSERT_THROW(test_helpers::connectInstanceWithClientType(url, ClientType::ExclusiveControl), ControlClientRejectedException);

    clientInstance = nullptr; // disconnect
    clientInstance = test_helpers::connectInstanceWithClientType(url, ClientType::ExclusiveControl);
    ASSERT_EQ(clientInstance.getDevices().getCount(), 1u);
}

TEST_F(NativeDeviceModulesTest, ClientTypeExclusiveControlDropOthers)
{
    const std::string url = "daq.nd://127.0.0.1";

    auto serverInstance = InstanceBuilder().build();
    serverInstance.addServer("OpenDAQNativeStreaming", nullptr);

    auto clientInstance1 = test_helpers::connectInstanceWithClientType(url, ClientType::Control);
    ASSERT_EQ(clientInstance1.getDevices().getCount(), 1u);

    auto clientInstance2 = test_helpers::connectInstanceWithClientType(url, ClientType::Control);
    ASSERT_EQ(clientInstance2.getDevices().getCount(), 1u);

    ASSERT_EQ(clientInstance1.getDevices()[0].getConnectionStatusContainer().getStatus("ConfigurationStatus"), "Connected");
    ASSERT_EQ(clientInstance2.getDevices()[0].getConnectionStatusContainer().getStatus("ConfigurationStatus"), "Connected");

    auto clientInstance3 = test_helpers::connectInstanceWithClientType(
        url, ClientType::ExclusiveControl, true);  // should cause all other control clients to disconnect

    ASSERT_EQ(clientInstance3.getDevices().getCount(), 1u);
    ASSERT_NE(clientInstance1.getDevices()[0].getConnectionStatusContainer().getStatus("ConfigurationStatus"), "Connected");
    ASSERT_NE(clientInstance2.getDevices()[0].getConnectionStatusContainer().getStatus("ConfigurationStatus"), "Connected");
    ASSERT_EQ(clientInstance3.getDevices()[0].getConnectionStatusContainer().getStatus("ConfigurationStatus"), "Connected");
}

TEST_F(NativeDeviceModulesTest, ClientTypeViewOnly)
{
    const std::string url = "daq.nd://127.0.0.1";

    auto serverInstance = InstanceBuilder().build();
    serverInstance.addServer("OpenDAQNativeStreaming", nullptr);

    auto clientInstance1 = test_helpers::connectInstanceWithClientType(url, ClientType::ExclusiveControl);
    ASSERT_EQ(clientInstance1.getDevices().getCount(), 1u);

    auto clientInstance2 = test_helpers::connectInstanceWithClientType(url, ClientType::ViewOnly);
    ASSERT_EQ(clientInstance2.getDevices().getCount(), 1u);
}

TEST_F(NativeDeviceModulesTest, ClientTypeViewOnlyDropOthers)
{
    const std::string url = "daq.nd://127.0.0.1";

    auto serverInstance = InstanceBuilder().build();
    serverInstance.addServer("OpenDAQNativeStreaming", nullptr);

    auto clientInstance1 = test_helpers::connectInstanceWithClientType(url, ClientType::Control);
    ASSERT_EQ(clientInstance1.getDevices().getCount(), 1u);

    auto clientInstance2 = test_helpers::connectInstanceWithClientType(url, ClientType::ViewOnly);
    ASSERT_EQ(clientInstance2.getDevices().getCount(), 1u);

    ASSERT_EQ(clientInstance1.getDevices()[0].getConnectionStatusContainer().getStatus("ConfigurationStatus"), "Connected");
    ASSERT_EQ(clientInstance2.getDevices()[0].getConnectionStatusContainer().getStatus("ConfigurationStatus"), "Connected");

    auto clientInstance3 = test_helpers::connectInstanceWithClientType(
        url, ClientType::ExclusiveControl, true);  // should cause all other control clients to disconnect

    ASSERT_EQ(clientInstance3.getDevices().getCount(), 1u);
    ASSERT_NE(clientInstance1.getDevices()[0].getConnectionStatusContainer().getStatus("ConfigurationStatus"), "Connected");
    ASSERT_EQ(clientInstance2.getDevices()[0].getConnectionStatusContainer().getStatus("ConfigurationStatus"), "Connected"); // view-only client should stay connected
    ASSERT_EQ(clientInstance3.getDevices()[0].getConnectionStatusContainer().getStatus("ConfigurationStatus"), "Connected");
}

TEST_F(NativeDeviceModulesTest, ClientTypeExclusiveControlDropOtherExclusiveControl)
{
    const std::string url = "daq.nd://127.0.0.1";

    auto serverInstance = InstanceBuilder().build();
    serverInstance.addServer("OpenDAQNativeStreaming", nullptr);

    auto clientInstance1 = test_helpers::connectInstanceWithClientType(url, ClientType::ExclusiveControl);
    ASSERT_EQ(clientInstance1.getDevices().getCount(), 1u);

    ASSERT_EQ(clientInstance1.getDevices()[0].getConnectionStatusContainer().getStatus("ConfigurationStatus"), "Connected");

    auto clientInstance2 = test_helpers::connectInstanceWithClientType(
        url, ClientType::ExclusiveControl, true);  // should cause first exclusive control client to disconnect

    ASSERT_EQ(clientInstance2.getDevices().getCount(), 1u);
    ASSERT_NE(clientInstance1.getDevices()[0].getConnectionStatusContainer().getStatus("ConfigurationStatus"), "Connected");
    ASSERT_EQ(clientInstance2.getDevices()[0].getConnectionStatusContainer().getStatus("ConfigurationStatus"), "Connected");
}

TEST_F(NativeDeviceModulesTest, PartialSerialization)
{
    auto users = List<IUser>();
    users.pushBack(User("jure", "jure123", List<IString>("user")));

    auto authProvider = StaticAuthenticationProvider(false, users);

    auto serverInstance = CreateCustomServerInstance(authProvider);
    serverInstance.addServer("OpenDAQNativeStreaming", nullptr);

    auto channels = serverInstance.getChannelsRecursive();
    ASSERT_EQ(channels.getCount(), 2u);

    auto permissions = PermissionsBuilder().inherit(true).deny("user", PermissionMaskBuilder().read()).build();
    channels.getItemAt(0).getPermissionManager().setPermissions(permissions);

    auto clientInstance = Instance();

    auto config = clientInstance.createDefaultAddDeviceConfig();
    PropertyObjectPtr generalConfig = config.getPropertyValue("General");
    generalConfig.setPropertyValue("Username", "jure");
    generalConfig.setPropertyValue("Password", "jure123");

    auto device = clientInstance.addDevice("daq.nd://127.0.0.1", config);
    ASSERT_TRUE(device.assigned());

    auto clientChannels = device.getChannelsRecursive();
    ASSERT_EQ(clientChannels.getCount(), 1u);
}

TEST_F(NativeDeviceModulesTest, PartialSerializationPropertyObjectClass)
{
    auto authProvider = AuthenticationProvider(true);

    auto serverInstance = CreateCustomServerInstance(authProvider);
    serverInstance.addServer("OpenDAQNativeStreaming", nullptr);

    auto typeManager = serverInstance.getContext().getTypeManager();

    const auto obj = PropertyObject();
    obj.addProperty(StringProperty("NestedStringProperty", "String"));
    const auto testClass = PropertyObjectClassBuilder("TestClass")
                               .addProperty(StringProperty("TestString", "String"))
                               .addProperty(ObjectProperty("TestChild", obj))
                               .build();

    typeManager.addType(testClass);

    auto clientInstance = Instance();
    auto device = clientInstance.addDevice("daq.nd://127.0.0.1");
    ASSERT_TRUE(device.assigned());
}

TEST_F(NativeDeviceModulesTest, DiscoveringServer)
{
    auto server = InstanceBuilder().addDiscoveryServer("mdns").setDefaultRootDeviceLocalId("local").build();
    server.addDevice("daqref://device1");

    auto serverConfig = server.getAvailableServerTypes().get("OpenDAQNativeStreaming").createDefaultConfig();
    auto path = "/test/native_configuration/discovery/";
    serverConfig.setPropertyValue("Path", path);
    server.addServer("OpenDAQNativeStreaming", serverConfig).enableDiscovery();

    auto client = Instance();
    DevicePtr device;
    for (const auto& deviceInfo : client.getAvailableDevices())
    {
        for (const auto& capability : deviceInfo.getServerCapabilities())
        {
            if (!test_helpers::isSufix(capability.getConnectionString(), path))
                break;

            if (capability.getProtocolName() == "OpenDAQNativeConfiguration")
            {
                device = client.addDevice(capability.getConnectionString(), nullptr);
                return;
            }
        }
    }
    ASSERT_TRUE(false) << "Device not found";
}

TEST_F(NativeDeviceModulesTest, DiscoveringServerInfoMerge)
{
    const auto info = DeviceInfo("", "foo");
    info.setMacAddress("custom_mac");
    auto server = InstanceBuilder().addDiscoveryServer("mdns")
                                   .setDefaultRootDeviceLocalId("local")
                                   .setDefaultRootDeviceInfo(info)
                                   .build();
    server.addDevice("daqref://device1");

    auto serverConfig = server.getAvailableServerTypes().get("OpenDAQNativeStreaming").createDefaultConfig();
    auto path = "/test/native_configuration/discovery/";
    serverConfig.setPropertyValue("Path", path);
    server.addServer("OpenDAQNativeStreaming", serverConfig).enableDiscovery();

    auto client = Instance();
    DevicePtr device;
    for (const auto & deviceInfo : client.getAvailableDevices())
    {
        ASSERT_EQ(deviceInfo.getMacAddress(), "");
        for (const auto & capability : deviceInfo.getServerCapabilities())
        {
            if (!test_helpers::isSufix(capability.getConnectionString(), path))
                break;
            
            if (capability.getProtocolName() == "OpenDAQNativeConfiguration")
            {
                device = client.addDevice(capability.getConnectionString(), nullptr);
                ASSERT_EQ(device.getInfo().getMacAddress(), "custom_mac");
                return;
            }
        }
    }
    ASSERT_TRUE(false) << "Device not found";
}

TEST_F(NativeDeviceModulesTest, RemoveServer)
{
    auto server = InstanceBuilder().addDiscoveryServer("mdns")
                                   .setDefaultRootDeviceLocalId("local")
                                   .build();
    server.addDevice("daqref://device1");

    auto serverConfig = server.getAvailableServerTypes().get("OpenDAQNativeStreaming").createDefaultConfig();
    auto path = "/test/native_configuration/removeServer/";
    serverConfig.setPropertyValue("Path", path);
    auto server1 = server.addServer("OpenDAQNativeStreaming", serverConfig);
    server1.enableDiscovery();

    // check that server is discoverable
    {
        auto client = Instance();
        size_t deviceFound = 0;
        for (const auto & deviceInfo : client.getAvailableDevices())
        {
            for (const auto & capability : deviceInfo.getServerCapabilities())
            {
                if (!test_helpers::isSufix(capability.getConnectionString(), path))
                    break;
            
                if (capability.getProtocolName() == "OpenDAQNativeConfiguration")
                {
                   deviceFound += 1;
                }
            }
        }
        ASSERT_EQ(deviceFound, 1u);
    }

    // remove device and check that server now is not discoverable
    server.removeServer(server1);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    {
        auto client = Instance();
        size_t deviceFound = 0;
        for (const auto & deviceInfo : client.getAvailableDevices())
        {
            for (const auto & capability : deviceInfo.getServerCapabilities())
            {
                if (!test_helpers::isSufix(capability.getConnectionString(), path))
                    break;
            
                if (capability.getProtocolName() == "OpenDAQNativeConfiguration")
                {
                   deviceFound += 1;
                }
            }
        }
        ASSERT_EQ(deviceFound, 0u);
    }

    // add server again and check that server is discoverable
    auto path2 = "/test/native_configuration/removeServer2/";
    serverConfig.setPropertyValue("Path", path2);
    auto server2 = server.addServer("OpenDAQNativeStreaming", serverConfig);
    server2.enableDiscovery();
    {
        auto client = Instance();
        size_t deviceFound = 0;
        for (const auto & deviceInfo : client.getAvailableDevices())
        {
            for (const auto & capability : deviceInfo.getServerCapabilities())
            {
                bool isRemovedServer = test_helpers::isSufix(capability.getConnectionString(), path);
                bool isNewServer = test_helpers::isSufix(capability.getConnectionString(), path2);
                if (!isRemovedServer && !isNewServer)
                    break;

                if (capability.getProtocolName() == "OpenDAQNativeConfiguration")
                {
                   deviceFound += 1;
                }
            }
        }
        ASSERT_EQ(deviceFound, 1u);
    }
}

TEST_F(NativeDeviceModulesTest, checkDeviceInfoPopulatedWithProvider)
{
    std::string filename = "populateDefaultConfig.json";
    std::string json = R"(
        {
            "Modules":
            {
                "OpenDAQNativeStreamingServerModule":
                {
                    "NativeStreamingPort": 1234,
                    "MaxAllowedConfigConnections": 123,
                    "StreamingPacketSendTimeout": 2000,
                    "Path": "/test/native_configurator/checkDeviceInfoPopulated/"
                }
            }
        }
    )";
    auto path = "/test/native_configurator/checkDeviceInfoPopulated/";
    auto finally = test_helpers::CreateConfigFile(filename, json);

    auto rootInfo = DeviceInfo("");
    rootInfo.setName("TestName");
    rootInfo.setManufacturer("TestManufacturer");
    rootInfo.setModel("TestModel");
    rootInfo.setSerialNumber("TestSerialNumber");

    auto provider = JsonConfigProvider(filename);
    auto instance = InstanceBuilder().addDiscoveryServer("mdns").addConfigProvider(provider).setDefaultRootDeviceInfo(rootInfo).build();
    auto serverConfig = instance.getAvailableServerTypes().get("OpenDAQNativeStreaming").createDefaultConfig();

    ASSERT_EQ(serverConfig.getPropertyValue("NativeStreamingPort").asPtr<IInteger>(), 1234);
    ASSERT_EQ(serverConfig.getPropertyValue("Path").asPtr<IString>(), path);
    ASSERT_EQ(serverConfig.getPropertyValue("MaxAllowedConfigConnections").asPtr<IInteger>(), 123);
    ASSERT_EQ(serverConfig.getPropertyValue("StreamingPacketSendTimeout").asPtr<IInteger>(), 2000);

    instance.addServer("OpenDAQNativeStreaming", serverConfig).enableDiscovery();

    auto client = Instance();

    for (const auto & deviceInfo : client.getAvailableDevices())
    {
        for (const auto & capability : deviceInfo.getServerCapabilities())
        {
            if (!test_helpers::isSufix(capability.getConnectionString(), path))
            {
                break;
            }
            if (capability.getProtocolName() == "OpenDAQNativeConfiguration")
            {
                client.addDevice(capability.getConnectionString(), nullptr);
                ASSERT_EQ(deviceInfo.getName(), rootInfo.getName());
                ASSERT_EQ(deviceInfo.getManufacturer(), rootInfo.getManufacturer());
                ASSERT_EQ(deviceInfo.getModel(), rootInfo.getModel());
                ASSERT_EQ(deviceInfo.getSerialNumber(), rootInfo.getSerialNumber());
                return;
            }
        }      
    }

    ASSERT_TRUE(false) << "Device not found";
}

#ifdef _WIN32

TEST_F(NativeDeviceModulesTest, TestDiscoveryReachability)
{
    if (test_helpers::Ipv6IsDisabled())
        return;

    auto instance = InstanceBuilder().addDiscoveryServer("mdns").build();
    auto serverConfig = instance.getAvailableServerTypes().get("OpenDAQNativeStreaming").createDefaultConfig();
    auto path = "/test/native_configurator/discovery_reachability/";
    serverConfig.setPropertyValue("Path", path);

    instance.addServer("OpenDAQNativeStreaming", serverConfig).enableDiscovery();

    auto client = Instance();

    for (const auto & deviceInfo : client.getAvailableDevices())
    {
        for (const auto & capability : deviceInfo.getServerCapabilities())
        {
            if (!test_helpers::isSufix(capability.getConnectionString(), path))
                break;

            if (capability.getProtocolName() == "OpenDAQNativeConfiguration")
            {
                const auto ipv4Info = capability.getAddressInfo()[0];
                const auto ipv6Info = capability.getAddressInfo()[1];
                ASSERT_EQ(ipv4Info.getReachabilityStatus(), AddressReachabilityStatus::Reachable);
                ASSERT_EQ(ipv6Info.getReachabilityStatus(), AddressReachabilityStatus::Unknown);
                
                ASSERT_EQ(ipv4Info.getType(), "IPv4");
                ASSERT_EQ(ipv6Info.getType(), "IPv6");

                ASSERT_EQ(ipv4Info.getConnectionString(), capability.getConnectionStrings()[0]);
                ASSERT_EQ(ipv6Info.getConnectionString(), capability.getConnectionStrings()[1]);
                
                ASSERT_EQ(ipv4Info.getAddress(), capability.getAddresses()[0]);
                ASSERT_EQ(ipv6Info.getAddress(), capability.getAddresses()[1]);
                return;
            }
            else
            {
                ASSERT_EQ(capability.getProtocolName(), "OpenDAQNativeStreaming");
            }
        }      
    }
    ASSERT_TRUE(false) << "Device not found";
}

TEST_F(NativeDeviceModulesTest, TestDiscoveryReachabilityAfterConnectIPv6)
{
    if (test_helpers::Ipv6IsDisabled())
        return;

    auto instance = InstanceBuilder().addDiscoveryServer("mdns").build();
    auto serverConfig = instance.getAvailableServerTypes().get("OpenDAQNativeStreaming").createDefaultConfig();
    auto path = "/test/native_configurator/discovery_reachability_after_connect_ipv6/";
    serverConfig.setPropertyValue("Path", path);

    instance.addServer("OpenDAQNativeStreaming", serverConfig).enableDiscovery();

    auto client = Instance();
    // client.getAvailableDevices(); // TODO: won't work with prefix which is not `daq://`
    // daq://[::1]/path.../ wont work as well because will be found daq://127.0.0.1/path.../ or similar
    DevicePtr device = client.addDevice(std::string("daq.nd://[::1]") + path);

    ASSERT_TRUE(device.assigned());

    const auto caps = device.getInfo().getServerCapabilities();
    ASSERT_EQ(caps.getCount(), 2u);

    for (const auto& capability : caps)
    {
        if (!test_helpers::isSufix(capability.getConnectionString(), path))
            break;

        if (capability.getProtocolName() == "OpenDAQNativeConfiguration")
        {
            const auto ipv6Info = capability.getAddressInfo()[0];
            // ASSERT_EQ(ipv4Info.getReachabilityStatus(), AddressReachabilityStatus::Reachable);
            ASSERT_EQ(ipv6Info.getReachabilityStatus(), AddressReachabilityStatus::Reachable);
            
            // ASSERT_EQ(ipv4Info.getType(), "IPv4");
            ASSERT_EQ(ipv6Info.getType(), "IPv6");

            // ASSERT_EQ(ipv4Info.getConnectionString(), capability.getConnectionStrings()[0]);
            ASSERT_EQ(ipv6Info.getConnectionString(), capability.getConnectionStrings()[0]);
            
            // ASSERT_EQ(ipv4Info.getAddress(), capability.getAddresses()[0]);
            ASSERT_EQ(ipv6Info.getAddress(), capability.getAddresses()[0]);
            return;
        }
        else
        {
            ASSERT_EQ(capability.getProtocolName(), "OpenDAQNativeStreaming");
        }
    }
    ASSERT_TRUE(false) << "Device not found";
}

#endif

DevicePtr FindNativeDeviceByPath(const InstancePtr& instance, const std::string& path, const PropertyObjectPtr& config = nullptr)
{
    for (const auto & deviceInfo : instance.getAvailableDevices())
    {
        for (const auto & capability : deviceInfo.getServerCapabilities())
        {
            if (!test_helpers::isSufix(capability.getConnectionString(), path))
                break;

            if (capability.getProtocolName() == "OpenDAQNativeConfiguration")
            {
                return instance.addDevice(capability.getConnectionString(), config);
            }
        }
    }
    return DevicePtr();
}

TEST_F(NativeDeviceModulesTest, TestDiscoveryReachabilityAfterConnect)
{
    if (test_helpers::Ipv6IsDisabled())
        return;

    auto instance = InstanceBuilder().addDiscoveryServer("mdns").build();
    auto serverConfig = instance.getAvailableServerTypes().get("OpenDAQNativeStreaming").createDefaultConfig();
    auto path = "/test/native_configurator/discovery_reachability/";
    serverConfig.setPropertyValue("Path", path);

    instance.addServer("OpenDAQNativeStreaming", serverConfig).enableDiscovery();

    auto client = Instance();
    DevicePtr device = FindNativeDeviceByPath(client, path);
    ASSERT_TRUE(device.assigned());

    const auto caps = device.getInfo().getServerCapabilities();
    ASSERT_EQ(caps.getCount(), 2u);

    for (const auto& capability : caps)
    {
        if (capability.getProtocolName() == "OpenDAQNativeConfiguration")
        {
            const auto ipv4Info = capability.getAddressInfo()[0];
            const auto ipv6Info = capability.getAddressInfo()[1];
            ASSERT_EQ(ipv4Info.getReachabilityStatus(), AddressReachabilityStatus::Reachable);
            ASSERT_EQ(ipv6Info.getReachabilityStatus(), AddressReachabilityStatus::Unknown);
            
            ASSERT_EQ(ipv4Info.getType(), "IPv4");
            ASSERT_EQ(ipv6Info.getType(), "IPv6");

            ASSERT_EQ(ipv4Info.getConnectionString(), capability.getConnectionStrings()[0]);
            ASSERT_EQ(ipv6Info.getConnectionString(), capability.getConnectionStrings()[1]);
            
            ASSERT_EQ(ipv4Info.getAddress(), capability.getAddresses()[0]);
            ASSERT_EQ(ipv6Info.getAddress(), capability.getAddresses()[1]);
            return;
        }
        else
        {
            ASSERT_EQ(capability.getProtocolName(), "OpenDAQNativeStreaming");
        }
    }
    ASSERT_TRUE(false) << "Device not found";
}

TEST_F(NativeDeviceModulesTest, TestProtocolVersion)
{
    auto instance = InstanceBuilder().addDiscoveryServer("mdns").build();
    auto serverConfig = instance.getAvailableServerTypes().get("OpenDAQNativeStreaming").createDefaultConfig();
    auto path = "/test/native_configurator/test_protocol_version/";
    serverConfig.setPropertyValue("Path", path);

    instance.addServer("OpenDAQNativeStreaming", serverConfig).enableDiscovery();

    auto client = Instance();
    DevicePtr device = FindNativeDeviceByPath(client, path);

    ASSERT_TRUE(device.assigned());

    const auto caps = device.getInfo().getServerCapabilities();
    ASSERT_EQ(caps.getCount(), 2u);

    for (const auto& capability : caps)
    {
        if (capability.getProtocolName() == "OpenDAQNativeConfiguration")
        {
            auto serverVersion = capability.getProtocolVersion();
            ASSERT_EQ(serverVersion, device.getInfo().getConfigurationConnectionInfo().getProtocolVersion());
            int version = std::atoi(serverVersion.getCharPtr());
            ASSERT_GT(version, 3);
        }
        else if (capability.getProtocolName() == "OpenDAQNativeStreaming")
        {
            ASSERT_EQ(capability.getProtocolVersion(), "");
        }
        else
        {
            ASSERT_TRUE(false) << "Unexpected protocol name" << capability.getProtocolName();
        }
    }      
}

TEST_F(NativeDeviceModulesTest, TestProtocolVersionClientIsOlder)
{
    auto instance = InstanceBuilder().addDiscoveryServer("mdns").build();
    auto serverConfig = instance.getAvailableServerTypes().get("OpenDAQNativeStreaming").createDefaultConfig();
    auto path = "/test/native_configurator/test_protocol_version_client_is_older/";
    serverConfig.setPropertyValue("Path", path);

    instance.addServer("OpenDAQNativeStreaming", serverConfig).enableDiscovery();

    auto clientConfig = PropertyObject();
    clientConfig.addProperty(IntProperty("ProtocolVersion", 3));

    auto client = Instance();
    DevicePtr device = FindNativeDeviceByPath(client, path, clientConfig);

    ASSERT_TRUE(device.assigned());

    const auto caps = device.getInfo().getServerCapabilities();
    ASSERT_EQ(caps.getCount(), 2u);

    for (const auto& capability : caps)
    {
        if (capability.getProtocolName() == "OpenDAQNativeConfiguration")
        {
            int version = std::atoi(capability.getProtocolVersion().getCharPtr());
            ASSERT_GT(version, 3);
            ASSERT_EQ(device.getInfo().getConfigurationConnectionInfo().getProtocolVersion(), "3");
        }
        else if (capability.getProtocolName() == "OpenDAQNativeStreaming")
        {
            ASSERT_EQ(capability.getProtocolVersion(), "");
        }
        else
        {
            ASSERT_TRUE(false) << "Unexpected protocol name" << capability.getProtocolName();
        }
    }      
}

TEST_F(NativeDeviceModulesTest, GetRemoteDeviceObjects)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    auto signals = client.getSignals(search::Recursive(search::Any()));
    auto signalsServer = server.getSignals(search::Recursive(search::Any()));
    ASSERT_EQ(signals.getCount(), 7u);
    auto signalsVisible = client.getSignals(search::Recursive(search::Visible()));
    ASSERT_EQ(signalsVisible.getCount(), 4u);
    auto devices = client.getDevices();
    ASSERT_EQ(devices.getCount(), 1u);
    auto fbs = devices[0].getFunctionBlocks();
    ASSERT_EQ(fbs.getCount(), 1u);
    auto channels = client.getChannels(search::Recursive(search::Any()));
    ASSERT_EQ(channels.getCount(), 2u);
    auto servers = devices[0].getServers();
    ASSERT_EQ(servers.getCount(), 1u);
}

TEST_F(NativeDeviceModulesTest, GetStatuses)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();
    auto statuses = client.getDevices()[0].getStatusContainer().getStatuses();
    auto connectionStatuses = client.getDevices()[0].getConnectionStatusContainer().getStatuses();

    ASSERT_EQ(statuses.getCount(), 2u);
    ASSERT_TRUE(statuses.hasKey("TestStatus"));
    ASSERT_EQ(statuses.get("TestStatus").getValue(), "On");
    ASSERT_TRUE(statuses.hasKey("ConnectionStatus"));
    ASSERT_EQ(statuses.get("ConnectionStatus").getValue(), "Connected");

    ASSERT_EQ(connectionStatuses.getCount(), 2u);
    ASSERT_TRUE(connectionStatuses.hasKey("ConfigurationStatus"));
    ASSERT_EQ(connectionStatuses.get("ConfigurationStatus").getValue(), "Connected");
    ASSERT_TRUE(connectionStatuses.hasKey("StreamingStatus_OpenDAQNativeStreaming_1"));
    ASSERT_EQ(connectionStatuses.get("StreamingStatus_OpenDAQNativeStreaming_1").getValue(), "Connected");
}

TEST_F(NativeDeviceModulesTest, RemoveDevice)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();
    auto device = client.getDevices()[0];

    ASSERT_NO_THROW(client.removeDevice(device));
    ASSERT_TRUE(device.isRemoved());
}

TEST_F(NativeDeviceModulesTest, ChangePropAfterRemove)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();
    auto device = client.getDevices()[0];

    bool propWriteCompleted = false;
    server.getDevices()[0].getOnPropertyValueWrite("CustomProp") +=
        [&propWriteCompleted](PropertyObjectPtr&, PropertyValueEventArgsPtr&)
    {
        while(!propWriteCompleted)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    };

    auto refDevice = client.getDevices()[0].getDevices()[0];
    auto thread =
        std::thread(
            [&refDevice]()
            {
                EXPECT_THROW(refDevice.setPropertyValue("CustomProp", 1), ComponentRemovedException);
            }
        );

    ASSERT_NO_THROW(client.removeDevice(device));
    ASSERT_TRUE(refDevice.isRemoved());
    ASSERT_THROW(refDevice.setPropertyValue("CustomProp", 2), ComponentRemovedException);

    propWriteCompleted = true;
    thread.join();
}

TEST_F(NativeDeviceModulesTest, RemoteGlobalIds)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    const auto clientRootId = client.getGlobalId();
    const auto serverDeviceId = server.getGlobalId();
    const auto clientDeviceId = client.getDevices()[0].getGlobalId();

    const auto serverSignalId = server.getSignalsRecursive()[0].getGlobalId();
    const auto clientSignalId = client.getSignalsRecursive()[0].getGlobalId();

    ASSERT_EQ(clientDeviceId, clientRootId + "/Dev" + serverDeviceId);
    ASSERT_EQ(clientSignalId, clientRootId + "/Dev" + serverSignalId);
}

TEST_F(NativeDeviceModulesTest, GetSetDeviceProperties)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    auto refDevice = client.getDevices()[0].getDevices()[0];
    auto serverRefDevice = server.getDevices()[0];

    PropertyPtr propInfo;
    ASSERT_NO_THROW(propInfo = refDevice.getProperty("NumberOfChannels"));
    ASSERT_EQ(propInfo.getValueType(), CoreType::ctInt);

    ASSERT_EQ(refDevice.getPropertyValue("NumberOfChannels"), 2);
    refDevice.setPropertyValue("NumberOfChannels", 3);
    ASSERT_EQ(refDevice.getPropertyValue("NumberOfChannels"), 3);
    ASSERT_EQ(serverRefDevice.getPropertyValue("NumberOfChannels"), 3);

    refDevice.setPropertyValue("NumberOfChannels", 1);
    ASSERT_EQ(refDevice.getPropertyValue("NumberOfChannels"), 1);
    ASSERT_EQ(serverRefDevice.getPropertyValue("NumberOfChannels"), 1);

    refDevice.setPropertyValue("GlobalSampleRate", 2000);
    ASSERT_EQ(refDevice.getPropertyValue("GlobalSampleRate"), 2000);
    ASSERT_EQ(serverRefDevice.getPropertyValue("GlobalSampleRate"), 2000);

    ASSERT_ANY_THROW(refDevice.setPropertyValue("InvalidProp", 100));

    auto properties = refDevice.getAllProperties();
    ASSERT_EQ(properties.getCount(), 10u);
}

TEST_F(NativeDeviceModulesTest, DeviceInfo)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    auto info = client.getDevices()[0].getInfo();
    ASSERT_TRUE(info.assigned());
    ASSERT_EQ(info.getConnectionString(), "daq.nd://127.0.0.1");
    ASSERT_TRUE(info.hasProperty("NativeConfigProtocolVersion"));
    ASSERT_EQ(info.getServerCapabilities().getCount(), 2u);
    ASSERT_EQ(info.getServerCapabilities()[0].getProtocolId(), "OpenDAQNativeStreaming");
    ASSERT_EQ(info.getServerCapabilities()[1].getProtocolId(), "OpenDAQNativeConfiguration");

    auto subDeviceInfo = client.getDevices()[0].getDevices()[0].getInfo();
    ASSERT_EQ(subDeviceInfo.getName(), "RefDev0");
    ASSERT_EQ(subDeviceInfo.getConnectionString(), "daqref://device0");
    ASSERT_EQ(subDeviceInfo.getModel(), "Reference device");
    ASSERT_EQ(subDeviceInfo.getSerialNumber(), "DevSer0");
}

TEST_F(NativeDeviceModulesTest, ChannelProps)
{
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();
    auto dev = client.getDevices()[0].getDevices()[0];
    auto customRangeValue = dev.getChannels()[0].getPropertyValue("CustomRange").asPtr<IStruct>();

    ASSERT_EQ(customRangeValue.get("LowValue"), -10.0);
    ASSERT_EQ(customRangeValue.get("HighValue"), 10.0);
}

TEST_F(NativeDeviceModulesTest, FunctionBlockProperties)
{
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    auto fb = client.getDevices()[0].getFunctionBlocks()[0];
    auto serverFb = server.getFunctionBlocks()[0];

    ASSERT_EQ(fb.getPropertyValue("BlockSize"), serverFb.getPropertyValue("BlockSize"));
    fb.setPropertyValue("BlockSize", 20);
    ASSERT_EQ(fb.getPropertyValue("BlockSize"), serverFb.getPropertyValue("BlockSize"));

    ASSERT_EQ(fb.getPropertyValue("DomainSignalType"), serverFb.getPropertyValue("DomainSignalType"));
    fb.setPropertyValue("DomainSignalType", 2);
    ASSERT_EQ(fb.getPropertyValue("DomainSignalType"), serverFb.getPropertyValue("DomainSignalType"));

    ASSERT_ANY_THROW(fb.setPropertyValue("DomainSignalType", 1000));
}

TEST_F(NativeDeviceModulesTest, ProcedureProp)
{
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    auto ch = client.getDevices()[0].getDevices()[0].getChannels()[0];
    ch.setPropertyValue("Waveform", 3);
    ProcedurePtr proc = ch.getPropertyValue("ResetCounter");
    ASSERT_NO_THROW(proc());
}

TEST_F(NativeDeviceModulesTest, SignalDescriptors)
{
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    DataDescriptorPtr dataDescriptor = client.getDevices()[0].getSignals(search::Recursive(search::Visible()))[0].getDescriptor();
    DataDescriptorPtr serverDataDescriptor = server.getSignals(search::Recursive(search::Visible()))[0].getDescriptor();

    DataDescriptorPtr domainDataDescriptor = client.getDevices()[0].getSignals(search::Recursive(search::Visible()))[2].getDescriptor();
    DataDescriptorPtr serverDomainDataDescriptor = server.getSignals(search::Recursive(search::Visible()))[2].getDescriptor();

    ASSERT_TRUE(dataDescriptor.assigned());
    ASSERT_TRUE(domainDataDescriptor.assigned());

    ASSERT_EQ(dataDescriptor, serverDataDescriptor);
    ASSERT_EQ(domainDataDescriptor, serverDomainDataDescriptor);

//    auto refChannel = client.getChannels(search::Recursive(search::Visible()))[0];
//    refChannel.setPropertyValue("ClientSideScaling", true);

//    dataDescriptor = client.getChannels(search::Recursive(search::Visible()))[0].getSignals(search::Recursive(search::Visible()))[0].getDescriptor();
//    serverDataDescriptor = server.getChannels(search::Recursive(search::Visible()))[0].getSignals(search::Recursive(search::Visible()))[0].getDescriptor();
//    ASSERT_EQ(dataDescriptor.getPostScaling().getParameters(), dataDescriptor.getPostScaling().getParameters());
}

TEST_F(NativeDeviceModulesTest, SubscribeReadUnsubscribe)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    auto device = client.getDevices()[0].getDevices()[0];
    const auto deviceChannel0 = device.getChannels()[0];
    const auto deviceSignal0 = deviceChannel0.getSignals(search::Recursive(search::Visible()))[0];

    ASSERT_TRUE(deviceSignal0.getDomainSignal().assigned());

    auto signal = deviceSignal0.template asPtr<IMirroredSignalConfig>();
    auto domainSignal = deviceSignal0.getDomainSignal().template asPtr<IMirroredSignalConfig>();

    StringPtr streamingSource = signal.getActiveStreamingSource();
    ASSERT_EQ(streamingSource, domainSignal.getActiveStreamingSource());

    std::promise<StringPtr> signalSubscribePromise;
    std::future<StringPtr> signalSubscribeFuture;
    test_helpers::setupSubscribeAckHandler(signalSubscribePromise, signalSubscribeFuture, signal);

    std::promise<StringPtr> domainSubscribePromise;
    std::future<StringPtr> domainSubscribeFuture;
    test_helpers::setupSubscribeAckHandler(domainSubscribePromise, domainSubscribeFuture, domainSignal);

    std::promise<StringPtr> signalUnsubscribePromise;
    std::future<StringPtr> signalUnsubscribeFuture;
    test_helpers::setupUnsubscribeAckHandler(signalUnsubscribePromise, signalUnsubscribeFuture, signal);

    std::promise<StringPtr> domainUnsubscribePromise;
    std::future<StringPtr> domainUnsubscribeFuture;
    test_helpers::setupUnsubscribeAckHandler(domainUnsubscribePromise, domainUnsubscribeFuture, domainSignal);

    using namespace std::chrono_literals;
    StreamReaderPtr reader = daq::StreamReader<double, uint64_t>(signal, ReadTimeoutType::Any);

    ASSERT_TRUE(test_helpers::waitForAcknowledgement(signalSubscribeFuture));
    ASSERT_EQ(signalSubscribeFuture.get(), streamingSource);

    ASSERT_TRUE(test_helpers::waitForAcknowledgement(domainSubscribeFuture));
    ASSERT_EQ(domainSubscribeFuture.get(), streamingSource);

    {
        daq::SizeT count = 0;
        reader.read(nullptr, &count, 100);
    }

    double samples[100];
    for (int i = 0; i < 5; ++i)
    {
        daq::SizeT count = 100;
        reader.read(samples, &count, 1000);
        EXPECT_GT(count, 0u) << "iteration " << i;
    }

    reader.release();

    ASSERT_TRUE(test_helpers::waitForAcknowledgement(signalUnsubscribeFuture));
    ASSERT_EQ(signalUnsubscribeFuture.get(), streamingSource);

    ASSERT_TRUE(test_helpers::waitForAcknowledgement(domainUnsubscribeFuture));
    ASSERT_EQ(domainUnsubscribeFuture.get(), streamingSource);
}

TEST_F(NativeDeviceModulesTest, DISABLED_RendererSimple)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    auto device = client.getDevices()[0];
    const auto deviceChannel0 = device.getChannels()[0];
    const auto deviceSignal0 = deviceChannel0.getSignals(search::Recursive(search::Visible()))[0];

    ASSERT_TRUE(deviceSignal0.getDomainSignal().assigned());

    const auto rendererFb = client.addFunctionBlock("RefFBModuleRenderer");
    const auto rendererInputPort0 = rendererFb.getInputPorts()[0];
    rendererInputPort0.connect(deviceSignal0);

    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
}

TEST_F(NativeDeviceModulesTest, NotPublicSignals)
{
    auto server = InstanceBuilder().setDefaultRootDeviceLocalId("customLocal").build();
    auto serverDevice = server.addDevice("daqref://device1");

    auto serverChannels = serverDevice.getChannels();
    ASSERT_TRUE(serverChannels.getCount() > 0);
    for (const auto& signal : serverChannels[0].getSignals(search::Any()))
        signal.setPublic(false);

    server.addServer("OpenDAQNativeStreaming", nullptr);

    auto client = CreateClientInstance();
    auto clientDevice = client.getDevices()[0].getDevices()[0];

    auto clientChannels = clientDevice.getChannels();
    ASSERT_TRUE(clientChannels.getCount() > 0);
    for (const auto & signal : clientChannels[0].getSignals(search::Any()))
    {
        ASSERT_FALSE(signal.getPublic());
    }
}

TEST_F(NativeDeviceModulesTest, AddStreamingPostConnection)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    auto clientMirroredDevice = client.getDevices()[0].template asPtrOrNull<IMirroredDevice>();
    ASSERT_TRUE(clientMirroredDevice.assigned());
    ASSERT_EQ(clientMirroredDevice.getStreamingSources().getCount(), 1u);

    const auto clientSignals = client.getSignals(search::Recursive(search::Any()));
    for (const auto& signal : clientSignals)
    {
        auto mirorredSignal = signal.template asPtr<IMirroredSignalConfig>();
        ASSERT_EQ(mirorredSignal.getStreamingSources().getCount(), 1u);
    }

    server.addServer("OpenDAQLTStreaming", nullptr);
    StreamingPtr streaming;
    ASSERT_NO_THROW(streaming = client.getDevices()[0].addStreaming("daq.lt://127.0.0.1"));
    ASSERT_EQ(clientMirroredDevice.getStreamingSources().getCount(), 2u);
    ASSERT_EQ(streaming, clientMirroredDevice.getStreamingSources()[1]);

    streaming.addSignals(clientSignals);
    for (const auto& signal : clientSignals)
    {
        auto mirorredSignal = signal.template asPtr<IMirroredSignalConfig>();
        ASSERT_EQ(mirorredSignal.getStreamingSources().getCount(), 2u);
        ASSERT_NO_THROW(mirorredSignal.setActiveStreamingSource(streaming.getConnectionString()));
    }
}

class AddComponentsTest : public NativeDeviceModulesTest, public testing::WithParamInterface<std::vector<std::string>>
{
public:
    InstancePtr createServerInstance()
    {
        InstancePtr instance = CreateDefaultServerInstance();

        instance.addServer("OpenDAQLTStreaming", nullptr);
        instance.addServer("OpenDAQNativeStreaming", nullptr);

        return instance;
    }

    InstancePtr createClientInstance()
    {
        auto logger = Logger();
        auto scheduler = Scheduler(logger);
        auto moduleManager = ModuleManager("");
        auto typeManager = TypeManager();
        auto authenticationProvider = AuthenticationProvider();
        auto context = Context(scheduler, logger, typeManager, moduleManager, authenticationProvider);
        auto instance = InstanceCustom(context, "client");
        
        auto config = instance.createDefaultAddDeviceConfig();
        PropertyObjectPtr general = config.getPropertyValue("General");

        auto prioritizedStreamingProtocols = List<IString>();
        for (const auto& protocolId : GetParam())
            prioritizedStreamingProtocols.pushBack(protocolId);

        general.setPropertyValue("PrioritizedStreamingProtocols", prioritizedStreamingProtocols);

        instance.addDevice("daq.nd://127.0.0.1", config);
        return instance;
    }

    size_t getStreamingSourcesCount()
    {
        return GetParam().size();
    }

    std::string getActiveStreamingSource()
    {
        auto streamingProtocolIDs = GetParam();
        if (streamingProtocolIDs.empty())
            return "unknown";
        if (streamingProtocolIDs[0] == "OpenDAQLTStreaming")
            return "daq.lt://127.0.0.1:7414";
        else if (streamingProtocolIDs[0] == "OpenDAQNativeStreaming")
            return "daq.ns://127.0.0.1:7420";
        else
            return "unknown";
    }
};

TEST_P(AddComponentsTest, ConnectOnly)
{
    SKIP_TEST_MAC_CI;
    auto server = createServerInstance();
    auto client = createClientInstance();

    auto clientSignals = client.getSignals(search::Recursive(search::Any()));
    for (const auto& signal : clientSignals)
    {
        auto mirroredSignalPtr = signal.asPtr<IMirroredSignalConfig>();
        ASSERT_EQ(mirroredSignalPtr.getStreamingSources().getCount(), getStreamingSourcesCount()) << signal.getGlobalId();
        ASSERT_TRUE(mirroredSignalPtr.getActiveStreamingSource().assigned()) << signal.getGlobalId();
        ASSERT_EQ(mirroredSignalPtr.getActiveStreamingSource(), getActiveStreamingSource()) << signal.getGlobalId();
    }
}

TEST_P(AddComponentsTest, AddFunctionBlock)
{
    SKIP_TEST_MAC_CI;
    auto server = createServerInstance();
    auto client = createClientInstance();

    std::promise<void> addFbPromise;
    std::future<void> addFbFuture = addFbPromise.get_future();

    client.getContext().getOnCoreEvent() +=
        [&](const ComponentPtr& comp, const CoreEventArgsPtr& args)
    {
        auto params = args.getParameters();
        if (static_cast<CoreEventId>(args.getEventId()) == CoreEventId::ComponentAdded)
        {
            ComponentPtr component = params.get("Component");
            if (component.asPtrOrNull<IFunctionBlock>().assigned())
            {
                auto addedFb = component.asPtr<IFunctionBlock>();
                if (addedFb.getFunctionBlockType().getId() == "RefFBModuleStatistics")
                {
                    addFbPromise.set_value();
                }
            }
        }
    };

    const auto serverAddedFb = server.addFunctionBlock("RefFBModuleStatistics");
    ASSERT_TRUE(addFbFuture.wait_for(std::chrono::seconds(1)) == std::future_status::ready);

    auto clientAddedFb = client.getDevices()[0].getFunctionBlocks()[1];
    auto clientAddedFbSignals = clientAddedFb.getSignals(search::Recursive(search::Any()));
    for (const auto& signal : clientAddedFbSignals)
    {
        auto mirroredSignalPtr = signal.asPtr<IMirroredSignalConfig>();
        ASSERT_EQ(mirroredSignalPtr.getStreamingSources().getCount(), getStreamingSourcesCount()) << signal.getGlobalId();
        ASSERT_TRUE(mirroredSignalPtr.getActiveStreamingSource().assigned()) << signal.getGlobalId();
        ASSERT_EQ(mirroredSignalPtr.getActiveStreamingSource(), getActiveStreamingSource()) << signal.getGlobalId();
    }
}

TEST_F(NativeDeviceModulesTest, RemoveFunctionBlock)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    auto serverFb = server.getFunctionBlocks()[0];
    auto clientFb = client.getDevices()[0].getFunctionBlocks()[0];

    StringPtr removedFbId = clientFb.getGlobalId();
    std::promise<void> removeFbPromise;
    std::future<void> removeFbFuture = removeFbPromise.get_future();

    client.getContext().getOnCoreEvent() +=
        [&](const ComponentPtr& comp, const CoreEventArgsPtr& args)
    {
        auto params = args.getParameters();
        if (static_cast<CoreEventId>(args.getEventId()) == CoreEventId::ComponentRemoved)
        {
            StringPtr id = params.get("Id");
            if ((comp.getGlobalId() + "/" + id) == removedFbId)
                removeFbPromise.set_value();
        }
    };

    const auto clientFbSignals = clientFb.getSignals(search::Recursive(search::Any()));
    server.removeFunctionBlock(serverFb);
    ASSERT_TRUE(removeFbFuture.wait_for(std::chrono::seconds(1)) == std::future_status::ready);

    for (const auto& signal : clientFbSignals)
    {
        auto mirroredSignalPtr = signal.asPtr<IMirroredSignalConfig>();
        ASSERT_EQ(mirroredSignalPtr.getStreamingSources().getCount(), 0u) << signal.getGlobalId();
        ASSERT_EQ(mirroredSignalPtr.getActiveStreamingSource(), nullptr) << signal.getGlobalId();
        ASSERT_TRUE(signal.isRemoved());
    }
}

TEST_P(AddComponentsTest, AddChannel)
{
    SKIP_TEST_MAC_CI;
    auto server = createServerInstance();
    auto client = createClientInstance();

    std::promise<void> addChPromise;
    std::future<void> addChFuture = addChPromise.get_future();

    client.getContext().getOnCoreEvent() +=
        [&](const ComponentPtr& comp, const CoreEventArgsPtr& args)
    {
        auto params = args.getParameters();
        if (static_cast<CoreEventId>(args.getEventId()) == CoreEventId::ComponentAdded)
        {
            ComponentPtr component = params.get("Component");
            if (component.asPtrOrNull<IChannel>().assigned())
            {
                auto addedCh = component.asPtr<IChannel>();
                if (addedCh.getFunctionBlockType().getId() == "RefChannel")
                {
                    addChPromise.set_value();
                }
            }
        }
    };

    auto refDevice = client.getDevices()[0].getDevices()[0];
    refDevice.setPropertyValue("NumberOfChannels", 3);
    ASSERT_TRUE(addChFuture.wait_for(std::chrono::seconds(1)) == std::future_status::ready);

    auto clientAddedCh = client.getDevices()[0].getDevices()[0].getChannels()[2];
    auto clientAddedChSignals = clientAddedCh.getSignals(search::Recursive(search::Any()));
    for (const auto& signal : clientAddedChSignals)
    {
        auto mirroredSignalPtr = signal.asPtr<IMirroredSignalConfig>();
        ASSERT_EQ(mirroredSignalPtr.getStreamingSources().getCount(), getStreamingSourcesCount()) << signal.getGlobalId();
        ASSERT_TRUE(mirroredSignalPtr.getActiveStreamingSource().assigned()) << signal.getGlobalId();
        ASSERT_EQ(mirroredSignalPtr.getActiveStreamingSource(), getActiveStreamingSource()) << signal.getGlobalId();
    }
}

TEST_F(NativeDeviceModulesTest, RemoveChannel)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    auto refDevice = client.getDevices()[0].getDevices()[0];
    auto clientCh = refDevice.getChannels()[1];

    StringPtr removedChId = clientCh.getGlobalId();
    std::promise<void> removeChPromise;
    std::future<void> removeChFuture = removeChPromise.get_future();

    client.getContext().getOnCoreEvent() +=
        [&](const ComponentPtr& comp, const CoreEventArgsPtr& args)
    {
        auto params = args.getParameters();
        if (static_cast<CoreEventId>(args.getEventId()) == CoreEventId::ComponentRemoved)
        {
            StringPtr id = params.get("Id");
            if ((comp.getGlobalId() + "/" + id) == removedChId)
                removeChPromise.set_value();
        }
    };

    const auto clientChSignals = clientCh.getSignals(search::Recursive(search::Any()));
    refDevice.setPropertyValue("NumberOfChannels", 1);
    ASSERT_TRUE(removeChFuture.wait_for(std::chrono::seconds(1)) == std::future_status::ready);

    for (const auto& signal : clientChSignals)
    {
        auto mirroredSignalPtr = signal.asPtr<IMirroredSignalConfig>();
        ASSERT_EQ(mirroredSignalPtr.getStreamingSources().getCount(), 0u) << signal.getGlobalId();
        ASSERT_EQ(mirroredSignalPtr.getActiveStreamingSource(), nullptr) << signal.getGlobalId();
        ASSERT_TRUE(signal.isRemoved());
    }
}

TEST_P(AddComponentsTest, AddDevice)
{
    SKIP_TEST_MAC_CI;
    auto server = createServerInstance();
    auto client = createClientInstance();

    std::promise<void> addDevPromise;
    std::future<void> addDevFuture = addDevPromise.get_future();

    client.getContext().getOnCoreEvent() +=
        [&](const ComponentPtr& comp, const CoreEventArgsPtr& args)
    {
        auto params = args.getParameters();
        if (static_cast<CoreEventId>(args.getEventId()) == CoreEventId::ComponentAdded)
        {
            ComponentPtr component = params.get("Component");
            if (component.asPtrOrNull<IDevice>().assigned())
            {
                auto addedDev = component.asPtr<IDevice>();
                if (addedDev.getLocalId() == "RefDev1")
                {
                    addDevPromise.set_value();
                }
            }
        }
    };

    const auto serverAddedDev = server.addDevice("daqref://device1");
    ASSERT_TRUE(addDevFuture.wait_for(std::chrono::seconds(1)) == std::future_status::ready);

    auto clientAddedDev = client.getDevices()[0].getDevices()[1];
    auto clientAddedDevSignals = clientAddedDev.getSignals(search::Recursive(search::Any()));
    for (const auto& signal : clientAddedDevSignals)
    {
        auto mirroredSignalPtr = signal.asPtr<IMirroredSignalConfig>();
        ASSERT_EQ(mirroredSignalPtr.getStreamingSources().getCount(), getStreamingSourcesCount()) << signal.getGlobalId();
        ASSERT_TRUE(mirroredSignalPtr.getActiveStreamingSource().assigned()) << signal.getGlobalId();
        ASSERT_EQ(mirroredSignalPtr.getActiveStreamingSource(), getActiveStreamingSource()) << signal.getGlobalId();
    }
}

TEST_F(NativeDeviceModulesTest, RemoveSubDevice)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    auto serverDevice = server.getDevices()[0];
    auto clientDevice = client.getDevices()[0].getDevices()[0];
    auto clientCh = clientDevice.getChannels()[1];

    StringPtr removedDevId = clientDevice.getGlobalId();
    std::promise<void> removeDevPromise;
    std::future<void> removeDevFuture = removeDevPromise.get_future();

    client.getContext().getOnCoreEvent() +=
        [&](const ComponentPtr& comp, const CoreEventArgsPtr& args)
    {
        auto params = args.getParameters();
        if (static_cast<CoreEventId>(args.getEventId()) == CoreEventId::ComponentRemoved)
        {
            StringPtr id = params.get("Id");
            if ((comp.getGlobalId() + "/" + id) == removedDevId)
                removeDevPromise.set_value();
        }
    };

    const auto clientDevSignals = clientDevice.getSignals(search::Recursive(search::Any()));
    server.removeDevice(serverDevice);
    ASSERT_TRUE(removeDevFuture.wait_for(std::chrono::seconds(1)) == std::future_status::ready);

    for (const auto& signal : clientDevSignals)
    {
        auto mirroredSignalPtr = signal.asPtr<IMirroredSignalConfig>();
        ASSERT_EQ(mirroredSignalPtr.getStreamingSources().getCount(), 0u) << signal.getGlobalId();
        ASSERT_EQ(mirroredSignalPtr.getActiveStreamingSource(), nullptr) << signal.getGlobalId();
        ASSERT_TRUE(signal.isRemoved());
    }
}

INSTANTIATE_TEST_SUITE_P(
    DynamicComponentsTestGroup,
    AddComponentsTest,
    testing::Values(
        std::vector<std::string>({"OpenDAQNativeStreaming"}),
        std::vector<std::string>({"OpenDAQLTStreaming"}),
        std::vector<std::string>({"OpenDAQLTStreaming", "OpenDAQNativeStreaming"}),
        std::vector<std::string>({"OpenDAQNativeStreaming", "OpenDAQLTStreaming"})
    )
);

TEST_F(NativeDeviceModulesTest, SdkPackageVersion)
{
    SKIP_TEST_MAC_CI;
    auto instance = InstanceBuilder().setDefaultRootDeviceInfo(DeviceInfo("", "dev", "custom")).build();
    instance.addServer("OpenDAQNativeStreaming", nullptr);
    auto client = CreateClientInstance();

    ASSERT_EQ(client.getDevices()[0].getInfo().getSdkVersion(),  "custom");
}

TEST_F(NativeDeviceModulesTest, ConfiguringWithOptions)
{
    std::string filename = "opendaq-config.json";
    std::string options = R"(
    {
    "Modules": {
        "OpenDAQNativeStreamingClientModule": {
            "MonitoringEnabled": true,
            "HeartbeatPeriod": 100,
            "InactivityTimeout": 200,
            "ConnectionTimeout": 300,
            "StreamingInitTimeout": 400,
            "ReconnectionPeriod": 500,
            "ProtocolVersion": 6,
            "ConfigProtocolRequestTimeout": 7000,
            "RestoreClientConfigOnReconnect": true
            }
        }
    }
    )";
    
    auto finally = test_helpers::CreateConfigFile(filename, options);

    InstancePtr instance;
    ASSERT_NO_THROW(instance = InstanceBuilder().addConfigProvider(JsonConfigProvider(filename)).build());

    auto deviceConfig = instance.getAvailableDeviceTypes().get("OpenDAQNativeConfiguration").createDefaultConfig();
    ASSERT_EQ(deviceConfig.getPropertyValue("ProtocolVersion"), 6);
    ASSERT_EQ(deviceConfig.getPropertyValue("ConfigProtocolRequestTimeout"), 7000);
    ASSERT_EQ(deviceConfig.getPropertyValue("RestoreClientConfigOnReconnect"), True);
    ASSERT_TRUE(deviceConfig.hasProperty("TransportLayerConfig"));

    PropertyObjectPtr transportLayerConfig = deviceConfig.getPropertyValue("TransportLayerConfig");
    ASSERT_EQ(transportLayerConfig.getPropertyValue("MonitoringEnabled"), True);
    ASSERT_EQ(transportLayerConfig.getPropertyValue("HeartbeatPeriod"), 100);
    ASSERT_EQ(transportLayerConfig.getPropertyValue("InactivityTimeout"), 200);
    ASSERT_EQ(transportLayerConfig.getPropertyValue("ConnectionTimeout"), 300);
    ASSERT_EQ(transportLayerConfig.getPropertyValue("StreamingInitTimeout"), 400);
    ASSERT_EQ(transportLayerConfig.getPropertyValue("ReconnectionPeriod"), 500);

    auto pseudoDeviceConfig = instance.getAvailableDeviceTypes().get("OpenDAQNativeStreaming").createDefaultConfig();

    // the next three is only for native config device and should not be included for streaming pseudo-device
    ASSERT_FALSE(pseudoDeviceConfig.hasProperty("ProtocolVersion"));
    ASSERT_FALSE(pseudoDeviceConfig.hasProperty("ConfigProtocolRequestTimeout"));
    ASSERT_FALSE(pseudoDeviceConfig.hasProperty("RestoreClientConfigOnReconnect"));

    ASSERT_TRUE(pseudoDeviceConfig.hasProperty("TransportLayerConfig"));
    transportLayerConfig = pseudoDeviceConfig.getPropertyValue("TransportLayerConfig");

    ASSERT_EQ(transportLayerConfig.getPropertyValue("MonitoringEnabled"), True);
    ASSERT_EQ(transportLayerConfig.getPropertyValue("HeartbeatPeriod"), 100);
    ASSERT_EQ(transportLayerConfig.getPropertyValue("InactivityTimeout"), 200);
    ASSERT_EQ(transportLayerConfig.getPropertyValue("ConnectionTimeout"), 300);
    ASSERT_EQ(transportLayerConfig.getPropertyValue("StreamingInitTimeout"), 400);
    ASSERT_EQ(transportLayerConfig.getPropertyValue("ReconnectionPeriod"), 500);
}

TEST_F(NativeDeviceModulesTest, Reconnection)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance(std::numeric_limits<uint16_t>::max(), False);

    ASSERT_EQ(client.getDevices()[0].getStatusContainer().getStatus("ConnectionStatus"), "Connected");
    ASSERT_EQ(client.getDevices()[0].getConnectionStatusContainer().getStatus("ConfigurationStatus"), "Connected");
    ASSERT_EQ(client.getDevices()[0].getConnectionStatusContainer().getStatus("StreamingStatus_OpenDAQNativeStreaming_1"), "Connected");

    std::promise<StringPtr> connectionOldStatusPromise;
    std::future<StringPtr> connectionOldStatusFuture = connectionOldStatusPromise.get_future();
    std::promise<StringPtr> configReconnectionStatusPromise;
    std::future<StringPtr> configReconnectionStatusFuture = configReconnectionStatusPromise.get_future();
    std::promise<StringPtr> streamingReconnectionStatusPromise;
    std::future<StringPtr> streamingReconnectionStatusFuture = streamingReconnectionStatusPromise.get_future();
    client.getDevices()[0].getOnComponentCoreEvent() += [&](ComponentPtr& /*comp*/, CoreEventArgsPtr& args)
    {
        auto params = args.getParameters();
        if (static_cast<CoreEventId>(args.getEventId()) == CoreEventId::ConnectionStatusChanged)
        {
            ASSERT_TRUE(args.getParameters().hasKey("StatusName"));
            ASSERT_TRUE(args.getParameters().hasKey("ConnectionString"));
            ASSERT_TRUE(args.getParameters().hasKey("StreamingObject"));
            ASSERT_TRUE(args.getParameters().hasKey("StatusValue"));
            if (args.getParameters().get("StatusName") == "ConfigurationStatus")
            {
                EXPECT_EQ(args.getParameters().get("ConnectionString"), "daq.nd://127.0.0.1");
                EXPECT_FALSE(args.getParameters().get("StreamingObject").assigned());
                configReconnectionStatusPromise.set_value(args.getParameters().get("StatusValue").toString());
            }
            else
            {
                EXPECT_TRUE(args.getParameters().get("StreamingObject").assigned());
                streamingReconnectionStatusPromise.set_value(args.getParameters().get("StatusValue").toString());
            }
        }
        else if (static_cast<CoreEventId>(args.getEventId()) == CoreEventId::StatusChanged)
        {
            ASSERT_TRUE(args.getParameters().hasKey("ConnectionStatus"));
            connectionOldStatusPromise.set_value(args.getParameters().get("ConnectionStatus").toString());
        }
    };

    // destroy server to emulate disconnection
    server.release();
    ASSERT_TRUE(connectionOldStatusFuture.wait_for(std::chrono::seconds(5)) == std::future_status::ready);
    ASSERT_EQ(connectionOldStatusFuture.get(), "Reconnecting");
    ASSERT_EQ(client.getDevices()[0].getStatusContainer().getStatus("ConnectionStatus"), "Reconnecting");
    ASSERT_TRUE(configReconnectionStatusFuture.wait_for(std::chrono::seconds(5)) == std::future_status::ready);
    ASSERT_EQ(configReconnectionStatusFuture.get(), "Reconnecting");
    ASSERT_EQ(client.getDevices()[0].getConnectionStatusContainer().getStatus("ConfigurationStatus"), "Reconnecting");
    ASSERT_TRUE(streamingReconnectionStatusFuture.wait_for(std::chrono::seconds(5)) == std::future_status::ready);
    ASSERT_EQ(streamingReconnectionStatusFuture.get(), "Reconnecting");
    ASSERT_EQ(client.getDevices()[0].getConnectionStatusContainer().getStatus("StreamingStatus_OpenDAQNativeStreaming_1"), "Reconnecting");

    ASSERT_THROW(client.getDevices()[0].getDevices()[0].setPropertyValue("CustomProp", 1), ConnectionLostException);

    // reset future / promise
    connectionOldStatusPromise = std::promise<StringPtr>();
    connectionOldStatusFuture = connectionOldStatusPromise.get_future();
    configReconnectionStatusPromise = std::promise<StringPtr>();
    configReconnectionStatusFuture = configReconnectionStatusPromise.get_future();
    streamingReconnectionStatusPromise = std::promise<StringPtr>();
    streamingReconnectionStatusFuture = streamingReconnectionStatusPromise.get_future();

    // re-create updated server
    server = CreateServerInstance(CreateUpdatedServerInstance());

    ASSERT_TRUE(connectionOldStatusFuture.wait_for(std::chrono::seconds(5)) == std::future_status::ready);
    ASSERT_EQ(connectionOldStatusFuture.get(), "Connected");
    ASSERT_EQ(client.getDevices()[0].getStatusContainer().getStatus("ConnectionStatus"), "Connected");
    ASSERT_TRUE(configReconnectionStatusFuture.wait_for(std::chrono::seconds(5)) == std::future_status::ready);
    ASSERT_EQ(configReconnectionStatusFuture.get(), "Connected");
    ASSERT_EQ(client.getDevices()[0].getConnectionStatusContainer().getStatus("ConfigurationStatus"), "Connected");
    ASSERT_TRUE(streamingReconnectionStatusFuture.wait_for(std::chrono::seconds(5)) == std::future_status::ready);
    ASSERT_EQ(streamingReconnectionStatusFuture.get(), "Connected");
    ASSERT_EQ(client.getDevices()[0].getConnectionStatusContainer().getStatus("StreamingStatus_OpenDAQNativeStreaming_1"), "Connected");

    auto channels = client.getDevices()[0].getChannels(search::Recursive(search::Any()));
    ASSERT_EQ(channels.getCount(), 3u);

    auto fbs = client.getDevices()[0].getFunctionBlocks(search::Recursive(search::Any()));
    ASSERT_EQ(fbs.getCount(), 1u);
    ASSERT_EQ(fbs[0].getFunctionBlockType().getId(), "RefFBModuleScaling");

    ASSERT_TRUE(client.getContext().getTypeManager().hasType("TestEnumType"));

    auto signals = client.getSignals(search::Recursive(search::Any()));
    for (const auto& signal : signals)
    {
        auto mirroredSignalPtr = signal.asPtr<IMirroredSignalConfig>();
        ASSERT_GT(mirroredSignalPtr.getStreamingSources().getCount(), 0u) << signal.getGlobalId();
        ASSERT_TRUE(mirroredSignalPtr.getActiveStreamingSource().assigned()) << signal.getGlobalId();
    }

    auto info = client.getDevices()[0].getInfo();
    ASSERT_TRUE(info.assigned());
    ASSERT_EQ(info.getConnectionString(), "daq.nd://127.0.0.1");
    ASSERT_TRUE(info.hasProperty("NativeConfigProtocolVersion"));
}

TEST_F(NativeDeviceModulesTest, ReconnectionRestoreClientConfig)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance(std::numeric_limits<uint16_t>::max(), True);

    ASSERT_EQ(client.getDevices()[0].getConnectionStatusContainer().getStatus("ConfigurationStatus"), "Connected");

    std::promise<StringPtr> reconnectionStatusPromise;
    std::future<StringPtr> reconnectionStatusFuture = reconnectionStatusPromise.get_future();
    client.getDevices()[0].getOnComponentCoreEvent() += [&](ComponentPtr& /*comp*/, CoreEventArgsPtr& args)
    {
        auto params = args.getParameters();
        if (static_cast<CoreEventId>(args.getEventId()) == CoreEventId::ConnectionStatusChanged)
        {
            ASSERT_TRUE(args.getParameters().hasKey("StatusName"));
            if (args.getParameters().get("StatusName") == "ConfigurationStatus")
            {
                ASSERT_TRUE(args.getParameters().hasKey("ConnectionString"));
                EXPECT_EQ(args.getParameters().get("ConnectionString"), "daq.nd://127.0.0.1");
                ASSERT_TRUE(args.getParameters().hasKey("StreamingObject"));
                EXPECT_EQ(args.getParameters().get("StreamingObject"), nullptr);
                ASSERT_TRUE(args.getParameters().hasKey("StatusValue"));
                reconnectionStatusPromise.set_value(args.getParameters().get("StatusValue").toString());
            }
        }
    };

    // destroy server to emulate disconnection
    server.release();
    ASSERT_TRUE(reconnectionStatusFuture.wait_for(std::chrono::seconds(5)) == std::future_status::ready);
    ASSERT_EQ(reconnectionStatusFuture.get(), "Reconnecting");
    ASSERT_EQ(client.getDevices()[0].getConnectionStatusContainer().getStatus("ConfigurationStatus"), "Reconnecting");

    ASSERT_THROW(client.getDevices()[0].getDevices()[0].setPropertyValue("CustomProp", 1), ConnectionLostException);

    // reset future / promise
    reconnectionStatusPromise = std::promise<StringPtr>();
    reconnectionStatusFuture = reconnectionStatusPromise.get_future();

    // re-create updated server
    server = CreateServerInstance(CreateUpdatedServerInstance());

    ASSERT_TRUE(reconnectionStatusFuture.wait_for(std::chrono::seconds(5)) == std::future_status::ready);
    ASSERT_EQ(reconnectionStatusFuture.get(), "Connected");
    ASSERT_EQ(client.getDevices()[0].getConnectionStatusContainer().getStatus("ConfigurationStatus"), "Connected");

    const auto testedDevices = List<IDevice>(client.getDevices()[0], server);
    for(const auto& testDevice : testedDevices)
    {
        auto signals = testDevice.getSignals(search::Recursive(search::Any()));
        ASSERT_EQ(signals.getCount(), 7u);
        auto signalsVisible = testDevice.getSignals(search::Recursive(search::Visible()));
        ASSERT_EQ(signalsVisible.getCount(), 4u);
        auto devices = testDevice.getDevices();
        ASSERT_EQ(devices.getCount(), 1u);
        auto fbs = testDevice.getFunctionBlocks();
        ASSERT_EQ(fbs.getCount(), 1u);
        ASSERT_EQ(fbs[0].getFunctionBlockType().getId(), "RefFBModuleStatistics");
        auto channels = testDevice.getChannels(search::Recursive(search::Any()));
        ASSERT_EQ(channels.getCount(), 2u);
        auto servers = testDevice.getServers();
        ASSERT_EQ(servers.getCount(), 1u);
    }

    auto signals = client.getSignals(search::Recursive(search::Any()));
    for (const auto& signal : signals)
    {
        auto mirroredSignalPtr = signal.asPtr<IMirroredSignalConfig>();
        ASSERT_GT(mirroredSignalPtr.getStreamingSources().getCount(), 0u) << signal.getGlobalId();
        ASSERT_TRUE(mirroredSignalPtr.getActiveStreamingSource().assigned()) << signal.getGlobalId();
    }

    auto info = client.getDevices()[0].getInfo();
    ASSERT_TRUE(info.assigned());
    ASSERT_EQ(info.getConnectionString(), "daq.nd://127.0.0.1");
    ASSERT_TRUE(info.hasProperty("NativeConfigProtocolVersion"));
}

TEST_F(NativeDeviceModulesTest, Update)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    std::promise<void> updatedPromise;
    std::future<void> updatedFuture = updatedPromise.get_future();

    client.getContext().getOnCoreEvent() +=
        [&](const ComponentPtr& component, const CoreEventArgsPtr& args)
    {
        auto params = args.getParameters();
        if (static_cast<CoreEventId>(args.getEventId()) == CoreEventId::ComponentUpdateEnd)
        {
            updatedPromise.set_value();
        }
    };

    // update device
    auto updatableDevice = client.getDevices()[0].getDevices()[0];
    const auto serializer = JsonSerializer();
    updatableDevice.serialize(serializer);
    const auto str = serializer.getOutput();
    const auto deserializer = JsonDeserializer();
    deserializer.update(updatableDevice, str);

    ASSERT_TRUE(updatedFuture.wait_for(std::chrono::seconds(1)) == std::future_status::ready);

    auto clientSignals = client.getSignals(search::Recursive(search::Any()));
    for (const auto& signal : clientSignals)
    {
        auto mirroredSignalPtr = signal.asPtr<IMirroredSignalConfig>();
        ASSERT_GT(mirroredSignalPtr.getStreamingSources().getCount(), 0u) << signal.getGlobalId();
        ASSERT_TRUE(mirroredSignalPtr.getActiveStreamingSource().assigned()) << signal.getGlobalId();
    }
}

TEST_F(NativeDeviceModulesTest, GetConfigurationConnectionInfo)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    auto devices = client.getDevices();
    ASSERT_EQ(devices.getCount(), 1u);

    auto connectionInfo = devices[0].getInfo().getConfigurationConnectionInfo();
    ASSERT_EQ(connectionInfo.getProtocolId(), "OpenDAQNativeConfiguration");
    ASSERT_EQ(connectionInfo.getProtocolName(), "OpenDAQNativeConfiguration");
    ASSERT_EQ(connectionInfo.getProtocolType(), ProtocolType::ConfigurationAndStreaming);
    ASSERT_EQ(connectionInfo.getConnectionType(), "TCP/IP");
    ASSERT_EQ(connectionInfo.getAddresses()[0], "127.0.0.1");
    ASSERT_EQ(connectionInfo.getPort(), 7420);
    ASSERT_EQ(connectionInfo.getPrefix(), "daq.nd");
    ASSERT_EQ(connectionInfo.getConnectionString(), "daq.nd://127.0.0.1");
}

TEST_F(NativeDeviceModulesTest, TestAddressInfoIPv4)
{
    auto server = InstanceBuilder().setRootDevice("daqref://device0").build();
    server.addServer("OpenDAQNativeStreaming", nullptr);
    server.addServer("OpenDAQLTStreaming", nullptr);
    server.addServer("OpenDAQOPCUA", nullptr);

    auto client = Instance();
    const auto dev = client.addDevice("daq.nd://127.0.0.1");
    const auto info = dev.getInfo();

    ASSERT_TRUE(info.hasServerCapability("OpenDAQNativeConfiguration"));
    ASSERT_TRUE(info.hasServerCapability("OpenDAQOPCUAConfiguration"));
    ASSERT_TRUE(info.hasServerCapability("OpenDAQNativeStreaming"));
    ASSERT_TRUE(info.hasServerCapability("OpenDAQLTStreaming"));

    const auto opcuaCapability = info.getServerCapability("OpenDAQOPCUAConfiguration");
    const auto nativeConfigCapability = info.getServerCapability("OpenDAQNativeConfiguration");
    const auto nativeStreamingCapability = info.getServerCapability("OpenDAQNativeStreaming");
    const auto LTCapability = info.getServerCapability("OpenDAQLTStreaming");

    ASSERT_TRUE(opcuaCapability.getConnectionString().assigned() && opcuaCapability.getConnectionString() != "");
    ASSERT_TRUE(nativeConfigCapability.getConnectionString().assigned() && nativeConfigCapability.getConnectionString() != "");
    ASSERT_TRUE(nativeStreamingCapability.getConnectionString().assigned() && nativeStreamingCapability.getConnectionString() != "");
    ASSERT_TRUE(LTCapability.getConnectionString().assigned() && LTCapability.getConnectionString() != "");

    const auto opcuaAddressInfo = opcuaCapability.getAddressInfo()[0];
    const auto nativeConfigAddressInfo= nativeConfigCapability.getAddressInfo()[0];
    const auto nativeStreamingAddressInfo = nativeStreamingCapability.getAddressInfo()[0];
    const auto LTAddressInfo = LTCapability.getAddressInfo()[0];

    ASSERT_EQ(opcuaAddressInfo.getType(), "IPv4");
    ASSERT_EQ(nativeConfigAddressInfo.getType(), "IPv4");
    ASSERT_EQ(nativeStreamingAddressInfo.getType(), "IPv4");
    ASSERT_EQ(LTAddressInfo.getType(), "IPv4");

    ASSERT_EQ(opcuaAddressInfo.getReachabilityStatus(), AddressReachabilityStatus::Reachable);
    ASSERT_EQ(nativeConfigAddressInfo.getReachabilityStatus(), AddressReachabilityStatus::Reachable);
    ASSERT_EQ(nativeStreamingAddressInfo.getReachabilityStatus(), AddressReachabilityStatus::Reachable);
    ASSERT_EQ(LTAddressInfo.getReachabilityStatus(), AddressReachabilityStatus::Reachable);
}

TEST_F(NativeDeviceModulesTest, TestAddressInfoIPv6)
{
    if (test_helpers::Ipv6IsDisabled())
        return;

    auto server = InstanceBuilder().setRootDevice("daqref://device0").build();
    server.addServer("OpenDAQNativeStreaming", nullptr);
    server.addServer("OpenDAQOPCUA", nullptr);
    server.addServer("OpenDAQLTStreaming", nullptr);

    auto client = Instance();
    const auto dev = client.addDevice("daq.nd://[::1]");
    const auto info = dev.getInfo();

    ASSERT_TRUE(info.hasServerCapability("OpenDAQNativeConfiguration"));
    ASSERT_TRUE(info.hasServerCapability("OpenDAQOPCUAConfiguration"));
    ASSERT_TRUE(info.hasServerCapability("OpenDAQNativeStreaming"));
    ASSERT_TRUE(info.hasServerCapability("OpenDAQLTStreaming"));

    const auto opcuaCapability = info.getServerCapability("OpenDAQOPCUAConfiguration");
    const auto nativeConfigCapability = info.getServerCapability("OpenDAQNativeConfiguration");
    const auto nativeStreamingCapability = info.getServerCapability("OpenDAQNativeStreaming");
    const auto LTCapability = info.getServerCapability("OpenDAQLTStreaming");

    ASSERT_TRUE(opcuaCapability.getConnectionString().assigned() && opcuaCapability.getConnectionString() != "");
    ASSERT_TRUE(nativeConfigCapability.getConnectionString().assigned() && nativeConfigCapability.getConnectionString() != "");
    ASSERT_TRUE(nativeStreamingCapability.getConnectionString().assigned() && nativeStreamingCapability.getConnectionString() != "");
    ASSERT_TRUE(LTCapability.getConnectionString().assigned() && LTCapability.getConnectionString() != "");

    const auto opcuaAddressInfo = opcuaCapability.getAddressInfo()[0];
    const auto nativeConfigAddressInfo= nativeConfigCapability.getAddressInfo()[0];
    const auto nativeStreamingAddressInfo = nativeStreamingCapability.getAddressInfo()[0];
    const auto LTAddressInfo = LTCapability.getAddressInfo()[0];

    ASSERT_EQ(opcuaAddressInfo.getType(), "IPv6");
    ASSERT_EQ(nativeConfigAddressInfo.getType(), "IPv6");
    ASSERT_EQ(nativeStreamingAddressInfo.getType(), "IPv6");
    ASSERT_EQ(LTAddressInfo.getType(), "IPv6");

    ASSERT_EQ(opcuaAddressInfo.getReachabilityStatus(), AddressReachabilityStatus::Reachable);
    ASSERT_EQ(nativeConfigAddressInfo.getReachabilityStatus(), AddressReachabilityStatus::Reachable);
    ASSERT_EQ(nativeStreamingAddressInfo.getReachabilityStatus(), AddressReachabilityStatus::Reachable);
    ASSERT_EQ(LTAddressInfo.getReachabilityStatus(), AddressReachabilityStatus::Reachable);
}

TEST_F(NativeDeviceModulesTest, TestAddressInfoGatewayDevice)
{
    auto server = InstanceBuilder().setRootDevice("daqref://device0").build();
    server.addServer("OpenDAQNativeStreaming", nullptr);
    server.addServer("OpenDAQLTStreaming", nullptr);
    server.addServer("OpenDAQOPCUA", nullptr);

    auto gateway = Instance();
    auto serverConfig = gateway.getAvailableServerTypes().get("OpenDAQNativeStreaming").createDefaultConfig();
    serverConfig.setPropertyValue("NativeStreamingPort", 7421);
    gateway.addDevice("daq.nd://127.0.0.1");
    gateway.addServer("OpenDAQNativeStreaming", serverConfig);

    auto client = Instance();
    const auto dev = client.addDevice("daq.nd://127.0.0.1:7421/");
    const auto info = dev.getDevices()[0].getInfo();

    ASSERT_TRUE(info.hasServerCapability("OpenDAQNativeConfiguration"));
    ASSERT_TRUE(info.hasServerCapability("OpenDAQOPCUAConfiguration"));
    ASSERT_TRUE(info.hasServerCapability("OpenDAQNativeStreaming"));
    ASSERT_TRUE(info.hasServerCapability("OpenDAQLTStreaming"));

    const auto opcuaCapability = info.getServerCapability("OpenDAQOPCUAConfiguration");
    const auto nativeConfigCapability = info.getServerCapability("OpenDAQNativeConfiguration");
    const auto nativeStreamingCapability = info.getServerCapability("OpenDAQNativeStreaming");
    const auto LTCapability = info.getServerCapability("OpenDAQLTStreaming");

    ASSERT_TRUE(opcuaCapability.getConnectionString().assigned() && opcuaCapability.getConnectionString() != "");
    ASSERT_TRUE(nativeConfigCapability.getConnectionString().assigned() && nativeConfigCapability.getConnectionString() != "");
    ASSERT_TRUE(nativeStreamingCapability.getConnectionString().assigned() && nativeStreamingCapability.getConnectionString() != "");
    ASSERT_TRUE(LTCapability.getConnectionString().assigned() && LTCapability.getConnectionString() != "");

    const auto opcuaAddressInfo = opcuaCapability.getAddressInfo()[0];
    const auto nativeConfigAddressInfo= nativeConfigCapability.getAddressInfo()[0];
    const auto nativeStreamingAddressInfo = nativeStreamingCapability.getAddressInfo()[0];
    const auto LTAddressInfo = LTCapability.getAddressInfo()[0];

    ASSERT_EQ(opcuaAddressInfo.getType(), "IPv4");
    ASSERT_EQ(nativeConfigAddressInfo.getType(), "IPv4");
    ASSERT_EQ(nativeStreamingAddressInfo.getType(), "IPv4");
    ASSERT_EQ(LTAddressInfo.getType(), "IPv4");

    ASSERT_EQ(opcuaAddressInfo.getReachabilityStatus(), AddressReachabilityStatus::Reachable);
    ASSERT_EQ(nativeConfigAddressInfo.getReachabilityStatus(), AddressReachabilityStatus::Reachable);
    ASSERT_EQ(nativeStreamingAddressInfo.getReachabilityStatus(), AddressReachabilityStatus::Reachable);
    ASSERT_EQ(LTAddressInfo.getReachabilityStatus(), AddressReachabilityStatus::Reachable);
}

TEST_F(NativeDeviceModulesTest, MultiClientReadChangingSignal)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();

    // Set up first client
    auto client1 = CreateClientInstance();
    auto client1Signal = client1.getDevices()[0].getDevices()[0].getSignalsRecursive()[0].template asPtr<IMirroredSignalConfig>();

    std::promise<StringPtr> client1SignalSubscribePromise;
    std::future<StringPtr> client1SignalSubscribeFuture;
    test_helpers::setupSubscribeAckHandler(client1SignalSubscribePromise, client1SignalSubscribeFuture, client1Signal);

    StreamReaderPtr client1Reader = daq::StreamReader<double, uint64_t>(client1Signal, ReadTimeoutType::Any);
    ASSERT_TRUE(test_helpers::waitForAcknowledgement(client1SignalSubscribeFuture));

    // change samplerate to trigger signal changes
    auto serverRefDevice = server.getDevices()[0];
    serverRefDevice.setPropertyValue("GlobalSampleRate", 2000);

    // Set up second client
    auto client2 = CreateClientInstance();
    auto client2Signal = client2.getDevices()[0].getDevices()[0].getSignalsRecursive()[0].template asPtr<IMirroredSignalConfig>();

    std::promise<StringPtr> client2SignalSubscribePromise;
    std::future<StringPtr> client2SignalSubscribeFuture;
    test_helpers::setupSubscribeAckHandler(client2SignalSubscribePromise, client2SignalSubscribeFuture, client2Signal);

    StreamReaderPtr client2Reader = daq::StreamReader<double, uint64_t>(client2Signal, ReadTimeoutType::Any);
    ASSERT_TRUE(test_helpers::waitForAcknowledgement(client2SignalSubscribeFuture));

    {
        SizeT count = 0;
        client1Reader.read(nullptr, &count, 100);
    }
    {
        SizeT count = 0;
        client2Reader.read(nullptr, &count, 100);
    }

    // read some
    double samples[100];
    for (int i = 0; i < 5; ++i)
    {
        daq::SizeT count = 100;
        client1Reader.read(samples, &count, 1000);
        //EXPECT_GT(count, 0u) << "iteration " << i;

        count = 100;
        client2Reader.read(samples, &count, 1000);
        //EXPECT_GT(count, 0u) << "iteration " << i;
    }
}

TEST_F(NativeDeviceModulesTest, ReadLastValue)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    const auto clientSignal = client.getDevices()[0].getDevices()[0].getSignalsRecursive()[0];
    BaseObjectPtr value;
    int cnt = 0;
    while (!value.assigned())
    {
        ASSERT_NO_THROW(value = clientSignal.getLastValue());
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        ASSERT_LT(cnt++, 10);
    }

    const auto ip = InputPort(client.getContext(), nullptr, "ip");
    ip.connect(clientSignal);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    ASSERT_NO_THROW(value = clientSignal.getLastValue());
    ASSERT_TRUE(value.assigned());
}

class MockNativeModule : public Module
{
public:
    MockNativeModule(const ContextPtr& context)
        : Module("mock", VersionInfo(0, 0, 0), context, "MockModule")
    {
    }

    ListPtr<IDeviceInfo> onGetAvailableDevices() override
    {
        std::vector<StringPtr> addresses{"123.123.123.123", "234.234.234.234", "127.0.0.1"};
        const auto info = DeviceInfo("daq.nd://127.0.0.1:7420/");
        info.setSerialNumber("DevSer0");
        info.setManufacturer("openDAQ");
        info.setName("Mock Reference Device");

        const auto streamingCap = ServerCapability("OpenDAQNativeStreaming", "OpenDAQNativeStreaming", ProtocolType::Streaming)
                                  .setPrefix("daq.ns")
                                  .setConnectionType("TCP/IP")
                                  .setPort(7420);
        const auto configCap = ServerCapability("OpenDAQNativeConfiguration", "OpenDAQNativeConfiguration", ProtocolType::ConfigurationAndStreaming)
                               .setPrefix("daq.nd")
                               .setConnectionType("TCP/IP")
                               .setPort(7420);

        const std::vector caps{streamingCap, configCap};
        for (const auto& cap : caps)
        {
            for (const auto& address : addresses)
            {
                const auto addressInfo = AddressInfoBuilder().setAddress(address)
                                                             .setReachabilityStatus(AddressReachabilityStatus::Reachable)
                                                             .setType("IPv4")
                                                             .setConnectionString(cap.getPrefix() + "://" + address + ":7420/")
                                                             .build();
                cap.addAddress(address).addConnectionString(cap.getPrefix() + "://" + address + ":7420/").addAddressInfo(addressInfo);
            }

            info.asPtr<IDeviceInfoInternal>().addServerCapability(cap);
        }

        return List<IDeviceInfo>(info);
    }
};

TEST_F(NativeDeviceModulesTest, SameStreamingAddress)
{
    SKIP_TEST_MAC_CI;

    const auto server = Instance();
    server.setRootDevice("daqref://device0");
    server.addServer("OpenDAQNativeStreaming", nullptr);

    const auto client = Instance();
    const auto _module = createWithImplementation<IModule, MockNativeModule>(client.getContext());
    client.getModuleManager().addModule(_module);

    client.getAvailableDevices();
    const MirroredDeviceConfigPtr dev = client.addDevice("daq.nd://127.0.0.1:7420/");
    const auto sources = dev.getStreamingSources();
    ASSERT_EQ(sources.getCount(), 1);
    ASSERT_EQ(sources[0].getConnectionString(), "daq.ns://127.0.0.1:7420/");
}

TEST_F(NativeDeviceModulesTest, LimitConfigConnections)
{
    SKIP_TEST_MAC_CI;

    const auto server = Instance();
    auto ns_config = server.getAvailableServerTypes().get("OpenDAQNativeStreaming").createDefaultConfig();
    ns_config.setPropertyValue("MaxAllowedConfigConnections", 1);
    server.addServer("OpenDAQNativeStreaming", ns_config);

    // Establish the first client connection, within the allowed limit
    const auto client1 = Instance();
    const MirroredDeviceConfigPtr dev = client1.addDevice("daq.nd://127.0.0.1");

    // Attempt to establish a second connection exceeding the limit
    const auto client2 = Instance();
    ASSERT_THROW(client2.addDevice("daq.nd://127.0.0.1"), ConnectionLimitReachedException);

    // Disconnect the first client, reducing the number of active connections
    client1.removeDevice(dev);

    // Give the server some time to process the disconnection
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Reattempt the second connection, now under the allowed limit
    ASSERT_NO_THROW(client2.addDevice("daq.nd://127.0.0.1"));

    // Attempt to reconnect the first client, exceeding the limit again
    ASSERT_THROW(client1.addDevice("daq.nd://127.0.0.1"), ConnectionLimitReachedException);

    // Attempt to establish a streaming connection which is not limited
    ASSERT_NO_THROW(client1.addDevice("daq.ns://127.0.0.1"));
}

TEST_F(NativeDeviceModulesTest, ClientSaveLoadConfiguration)
{
    StringPtr config;

    {
        auto server = CreateServerInstance();
        auto client = CreateClientInstance(0);
        auto clientFb = client.addFunctionBlock("RefFBModuleStatistics");
        clientFb.addFunctionBlock("RefFBModuleTrigger");

        config = client.saveConfiguration();
    }

    auto server = CreateServerInstance();
    auto restoredClient = Instance();
    restoredClient.loadConfiguration(config);
    auto restoredFb = restoredClient.getFunctionBlocks();
    ASSERT_EQ(restoredFb.getCount(), 1u);
    ASSERT_EQ(restoredFb[0].getFunctionBlockType().getId(), "RefFBModuleStatistics");
    auto nestedFb = restoredFb[0].getFunctionBlocks();
    ASSERT_EQ(nestedFb.getCount(), 1u);
    ASSERT_EQ(nestedFb[0].getFunctionBlockType().getId(), "RefFBModuleTrigger");

    auto signals = restoredClient.getDevices()[0].getSignals(search::Recursive(search::Any()));
    for (const auto& signal : signals)
    {
        auto mirroredSignalPtr = signal.asPtr<IMirroredSignalConfig>();
        ASSERT_GT(mirroredSignalPtr.getStreamingSources().getCount(), 0u) << signal.getGlobalId();
        ASSERT_TRUE(mirroredSignalPtr.getActiveStreamingSource().assigned()) << signal.getGlobalId();
    }

    auto info = restoredClient.getDevices()[0].getInfo();
    ASSERT_TRUE(info.assigned());
    ASSERT_EQ(info.getConnectionString(), "daq.nd://127.0.0.1");
    ASSERT_TRUE(info.hasProperty("NativeConfigProtocolVersion"));
}

TEST_F(NativeDeviceModulesTest, ClientSaveLoadConfiguration2)
{
    StringPtr config;
    auto server = CreateServerInstance();

    {
        auto client = CreateClientInstance();
        auto clientFb = client.addFunctionBlock("RefFBModuleStatistics");
        clientFb.addFunctionBlock("RefFBModuleTrigger");

        config = client.saveConfiguration();
    }

    auto restoredClient = Instance();
    restoredClient.addFunctionBlock("RefFBModuleStatistics");

    restoredClient.loadConfiguration(config);
    auto restoredFb = restoredClient.getFunctionBlocks();
    ASSERT_EQ(restoredFb.getCount(), 1u);
    ASSERT_EQ(restoredFb[0].getFunctionBlockType().getId(), "RefFBModuleStatistics");
    auto nestedFb = restoredFb[0].getFunctionBlocks();
    ASSERT_EQ(nestedFb.getCount(), 1u);
    ASSERT_EQ(nestedFb[0].getFunctionBlockType().getId(), "RefFBModuleTrigger");
}

TEST_F(NativeDeviceModulesTest, ClientSaveLoadConfiguration3)
{
    StringPtr config;
    auto server = CreateServerInstance();

    {
        auto client = CreateClientInstance();
        auto clientFb = client.addFunctionBlock("RefFBModuleStatistics");
        clientFb.addFunctionBlock("RefFBModuleTrigger");

        config = client.saveConfiguration();
    }
    
    auto restoredClient = Instance();
    auto fb = restoredClient.addFunctionBlock("RefFBModuleStatistics");
    fb.addFunctionBlock("RefFBModuleTrigger");

    restoredClient.loadConfiguration(config);
    auto restoredFb = restoredClient.getFunctionBlocks();
    ASSERT_EQ(restoredFb.getCount(), 1u);
    ASSERT_EQ(restoredFb[0].getFunctionBlockType().getId(), "RefFBModuleStatistics");
    auto nestedFb = restoredFb[0].getFunctionBlocks();
    ASSERT_EQ(nestedFb.getCount(), 1u);
    ASSERT_EQ(nestedFb[0].getFunctionBlockType().getId(), "RefFBModuleTrigger");
}

TEST_F(NativeDeviceModulesTest, ClientSaveLoadConfigurationWithAnotherDevice)
{
    StringPtr config;
    auto server = CreateServerInstance();

    {
        auto client = CreateClientInstance();
        config = client.saveConfiguration();
    }
    
    auto restoredClient = Instance();
    restoredClient.addDevice("daqref://device0");

    ASSERT_NO_THROW(restoredClient.loadConfiguration(config));

    auto devices = restoredClient.getDevices();
    ASSERT_EQ(devices.getCount(), 2u);
    ASSERT_EQ(devices[0].getInfo().getConnectionString(), "daqref://device0");
    ASSERT_EQ(devices[1].getInfo().getConnectionString(), "daq.nd://127.0.0.1");
    auto serverDevices = devices[1].getDevices();
    ASSERT_EQ(serverDevices.getCount(), 1u);
    ASSERT_EQ(serverDevices[0].getInfo().getConnectionString(), "daqref://device0");
}


InstancePtr CreateServerInstanceWithEnabledLogFileInfo(const StringPtr& loggerPath)
{
    PropertyObjectPtr config = PropertyObject();
    config.addProperty(BoolProperty("EnableLogging", true));
    config.addProperty(StringProperty("LoggingPath", loggerPath));
    config.addProperty(StringProperty("SerialNumber", "NativeDeviceModulesTestSerial"));

    auto instance = InstanceBuilder().setLogger(Logger(loggerPath))
                                     .setRootDevice("daqref://device0", config)
                                     .build();
    
    instance.addServer("OpenDAQNativeStreaming", nullptr);
    return instance;
}

TEST_F(NativeDeviceModulesTest, ClientSaveLoadRestoreServerConfiguration)
{
    StringPtr config;

    {
        auto server = CreateServerInstanceWithEnabledLogFileInfo("native_ref_device.log");
        auto client = CreateClientInstance();
        auto clientRoot = client.getDevices()[0];
        auto fb = clientRoot.addFunctionBlock("RefFBModuleStatistics");
        fb.getInputPorts()[0].connect(clientRoot.getSignals(search::Recursive(search::Visible()))[0]);
        config = client.saveConfiguration();
    }

    auto server = CreateServerInstanceWithEnabledLogFileInfo("native_ref_device.log");
    
    auto restoredClient = Instance();
    ASSERT_NO_THROW(restoredClient.loadConfiguration(config));

    auto devices = restoredClient.getDevices();
    ASSERT_EQ(devices.getCount(), 1u);
    auto clientRoot = devices[0];
    ASSERT_EQ(clientRoot.getFunctionBlocks().getCount(), 1u);

    auto fb = clientRoot.getFunctionBlocks()[0];
    ASSERT_EQ(fb.getFunctionBlockType().getId(), "RefFBModuleStatistics");

    auto signal = fb.getInputPorts()[0].getSignal();
    ASSERT_TRUE(signal.assigned());
    ASSERT_EQ(signal.getGlobalId(), clientRoot.getSignals(search::Recursive(search::Visible()))[0].getGlobalId());
}

TEST_F(NativeDeviceModulesTest, ClientSaveLoadRestoreClientConnectedToServer)
{
    StringPtr config;

    {
        auto server = CreateServerInstanceWithEnabledLogFileInfo("native_ref_device.log");
        auto client = CreateClientInstance();
        auto clientRoot = client.getDevices()[0];
        auto fb = client.addFunctionBlock("RefFBModuleStatistics");
        fb.getInputPorts()[0].connect(clientRoot.getSignals(search::Recursive(search::Visible()))[0]);
        config = client.saveConfiguration();
    }

    auto server = CreateServerInstanceWithEnabledLogFileInfo("native_ref_device.log");
    
    auto restoredClient = Instance();
    ASSERT_NO_THROW(restoredClient.loadConfiguration(config));

    auto devices = restoredClient.getDevices();
    ASSERT_EQ(devices.getCount(), 1u);
    auto clientRoot = devices[0];

    ASSERT_EQ(restoredClient.getFunctionBlocks().getCount(), 1u);

    auto fb = restoredClient.getFunctionBlocks()[0];
    ASSERT_EQ(fb.getFunctionBlockType().getId(), "RefFBModuleStatistics");

    auto signal = fb.getInputPorts()[0].getSignal();
    ASSERT_TRUE(signal.assigned());
    ASSERT_EQ(signal.getGlobalId(), clientRoot.getSignals(search::Recursive(search::Visible()))[0].getGlobalId());
}

TEST_F(NativeDeviceModulesTest, DISABLED_lientSaveLoadRestoreServerConnectedToClient)
{
    StringPtr config;
    {
        auto server = CreateServerInstanceWithEnabledLogFileInfo("native_ref_device.log");
        auto client = CreateClientInstance();
        auto clientRefDevice = client.addDevice("daqref://device1");
        auto clientRoot = client.getDevices()[0];
        auto fb = clientRoot.addFunctionBlock("RefFBModuleStatistics");
        fb.getInputPorts()[0].connect(clientRefDevice.getSignals(search::Recursive(search::Visible()))[0]);
        config = client.saveConfiguration();
    }

    auto server = CreateServerInstanceWithEnabledLogFileInfo("native_ref_device.log");
    
    auto restoredClient = Instance();
    ASSERT_NO_THROW(restoredClient.loadConfiguration(config));

    auto devices = restoredClient.getDevices();
    ASSERT_EQ(devices.getCount(), 2u);
    DevicePtr clientRoot;
    DevicePtr clientRefDevice;
    for (const auto& dev : devices)
    {
        if (dev.getInfo().getConnectionString() == "daqref://device1")
            clientRefDevice = dev;
        else
            clientRoot = dev;
    }
    ASSERT_TRUE(clientRoot.assigned());
    ASSERT_TRUE(clientRefDevice.assigned());
    
    ASSERT_EQ(clientRoot.getFunctionBlocks().getCount(), 1u);

    auto fb = clientRoot.getFunctionBlocks()[0];
    ASSERT_EQ(fb.getFunctionBlockType().getId(), "RefFBModuleStatistics");

    auto signal = fb.getInputPorts()[0].getSignal();
    ASSERT_TRUE(signal.assigned());
    ASSERT_EQ(signal.getGlobalId(), clientRefDevice.getSignals(search::Recursive(search::Visible()))[0].getGlobalId());
}

StringPtr getFileLastModifiedTime(const std::string& path)
{
    auto ftime = fs::last_write_time(path);
    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
    );
    std::time_t cftime = std::chrono::system_clock::to_time_t(sctp);

    std::ostringstream oss;
    oss << std::put_time(std::gmtime(&cftime), "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

TEST_F(NativeDeviceModulesTest, GetAvailableLogFileInfos)
{
    StringPtr loggerPath = "native_ref_device.log";
    auto server = CreateServerInstanceWithEnabledLogFileInfo(loggerPath);
    auto client = CreateClientInstance();
    auto clientDevice = client.getDevices()[0];

    {
        auto logFiles = clientDevice.getLogFileInfos();
        auto logFileLastModified = getFileLastModifiedTime(loggerPath);
        ASSERT_EQ(logFiles.getCount(), 1u);
        auto logFile = logFiles[0];

        ASSERT_EQ(logFile.getName(), loggerPath);
        ASSERT_NE(logFile.getSize(), 0);
        ASSERT_EQ(logFile.getLastModified(), logFileLastModified);

        StringPtr firstSymb = clientDevice.getLog(loggerPath, 1, 0);
        ASSERT_EQ(firstSymb, "[");
    }

    {
        clientDevice.setPropertyValue("EnableLogging", false);
        auto logFiles = clientDevice.getLogFileInfos();
        ASSERT_EQ(logFiles.getCount(), 0u);
    }

    {
        clientDevice.setPropertyValue("EnableLogging", true);
        auto logFiles = clientDevice.getLogFileInfos();
        ASSERT_EQ(logFiles.getCount(), 1u);

        StringPtr firstSymb = clientDevice.getLog(loggerPath, 1, 0);
        ASSERT_EQ(firstSymb, "[");
    }
}

InstancePtr CreateServerSimulator(const StringPtr& name)
{
    PropertyObjectPtr config = PropertyObject();
    config.addProperty(StringProperty("Name", name));
    config.addProperty(StringProperty("SerialNumber", name));

    auto instance = InstanceBuilder().setRootDevice("daqref://device0", config)
                                     .addDiscoveryServer("mdns")
                                     .build();

    instance.addDevice("daqref://device1");
    instance.addServer("OpenDAQNativeStreaming", nullptr)
            .enableDiscovery();
    return instance;
}

InstancePtr CreateClientConnectedToSimulator(const StringPtr& name)
{
    auto instance = Instance();
    for (const auto & devInfo : instance.getAvailableDevices())
    {
        if (devInfo.getName() == name)
        {
            instance.addDevice(devInfo.getConnectionString());
            return instance;
        }
    }
    return nullptr;
}

TEST_F(NativeDeviceModulesTest, GetAvailableDevicesCheck)
{
    StringPtr name = "AvailableDevicesCheck";
    auto server = CreateServerSimulator(name);
    auto client = CreateClientConnectedToSimulator(name);
    ASSERT_TRUE(client.assigned());
    auto clientDevice = client.getDevices()[0];
    auto availableDevices = clientDevice.getAvailableDevices();

    // if server discovered itself, it should should have server capabilities of itself with address info
    for (const auto & devInfo : availableDevices)
    {
        if (devInfo.getName() == name)
        {
            auto capabilities = devInfo.getServerCapabilities();
            ASSERT_GT(capabilities.getCount(), 0u);
            for (const auto & cap : capabilities)
            {
                ASSERT_GT(cap.getAddressInfo().getCount(), 0u);
            }
        }
    }
}

using NativeC2DStreamingTest = testing::Test;

TEST_F(NativeC2DStreamingTest, ConnectSignalWithOldProtocolVersion)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance(1);

    const auto mirroredDevice = client.getDevices()[0];
    const auto clientRefDevice = client.addDevice("daqref://device0");
    const auto clientLocalSignal = clientRefDevice.getSignals(search::Recursive(search::Visible()))[0];
    const auto mirroredInputPort = mirroredDevice.getFunctionBlocks()[0].getInputPorts()[0];

    ASSERT_THROW_MSG(mirroredInputPort.connect(clientLocalSignal),
                     SignalNotAcceptedException,
                     "Client-to-device streaming operations are not supported by the protocol version currently in use");
}

TEST_F(NativeC2DStreamingTest, ConnectAndRead)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    const auto mirroredDevice = client.getDevices()[0];
    const auto clientRefDevice = client.addDevice("daqref://device0");
    const auto clientLocalSignal = clientRefDevice.getSignals(search::Recursive(search::Visible()))[0];
    const auto mirroredInputPort = mirroredDevice.getFunctionBlocks()[0].getInputPorts()[0];

    mirroredInputPort.connect(clientLocalSignal);

    // read output signal of function block to which external signal connected
    {
        auto fbSignal = server.getFunctionBlocks()[0].getSignals()[0];
        StreamReaderPtr reader = daq::StreamReader<double, uint64_t>(fbSignal, ReadTimeoutType::Any);
        {
            daq::SizeT count = 0;
            reader.read(nullptr, &count, 100);
        }
        double samples[100];
        for (int i = 0; i < 5; ++i)
        {
            daq::SizeT count = 100;
            reader.read(samples, &count, 1000);
            EXPECT_GT(count, 0u) << "iteration " << i;
        }
    }

    // read mirrored external signal directly
    {
        auto mirroredExternalSignal = server.getServers()[0].getSignals()[0];
        StreamReaderPtr reader = daq::StreamReader<double, uint64_t>(mirroredExternalSignal, ReadTimeoutType::Any);
        {
            daq::SizeT count = 0;
            reader.read(nullptr, &count, 100);
        }
        double samples[100];
        for (int i = 0; i < 5; ++i)
        {
            daq::SizeT count = 100;
            reader.read(samples, &count, 1000);
            EXPECT_GT(count, 0u) << "iteration " << i;
        }
    }
}

TEST_F(NativeC2DStreamingTest, ServerCoreEvents)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();

    MirroredSignalConfigPtr mirroredExtSig;
    SignalPtr clientLocalSignal;
    InputPortPtr serverIp = server.getFunctionBlocks()[0].getInputPorts()[0];
    const ComponentPtr mirroredExtSigFolder = server.getServers()[0].getItem("Sig");

    std::promise<void> signalAddedPromise;
    std::promise<void> signalConnectedPromise;
    std::promise<void> signalDescChangedPromise;
    std::promise<void> signalDisconnectedPromise;
    std::promise<void> signalRemovedPromise;

    serverIp.getOnComponentCoreEvent() += [&](ComponentPtr& /*comp*/, CoreEventArgsPtr& args)
    {
        auto params = args.getParameters();
        auto coreEventId = static_cast<CoreEventId>(args.getEventId());
        if (coreEventId == CoreEventId::SignalConnected)
            signalConnectedPromise.set_value();
        else if (coreEventId == CoreEventId::SignalDisconnected)
            signalDisconnectedPromise.set_value();
    };

    mirroredExtSigFolder.getOnComponentCoreEvent() += [&](ComponentPtr& /*comp*/, CoreEventArgsPtr& args)
    {
        auto params = args.getParameters();
        auto coreEventId = static_cast<CoreEventId>(args.getEventId());
        if (coreEventId == CoreEventId::ComponentAdded &&
            params.get("Component").asPtr<IMirroredSignalConfig>().getRemoteId() == clientLocalSignal.getGlobalId())
            signalAddedPromise.set_value();
        else if (coreEventId == CoreEventId::ComponentRemoved && params.get("Id") == mirroredExtSig.getLocalId())
            signalRemovedPromise.set_value();
    };

    std::future<void> signalAddedFuture = signalAddedPromise.get_future();
    std::future<void> signalConnectedFuture = signalConnectedPromise.get_future();
    std::future<void> signalDescChangedFuture = signalDescChangedPromise.get_future();
    std::future<void> signalDisconnectedFuture = signalDisconnectedPromise.get_future();
    std::future<void> signalRemovedFuture = signalRemovedPromise.get_future();

    auto client = CreateClientInstance();
    auto mirroredDevice = client.getDevices()[0];
    const auto clientRefDevice = client.addDevice("daqref://device0");
    clientLocalSignal = clientRefDevice.getSignals(search::Recursive(search::Visible()))[0];
    const auto mirroredInputPort = mirroredDevice.getFunctionBlocks()[0].getInputPorts()[0];

    // connects external signal to server input port
    mirroredInputPort.connect(clientLocalSignal);
    ASSERT_TRUE(signalAddedFuture.wait_for(std::chrono::seconds(5)) == std::future_status::ready);
    ASSERT_TRUE(signalConnectedFuture.wait_for(std::chrono::seconds(5)) == std::future_status::ready);

    mirroredExtSig = serverIp.getSignal();
    ASSERT_EQ(mirroredExtSig.getRemoteId(), clientLocalSignal.getGlobalId());

    mirroredExtSig.getOnComponentCoreEvent() += [&](ComponentPtr& /*comp*/, CoreEventArgsPtr& args)
    {
        if (static_cast<CoreEventId>(args.getEventId()) == CoreEventId::DataDescriptorChanged)
            signalDescChangedPromise.set_value();
    };

    // changes descriptor
    clientLocalSignal.asPtr<ISignalConfig>().setDescriptor(
        DataDescriptorBuilderCopy(clientLocalSignal.getDescriptor()).setName("Test").build()
    );
    ASSERT_TRUE(signalDescChangedFuture.wait_for(std::chrono::seconds(5)) == std::future_status::ready);

    // disconnects external signal from server input port
    mirroredInputPort.disconnect();
    ASSERT_TRUE(signalDisconnectedFuture.wait_for(std::chrono::seconds(5)) == std::future_status::ready);
    ASSERT_TRUE(signalRemovedFuture.wait_for(std::chrono::seconds(5)) == std::future_status::ready);
}

TEST_F(NativeC2DStreamingTest, ClientLostConnection)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    const auto mirroredDevice = client.getDevices()[0];
    const auto clientRefDevice = client.addDevice("daqref://device0");
    const auto clientLocalSignal = clientRefDevice.getSignals(search::Recursive(search::Visible()))[0];
    const auto mirroredInputPort = mirroredDevice.getFunctionBlocks()[0].getInputPorts()[0];

    mirroredInputPort.connect(clientLocalSignal);
    ASSERT_EQ(mirroredInputPort.getSignal(), clientLocalSignal);
    ASSERT_GT(clientLocalSignal.getConnections().getCount(), 0u);
    auto mirroredExternalSignals = server.getServers()[0].getSignals(search::Any());
    ASSERT_EQ(mirroredExternalSignals.getCount(), 2u);

    std::promise<void> reconnectionStatusPromise;
    std::future<void> reconnectionStatusFuture = reconnectionStatusPromise.get_future();
    client.getDevices()[0].getOnComponentCoreEvent() += [&](ComponentPtr& /*comp*/, CoreEventArgsPtr& args)
    {
        auto params = args.getParameters();
        if (static_cast<CoreEventId>(args.getEventId()) == CoreEventId::ConnectionStatusChanged)
        {
            ASSERT_TRUE(args.getParameters().hasKey("StatusName"));
            if (args.getParameters().get("StatusName") == "ConfigurationStatus")
                reconnectionStatusPromise.set_value();
        }
    };

    // remove server to emulate disconnection
    server.removeServer(server.getServers()[0]);
    ASSERT_TRUE(reconnectionStatusFuture.wait_for(std::chrono::seconds(5)) == std::future_status::ready);

    ASSERT_FALSE(mirroredInputPort.getSignal().assigned());
    ASSERT_EQ(clientLocalSignal.getConnections().getCount(), 0u);
    for (const auto& mirroredExternalSignal : mirroredExternalSignals)
    {
        ASSERT_TRUE(mirroredExternalSignal.isRemoved());
    }
}

TEST_F(NativeC2DStreamingTest, ServerLostConnection)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    auto mirroredDevice = client.getDevices()[0];
    const auto clientRefDevice = client.addDevice("daqref://device0");
    const auto clientLocalSignal = clientRefDevice.getSignals(search::Recursive(search::Visible()))[0];
    const auto mirroredInputPort = mirroredDevice.getFunctionBlocks()[0].getInputPorts()[0];

    mirroredInputPort.connect(clientLocalSignal);
    auto mirroredExternalSignals = server.getServers()[0].getSignals(search::Any());
    ASSERT_EQ(mirroredExternalSignals.getCount(), 2u);

    std::promise<void> signalsRemovalPromise;
    std::future<void> signalsRemovalFuture = signalsRemovalPromise.get_future();
    SizeT signalsRemovalCounter = 0;
    server.getServers()[0].getItem("Sig").getOnComponentCoreEvent() +=  [&](ComponentPtr& /*comp*/, CoreEventArgsPtr& args)
    {
        auto params = args.getParameters();
        if (static_cast<CoreEventId>(args.getEventId()) == CoreEventId::ComponentRemoved && ++signalsRemovalCounter == 2)
            signalsRemovalPromise.set_value();
    };

    // disconnect client
    client.removeDevice(mirroredDevice);
    ASSERT_TRUE(signalsRemovalFuture.wait_for(std::chrono::seconds(5)) == std::future_status::ready);

    for (const auto& mirroredExternalSignal : mirroredExternalSignals)
    {
        ASSERT_TRUE(mirroredExternalSignal.isRemoved());
    }
    ASSERT_EQ(server.getServers()[0].getSignals(search::Any()).getCount(), 0u);
}

TEST_F(NativeC2DStreamingTest, StreamingData)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    const auto mirroredDevice = client.getDevices()[0];
    const auto clientLocalDevice = client.addDevice("daqmock://phys_device");
    const auto clientLocalSignal = clientLocalDevice.getSignalsRecursive(search::LocalId("ByteStep"))[0];
    const auto mirroredInputPort = mirroredDevice.getFunctionBlocks()[0].getInputPorts()[0];

    const ComponentPtr mirroredExtSigFolder = server.getServers()[0].getItem("Sig");
    std::promise<void> signalAddedPromise;
    mirroredExtSigFolder.getOnComponentCoreEvent() += [&](ComponentPtr& /*comp*/, CoreEventArgsPtr& args)
    {
        auto params = args.getParameters();
        if (static_cast<CoreEventId>(args.getEventId()) == CoreEventId::ComponentAdded &&
            params.get("Component").asPtr<IMirroredSignalConfig>().getRemoteId() == clientLocalSignal.getGlobalId())
            signalAddedPromise.set_value();
    };
    std::future<void> signalAddedFuture = signalAddedPromise.get_future();

    // connects external signal to server input port
    mirroredInputPort.connect(clientLocalSignal);

    // wait for mirrored external signal appears
    ASSERT_TRUE(signalAddedFuture.wait_for(std::chrono::seconds(5)) == std::future_status::ready);

    auto mirroredExternalSignal = server.getServers()[0].getSignals()[1];
    auto clientReader = PacketReader(clientLocalSignal);
    auto serverReader = PacketReader(mirroredExternalSignal);

    // Expect to receive all data packets,
    // +1 signal initial descriptor changed event packet
    const int packetsToGenerate = 10;
    const int packetsToRead = packetsToGenerate + 1;

    clientLocalDevice.setPropertyValue("GeneratePackets", packetsToRead);

    auto serverReceivedPackets = test_helpers::tryReadPackets(serverReader, packetsToRead);
    auto clientReceivedPackets = test_helpers::tryReadPackets(clientReader, packetsToRead);

    EXPECT_EQ(serverReceivedPackets.getCount(), packetsToRead);
    EXPECT_EQ(clientReceivedPackets.getCount(), packetsToRead);
    EXPECT_TRUE(test_helpers::packetsEqual(clientReceivedPackets, serverReceivedPackets));
}
