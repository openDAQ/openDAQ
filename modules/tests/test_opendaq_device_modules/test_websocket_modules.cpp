#include "test_helpers/test_helpers.h"
#include <coreobjects/authentication_provider_factory.h>

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
            const auto domainIndex = i;
            const auto valueIndex = i + 1;

            ASSERT_TRUE(clientSignals[valueIndex].getDomainSignal().assigned());
            ASSERT_EQ(clientSignals[valueIndex].getDomainSignal(), clientSignals[domainIndex]);
            ASSERT_FALSE(clientSignals[domainIndex].getDomainSignal().assigned());

            DataDescriptorPtr dataDescriptor = clientSignals[valueIndex].getDescriptor();
            DataDescriptorPtr serverDataDescriptor = serverSignals[valueIndex].getDescriptor();

            DataDescriptorPtr domainDataDescriptor = clientSignals[valueIndex].getDomainSignal().getDescriptor();
            DataDescriptorPtr serverDomainDataDescriptor = serverSignals[valueIndex].getDomainSignal().getDescriptor();

            ASSERT_EQ(dataDescriptor, serverDataDescriptor);

            ASSERT_EQ(domainDataDescriptor.getRule().getParameters(), serverDomainDataDescriptor.getRule().getParameters());
            ASSERT_EQ(domainDataDescriptor.getOrigin(), serverDomainDataDescriptor.getOrigin());
            ASSERT_EQ(domainDataDescriptor.getTickResolution(), serverDomainDataDescriptor.getTickResolution());
        }
    }

    InstancePtr CreateServerInstance()
    {
        auto logger = Logger();
        auto scheduler = Scheduler(logger);
        auto moduleManager = ModuleManager("");
        auto typeManager = TypeManager();
        auto authenticationProvider = AuthenticationProvider();
        auto context = Context(scheduler, logger, typeManager, moduleManager, authenticationProvider);

        auto instance = InstanceCustom(context, "local");

        const auto refDevice = instance.addDevice("daqref://device1");

        instance.addServer("openDAQ LT Streaming", nullptr);

        return instance;
    }

    InstancePtr CreateClientInstance()
    {
        auto instance = Instance();
        auto refDevice = instance.addDevice("daq.lt://127.0.0.1/");
        return instance;
    }
};

TEST_F(WebsocketModulesTest, ConnectFail)
{
    ASSERT_THROW(CreateClientInstance(), NotFoundException);
}

TEST_F(WebsocketModulesTest, ConnectAndDisconnect)
{
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();
}

TEST_F(WebsocketModulesTest, ConnectAndDisconnectBackwardCompatibility)
{
    auto server = CreateServerInstance();
    auto client = Instance();
    client.addDevice("daq.ws://127.0.0.1/", nullptr);
}

TEST_F(WebsocketModulesTest, ConnectViaIpv6)
{
    if (test_helpers::Ipv6IsDisabled())
    {
        GTEST_SKIP() << "Ipv6 is disabled";
    }

    auto server = CreateServerInstance();
    auto client = Instance();
    client.addDevice("daq.lt://[::1]", nullptr);
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
    auto instance = InstanceBuilder().addConfigProvider(provider).build();
    auto serverConfig = instance.getAvailableServerTypes().get("OpenDAQLTStreaming").createDefaultConfig();

    ASSERT_EQ(serverConfig.getPropertyValue("WebsocketStreamingPort").asPtr<IInteger>(), 1234);
    ASSERT_EQ(serverConfig.getPropertyValue("Path").asPtr<IString>(), "/some/path");
}

TEST_F(WebsocketModulesTest, DiscoveringServer)
{
    auto server = InstanceBuilder().addDiscoveryServer("mdns").setDefaultRootDeviceLocalId("local").build();
    server.addDevice("daqref://device1");

    auto serverConfig = server.getAvailableServerTypes().get("OpenDAQLTStreaming").createDefaultConfig();
    auto path = "/test/streaming_lt/discovery/";
    serverConfig.setPropertyValue("Path", path);
    server.addServer("OpenDAQLTStreaming", serverConfig).enableDiscovery();

    auto client = Instance();
    DevicePtr device;
    for (const auto & deviceInfo : client.getAvailableDevices())
    {
        for (const auto & capability : deviceInfo.getServerCapabilities())
        {
            if (!test_helpers::isSufix(deviceInfo.getConnectionString(), path))
            {
                break;
            }
            if (capability.getProtocolName() == "OpenDAQLTStreaming")
            {
                device = client.addDevice(deviceInfo.getConnectionString(), nullptr);
                return;
            }
        }
    }
    ASSERT_TRUE(false) << "Device not found";
}


TEST_F(WebsocketModulesTest, checkDeviceInfoPopulatedWithProvider)
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
    auto instance = InstanceBuilder().addDiscoveryServer("mdns")
                                     .addConfigProvider(provider)
                                     .setDefaultRootDeviceInfo(rootInfo)
                                     .build();
    instance.addDevice("daqref://device1");
    auto serverConfig = instance.getAvailableServerTypes().get("OpenDAQLTStreaming").createDefaultConfig();
    instance.addServer("OpenDAQLTStreaming", serverConfig).enableDiscovery();

    auto client = Instance();

    for (const auto & deviceInfo : client.getAvailableDevices())
    {
        for (const auto & capability : deviceInfo.getServerCapabilities())
        {
            if (capability.getProtocolName() == "OpenDAQLTStreaming")
            {
                if (!test_helpers::isSufix(capability.getConnectionString(), path))
                {
                    break;
                }

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

TEST_F(WebsocketModulesTest, TestDiscoveryReachability)
{
    if (test_helpers::Ipv6IsDisabled())
    {
        GTEST_SKIP() << "Ipv6 is disabled";
    }

    auto instance = InstanceBuilder().addDiscoveryServer("mdns").build();
    auto serverConfig = instance.getAvailableServerTypes().get("OpenDAQLTStreaming").createDefaultConfig();
    auto path = "/test/lt/discovery_reachability/";
    serverConfig.setPropertyValue("Path", path);

    instance.addServer("OpenDAQLTStreaming", serverConfig).enableDiscovery();

    auto client = Instance();

    for (const auto & deviceInfo : client.getAvailableDevices())
    {
        for (const auto & capability : deviceInfo.getServerCapabilities())
        {
            if (!test_helpers::isSufix(capability.getConnectionString(), path))
                break;

            if (capability.getProtocolName() == "OpenDAQLTStreaming")
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
            }
        }      
    }
}

#endif

TEST_F(WebsocketModulesTest, GetRemoteDeviceObjects)
{
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    ASSERT_EQ(client.getDevices().getCount(), 1u);
    auto signals = client.getSignals(search::Recursive(search::Visible()));
    ASSERT_EQ(signals.getCount(), 5u);
}

TEST_F(WebsocketModulesTest, RemoveDevice)
{
    auto server = CreateServerInstance();
    auto client = Instance();
    auto device = client.addDevice("daq.lt://127.0.0.1/");

    ASSERT_NO_THROW(client.removeDevice(device));
    ASSERT_TRUE(device.isRemoved());
}

TEST_F(WebsocketModulesTest, SignalConfig_Server)
{
    const std::string newSignalName{"some new name"};

    auto server = CreateServerInstance();

    auto serverSignal = server.getSignals(search::Recursive(search::Visible()))[0].asPtr<ISignalConfig>();
    auto serverSignalDataDescriptor = DataDescriptorBuilderCopy(serverSignal.getDescriptor()).setName(newSignalName).build();
    serverSignal.setDescriptor(serverSignalDataDescriptor);

    auto client = CreateClientInstance();

    auto clientSignals = client.getDevices()[0].getSignals(search::Recursive(search::Visible()));
    auto clientSignal = clientSignals[1].asPtr<ISignalConfig>();

    auto clientSignalDataDescriptor = DataDescriptorBuilderCopy(clientSignal.getDescriptor()).build();

    ASSERT_EQ(serverSignal.getDescriptor().getName(), newSignalName);
    ASSERT_EQ(serverSignal.getDescriptor().getName(), clientSignal.getDescriptor().getName());
}

TEST_F(WebsocketModulesTest, DataDescriptor)
{
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    testSignalDescriptors(0u,
                          4u,
                          client.getSignals(search::Recursive(search::Any())),
                          server.getSignals(search::Recursive(search::Any())));
}

TEST_F(WebsocketModulesTest, SubscribeReadUnsubscribe)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    auto signal = client.getSignalsRecursive()[1].template asPtr<IMirroredSignalConfig>();

    StringPtr streamingSource = signal.getActiveStreamingSource();

    std::promise<StringPtr> signalSubscribePromise;
    std::future<StringPtr> signalSubscribeFuture;
    test_helpers::setupSubscribeAckHandler(signalSubscribePromise, signalSubscribeFuture, signal);

    std::promise<StringPtr> signalUnsubscribePromise;
    std::future<StringPtr> signalUnsubscribeFuture;
    test_helpers::setupUnsubscribeAckHandler(signalUnsubscribePromise, signalUnsubscribeFuture, signal);

    using namespace std::chrono_literals;
    StreamReaderPtr reader = daq::StreamReader<double, uint64_t>(signal, ReadTimeoutType::Any);

    ASSERT_TRUE(test_helpers::waitForAcknowledgement(signalSubscribeFuture));
    ASSERT_EQ(signalSubscribeFuture.get(), streamingSource);

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
}

TEST_F(WebsocketModulesTest, DISABLED_RenderSignal)
{
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    auto signals = client.getSignals(search::Recursive(search::Visible()));
    const auto renderer = client.addFunctionBlock("RefFBModuleRenderer");
    renderer.getInputPorts()[0].connect(signals[0]);

    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
}

TEST_F(WebsocketModulesTest, GetConfigurationConnectionInfo)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    auto devices = client.getDevices();
    ASSERT_EQ(devices.getCount(), 1u);

    auto connectionInfo = devices[0].getInfo().getConfigurationConnectionInfo();
    ASSERT_EQ(connectionInfo.getProtocolId(), "OpenDAQLTStreaming");
    ASSERT_EQ(connectionInfo.getProtocolName(), "OpenDAQLTStreaming");
    ASSERT_EQ(connectionInfo.getProtocolType(), ProtocolType::Streaming);
    ASSERT_EQ(connectionInfo.getConnectionType(), "TCP/IP");
    ASSERT_EQ(connectionInfo.getAddresses()[0], "127.0.0.1");
    ASSERT_EQ(connectionInfo.getPort(), 7414);
    ASSERT_EQ(connectionInfo.getPrefix(), "daq.lt");
    ASSERT_EQ(connectionInfo.getConnectionString(), "daq.lt://127.0.0.1/");
}
TEST_F(WebsocketModulesTest, AddSignals)
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

    removeDeviceDomainSignal(serverSignals);
    removeDeviceDomainSignal(clientSignals);

    testSignalDescriptors(4u, 6u, clientSignals, serverSignals);

    for (size_t i = 4; i < clientSignals.getCount(); ++i)
    {
        auto mirroredSignalPtr = clientSignals[i].asPtr<IMirroredSignalConfig>();
        ASSERT_EQ(mirroredSignalPtr.getStreamingSources().getCount(), 1u) << clientSignals[i].getGlobalId();
        ASSERT_TRUE(mirroredSignalPtr.getActiveStreamingSource().assigned()) << clientSignals[i].getGlobalId();
    }
}

TEST_F(WebsocketModulesTest, RemoveSignals)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    auto clientSignals = client.getSignals(search::Recursive(search::Any()));

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

            ASSERT_TRUE((comp.getGlobalId() + "/" + id) == clientSignals[2].getGlobalId() ||
                        (comp.getGlobalId() + "/" + id) == clientSignals[3].getGlobalId());
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

    auto mirroredSignalPtr = clientSignals[2].asPtr<IMirroredSignalConfig>();
    ASSERT_EQ(mirroredSignalPtr.getStreamingSources().getCount(), 0u) << clientSignals[2].getGlobalId();
    ASSERT_FALSE(mirroredSignalPtr.getActiveStreamingSource().assigned()) << clientSignals[2].getGlobalId();
    ASSERT_TRUE(clientSignals[2].isRemoved());

    mirroredSignalPtr = clientSignals[3].asPtr<IMirroredSignalConfig>();
    ASSERT_EQ(mirroredSignalPtr.getStreamingSources().getCount(), 0u) << clientSignals[3].getGlobalId();
    ASSERT_FALSE(mirroredSignalPtr.getActiveStreamingSource().assigned()) << clientSignals[3].getGlobalId();
    ASSERT_TRUE(clientSignals[3].isRemoved());

    clientSignals = client.getSignals(search::Recursive(search::Any()));
    ASSERT_EQ(clientSignals.getCount(), 3u);
}

TEST_F(WebsocketModulesTest, UpdateAddSignals)
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

    // update device to restore removed channels
    const auto deserializer = JsonDeserializer();
    deserializer.update(serverRefDevice, str);

    ASSERT_TRUE(addSignalsFuture.wait_for(std::chrono::seconds(10)) == std::future_status::ready);

    auto serverSignals = server.getSignals(search::Recursive(search::Any()));
    auto clientSignals = client.getSignals(search::Recursive(search::Any()));
    ASSERT_EQ(clientSignals.getCount(), 5u);

    removeDeviceDomainSignal(serverSignals);
    removeDeviceDomainSignal(clientSignals);

    testSignalDescriptors(0u, 4u, clientSignals, serverSignals);

    for (size_t i = 0; i < clientSignals.getCount(); ++i)
    {
        auto mirroredSignalPtr = clientSignals[i].asPtr<IMirroredSignalConfig>();
        ASSERT_EQ(mirroredSignalPtr.getStreamingSources().getCount(), 1u) << clientSignals[i].getGlobalId();
        ASSERT_TRUE(mirroredSignalPtr.getActiveStreamingSource().assigned()) << clientSignals[i].getGlobalId();
    }
}

TEST_F(WebsocketModulesTest, UpdateRemoveSignals)
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

            ASSERT_TRUE((comp.getGlobalId() + "/" + id) == clientSignals[4].getGlobalId() ||
                        (comp.getGlobalId() + "/" + id) == clientSignals[5].getGlobalId());
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

    auto mirroredSignalPtr = clientSignals[4].asPtr<IMirroredSignalConfig>();
    ASSERT_EQ(mirroredSignalPtr.getStreamingSources().getCount(), 0u) << clientSignals[4].getGlobalId();
    ASSERT_FALSE(mirroredSignalPtr.getActiveStreamingSource().assigned()) << clientSignals[4].getGlobalId();
    ASSERT_TRUE(clientSignals[4].isRemoved());

    mirroredSignalPtr = clientSignals[5].asPtr<IMirroredSignalConfig>();
    ASSERT_EQ(mirroredSignalPtr.getStreamingSources().getCount(), 0u) << clientSignals[5].getGlobalId();
    ASSERT_FALSE(mirroredSignalPtr.getActiveStreamingSource().assigned()) << clientSignals[5].getGlobalId();
    ASSERT_TRUE(clientSignals[5].isRemoved());

    clientSignals = client.getSignals(search::Recursive(search::Any()));
    ASSERT_EQ(clientSignals.getCount(), 5u);
}
