#include "test_helpers/test_helpers.h"
#include <coreobjects/authentication_provider_factory.h>
#include <thread>

#include "test_helpers/device_modules.h"
#include "test_helpers/lt_tls.h"

using namespace daq;

class WebsocketModulesTest : public testing::Test
{
public:
    void testSignalDescriptors(size_t rangeBegin,
                               size_t rangeEnd,
                               const ListPtr<ISignal>& clientSignals,
                               const ListPtr<ISignal>& serverSignals)
    {
        ASSERT_EQ(clientSignals.getCount(), serverSignals.getCount());
        ASSERT_GT(rangeEnd, rangeBegin);
        ASSERT_GE(clientSignals.getCount(), rangeBegin);

        for (size_t i = rangeBegin; i < rangeEnd; i += 2)
        {
            ASSERT_TRUE(clientSignals[i].getDomainSignal().assigned());
            ASSERT_EQ(clientSignals[i].getDomainSignal(), clientSignals[i+1]);
            ASSERT_FALSE(clientSignals[i+1].getDomainSignal().assigned());

            DataDescriptorPtr dataDescriptor = clientSignals[i].getDescriptor();
            DataDescriptorPtr serverDataDescriptor = serverSignals[i].getDescriptor();

            DataDescriptorPtr domainDataDescriptor = clientSignals[i].getDomainSignal().getDescriptor();
            DataDescriptorPtr serverDomainDataDescriptor = serverSignals[i].getDomainSignal().getDescriptor();

            ASSERT_EQ(dataDescriptor, serverDataDescriptor);

            ASSERT_EQ(domainDataDescriptor.getRule().getParameters(), serverDomainDataDescriptor.getRule().getParameters());
            ASSERT_EQ(domainDataDescriptor.getOrigin(), serverDomainDataDescriptor.getOrigin());
            ASSERT_EQ(domainDataDescriptor.getTickResolution(), serverDomainDataDescriptor.getTickResolution());
        }
    }

    // Finds a signal by its name (the leaf of its local ID)
    // The LT streaming client mangles the full server signal path into its local ID (replacing '/' with '#')
    static SignalPtr getSignalByName(const ListPtr<ISignal>& signals, const std::string& name)
    {
        for (const auto& signal : signals)
        {
            const std::string localId = signal.getLocalId().toStdString();
            const auto pos = localId.find_last_of("#/");
            const std::string leaf = pos == std::string::npos ? localId : localId.substr(pos + 1);
            if (leaf == name)
                return signal;
        }
        return nullptr;
    }

    void testSignalDescriptorsByLocalId(const std::vector<std::string>& valueSignalNames,
                                        const ListPtr<ISignal>& clientSignals,
                                        const ListPtr<ISignal>& serverSignals)
    {
        ASSERT_EQ(clientSignals.getCount(), serverSignals.getCount());

        for (const auto& name : valueSignalNames)
        {
            SignalPtr clientSignal = getSignalByName(clientSignals, name);
            SignalPtr serverSignal = getSignalByName(serverSignals, name);

            ASSERT_TRUE(clientSignal.assigned()) << "client signal not found: " << name;
            ASSERT_TRUE(serverSignal.assigned()) << "server signal not found: " << name;

            ASSERT_TRUE(clientSignal.getDomainSignal().assigned()) << name;

            DataDescriptorPtr dataDescriptor = clientSignal.getDescriptor();
            DataDescriptorPtr serverDataDescriptor = serverSignal.getDescriptor();

            DataDescriptorPtr domainDataDescriptor = clientSignal.getDomainSignal().getDescriptor();
            DataDescriptorPtr serverDomainDataDescriptor = serverSignal.getDomainSignal().getDescriptor();

            ASSERT_EQ(dataDescriptor, serverDataDescriptor);

            ASSERT_EQ(domainDataDescriptor.getRule().getParameters(), serverDomainDataDescriptor.getRule().getParameters());
            ASSERT_EQ(domainDataDescriptor.getOrigin(), serverDomainDataDescriptor.getOrigin());
            ASSERT_EQ(domainDataDescriptor.getTickResolution(), serverDomainDataDescriptor.getTickResolution());
        }
    }
};

enum class Channel
{
    Plain,
    Tls
};

static std::vector<Channel> GetChannelSuite()
{
    std::vector<Channel> suite{Channel::Plain};
#ifdef OPENDAQ_ENABLE_WEBSOCKET_STREAMING_WITH_TLS
    suite.push_back(Channel::Tls);
#endif
    return suite;
}

static std::string ChannelName(const testing::TestParamInfo<Channel>& info)
{
    return info.param == Channel::Tls ? "Tls" : "Plain";
}

// Tests which are not specific to one channel and run over both of them.
class WebsocketModulesChannelTest : public WebsocketModulesTest, public testing::WithParamInterface<Channel>
{
public:
    bool secure() const
    {
        return GetParam() == Channel::Tls;
    }

    std::string connectionString(const std::string& host = "127.0.0.1", const std::string& path = "/") const
    {
        return (secure() ? "daq.lts://" : "daq.lt://") + host + path;
    }

    std::string expectedProtocolId() const
    {
        return secure() ? "OpenDAQLTStreamingSecure" : "OpenDAQLTStreaming";
    }

    std::string expectedPrefix() const
    {
        return secure() ? "daq.lts" : "daq.lt";
    }

    Int expectedPort() const
    {
        return secure() ? 7415 : 7414;
    }

    PropertyObjectPtr serverConfig([[maybe_unused]] const InstancePtr& server) const
    {
#ifdef OPENDAQ_ENABLE_WEBSOCKET_STREAMING_WITH_TLS
        if (secure())
            return test_helpers::lt_tls::secureServerConfig(server);
#endif
        return nullptr;
    }

    // Same as serverConfig(), but always assigned so the caller can set further properties (path, port, ...)
    PropertyObjectPtr serverConfigWithDefaults(const InstancePtr& server) const
    {
        const auto config = serverConfig(server);
        if (config.assigned())
            return config;
        return server.getAvailableServerTypes().get("OpenDAQLTStreaming").createDefaultConfig();
    }

    PropertyObjectPtr deviceConfig([[maybe_unused]] const InstancePtr& client) const
    {
#ifdef OPENDAQ_ENABLE_WEBSOCKET_STREAMING_WITH_TLS
        if (secure())
            return test_helpers::lt_tls::secureDeviceConfig(client);
#endif
        return nullptr;
    }

    InstancePtr CreateServerInstance()
    {
        auto logger = Logger();
        auto scheduler = Scheduler(logger);
        auto moduleManager = ModuleManager("[[none]]");
        auto typeManager = TypeManager();
        auto authenticationProvider = AuthenticationProvider();
        auto context = Context(scheduler, logger, typeManager, moduleManager, authenticationProvider);

        auto server = InstanceCustom(context, "local");
        addRefDeviceModule(server);
        addLtServerModule(server);

        const auto refDevice = server.addDevice("daqref://device1");

        if (secure())
            server.addServer("OpenDAQLTStreaming", serverConfig(server));
        else
            server.addServer("openDAQ LT Streaming", nullptr);

        return server;
    }

    InstancePtr CreateClientInstance(const bool withDelay = true)
    {
        auto client = Instance("[[none]]");
        addLtClientModule(client);

        auto refDevice = client.addDevice(connectionString(), deviceConfig(client));
        if (withDelay)
        {
            CONDITIONAL_SLEEP;
        }
        return client;
    }
};

TEST_P(WebsocketModulesChannelTest, ConnectFail)
{
    ASSERT_THROW(CreateClientInstance(), NotFoundException);
}

TEST_P(WebsocketModulesChannelTest, ConnectAndDisconnect)
{
    auto server = CreateServerInstance();
    auto client = CreateClientInstance(false);
}

TEST_P(WebsocketModulesChannelTest, ConnectAndDisconnectBackwardCompatibility)
{
    auto server = CreateServerInstance();

    auto client = Instance("[[none]]");
    addLtClientModule(client);

    // daq.ws:// is the legacy alias of the plaintext daq.lt:// channel. The secure channel was introduced
    // together with daq.lts:// and has no legacy alias: the client module accepts daq.ws, daq.lt and
    // daq.lts only, so daq.wss:// must be rejected
    if (secure())
    {
        ASSERT_THROW(client.addDevice("daq.wss://127.0.0.1/", deviceConfig(client)), NotFoundException);
        return;
    }

    client.addDevice("daq.ws://127.0.0.1/", deviceConfig(client));
}

TEST_P(WebsocketModulesChannelTest, ConnectViaIpv6)
{
    if (test_helpers::Ipv6IsDisabled())
    {
        GTEST_SKIP() << "Ipv6 is disabled";
    }

    auto server = CreateServerInstance();

    auto client = Instance("[[none]]");
    addLtClientModule(client);
    client.addDevice(connectionString("[::1]", ""), deviceConfig(client));
}

TEST_F(WebsocketModulesTest, PopulateDefaultConfigFromProvider)
{
    std::string filename = "populateDefaultConfig.json";
    std::string json = R"(
        {
            "Modules":
            {
                "StreamingLtServer":
                {
                    "WebsocketStreamingPort": 1234,
                    "Path": "/some/path"
                }
            }
        }
    )";
    auto finally = test_helpers::CreateConfigFile(filename, json);

    auto provider = JsonConfigProvider(filename);
    auto instance = InstanceBuilder()
        .setModulePath("[[none]]")
        .addConfigProvider(provider)
        .build();

    addLtServerModule(instance);
    auto serverConfig = instance.getAvailableServerTypes().get("OpenDAQLTStreaming").createDefaultConfig();

    ASSERT_EQ(serverConfig.getPropertyValue("WebsocketStreamingPort").asPtr<IInteger>(), 1234);
    ASSERT_EQ(serverConfig.getPropertyValue("Path").asPtr<IString>(), "/some/path");
}

TEST_P(WebsocketModulesChannelTest, DiscoveringServer)
{
    auto server = InstanceBuilder()
        .setModulePath("[[none]]")
        .addDiscoveryServer("mdns")
        .setDefaultRootDeviceLocalId("local")
        .build();

    addRefDeviceModule(server);
    server.addDevice("daqref://device1");

    addLtServerModule(server);
    auto config = serverConfigWithDefaults(server);
    auto path = "/test/streaming_lt/discovery/";
    config.setPropertyValue("Path", path);
    server.addServer("OpenDAQLTStreaming", config).enableDiscovery();

    auto client = Instance("[[none]]");
    addLtClientModule(client);

    DevicePtr device;
    for (const auto & deviceInfo : client.getAvailableDevices())
    {
        for (const auto & capability : deviceInfo.getServerCapabilities())
        {
            if (!test_helpers::isSufix(capability.getConnectionString(), path))
            {
                continue;
            }
            if (capability.getProtocolName() == expectedProtocolId())
            {
                device = client.addDevice(capability.getConnectionString(), deviceConfig(client));
                return;
            }
        }
    }
    ASSERT_TRUE(false) << "Device not found";
}


TEST_P(WebsocketModulesChannelTest, CheckDeviceInfoPopulatedWithProvider)
{
    std::string filename = "populateDefaultConfig.json";
    std::string json = R"(
        {
            "Modules":
            {
                "StreamingLtServer":
                {
                    "WebsocketStreamingPort": 1234,
                    "Path": "/test/streaming_lt/checkDeviceInfoPopulated"
                }
            }
        }
    )";
    auto path = "/test/streaming_lt/checkDeviceInfoPopulated";
    auto finally = test_helpers::CreateConfigFile(filename, json);

    auto rootInfo = DeviceInfo("");
    rootInfo.setName("TestName");
    rootInfo.setManufacturer("TestManufacturer");
    rootInfo.setModel("TestModel");
    rootInfo.setSerialNumber("TestSerialNumber");

    auto provider = JsonConfigProvider(filename);
    auto instance = InstanceBuilder()
        .setModulePath("[[none]]")
        .addDiscoveryServer("mdns")
        .addConfigProvider(provider)
        .setDefaultRootDeviceInfo(rootInfo)
        .build();

    addRefDeviceModule(instance);
    instance.addDevice("daqref://device1");

    addLtServerModule(instance);
    auto config = serverConfigWithDefaults(instance);
    instance.addServer("OpenDAQLTStreaming", config).enableDiscovery();

    auto client = Instance("[[none]]");
    addLtClientModule(client);

    for (const auto & deviceInfo : client.getAvailableDevices())
    {
        for (const auto & capability : deviceInfo.getServerCapabilities())
        {
            if (capability.getProtocolName() == expectedProtocolId())
            {
                if (!test_helpers::isSufix(capability.getConnectionString(), path))
                {
                    continue;
                }

                client.addDevice(capability.getConnectionString(), deviceConfig(client));
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

TEST_P(WebsocketModulesChannelTest, TestDiscoveryReachability)
{
    bool checkIPv6 = !test_helpers::Ipv6IsDisabled();
    // ICMP ping (and thus active IPv4 reachability detection) requires root on Linux/macOS.
    const auto expectedIpv4Reachability =
        test_helpers::icmpPingAvailable() ? AddressReachabilityStatus::Reachable : AddressReachabilityStatus::Unknown;

    auto instance = InstanceBuilder()
        .setModulePath("[[none]]")
        .addDiscoveryServer("mdns")
        .build();

    addLtServerModule(instance);

    auto config = serverConfigWithDefaults(instance);
    auto path = "/test/native_configurator/discovery_reachability/";
    config.setPropertyValue("Path", path);

    instance.addServer("OpenDAQLTStreaming", config).enableDiscovery();

    auto client = Instance("[[none]]");
    addLtClientModule(client);

    for (const auto & deviceInfo : client.getAvailableDevices())
    {
        for (const auto & capability : deviceInfo.getServerCapabilities())
        {
            if (!test_helpers::isSufix(capability.getConnectionString(), path))
                continue;

            if (capability.getProtocolName() != expectedProtocolId())
                continue;

            bool hasIPv4 = false;
            bool hasIPv6 = false;
            int cnt = 0;
            for (const auto& addressInfo : capability.getAddressInfo())
            {
                ASSERT_EQ(addressInfo.getConnectionString(), capability.getConnectionStrings()[cnt]);
                ASSERT_EQ(addressInfo.getAddress(), capability.getAddresses()[cnt]);
                if (addressInfo.getType() == "IPv4")
                {
                    hasIPv4 = true;
                    ASSERT_EQ(addressInfo.getReachabilityStatus(), expectedIpv4Reachability);
                }
                else if (addressInfo.getType() == "IPv6")
                {
                    hasIPv6 = true;
                    ASSERT_EQ(addressInfo.getReachabilityStatus(), AddressReachabilityStatus::Unknown);
                }

                if (hasIPv4 && (hasIPv6 || !checkIPv6))
                    return;

                cnt++;
            }
        }
    }

    ASSERT_TRUE(false) << "Device not found";
}

#ifdef OPENDAQ_ENABLE_WEBSOCKET_STREAMING_WITH_TLS
// A server with the TLS channel enabled keeps serving the plaintext one and advertises both services
// (_streaming-lt._tcp and _streaming-lts._tcp). Both capabilities are merged into a single discovered device
// info, because the root device info provides a manufacturer and a serial number to group them by
TEST_F(WebsocketModulesTest, DiscoveringBothChannels)
{
    const std::string path = "/test/streaming_lt/discovery/both_channels/";

    auto rootInfo = DeviceInfo("");
    rootInfo.setName("TestName");
    rootInfo.setManufacturer("TestManufacturer");
    rootInfo.setSerialNumber("TestSerialNumberBothChannels");

    auto server = InstanceBuilder()
        .setModulePath("[[none]]")
        .addDiscoveryServer("mdns")
        .setDefaultRootDeviceInfo(rootInfo)
        .build();

    addRefDeviceModule(server);
    server.addDevice("daqref://device1");

    addLtServerModule(server);
    auto config = test_helpers::lt_tls::secureServerConfig(server);
    config.setPropertyValue("Path", path);
    server.addServer("OpenDAQLTStreaming", config).enableDiscovery();

    auto client = Instance("[[none]]");
    addLtClientModule(client);

    DeviceInfoPtr discovered;
    for (const auto& deviceInfo : client.getAvailableDevices())
    {
        for (const auto& capability : deviceInfo.getServerCapabilities())
        {
            if (test_helpers::isSufix(capability.getConnectionString(), path))
            {
                discovered = deviceInfo;
                break;
            }
        }
        if (discovered.assigned())
            break;
    }
    ASSERT_TRUE(discovered.assigned()) << "Device not found";

    ASSERT_TRUE(discovered.hasServerCapability(test_helpers::lt_tls::PlainProtocolId));
    ASSERT_TRUE(discovered.hasServerCapability(test_helpers::lt_tls::SecureProtocolId));

    const auto plainCap = discovered.getServerCapability(test_helpers::lt_tls::PlainProtocolId);
    const auto secureCap = discovered.getServerCapability(test_helpers::lt_tls::SecureProtocolId);

    ASSERT_EQ(plainCap.getProtocolType(), ProtocolType::Streaming);
    ASSERT_EQ(secureCap.getProtocolType(), ProtocolType::Streaming);
    ASSERT_EQ(plainCap.getPrefix(), "daq.lt");
    ASSERT_EQ(secureCap.getPrefix(), "daq.lts");
    ASSERT_EQ(plainCap.getPort(), 7414);
    ASSERT_EQ(secureCap.getPort(), 7415);
    // both channels belong to the same protocol group, so a client can pick either of them
    ASSERT_EQ(secureCap.getProtocolGroupId(), plainCap.getProtocolGroupId());
    // and the secure one wins whenever protocols are ordered by their security level
    ASSERT_GT(secureCap.getProtocolSecurityLevel(), plainCap.getProtocolSecurityLevel());

    auto device = client.addDevice(secureCap.getConnectionString(), test_helpers::lt_tls::secureDeviceConfig(client));
    const auto connectionInfo = device.getInfo().getConfigurationConnectionInfo();
    ASSERT_EQ(connectionInfo.getProtocolId(), test_helpers::lt_tls::SecureProtocolId);
    ASSERT_EQ(connectionInfo.getPort(), 7415);
}
#endif

TEST_P(WebsocketModulesChannelTest, GetConnectedClientsInfo)
{
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    // one streaming connection
    auto serverSideClientsInfo = server.getRootDevice().getInfo().getConnectedClientsInfo();
    ASSERT_EQ(serverSideClientsInfo.getCount(), 1u);
    // the server reports the protocol the client arrived on, so the two channels are told apart
    ASSERT_EQ(serverSideClientsInfo[0].getProtocolName(), expectedProtocolId());
    ASSERT_EQ(serverSideClientsInfo[0].getHostName(), "");
    ASSERT_TRUE(serverSideClientsInfo[0].getAddress().toStdString().find("127.0.0.1") != std::string::npos);
    ASSERT_EQ(serverSideClientsInfo[0].getClientTypeName(), "");
    ASSERT_EQ(serverSideClientsInfo[0].getProtocolType(), ProtocolType::Streaming);
}

TEST_P(WebsocketModulesChannelTest, GetRemoteDeviceObjects)
{
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    ASSERT_EQ(client.getDevices().getCount(), 1u);
    auto signals = client.getSignals(search::Recursive(search::Visible()));
    ASSERT_EQ(signals.getCount(), 5u);
}

TEST_P(WebsocketModulesChannelTest, RemoveDevice)
{
    auto server = CreateServerInstance();

    auto client = Instance("[[none]]");

    addLtClientModule(client);
    auto device = client.addDevice(connectionString(), deviceConfig(client));

    ASSERT_NO_THROW(client.removeDevice(device));
    ASSERT_TRUE(device.isRemoved());
}

TEST_P(WebsocketModulesChannelTest, SignalConfig_Server)
{
    const std::string newSignalName{"some new name"};

    auto server = CreateServerInstance();

    auto serverSignal = getSignalByName(server.getSignals(search::Recursive(search::Any())), "AI0").asPtr<ISignalConfig>();
    auto serverSignalDataDescriptor = DataDescriptorBuilderCopy(serverSignal.getDescriptor()).setName(newSignalName).build();
    serverSignal.setDescriptor(serverSignalDataDescriptor);

    auto client = CreateClientInstance();
    auto clientSignals = client.getDevices()[0].getSignals(search::Recursive(search::Any()));
    auto clientSignal = getSignalByName(clientSignals, "AI0").asPtr<ISignalConfig>();

    auto clientSignalDataDescriptor = DataDescriptorBuilderCopy(clientSignal.getDescriptor()).build();

    ASSERT_TRUE(clientSignal.assigned());
    ASSERT_EQ(serverSignal.getDescriptor().getName(), newSignalName);
    ASSERT_EQ(serverSignal.getDescriptor().getName(), clientSignal.getDescriptor().getName());
}

TEST_P(WebsocketModulesChannelTest, DataDescriptor)
{
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();
    testSignalDescriptorsByLocalId({"AI0", "AI1"},
                                   client.getSignals(search::Recursive(search::Any())),
                                   server.getSignals(search::Recursive(search::Any())));
}

TEST_P(WebsocketModulesChannelTest, SubscribeReadUnsubscribe)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();
    auto signal = getSignalByName(client.getSignals(search::Recursive(search::Any())), "AI0")
                      .template asPtr<IMirroredSignalConfig>();

    StringPtr streamingSource = signal.getActiveStreamingSource();

    test_helpers::SignalAckListener acks(signal);

    using namespace std::chrono_literals;
    StreamReaderPtr reader = daq::StreamReader<double, uint64_t>(signal, ReadTimeoutType::Any);

    ASSERT_TRUE(acks.waitForSubscribeAck());
    ASSERT_EQ(acks.subscribeAckStreaming(), streamingSource);

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

    ASSERT_TRUE(acks.waitForUnsubscribeAck());
    ASSERT_EQ(acks.unsubscribeAckStreaming(), streamingSource);
}

TEST_P(WebsocketModulesChannelTest, DISABLED_RenderSignal)
{
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    auto signals = client.getSignals(search::Recursive(search::Visible()));
    const auto renderer = client.addFunctionBlock("RefFBModuleRenderer");
    renderer.getInputPorts()[0].connect(signals[0]);

    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
}

TEST_P(WebsocketModulesChannelTest, GetConfigurationConnectionInfoIPv4)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance(false);

    auto devices = client.getDevices();
    ASSERT_EQ(devices.getCount(), 1u);

    auto connectionInfo = devices[0].getInfo().getConfigurationConnectionInfo();
    ASSERT_EQ(connectionInfo.getProtocolId(), expectedProtocolId());
    ASSERT_EQ(connectionInfo.getProtocolName(), expectedProtocolId());
    ASSERT_EQ(connectionInfo.getProtocolType(), ProtocolType::Streaming);
    ASSERT_EQ(connectionInfo.getConnectionType(), "TCP/IP");
    ASSERT_EQ(connectionInfo.getAddresses()[0], "127.0.0.1");
    ASSERT_EQ(connectionInfo.getPort(), expectedPort());
    ASSERT_EQ(connectionInfo.getPrefix(), expectedPrefix());
    ASSERT_EQ(connectionInfo.getConnectionString(), connectionString());
}

TEST_P(WebsocketModulesChannelTest, GetConfigurationConnectionInfoIPv6)
{
    // SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();

    auto client = Instance("[[none]]");
    addLtClientModule(client);
    client.addDevice(connectionString("[::1]", ""), deviceConfig(client));

    auto devices = client.getDevices();
    ASSERT_EQ(devices.getCount(), 1u);

    auto connectionInfo = devices[0].getInfo().getConfigurationConnectionInfo();
    ASSERT_EQ(connectionInfo.getProtocolId(), expectedProtocolId());
    ASSERT_EQ(connectionInfo.getProtocolName(), expectedProtocolId());
    ASSERT_EQ(connectionInfo.getProtocolType(), ProtocolType::Streaming);
    ASSERT_EQ(connectionInfo.getConnectionType(), "TCP/IP");
    ASSERT_EQ(connectionInfo.getAddresses()[0], "[::1]");
    ASSERT_EQ(connectionInfo.getPort(), expectedPort());
    ASSERT_EQ(connectionInfo.getPrefix(), expectedPrefix());
    ASSERT_EQ(connectionInfo.getConnectionString(), connectionString("[::1]", ""));
}

TEST_P(WebsocketModulesChannelTest, AddSignals)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();
    size_t addedSignalsCount = 0;
    std::promise<void> addSignalsPromise;
    std::future<void> addSignalsFuture = addSignalsPromise.get_future();
    client.getContext().getOnCoreEvent() +=
        [&](const ComponentPtr& /*comp*/, const CoreEventArgsPtr& args)
    {
        auto params = args.getParameters();
        if (static_cast<CoreEventId>(args.getEventId()) == CoreEventId::ComponentAdded)
        {
            ComponentPtr component = params.get("Component");
            ASSERT_TRUE(component.asPtrOrNull<ISignal>().assigned());
            addedSignalsCount++;
            if (addedSignalsCount == 2)
            {
                addSignalsPromise.set_value();
            }
        }
    };

    auto serverRefDevice = server.getDevices()[0];
    serverRefDevice.setPropertyValue("NumberOfChannels", 3);

    ASSERT_TRUE(addSignalsFuture.wait_for(std::chrono::seconds(10)) == std::future_status::ready);

    auto serverSignals = server.getSignals(search::Recursive(search::Any()));
    auto clientSignals = client.getSignals(search::Recursive(search::Any()));
    ASSERT_EQ(clientSignals.getCount(), 7u);

    testSignalDescriptorsByLocalId({"AI2"}, clientSignals, serverSignals);

    for (const auto& name : {"AI2", "AI2Time"})
    {
        auto signal = getSignalByName(clientSignals, name);
        ASSERT_TRUE(signal.assigned()) << "client signal not found: " << name;
        auto mirroredSignalPtr = signal.asPtr<IMirroredSignalConfig>();
        ASSERT_EQ(mirroredSignalPtr.getStreamingSources().getCount(), 1u) << signal.getGlobalId();
        ASSERT_TRUE(mirroredSignalPtr.getActiveStreamingSource().assigned()) << signal.getGlobalId();
    }
}

TEST_P(WebsocketModulesChannelTest, RemoveSignals)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();
    auto clientSignals = client.getSignals(search::Recursive(search::Any()));

    auto removedValueSignal = getSignalByName(clientSignals, "AI1");
    auto removedDomainSignal = getSignalByName(clientSignals, "AI1Time");
    ASSERT_TRUE(removedValueSignal.assigned());
    ASSERT_TRUE(removedDomainSignal.assigned());

    size_t removedSignalsCount = 0;
    std::promise<void> removedSignalsPromise;
    std::future<void> removedSignalsFuture = removedSignalsPromise.get_future();
    client.getContext().getOnCoreEvent() +=
        [&](const ComponentPtr& comp, const CoreEventArgsPtr& args)
    {
        auto params = args.getParameters();
        if (static_cast<CoreEventId>(args.getEventId()) == CoreEventId::ComponentRemoved)
        {
            StringPtr id = params.get("Id");
            const auto removedGlobalId = comp.getGlobalId() + "/" + id;

            ASSERT_TRUE(removedGlobalId == removedValueSignal.getGlobalId() ||
                        removedGlobalId == removedDomainSignal.getGlobalId());
            removedSignalsCount++;
            if (removedSignalsCount == 2)
            {
                removedSignalsPromise.set_value();
            }
        }
    };

    auto serverRefDevice = server.getDevices()[0];
    serverRefDevice.setPropertyValue("NumberOfChannels", 1);

    ASSERT_TRUE(removedSignalsFuture.wait_for(std::chrono::seconds(10)) == std::future_status::ready);

    auto mirroredSignalPtr = removedValueSignal.asPtr<IMirroredSignalConfig>();
    ASSERT_EQ(mirroredSignalPtr.getStreamingSources().getCount(), 0u) << removedValueSignal.getGlobalId();
    ASSERT_FALSE(mirroredSignalPtr.getActiveStreamingSource().assigned()) << removedValueSignal.getGlobalId();
    ASSERT_TRUE(removedValueSignal.isRemoved());

    mirroredSignalPtr = removedDomainSignal.asPtr<IMirroredSignalConfig>();
    ASSERT_EQ(mirroredSignalPtr.getStreamingSources().getCount(), 0u) << removedDomainSignal.getGlobalId();
    ASSERT_FALSE(mirroredSignalPtr.getActiveStreamingSource().assigned()) << removedDomainSignal.getGlobalId();
    ASSERT_TRUE(removedDomainSignal.isRemoved());

    clientSignals = client.getSignals(search::Recursive(search::Any()));
    ASSERT_EQ(clientSignals.getCount(), 3u);
}

TEST_P(WebsocketModulesChannelTest, UpdateAddSignals)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();

    // save device config
    auto serverRefDevice = server.getDevices()[0];
    const auto serializer = JsonSerializer();
    serverRefDevice.serialize(serializer);
    const auto str = serializer.getOutput();

    // remove channel
    serverRefDevice.setPropertyValue("NumberOfChannels", 1);

    auto client = CreateClientInstance();

    size_t addedSignalsCount = 0;
    std::promise<void> addSignalsPromise;
    std::future<void> addSignalsFuture = addSignalsPromise.get_future();
    client.getContext().getOnCoreEvent() +=
        [&](const ComponentPtr& /*comp*/, const CoreEventArgsPtr& args)
    {
        auto params = args.getParameters();
        if (static_cast<CoreEventId>(args.getEventId()) == CoreEventId::ComponentAdded)
        {
            ComponentPtr component = params.get("Component");
            ASSERT_TRUE(component.asPtrOrNull<ISignal>().assigned());
            addedSignalsCount++;
            if (addedSignalsCount == 2)
            {
                addSignalsPromise.set_value();
            }
        }
    };

    // update device to backup removed channels
    const auto deserializer = JsonDeserializer();
    deserializer.update(serverRefDevice, str);

    ASSERT_TRUE(addSignalsFuture.wait_for(std::chrono::seconds(10)) == std::future_status::ready);

    auto serverSignals = server.getSignals(search::Recursive(search::Any()));
    auto clientSignals = client.getSignals(search::Recursive(search::Any()));
    ASSERT_EQ(clientSignals.getCount(), 5u);

    testSignalDescriptorsByLocalId({"AI0", "AI1"}, clientSignals, serverSignals);

    for (const auto& name : {"AI0", "AI0Time", "AI1", "AI1Time"})
    {
        auto signal = getSignalByName(clientSignals, name);
        ASSERT_TRUE(signal.assigned()) << "client signal not found: " << name;
        auto mirroredSignalPtr = signal.asPtr<IMirroredSignalConfig>();
        ASSERT_EQ(mirroredSignalPtr.getStreamingSources().getCount(), 1u) << signal.getGlobalId();
        ASSERT_TRUE(mirroredSignalPtr.getActiveStreamingSource().assigned()) << signal.getGlobalId();
    }
}

TEST_P(WebsocketModulesChannelTest, UpdateRemoveSignals)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();

    // save device config
    auto serverRefDevice = server.getDevices()[0];
    const auto serializer = JsonSerializer();
    serverRefDevice.serialize(serializer);
    const auto str = serializer.getOutput();

    // add extra channel
    serverRefDevice.setPropertyValue("NumberOfChannels", 3);

    auto client = CreateClientInstance();
    auto clientSignals = client.getSignals(search::Recursive(search::Any()));

    auto removedValueSignal = getSignalByName(clientSignals, "AI2");
    auto removedDomainSignal = getSignalByName(clientSignals, "AI2Time");
    ASSERT_TRUE(removedValueSignal.assigned());
    ASSERT_TRUE(removedDomainSignal.assigned());

    size_t removedSignalsCount = 0;
    std::promise<void> removedSignalsPromise;
    std::future<void> removedSignalsFuture = removedSignalsPromise.get_future();
    client.getContext().getOnCoreEvent() +=
        [&](const ComponentPtr& comp, const CoreEventArgsPtr& args)
    {
        auto params = args.getParameters();
        if (static_cast<CoreEventId>(args.getEventId()) == CoreEventId::ComponentRemoved)
        {
            StringPtr id = params.get("Id");
            const auto removedGlobalId = comp.getGlobalId() + "/" + id;

            ASSERT_TRUE(removedGlobalId == removedValueSignal.getGlobalId() ||
                        removedGlobalId == removedDomainSignal.getGlobalId());
            removedSignalsCount++;
            if (removedSignalsCount == 2)
            {
                removedSignalsPromise.set_value();
            }
        }
    };

    // update device to remove extra channel
    const auto deserializer = JsonDeserializer();
    deserializer.update(serverRefDevice, str);

    ASSERT_TRUE(removedSignalsFuture.wait_for(std::chrono::seconds(10)) == std::future_status::ready);

    auto mirroredSignalPtr = removedValueSignal.asPtr<IMirroredSignalConfig>();
    ASSERT_EQ(mirroredSignalPtr.getStreamingSources().getCount(), 0u) << removedValueSignal.getGlobalId();
    ASSERT_FALSE(mirroredSignalPtr.getActiveStreamingSource().assigned()) << removedValueSignal.getGlobalId();
    ASSERT_TRUE(removedValueSignal.isRemoved());

    mirroredSignalPtr = removedDomainSignal.asPtr<IMirroredSignalConfig>();
    ASSERT_EQ(mirroredSignalPtr.getStreamingSources().getCount(), 0u) << removedDomainSignal.getGlobalId();
    ASSERT_FALSE(mirroredSignalPtr.getActiveStreamingSource().assigned()) << removedDomainSignal.getGlobalId();
    ASSERT_TRUE(removedDomainSignal.isRemoved());

    clientSignals = client.getSignals(search::Recursive(search::Any()));
    ASSERT_EQ(clientSignals.getCount(), 5u);
}

INSTANTIATE_TEST_SUITE_P(WebsocketModulesTestGroup, WebsocketModulesChannelTest, testing::ValuesIn(GetChannelSuite()), ChannelName);
