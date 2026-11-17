#include "test_helpers/test_helpers.h"
#include "test_helpers/device_modules.h"

using namespace daq;

class LegacyAddDeviceConfigTest : public testing::Test
{
protected:
    using ConnectionStrings = std::vector<std::string>;
    using GeneralValues = std::vector<std::pair<std::string, BaseObjectPtr>>;

    const uint16_t GATEWAY_PORT = test_helpers::serverPort(7421);

    InstancePtr server;
    InstancePtr gateway;
    InstancePtr client = createClient();

    static InstancePtr createServer(bool withLt = true)
    {
        auto instance = test_helpers::createInstance("[[none]]");
        addNativeServerModule(instance);
        test_helpers::addServer(instance, "OpenDAQNativeStreaming");
        if (withLt)
        {
            addLtServerModule(instance);
            test_helpers::addServer(instance, "OpenDAQLTStreaming");
        }
        return instance;
    }

    static InstancePtr createAuthenticatedServer()
    {
        const auto logger = Logger();
        const auto moduleManager = ModuleManager("[[none]]");
        const auto authenticationProvider = StaticAuthenticationProvider(false, List<IUser>(User("user", "password")));
        auto instance = InstanceCustom(Context(Scheduler(logger), logger, TypeManager(), moduleManager, authenticationProvider), "server");
        addNativeServerModule(instance);
        test_helpers::addServer(instance, "OpenDAQNativeStreaming");
        return instance;
    }

    InstancePtr createGateway() const
    {
        auto instance = test_helpers::createInstance("[[none]]");
        addNativeClientModule(instance);
        addLtClientModule(instance);
        addNativeServerModule(instance);
        auto serverConfig = instance.getAvailableServerTypes().get("OpenDAQNativeStreaming").createDefaultConfig();
        serverConfig.setPropertyValue("NativeStreamingPort", GATEWAY_PORT);
        instance.addServer("OpenDAQNativeStreaming", serverConfig);
        return instance;
    }

    static InstancePtr createClient()
    {
        auto instance = test_helpers::createInstance("[[none]]");
        addNativeClientModule(instance);
        addLtClientModule(instance);
        return instance;
    }

    static std::string deviceConnectionString()
    {
        return test_helpers::withPort("daq.nd://127.0.0.1");
    }

    static std::string nativeStreaming()
    {
        return fmt::format("daq.ns://127.0.0.1:{}", test_helpers::serverPort(7420));
    }

    static std::string ltStreaming()
    {
        return fmt::format("daq.lt://127.0.0.1:{}", test_helpers::serverPort(7414));
    }

    static PropertyObjectPtr general(const PropertyObjectPtr& config)
    {
        return config.getPropertyValue("General");
    }

    PropertyObjectPtr legacyConfig(const GeneralValues& generalValues = {}) const
    {
        const auto config = client.createDefaultAddDeviceConfig();
        for (const auto& [name, value] : generalValues)
            general(config).setPropertyValue(name, value);
        return config;
    }

    static void setCredentials(const PropertyObjectPtr& config)
    {
        config.setPropertyValue("Username", "user");
        config.setPropertyValue("Password", "password");
    }

    static ConnectionStrings streamingConnectionStrings(const DevicePtr& device)
    {
        ConnectionStrings connectionStrings;
        for (const auto& streaming : device.asPtr<IMirroredDevice>().getStreamingSources())
            connectionStrings.push_back(streaming.getConnectionString());
        return connectionStrings;
    }

    ConnectionStrings addAndGetStreamingConnectionStrings(const PropertyObjectPtr& config, bool withLt = true)
    {
        server = createServer(withLt);
        return streamingConnectionStrings(client.addDevice(deviceConnectionString(), config));
    }

    ConnectionStrings remoteAddAndGetStreamingConnectionStrings(const PropertyObjectPtr& config)
    {
        server = createServer();
        gateway = createGateway();
        const auto gatewayDevice = client.addDevice(fmt::format("daq.nd://127.0.0.1:{}", GATEWAY_PORT));
        gatewayDevice.addDevice(deviceConnectionString(), config);

        const auto gatewayDevices = gateway.getDevices();
        EXPECT_EQ(gatewayDevices.getCount(), 1u);
        return streamingConnectionStrings(gatewayDevices[0]);
    }
};

TEST_F(LegacyAddDeviceConfigTest, NullConfig)
{
    ASSERT_EQ(addAndGetStreamingConnectionStrings(nullptr), ConnectionStrings{nativeStreaming()});
}

TEST_F(LegacyAddDeviceConfigTest, DefaultConfig)
{
    ASSERT_EQ(addAndGetStreamingConnectionStrings(legacyConfig()), ConnectionStrings{nativeStreaming()});
}

TEST_F(LegacyAddDeviceConfigTest, PrioritizedLtFirst)
{
    const auto config = legacyConfig({{"PrioritizedStreamingProtocols", List<IString>("OpenDAQLTStreaming", "OpenDAQNativeStreaming")}});
    ASSERT_EQ(addAndGetStreamingConnectionStrings(config), ConnectionStrings{ltStreaming()});
}

TEST_F(LegacyAddDeviceConfigTest, PrioritizedUnavailableDoesNotFallBack)
{
    const auto config = legacyConfig({{"PrioritizedStreamingProtocols", List<IString>("OpenDAQLTStreaming", "OpenDAQNativeStreaming")}});
    ASSERT_EQ(addAndGetStreamingConnectionStrings(config, false), ConnectionStrings{});
}

TEST_F(LegacyAddDeviceConfigTest, AllowedLtOnly)
{
    const auto config = legacyConfig({{"AllowedStreamingProtocols", List<IString>("OpenDAQLTStreaming")}});
    ASSERT_EQ(addAndGetStreamingConnectionStrings(config), ConnectionStrings{ltStreaming()});
}

TEST_F(LegacyAddDeviceConfigTest, AllowedNotPrioritized)
{
    const auto config = legacyConfig({{"PrioritizedStreamingProtocols", List<IString>("OpenDAQNativeStreaming")},
                                      {"AllowedStreamingProtocols", List<IString>("OpenDAQLTStreaming")}});
    ASSERT_EQ(addAndGetStreamingConnectionStrings(config), ConnectionStrings{});
}

TEST_F(LegacyAddDeviceConfigTest, EmptyPrioritized)
{
    const auto config = legacyConfig({{"PrioritizedStreamingProtocols", List<IString>()}});
    ASSERT_EQ(addAndGetStreamingConnectionStrings(config), ConnectionStrings{});
}

TEST_F(LegacyAddDeviceConfigTest, AutomaticallyConnectStreamingOff)
{
    const auto config = legacyConfig({{"AutomaticallyConnectStreaming", False}});
    ASSERT_EQ(addAndGetStreamingConnectionStrings(config), ConnectionStrings{});
}

TEST_F(LegacyAddDeviceConfigTest, NotConnectedHeuristic)
{
    const auto config = legacyConfig({{"StreamingConnectionHeuristic", Integer(2)}});
    ASSERT_EQ(addAndGetStreamingConnectionStrings(config), ConnectionStrings{});
}

TEST_F(LegacyAddDeviceConfigTest, FlatAutomaticallyConnectStreamingOff)
{
    auto config = PropertyObject();
    config.addProperty(BoolProperty("AutomaticallyConnectStreaming", false));

    ASSERT_EQ(addAndGetStreamingConnectionStrings(config), ConnectionStrings{});
}

TEST_F(LegacyAddDeviceConfigTest, FlatPrioritizedLt)
{
    auto config = PropertyObject();
    config.addProperty(ListProperty("PrioritizedStreamingProtocols", List<IString>("OpenDAQLTStreaming")));

    ASSERT_EQ(addAndGetStreamingConnectionStrings(config), ConnectionStrings{ltStreaming()});
}

TEST_F(LegacyAddDeviceConfigTest, BareTypeConfig)
{
    const PropertyObjectPtr config = client.getAvailableDeviceTypes().get("OpenDAQNativeConfiguration").createDefaultConfig();
    ASSERT_EQ(addAndGetStreamingConnectionStrings(config), ConnectionStrings{nativeStreaming()});
}

TEST_F(LegacyAddDeviceConfigTest, RemoteAddWithoutStreaming)
{
    const auto config = legacyConfig({{"AutomaticallyConnectStreaming", False}});
    ASSERT_EQ(remoteAddAndGetStreamingConnectionStrings(config), ConnectionStrings{});
}

// Hangs at times: ConfigProtocolServer::SendOutCoreEvents deadlocks with the mirrored device's config lock.
TEST_F(LegacyAddDeviceConfigTest, DISABLED_RemoteAddPrioritizedLt)
{
    const auto config = legacyConfig({{"PrioritizedStreamingProtocols", List<IString>("OpenDAQLTStreaming")}});
    ASSERT_EQ(remoteAddAndGetStreamingConnectionStrings(config), ConnectionStrings{ltStreaming()});
}

TEST_F(LegacyAddDeviceConfigTest, CredentialsReachStreaming)
{
    server = createAuthenticatedServer();

    const auto generalCredentials = legacyConfig();
    setCredentials(general(generalCredentials));

    const auto sectionCredentials = legacyConfig();
    setCredentials(sectionCredentials.getPropertyValue("Device.OpenDAQNativeConfiguration"));
    setCredentials(sectionCredentials.getPropertyValue("Streaming.OpenDAQNativeStreaming"));

    auto flatCredentials = PropertyObject();
    flatCredentials.addProperty(StringProperty("Username", "user"));
    flatCredentials.addProperty(StringProperty("Password", "password"));

    const std::vector<PropertyObjectPtr> configs{generalCredentials, sectionCredentials, flatCredentials};
    for (size_t i = 0; i < configs.size(); ++i)
    {
        const auto device = client.addDevice(deviceConnectionString(), configs[i]);
        ASSERT_EQ(streamingConnectionStrings(device), ConnectionStrings{nativeStreaming()}) << "row " << i;
        client.removeDevice(device);
    }
}

TEST_F(LegacyAddDeviceConfigTest, AddStreamingWithOldConfig)
{
    server = createAuthenticatedServer();
    const auto deviceConfig = legacyConfig({{"AutomaticallyConnectStreaming", False}});
    setCredentials(general(deviceConfig));
    const auto device = client.addDevice(deviceConnectionString(), deviceConfig);

    const auto streamingConfig = legacyConfig();
    setCredentials(streamingConfig.getPropertyValue("Streaming.OpenDAQNativeStreaming"));
    device.addStreaming(nativeStreaming(), streamingConfig);

    ASSERT_EQ(streamingConnectionStrings(device), ConnectionStrings{nativeStreaming()});
}

TEST_F(LegacyAddDeviceConfigTest, SaveLoadKeepsPrioritizedLt)
{
    const auto config = legacyConfig({{"PrioritizedStreamingProtocols", List<IString>("OpenDAQLTStreaming")}});
    addAndGetStreamingConnectionStrings(config);

    const auto loaded = createClient();
    loaded.loadConfiguration(client.saveConfiguration());

    const auto devices = loaded.getDevices();
    ASSERT_EQ(devices.getCount(), 1u);
    ASSERT_EQ(streamingConnectionStrings(devices[0]), ConnectionStrings{ltStreaming()});
}

// Saved by a client that added daq.nd://127.0.0.1 with the old three-section config and LT streaming prioritized
static const char* oldSave =
    R"json({"__type":"Instance","rootDevice":{"openDAQDevice":{"__type":"Device","isRootDevice":true,"name":"OpenDAQClient","Dev":{"__type":"Folder","name":"Dev","items":{"openDAQDevice":{"__type":"Device","__version":{"__type":"VersionInfo","major":3,"minor":41,"patch":0},"isRootDevice":true,"name":"OpenDAQClient","ComponentConfig":{"__type":"PropertyObject","propValues":{"Device":{"__type":"PropertyObject","propValues":{"OpenDAQLTStreaming":{"__type":"PropertyObject"},"OpenDAQLTStreamingOld":{"__type":"PropertyObject"},"OpenDAQLTStreamingSecure":{"__type":"PropertyObject"},"OpenDAQNativeConfiguration":{"__type":"PropertyObject","propValues":{"TransportLayerConfig":{"__type":"PropertyObject"}}},"OpenDAQNativeStreaming":{"__type":"PropertyObject","propValues":{"TransportLayerConfig":{"__type":"PropertyObject"}}}}},"General":{"__type":"PropertyObject","propValues":{"PrioritizedStreamingProtocols":{"__type":"List","itemIntfID":"{D2ED1120-F7FF-556F-A98D-3F3EDF1A3874}","values":["OpenDAQLTStreaming"]}}},"Streaming":{"__type":"PropertyObject","propValues":{"OpenDAQLTStreaming":{"__type":"PropertyObject"},"OpenDAQLTStreamingSecure":{"__type":"PropertyObject"},"OpenDAQNativeStreaming":{"__type":"PropertyObject","propValues":{"TransportLayerConfig":{"__type":"PropertyObject"}}}}}}},"Srv":{"__type":"Folder","name":"Srv","items":{"OpenDAQNativeStreaming":{"__type":"Server","id":"OpenDAQNativeStreaming","name":"OpenDAQNativeStreaming"},"OpenDAQLTStreaming":{"__type":"Server","id":"OpenDAQLTStreaming","name":"OpenDAQLTStreaming"}}},"connectionString":"daq.nd://127.0.0.1","Synchronization":{"__type":"SyncComponent","visible":false,"name":"Synchronization","propValues":{"Interfaces":{"__type":"PropertyObject"}}},"connectionStatuses":{"__type":"ConnectionStatusContainer","connectionStatuses":{"__type":"Dict","keyIntfID":"{D2ED1120-F7FF-556F-A98D-3F3EDF1A3874}","valueIntfID":"{5E7D128C-87ED-5FE3-9480-CCB7E7CF8F49}","values":[{"key":"daq.nd://127.0.0.1","value":{"__type":"Enumeration","typeName":"ConnectionStatusType","value":"Connected"}},{"key":"daq.lt://127.0.0.1:7414","value":{"__type":"Enumeration","typeName":"ConnectionStatusType","value":"Connected"}}]},"statusNames":{"__type":"Dict","keyIntfID":"{D2ED1120-F7FF-556F-A98D-3F3EDF1A3874}","valueIntfID":"{D2ED1120-F7FF-556F-A98D-3F3EDF1A3874}","values":[{"key":"daq.nd://127.0.0.1","value":"ConfigurationStatus"},{"key":"daq.lt://127.0.0.1:7414","value":"StreamingStatus_OpenDAQLTStreaming_1"}]},"messages":{"__type":"Dict","keyIntfID":"{D2ED1120-F7FF-556F-A98D-3F3EDF1A3874}","valueIntfID":"{D2ED1120-F7FF-556F-A98D-3F3EDF1A3874}","values":[{"key":"daq.nd://127.0.0.1","value":""},{"key":"daq.lt://127.0.0.1:7414","value":""}]}},"propValues":{"DaqDeviceInfo":{"__type":"DeviceInfo","propValues":{}}}}}},"connectionString":"daq.root://default_client","Synchronization":{"__type":"SyncComponent","visible":false,"name":"Synchronization","propValues":{"Interfaces":{"__type":"PropertyObject"}}},"propValues":{"DaqDeviceInfo":{"__type":"DeviceInfo","propValues":{}}}}}})json";

TEST_F(LegacyAddDeviceConfigTest, LoadOldSave)
{
    std::string saved = oldSave;
    const std::string savedConnectionString = "\"daq.nd://127.0.0.1\"";
    saved.replace(saved.find(savedConnectionString), savedConnectionString.size(), "\"" + deviceConnectionString() + "\"");

    server = createServer();
    client.loadConfiguration(saved);

    const auto devices = client.getDevices();
    ASSERT_EQ(devices.getCount(), 1u);
    ASSERT_EQ(streamingConnectionStrings(devices[0]), ConnectionStrings{ltStreaming()});
}
