#include "test_helpers/test_helpers.h"
#include <coreobjects/authentication_provider_factory.h>
#include <coreobjects/permissions_builder_factory.h>
#include <coreobjects/permission_mask_builder_factory.h>
#include <coreobjects/user_factory.h>
#include <opendaq/mock/mock_device_module.h>

using NativeStreamingModulesTest = testing::Test;

using namespace daq;

static InstancePtr CreateServerInstance(const AuthenticationProviderPtr& authenticationProvider)
{
    auto logger = Logger();
    auto scheduler = Scheduler(logger);
    auto moduleManager = ModuleManager("");
    auto context = Context(scheduler, logger, TypeManager(), moduleManager, authenticationProvider);

    auto instance = InstanceCustom(context, "local");

    const auto refDevice = instance.addDevice("daqref://device1");

    instance.addServer("OpenDAQNativeStreaming", nullptr);

    return instance;
}

static InstancePtr CreateServerInstance()
{
    auto authenticationProvider = AuthenticationProvider();
    return CreateServerInstance(authenticationProvider);
}

static InstancePtr CreateClientInstance(const std::string& username, const std::string& password)
{
    auto instance = Instance();

    auto config = instance.createDefaultAddDeviceConfig();
    PropertyObjectPtr general = config.getPropertyValue("General");
    general.setPropertyValue("Username", username);
    general.setPropertyValue("Password", password);

    auto refDevice = instance.addDevice("daq.ns://127.0.0.1/", config);
    return instance;
}

static InstancePtr CreateClientInstance()
{
    auto instance = Instance();
    auto refDevice = instance.addDevice("daq.ns://127.0.0.1/");
    return instance;
}

TEST_F(NativeStreamingModulesTest, ConnectFail)
{
    ASSERT_THROW(CreateClientInstance(), NotFoundException);
}

TEST_F(NativeStreamingModulesTest, ConnectAndDisconnect)
{
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();
}

TEST_F(NativeStreamingModulesTest, ConnectViaIpv6)
{
    if (test_helpers::Ipv6IsDisabled())
        return;

    auto server = CreateServerInstance();

    auto client = Instance();
    ASSERT_NO_THROW(client.addDevice("daq.ns://[::1]", nullptr));
}

TEST_F(NativeStreamingModulesTest, PopulateDefaultConfigFromProvider)
{
    std::string filename = "populateDefaultConfig.json";
    std::string json = R"(
        {
            "Modules":
            {
                "OpenDAQNativeStreamingServerModule":
                {
                    "NativeStreamingPort": 1234,
                    "Path": "/some/path"
                }
            }
        }
    )";
    auto finally = test_helpers::CreateConfigFile(filename, json);

    auto provider = JsonConfigProvider(filename);
    auto instance = InstanceBuilder().addConfigProvider(provider).build();
    auto serverConfig = instance.getAvailableServerTypes().get("OpenDAQNativeStreaming").createDefaultConfig();

    ASSERT_EQ(serverConfig.getPropertyValue("NativeStreamingPort").asPtr<IInteger>(), 1234);
    ASSERT_EQ(serverConfig.getPropertyValue("Path").asPtr<IString>(), "/some/path");
}

TEST_F(NativeStreamingModulesTest, DiscoveringServer)
{
    auto server = InstanceBuilder().addDiscoveryServer("mdns")
                                   .setDefaultRootDeviceLocalId("local")
                                   .build();
    server.addDevice("daqref://device1");

    auto serverConfig = server.getAvailableServerTypes().get("OpenDAQNativeStreaming").createDefaultConfig();
    auto path = "/test/native_streaming/discovery/";
    serverConfig.setPropertyValue("Path", path);
    server.addServer("OpenDAQNativeStreaming", serverConfig).enableDiscovery();

    auto client = Instance();
    DevicePtr device;
    for (const auto & deviceInfo : client.getAvailableDevices())
    {
        for (const auto & capability : deviceInfo.getServerCapabilities())
        {
            if (!test_helpers::isSufix(capability.getConnectionString(), path))
            {
                break;
            }
            if (capability.getProtocolName() == "OpenDAQNativeStreaming")
            {
                device = client.addDevice(capability.getConnectionString(), nullptr);
                return;
            }
        }
    }
    ASSERT_TRUE(false) << "Device not found";
}

TEST_F(NativeStreamingModulesTest, DiscoveringServerUsernameLocation)
{
    auto server = InstanceBuilder().addDiscoveryServer("mdns")
                                   .setDefaultRootDeviceLocalId("local")
                                   .setRootDevice("daqref://device0")
                                   .build();

    // set initial username and location
    server.setPropertyValue("userName", "testUser1");
    server.setPropertyValue("location", "testLocation1");
    server.getInfo().setPropertyValue("SetupDate", "2025-01-16T08:23:22Z");

    ASSERT_EQ(server.getInfo().getPropertyValue("userName"), "testUser1");
    ASSERT_EQ(server.getInfo().getPropertyValue("location"), "testLocation1");
    ASSERT_EQ(server.getInfo().getPropertyValue("SetupDate"), "2025-01-16T08:23:22Z");

    auto serverConfig = server.getAvailableServerTypes().get("OpenDAQNativeStreaming").createDefaultConfig();
    auto path = "/test/native_streaming/discovery/username_location/";
    serverConfig.setPropertyValue("Path", path);
    server.addServer("OpenDAQNativeStreaming", serverConfig).enableDiscovery();

    // update the username and location after server creation
    server.setPropertyValue("userName", "testUser2");
    server.setPropertyValue("location", "testLocation2");
    server.getInfo().setPropertyValue("SetupDate", "2025-01-17T08:23:22Z");

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    auto client = Instance();
    DevicePtr device;
    for (const auto & deviceInfo : client.getAvailableDevices())
    {
        for (const auto & capability : deviceInfo.getServerCapabilities())
        {
            if (!test_helpers::isSufix(capability.getConnectionString(), path))
                break;

            ASSERT_EQ(deviceInfo.getPropertyValue("userName"), "testUser2");
            ASSERT_EQ(deviceInfo.getPropertyValue("location"), "testLocation2");
            ASSERT_EQ(deviceInfo.getPropertyValue("SetupDate"), "2025-01-17T08:23:22Z");
            if (capability.getProtocolName() == "OpenDAQNativeStreaming")
                return;
        }
    }
    ASSERT_TRUE(false) << "Device not found";
}

#ifdef _WIN32

TEST_F(NativeStreamingModulesTest, TestDiscoveryReachability)
{
    if (test_helpers::Ipv6IsDisabled())
        return;

    auto instance = InstanceBuilder().addDiscoveryServer("mdns").build();
    auto serverConfig = instance.getAvailableServerTypes().get("OpenDAQNativeStreaming").createDefaultConfig();
    auto path = "/test/native_streaming/discovery_reachability/";
    serverConfig.setPropertyValue("Path", path);

    instance.addServer("OpenDAQNativeStreaming", serverConfig).enableDiscovery();

    auto client = Instance();

    for (const auto & deviceInfo : client.getAvailableDevices())
    {
        for (const auto & capability : deviceInfo.getServerCapabilities())
        {
            if (!test_helpers::isSufix(capability.getConnectionString(), path))
                break;

            if (capability.getProtocolName() == "OpenDAQNativeStreaming")
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

TEST_F(NativeStreamingModulesTest, checkDeviceInfoPopulatedWithProvider)
{
    std::string filename = "populateDefaultConfig.json";
    std::string json = R"(
        {
            "Modules":
            {
                "OpenDAQNativeStreamingServerModule":
                {
                    "NativeStreamingPort": 1234,
                    "Path": "/test/native/checkDeviceInfoPopulated/"
                }
            }
        }
    )";
    auto path = "/test/native/checkDeviceInfoPopulated/";
    auto finally = test_helpers::CreateConfigFile(filename, json);

    auto rootInfo = DeviceInfo("");
    rootInfo.setName("TestName");
    rootInfo.setManufacturer("TestManufacturer");
    rootInfo.setModel("TestModel");
    rootInfo.setSerialNumber("TestSerialNumber");

    auto provider = JsonConfigProvider(filename);
    auto instance = InstanceBuilder().addDiscoveryServer("mdns").addConfigProvider(provider).setDefaultRootDeviceInfo(rootInfo).build();
    auto serverConfig = instance.getAvailableServerTypes().get("OpenDAQNativeStreaming").createDefaultConfig();
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
            if (capability.getProtocolName() == "OpenDAQNativeStreaming")
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


TEST_F(NativeStreamingModulesTest, GetRemoteDeviceObjects)
{
    auto server = CreateServerInstance();
    auto serverSignals = server.getSignals(search::Recursive(search::Any()));
    serverSignals[0].setName("NewName");
    serverSignals[0].setDescription("NewDescription");

    auto client = CreateClientInstance();

    ASSERT_EQ(client.getDevices().getCount(), 1u);
    auto clientSignals = client.getSignals(search::Recursive(search::Any()));
    ASSERT_EQ(clientSignals.getCount(), 5u);

    ASSERT_EQ(clientSignals[0].getDomainSignal(), clientSignals[1]);
    ASSERT_TRUE(!clientSignals[1].getDomainSignal().assigned());
    ASSERT_EQ(clientSignals[2].getDomainSignal(), clientSignals[3]);
    ASSERT_TRUE(!clientSignals[3].getDomainSignal().assigned());

    for (size_t i = 0; i < clientSignals.getCount(); ++i)
    {
        auto serverSignal = serverSignals[i];
        auto clientSignal = clientSignals[i];
        auto serverDataDescriptor = serverSignal.getDescriptor();
        auto clientDataDescriptor = clientSignal.getDescriptor();

        ASSERT_EQ(clientDataDescriptor, serverDataDescriptor);

        ASSERT_EQ(serverSignal.getName(), clientSignal.getName());
        ASSERT_EQ(serverSignal.getDescription(), clientSignal.getDescription());

        auto mirroredSignalPtr = clientSignal.asPtr<IMirroredSignalConfig>();
        ASSERT_GT(mirroredSignalPtr.getStreamingSources().getCount(), 0u) << clientSignal.getGlobalId();
        ASSERT_TRUE(mirroredSignalPtr.getActiveStreamingSource().assigned()) << clientSignal.getGlobalId();
    }

    ASSERT_EQ(serverSignals[0].getName(), clientSignals[0].getName());

    DeviceInfoPtr info;
    ASSERT_NO_THROW(info = client.getDevices()[0].getInfo());
    ASSERT_TRUE(info.assigned());
    ASSERT_EQ(info.getConnectionString(), "daq.ns://127.0.0.1/");
    ASSERT_EQ(info.getName(), "NativeStreamingClientPseudoDevice");
}

TEST_F(NativeStreamingModulesTest, RemoveDevice)
{
    auto server = CreateServerInstance();
    auto client = Instance();
    auto device = client.addDevice("daq.ns://127.0.0.1/");

    ASSERT_NO_THROW(client.removeDevice(device));
    ASSERT_TRUE(device.isRemoved());
}

TEST_F(NativeStreamingModulesTest, SubscribeReadUnsubscribe)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    auto signal = client.getSignalsRecursive()[0].template asPtr<IMirroredSignalConfig>();
    auto domainSignal = signal.getDomainSignal().template asPtr<IMirroredSignalConfig>();

    StringPtr streamingSource = signal.getActiveStreamingSource();
    ASSERT_TRUE(domainSignal.assigned());
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

TEST_F(NativeStreamingModulesTest, DISABLED_RenderSignal)
{
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    auto signals = client.getSignals(search::Recursive(search::Visible()));
    const auto renderer = client.addFunctionBlock("RefFBModuleRenderer");
    renderer.getInputPorts()[0].connect(signals[0]);

    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
}

TEST_F(NativeStreamingModulesTest, GetRemoteDeviceObjectsAfterReconnect)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    ASSERT_EQ(client.getDevices()[0].getConnectionStatusContainer().getStatus("StreamingStatus_OpenDAQNativeStreaming_1"), "Connected");
    ASSERT_EQ(client.getDevices()[0].getConnectionStatusContainer().getStatusMessage("StreamingStatus_OpenDAQNativeStreaming_1"), "");

    std::promise<StringPtr> connectionStatusPromise;
    std::future<StringPtr> connectionStatusFuture = connectionStatusPromise.get_future();
    client.getDevices()[0].getOnComponentCoreEvent() +=
        [&](const ComponentPtr& /*comp*/, const CoreEventArgsPtr& args)
    {
        auto params = args.getParameters();
        if (static_cast<CoreEventId>(args.getEventId()) == CoreEventId::ConnectionStatusChanged)
        {
            ASSERT_TRUE(args.getParameters().hasKey("StatusName"));
            EXPECT_TRUE(args.getParameters().get("StatusName").assigned());
            ASSERT_TRUE(args.getParameters().hasKey("ConnectionString"));
            EXPECT_EQ(args.getParameters().get("ConnectionString"), "daq.ns://127.0.0.1/");
            ASSERT_TRUE(args.getParameters().hasKey("StreamingObject"));
            EXPECT_TRUE(args.getParameters().get("StreamingObject").assigned());
            ASSERT_TRUE(args.getParameters().hasKey("StatusValue"));
            const EnumerationPtr statusValue = args.getParameters().get("StatusValue");
            connectionStatusPromise.set_value(statusValue.toString());

            ASSERT_TRUE(args.getParameters().hasKey("Message"));
            const StringPtr statusMessage = args.getParameters().get("Message");
            if (statusValue == "Reconnecting")
            {
                EXPECT_EQ(statusMessage, "Network connection interrupted or closed by the remote device");
            }
            else if (statusValue == "Connected")
            {
                EXPECT_EQ(statusMessage, "");
            }
        }
    };

    auto clientSignalsBeforeDisconnection = client.getSignals(search::Recursive(search::Any()));

    // destroy server to emulate disconnection
    server.release();
    ASSERT_TRUE(connectionStatusFuture.wait_for(std::chrono::seconds(5)) == std::future_status::ready);
    ASSERT_EQ(connectionStatusFuture.get(), "Reconnecting");
    ASSERT_EQ(client.getDevices()[0].getConnectionStatusContainer().getStatus("StreamingStatus_OpenDAQNativeStreaming_1"), "Reconnecting");
    ASSERT_EQ(client.getDevices()[0].getConnectionStatusContainer().getStatusMessage("StreamingStatus_OpenDAQNativeStreaming_1"),
              "Network connection interrupted or closed by the remote device");

    // reset future / promise
    connectionStatusPromise = std::promise<StringPtr>();
    connectionStatusFuture = connectionStatusPromise.get_future();

    // re-create server to enable reconnection
    server = CreateServerInstance();
    auto serverSignals = server.getSignals(search::Recursive(search::Any()));

    ASSERT_TRUE(connectionStatusFuture.wait_for(std::chrono::seconds(5)) == std::future_status::ready);
    ASSERT_EQ(connectionStatusFuture.get(), "Connected");
    ASSERT_EQ(client.getDevices()[0].getConnectionStatusContainer().getStatus("StreamingStatus_OpenDAQNativeStreaming_1"), "Connected");
    ASSERT_EQ(client.getDevices()[0].getConnectionStatusContainer().getStatusMessage("StreamingStatus_OpenDAQNativeStreaming_1"), "");

    auto clientSignalsAfterReconnection = client.getSignals(search::Recursive(search::Any()));
    ASSERT_EQ(clientSignalsAfterReconnection.getCount(), clientSignalsBeforeDisconnection.getCount());
    for (size_t i = 0; i < clientSignalsAfterReconnection.getCount(); ++i)
    {
        ASSERT_EQ(clientSignalsAfterReconnection[i], clientSignalsBeforeDisconnection[i]);
    }

    ASSERT_EQ(clientSignalsAfterReconnection[0].getDomainSignal(), clientSignalsAfterReconnection[1]);
    ASSERT_TRUE(!clientSignalsAfterReconnection[1].getDomainSignal().assigned());
    ASSERT_EQ(clientSignalsAfterReconnection[2].getDomainSignal(), clientSignalsAfterReconnection[3]);
    ASSERT_TRUE(!clientSignalsAfterReconnection[3].getDomainSignal().assigned());

    for (size_t i = 0; i < serverSignals.getCount(); ++i)
    {
        auto serverDataDescriptor = serverSignals[i].getDescriptor();
        auto clientDataDescriptor = clientSignalsAfterReconnection[i].getDescriptor();

        ASSERT_EQ(clientDataDescriptor, serverDataDescriptor);

        //ASSERT_EQ(serverSignals[i].getName(), clientSignalsAfterReconnection[i].getName());
        ASSERT_EQ(serverSignals[i].getDescription(), clientSignalsAfterReconnection[i].getDescription());
    }
}

TEST_F_UNSTABLE_SKIPPED(NativeStreamingModulesTest, ReconnectWhileRead)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    ASSERT_EQ(client.getDevices()[0].getStatusContainer().getStatus("ConnectionStatus"), "Connected");
    ASSERT_EQ(client.getDevices()[0].getConnectionStatusContainer().getStatus("StreamingStatus_OpenDAQNativeStreaming_1"), "Connected");

    std::promise<StringPtr> connectionOldStatusPromise;
    std::future<StringPtr> connectionOldStatusFuture = connectionOldStatusPromise.get_future();
    std::promise<StringPtr> connectionStatusPromise;
    std::future<StringPtr> connectionStatusFuture = connectionStatusPromise.get_future();
    client.getDevices()[0].getOnComponentCoreEvent() +=
        [&](const ComponentPtr& /*comp*/, const CoreEventArgsPtr& args)
    {
        auto params = args.getParameters();
        if (static_cast<CoreEventId>(args.getEventId()) == CoreEventId::ConnectionStatusChanged)
        {
            ASSERT_TRUE(args.getParameters().hasKey("StatusName"));
            EXPECT_TRUE(args.getParameters().get("StatusName").assigned());
            ASSERT_TRUE(args.getParameters().hasKey("ConnectionString"));
            EXPECT_EQ(args.getParameters().get("ConnectionString"), "daq.ns://127.0.0.1/");
            ASSERT_TRUE(args.getParameters().hasKey("StreamingObject"));
            EXPECT_TRUE(args.getParameters().get("StreamingObject").assigned());
            ASSERT_TRUE(args.getParameters().hasKey("StatusValue"));
            connectionStatusPromise.set_value(args.getParameters().get("StatusValue").toString());
        }
        else if (static_cast<CoreEventId>(args.getEventId()) == CoreEventId::StatusChanged)
        {
            ASSERT_TRUE(args.getParameters().hasKey("ConnectionStatus"));
            connectionOldStatusPromise.set_value(args.getParameters().get("ConnectionStatus").toString());
        }
    };

    auto signal = client.getSignals(search::Recursive(search::Any()))[0].template asPtr<IMirroredSignalConfig>();
    auto domainSignal = signal.getDomainSignal().template asPtr<IMirroredSignalConfig>();

    std::promise<StringPtr> signalSubscribePromise;
    std::future<StringPtr> signalSubscribeFuture;
    std::promise<StringPtr> domainSubscribePromise;
    std::future<StringPtr> domainSubscribeFuture;

    test_helpers::setupSubscribeAckHandler(signalSubscribePromise, signalSubscribeFuture, signal);
    test_helpers::setupSubscribeAckHandler(domainSubscribePromise, domainSubscribeFuture, domainSignal);

    using namespace std::chrono_literals;
    StreamReaderPtr reader = daq::StreamReader<double, uint64_t>(signal, ReadTimeoutType::Any);

    // wait for subscribe ack before read
    ASSERT_TRUE(test_helpers::waitForAcknowledgement(signalSubscribeFuture));
    ASSERT_TRUE(test_helpers::waitForAcknowledgement(domainSubscribeFuture));

    {
        daq::SizeT count = 0;
        reader.read(nullptr, &count, 100);
    }

    // destroy server to emulate disconnection
    server.release();
    ASSERT_TRUE(connectionOldStatusFuture.wait_for(std::chrono::seconds(5)) == std::future_status::ready);
    ASSERT_EQ(connectionOldStatusFuture.get(), "Reconnecting");
    ASSERT_EQ(client.getDevices()[0].getStatusContainer().getStatus("ConnectionStatus"), "Reconnecting");
    ASSERT_TRUE(connectionStatusFuture.wait_for(std::chrono::seconds(5)) == std::future_status::ready);
    ASSERT_EQ(connectionStatusFuture.get(), "Reconnecting");
    ASSERT_EQ(client.getDevices()[0].getConnectionStatusContainer().getStatus("StreamingStatus_OpenDAQNativeStreaming_1"), "Reconnecting");

    // reset future / promise
    connectionOldStatusPromise = std::promise<StringPtr>();
    connectionOldStatusFuture = connectionOldStatusPromise.get_future();
    connectionStatusPromise = std::promise<StringPtr>();
    connectionStatusFuture = connectionStatusPromise.get_future();

    {
        double samples[1000];

        // read all data received from server before disconnection
        daq::SizeT count = 1000;
        reader.read(samples, &count, 100);

        count = 1000;
        reader.read(samples, &count, 100);
        EXPECT_EQ(count, 0u);
    }

    signalSubscribePromise = std::promise<StringPtr>();
    signalSubscribeFuture = signalSubscribePromise.get_future();
    domainSubscribePromise = std::promise<StringPtr>();
    domainSubscribeFuture = domainSubscribePromise.get_future();

    // re-create server to enable reconnection
    server = CreateServerInstance();

    ASSERT_TRUE(connectionOldStatusFuture.wait_for(std::chrono::seconds(5)) == std::future_status::ready);
    ASSERT_EQ(connectionOldStatusFuture.get(), "Connected");
    ASSERT_EQ(client.getDevices()[0].getStatusContainer().getStatus("ConnectionStatus"), "Connected");
    ASSERT_TRUE(connectionStatusFuture.wait_for(std::chrono::seconds(5)) == std::future_status::ready);
    ASSERT_EQ(connectionStatusFuture.get(), "Connected");
    ASSERT_EQ(client.getDevices()[0].getConnectionStatusContainer().getStatus("StreamingStatus_OpenDAQNativeStreaming_1"), "Connected");

    // wait for new subscribe ack before further reading
    ASSERT_TRUE(test_helpers::waitForAcknowledgement(signalSubscribeFuture));
    ASSERT_TRUE(test_helpers::waitForAcknowledgement(domainSubscribeFuture));

    // No descriptor changed packet, as the descriptor did not change
    // read data received from server after reconnection
    for (int i = 0; i < 5; ++i)
    {
        daq::SizeT count = 100;
        double samples[100];
        reader.read(samples, &count, 1000);
        EXPECT_GT(count, 0u) << "iteration " << i;
    }
}

TEST_F(NativeStreamingModulesTest, AddSignals)
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

    ASSERT_TRUE(addSignalsFuture.wait_for(std::chrono::seconds(1)) == std::future_status::ready);

    auto serverSignals = server.getSignals(search::Recursive(search::Any()));
    auto clientSignals = client.getSignals(search::Recursive(search::Any()));
    ASSERT_EQ(clientSignals.getCount(), 7u);

    ASSERT_EQ(clientSignals[5].getDomainSignal(), clientSignals[6]);
    ASSERT_TRUE(!clientSignals[6].getDomainSignal().assigned());

    ASSERT_EQ(serverSignals[6].getDescriptor(), clientSignals[4].getDescriptor());

    removeDeviceDomainSignal(serverSignals);
    removeDeviceDomainSignal(clientSignals);

    for (size_t i = 0; i < clientSignals.getCount(); ++i)
    {
        auto serverDataDescriptor = serverSignals[i].getDescriptor();
        auto clientDataDescriptor = clientSignals[i].getDescriptor();

        ASSERT_EQ(clientDataDescriptor, serverDataDescriptor);

        auto mirroredSignalPtr = clientSignals[i].asPtr<IMirroredSignalConfig>();
        ASSERT_GT(mirroredSignalPtr.getStreamingSources().getCount(), 0u) << clientSignals[i].getGlobalId();
        ASSERT_TRUE(mirroredSignalPtr.getActiveStreamingSource().assigned()) << clientSignals[i].getGlobalId();
    }
}

TEST_F(NativeStreamingModulesTest, RemoveSignals)
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

    ASSERT_TRUE(removedSignalsFuture.wait_for(std::chrono::seconds(1)) == std::future_status::ready);

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

TEST_F(NativeStreamingModulesTest, GetConfigurationConnectionInfoIPv4)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    auto devices = client.getDevices();
    ASSERT_EQ(devices.getCount(), 1u);

    auto connectionInfo = devices[0].getInfo().getConfigurationConnectionInfo();
    ASSERT_EQ(connectionInfo.getProtocolId(), "OpenDAQNativeStreaming");
    ASSERT_EQ(connectionInfo.getProtocolName(), "OpenDAQNativeStreaming");
    ASSERT_EQ(connectionInfo.getProtocolType(), ProtocolType::Streaming);
    ASSERT_EQ(connectionInfo.getConnectionType(), "TCP/IP");
    ASSERT_EQ(connectionInfo.getAddresses()[0], "127.0.0.1");
    ASSERT_EQ(connectionInfo.getPort(), 7420);
    ASSERT_EQ(connectionInfo.getPrefix(), "daq.ns");
    ASSERT_EQ(connectionInfo.getConnectionString(), "daq.ns://127.0.0.1/");
}

TEST_F(NativeStreamingModulesTest, GetConfigurationConnectionInfoIPv6)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = Instance();
    client.addDevice("daq.ns://[::1]", nullptr);

    auto devices = client.getDevices();
    ASSERT_EQ(devices.getCount(), 1u);

    auto connectionInfo = devices[0].getInfo().getConfigurationConnectionInfo();
    ASSERT_EQ(connectionInfo.getProtocolId(), "OpenDAQNativeStreaming");
    ASSERT_EQ(connectionInfo.getProtocolName(), "OpenDAQNativeStreaming");
    ASSERT_EQ(connectionInfo.getProtocolType(), ProtocolType::Streaming);
    ASSERT_EQ(connectionInfo.getConnectionType(), "TCP/IP");
    ASSERT_EQ(connectionInfo.getAddresses()[0], "[::1]");
    ASSERT_EQ(connectionInfo.getPort(), 7420);
    ASSERT_EQ(connectionInfo.getPrefix(), "daq.ns");
    ASSERT_EQ(connectionInfo.getConnectionString(), "daq.ns://[::1]");
}

TEST_F(NativeStreamingModulesTest, ProtectedSignals)
{
    auto users = List<IUser>();
    users.pushBack(User("admin", "admin", {"admin"}));
    users.pushBack(User("opendaq", "opendaq"));
    auto authenticationProvider = StaticAuthenticationProvider(false, users);

    auto permissions = PermissionsBuilder().inherit(false).allow("admin", PermissionMaskBuilder().read().write().execute()).build();

    auto server = CreateServerInstance(authenticationProvider);
    auto serverSignals = server.getSignalsRecursive(search::Any());
    ASSERT_EQ(serverSignals.getCount(), 5u);

    serverSignals[0].getPermissionManager().setPermissions(permissions);
    serverSignals[1].getPermissionManager().setPermissions(permissions);

    {
        auto client = CreateClientInstance("admin", "admin");
        auto clientSignals = client.getSignalsRecursive(search::Any());
        ASSERT_EQ(clientSignals.getCount(), 5u);
    }

    {
        auto client = CreateClientInstance("opendaq", "opendaq");
        auto clientSignals = client.getSignalsRecursive(search::Any());

#ifdef OPENDAQ_ENABLE_ACCESS_CONTROL
        ASSERT_EQ(clientSignals.getCount(), 3u);
        ASSERT_EQ(clientSignals[0].getName(), "AI1");
        ASSERT_EQ(clientSignals[1].getName(), "AI1Time");
#else
        ASSERT_EQ(clientSignals.getCount(), 5u);
#endif
    }
}

TEST_F(NativeStreamingModulesTest, ProtectedSignalSubscribeDenied)
{
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    auto signal = client.getSignalsRecursive()[0].template asPtr<IMirroredSignalConfig>();

    // deny everyone on signal
    auto permissionsDenyAll = PermissionsBuilder().inherit(false).build();
    auto serverSignal = server.getSignalsRecursive()[0];
    serverSignal.getPermissionManager().setPermissions(permissionsDenyAll);

    // try to subscribe
    std::promise<StringPtr> signalSubscribePromise;
    std::future<StringPtr> signalSubscribeFuture;
    test_helpers::setupSubscribeAckHandler(signalSubscribePromise, signalSubscribeFuture, signal);

    auto reader = daq::StreamReader<double, uint64_t>(signal, ReadTimeoutType::Any);

#ifdef OPENDAQ_ENABLE_ACCESS_CONTROL
    ASSERT_FALSE(test_helpers::waitForAcknowledgement(signalSubscribeFuture, std::chrono::seconds(1)));
#else
    ASSERT_TRUE(test_helpers::waitForAcknowledgement(signalSubscribeFuture, std::chrono::seconds(1)));
#endif
}


TEST_F(NativeStreamingModulesTest, ProtectedSignalSubscribeAllowed)
{
    auto users = List<IUser>();
    users.pushBack(User("admin", "admin", {"admin"}));
    auto authenticationProvider = StaticAuthenticationProvider(false, users);

    auto server = CreateServerInstance(authenticationProvider);
    auto client = CreateClientInstance("admin", "admin");

    auto signal = client.getSignalsRecursive()[0].template asPtr<IMirroredSignalConfig>();

    // allow admin on signal
    auto permissionsAllowAdmin =
        PermissionsBuilder().inherit(false).assign("admin", PermissionMaskBuilder().read().write().execute()).build();
    auto serverSignal = server.getSignalsRecursive()[0];
    serverSignal.getPermissionManager().setPermissions(permissionsAllowAdmin);

    // subscribe
    std::promise<StringPtr> signalSubscribePromise;
    std::future<StringPtr> signalSubscribeFuture;
    test_helpers::setupSubscribeAckHandler(signalSubscribePromise, signalSubscribeFuture, signal);

    auto reader = daq::StreamReader<double, uint64_t>(signal, ReadTimeoutType::Any);
    ASSERT_TRUE(test_helpers::waitForAcknowledgement(signalSubscribeFuture));

    // unsibscribe
    std::promise<StringPtr> signalUnsubscribePromise;
    std::future<StringPtr> signalUnsubscribeFuture;
    test_helpers::setupUnsubscribeAckHandler(signalUnsubscribePromise, signalUnsubscribeFuture, signal);

    reader.release();
    ASSERT_TRUE(test_helpers::waitForAcknowledgement(signalUnsubscribeFuture));
}

TEST_F(NativeStreamingModulesTest, ProtectedSignalUnsubscribeDenied)
{
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    auto signal = client.getSignalsRecursive()[0].template asPtr<IMirroredSignalConfig>();

    // subscribe
    std::promise<StringPtr> signalSubscribePromise;
    std::future<StringPtr> signalSubscribeFuture;
    test_helpers::setupSubscribeAckHandler(signalSubscribePromise, signalSubscribeFuture, signal);

    auto reader = daq::StreamReader<double, uint64_t>(signal, ReadTimeoutType::Any);
    ASSERT_TRUE(test_helpers::waitForAcknowledgement(signalSubscribeFuture));

    // deny everyone on signal
    auto permissionsDenyAll = PermissionsBuilder().inherit(false).build();
    auto serverSignal = server.getSignalsRecursive()[0];
    serverSignal.getPermissionManager().setPermissions(permissionsDenyAll);

    // try to unsubscribe
    std::promise<StringPtr> signalUnsubscribePromise;
    std::future<StringPtr> signalUnsubscribeFuture;
    test_helpers::setupUnsubscribeAckHandler(signalUnsubscribePromise, signalUnsubscribeFuture, signal);

    reader.release();

#ifdef OPENDAQ_ENABLE_ACCESS_CONTROL
    ASSERT_FALSE(test_helpers::waitForAcknowledgement(signalUnsubscribeFuture, std::chrono::seconds(1)));
#else
    ASSERT_TRUE(test_helpers::waitForAcknowledgement(signalUnsubscribeFuture, std::chrono::seconds(1)));
#endif
}

TEST_F(NativeStreamingModulesTest, StreamDataLowMaxPacketReadCount)
{
    auto server = Instance();
    auto moduleManager = server.getModuleManager();
    const ModulePtr deviceModule(MockDeviceModule_Create(server.getContext()));
    moduleManager.addModule(deviceModule);

    const auto serverDevice = server.addDevice("daqmock://phys_device");

    auto config = PropertyObject();
    config.addProperty(IntProperty("MaxPacketReadCount", 1));
    server.addServer("OpenDAQNativeStreaming", config);

    auto client = Instance();
    auto clientDevice = client.addDevice("daq.nd://127.0.0.1");
    
    auto clientSignal = clientDevice.getSignals(search::Recursive(search::LocalId("ByteStep")))[0];
    auto serverSignal = serverDevice.getSignals(search::Recursive(search::LocalId("ByteStep")))[0];

    auto mirroredSignalPtr = clientSignal.asPtr<IMirroredSignalConfig>();
    std::promise<StringPtr> subscribeCompletePromise;
    std::future<StringPtr> subscribeCompleteFuture;
    test_helpers::setupSubscribeAckHandler(subscribeCompletePromise, subscribeCompleteFuture, mirroredSignalPtr);

    auto serverReader = PacketReader(serverSignal);
    auto clientReader = PacketReader(clientSignal);

    ASSERT_TRUE(test_helpers::waitForAcknowledgement(subscribeCompleteFuture));

    const size_t packetsToGenerate = 50;
    const size_t packetsToRead = packetsToGenerate + 1;

    serverDevice.setPropertyValue("GeneratePackets", packetsToGenerate); 

    auto serverReceivedPackets = test_helpers::tryReadPackets(serverReader, packetsToRead);
    auto clientReceivedPackets = test_helpers::tryReadPackets(clientReader, packetsToRead);

    EXPECT_EQ(serverReceivedPackets.getCount(), packetsToRead);
    EXPECT_EQ(clientReceivedPackets.getCount(), packetsToRead);
    EXPECT_TRUE(test_helpers::packetsEqual(serverReceivedPackets, clientReceivedPackets));
}
