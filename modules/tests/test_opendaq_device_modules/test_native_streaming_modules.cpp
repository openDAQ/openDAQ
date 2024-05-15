#include "test_helpers/test_helpers.h"
#include "../../../core/opendaq/opendaq/tests/test_config_provider.h"

using NativeStreamingModulesTest = testing::Test;
using namespace test_config_provider_helpers;
using NativeStreamingModuleTestConfig = ConfigProviderTest;

using namespace daq;

static InstancePtr CreateServerInstance()
{
    auto logger = Logger();
    auto scheduler = Scheduler(logger);
    auto moduleManager = ModuleManager("");
    auto context = Context(scheduler, logger, TypeManager(), moduleManager);

    auto instance = InstanceCustom(context, "local");

    const auto refDevice = instance.addDevice("daqref://device1");

    instance.addServer("openDAQ Native Streaming", nullptr);

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

TEST_F(NativeStreamingModuleTestConfig, PopulateDefaultConfigFromProvider)
{
    std::string filename = "populateDefaultConfig.json";
    std::string json = R"(
        {
            "Modules":
            {
                "NativeStreamingServer":
                {
                    "ServiceDiscoverable": true,
                    "Port": 1234,
                    "Name": "native streaming server",
                    "Manufacturer": "test",
                    "Model": "test device",
                    "SerialNumber": "test_serial_number",
                    "ServicePath": "/some/path"
                }
            }
        }
    )";
    createConfigFile(filename, json);

    auto provider = JsonConfigProvider(filename);
    auto instance = InstanceBuilder().addConfigProvider(provider).build();
    auto serverConfig = instance.getAvailableServerTypes().get("openDAQ Native Streaming").createDefaultConfig();

    ASSERT_TRUE(serverConfig.getPropertyValue("ServiceDiscoverable").asPtr<IBoolean>());
    ASSERT_EQ(serverConfig.getPropertyValue("Port").asPtr<IInteger>(), 1234);
    ASSERT_EQ(serverConfig.getPropertyValue("Name").asPtr<IString>(), "native streaming server");
    ASSERT_EQ(serverConfig.getPropertyValue("Manufacturer").asPtr<IString>(), "test");
    ASSERT_EQ(serverConfig.getPropertyValue("Model").asPtr<IString>(), "test device");
    ASSERT_EQ(serverConfig.getPropertyValue("SerialNumber").asPtr<IString>(), "test_serial_number");
    ASSERT_EQ(serverConfig.getPropertyValue("ServicePath").asPtr<IString>(), "/some/path");
}

TEST_F(NativeStreamingModulesTest, DiscoveringServer)
{
    auto server = InstanceBuilder().setDefaultRootDeviceLocalId("local").build();
    const auto refDevice = server.addDevice("daqref://device1");

    auto serverConfig = server.getAvailableServerTypes().get("openDAQ Native Streaming").createDefaultConfig();
    auto serialNumber = "NativeStreamingModulesTest_DiscoveringServer_" + test_helpers::getHostname() + "_" + serverConfig.getPropertyValue("Port").toString();
    serverConfig.setPropertyValue("SerialNumber", serialNumber);
    serverConfig.setPropertyValue("ServiceDiscoverable", true);
    server.addServer("openDAQ Native Streaming", serverConfig);

    auto client = Instance();
    DevicePtr device;
    for (const auto & deviceInfo : client.getAvailableDevices())
    {
        if (deviceInfo.getSerialNumber() != serialNumber)
            continue;

        // expected native streaming and native device capabilities
        ASSERT_EQ(deviceInfo.getServerCapabilities().getCount(), 2u);

        for (const auto & capability : deviceInfo.getServerCapabilities())
        {
            if (capability.getProtocolName() == "openDAQ Native Streaming")
            {
                device = client.addDevice(deviceInfo.getConnectionString(), nullptr);
                break;
            }
        }
    }
    ASSERT_TRUE(device.assigned());
}

TEST_F(NativeStreamingModuleTestConfig, checkDeviceInfoPopulatedWithProvider)
{
    std::string filename = "populateDefaultConfig.json";
    std::string json = R"(
        {
            "Modules":
            {
                "NativeStreamingServer":
                {
                    "ServiceDiscoverable": true,
                    "Port": 1234,
                    "Name": "native streaming server",
                    "Manufacturer": "test",
                    "Model": "test device",
                    "SerialNumber": "native_test_serial_number",
                    "ServicePath": "/some/path"
                }
            }
        }
    )";
    createConfigFile(filename, json);

    auto provider = JsonConfigProvider(filename);
    auto instance = InstanceBuilder().addConfigProvider(provider).build();
    auto serverConfig = instance.getAvailableServerTypes().get("openDAQ Native Streaming").createDefaultConfig();
    instance.addServer("openDAQ Native Streaming", serverConfig);

    auto client = Instance();
    DevicePtr device;
    for (const auto & deviceInfo : client.getAvailableDevices())
    {
        if (deviceInfo.getSerialNumber() != "native_test_serial_number")
            continue;

        // expected native streaming and native device capabilities
        ASSERT_EQ(deviceInfo.getServerCapabilities().getCount(), 2u);

        for (const auto & capability : deviceInfo.getServerCapabilities())
        {
            if (device == nullptr)
                device = client.addDevice(deviceInfo.getConnectionString(), nullptr);

            ASSERT_EQ(deviceInfo.getName(), "native streaming server");
            ASSERT_EQ(deviceInfo.getManufacturer(), "test");
            ASSERT_EQ(deviceInfo.getModel(), "test device");
            ASSERT_EQ(deviceInfo.getSerialNumber(), "native_test_serial_number");

            std::string connectionString = capability.getConnectionString();    
            std::string servicePath = "/some/path";
            std::string connectionPath = connectionString.substr(connectionString.size() - servicePath.size());
            ASSERT_EQ(connectionPath, servicePath);

        }      
    }

    ASSERT_TRUE(device.assigned());
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
    ASSERT_EQ(clientSignals.getCount(), 4u);

    ASSERT_EQ(clientSignals[0].getDomainSignal(), clientSignals[1]);
    ASSERT_TRUE(!clientSignals[1].getDomainSignal().assigned());
    ASSERT_EQ(clientSignals[2].getDomainSignal(), clientSignals[3]);
    ASSERT_TRUE(!clientSignals[3].getDomainSignal().assigned());

    for (size_t i = 0; i < clientSignals.getCount(); ++i)
    {
        auto serverDataDescriptor = serverSignals[i].getDescriptor();
        auto clientDataDescriptor = clientSignals[i].getDescriptor();

        ASSERT_EQ(clientDataDescriptor, serverDataDescriptor);

        //ASSERT_EQ(serverSignals[i].getName(), clientSignals[i].getName());
        ASSERT_EQ(serverSignals[i].getDescription(), clientSignals[i].getDescription());

        auto mirroredSignalPtr = clientSignals[i].asPtr<IMirroredSignalConfig>();
        ASSERT_GT(mirroredSignalPtr.getStreamingSources().getCount(), 0u) << clientSignals[i].getGlobalId();
        ASSERT_TRUE(mirroredSignalPtr.getActiveStreamingSource().assigned()) << clientSignals[i].getGlobalId();
    }

    ASSERT_EQ(serverSignals[0].getName(), clientSignals[0].getName());

    DeviceInfoPtr info;
    ASSERT_NO_THROW(info = client.getDevices()[0].getInfo());
    ASSERT_TRUE(info.assigned());
    ASSERT_EQ(info.getConnectionString(), "daq.ns://127.0.0.1/");
    ASSERT_EQ(info.getName(), "NativeStreamingClientPseudoDevice");
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
    StreamReaderPtr reader = daq::StreamReader<double, uint64_t>(signal);

    ASSERT_TRUE(test_helpers::waitForAcknowledgement(signalSubscribeFuture));
    ASSERT_EQ(signalSubscribeFuture.get(), streamingSource);

    ASSERT_TRUE(test_helpers::waitForAcknowledgement(domainSubscribeFuture));
    ASSERT_EQ(domainSubscribeFuture.get(), streamingSource);

    double samples[100];
    for (int i = 0; i < 10; ++i)
    {
        std::this_thread::sleep_for(100ms);
        daq::SizeT count = 100;
        reader.read(samples, &count);
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
    const auto renderer = client.addFunctionBlock("ref_fb_module_renderer");
    renderer.getInputPorts()[0].connect(signals[0]);

    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
}

TEST_F(NativeStreamingModulesTest, GetRemoteDeviceObjectsAfterReconnect)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    ASSERT_EQ(client.getDevices()[0].getStatusContainer().getStatus("ConnectionStatus"), "Connected");

    std::promise<StringPtr> connectionStatusPromise;
    std::future<StringPtr> connectionStatusFuture = connectionStatusPromise.get_future();
    client.getDevices()[0].getOnComponentCoreEvent() +=
        [&](const ComponentPtr& /*comp*/, const CoreEventArgsPtr& args)
    {
        auto params = args.getParameters();
        if (static_cast<CoreEventId>(args.getEventId()) == CoreEventId::StatusChanged)
        {
            ASSERT_TRUE(args.getParameters().hasKey("ConnectionStatus"));
            connectionStatusPromise.set_value(args.getParameters().get("ConnectionStatus").toString());
        }
    };

    auto clientSignalsBeforeDisconnection = client.getSignals(search::Recursive(search::Any()));

    // destroy server to emulate disconnection
    server.release();
    ASSERT_TRUE(connectionStatusFuture.wait_for(std::chrono::seconds(5)) == std::future_status::ready);
    ASSERT_EQ(connectionStatusFuture.get(), "Reconnecting");
    ASSERT_EQ(client.getDevices()[0].getStatusContainer().getStatus("ConnectionStatus"), "Reconnecting");

    // reset future / promise
    connectionStatusPromise = std::promise<StringPtr>();
    connectionStatusFuture = connectionStatusPromise.get_future();

    // re-create server to enable reconnection
    server = CreateServerInstance();
    auto serverSignals = server.getSignals(search::Recursive(search::Any()));

    ASSERT_TRUE(connectionStatusFuture.wait_for(std::chrono::seconds(5)) == std::future_status::ready);
    ASSERT_EQ(connectionStatusFuture.get(), "Connected");
    ASSERT_EQ(client.getDevices()[0].getStatusContainer().getStatus("ConnectionStatus"), "Connected");

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

TEST_F(NativeStreamingModulesTest, ReconnectWhileRead)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    ASSERT_EQ(client.getDevices()[0].getStatusContainer().getStatus("ConnectionStatus"), "Connected");

    std::promise<StringPtr> connectionStatusPromise;
    std::future<StringPtr> connectionStatusFuture = connectionStatusPromise.get_future();
    client.getDevices()[0].getOnComponentCoreEvent() +=
        [&](const ComponentPtr& /*comp*/, const CoreEventArgsPtr& args)
    {
        auto params = args.getParameters();
        if (static_cast<CoreEventId>(args.getEventId()) == CoreEventId::StatusChanged)
        {
            ASSERT_TRUE(args.getParameters().hasKey("ConnectionStatus"));
            connectionStatusPromise.set_value(args.getParameters().get("ConnectionStatus").toString());
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
    StreamReaderPtr reader = daq::StreamReader<double, uint64_t>(signal);

    // wait for subscribe ack before read
    ASSERT_TRUE(test_helpers::waitForAcknowledgement(signalSubscribeFuture));
    ASSERT_TRUE(test_helpers::waitForAcknowledgement(domainSubscribeFuture));

    // destroy server to emulate disconnection
    server.release();
    ASSERT_TRUE(connectionStatusFuture.wait_for(std::chrono::seconds(5)) == std::future_status::ready);
    ASSERT_EQ(connectionStatusFuture.get(), "Reconnecting");
    ASSERT_EQ(client.getDevices()[0].getStatusContainer().getStatus("ConnectionStatus"), "Reconnecting");

    // reset future / promise
    connectionStatusPromise = std::promise<StringPtr>();
    connectionStatusFuture = connectionStatusPromise.get_future();

    {
        double samples[1000];

        // read all data received from server before disconnection
        daq::SizeT count = 1000;
        reader.read(samples, &count);

        count = 1000;
        // and test there is no more data to read
        std::this_thread::sleep_for(100ms);
        reader.read(samples, &count);
        EXPECT_EQ(count, 0u);
    }

    signalSubscribePromise = std::promise<StringPtr>();
    signalSubscribeFuture = signalSubscribePromise.get_future();
    domainSubscribePromise = std::promise<StringPtr>();
    domainSubscribeFuture = domainSubscribePromise.get_future();

    // re-create server to enable reconnection
    server = CreateServerInstance();

    ASSERT_TRUE(connectionStatusFuture.wait_for(std::chrono::seconds(5)) == std::future_status::ready);
    ASSERT_EQ(connectionStatusFuture.get(), "Connected");
    ASSERT_EQ(client.getDevices()[0].getStatusContainer().getStatus("ConnectionStatus"), "Connected");

    // wait for new subscribe ack before further reading
    ASSERT_TRUE(test_helpers::waitForAcknowledgement(signalSubscribeFuture));
    ASSERT_TRUE(test_helpers::waitForAcknowledgement(domainSubscribeFuture));

    // No descriptor changed packet, as the descriptor did not change
    // read data received from server after reconnection
    for (int i = 0; i < 10; ++i)
    {
        std::this_thread::sleep_for(100ms);
        daq::SizeT count = 100;
        double samples[100];
        reader.read(samples, &count);
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
    ASSERT_EQ(clientSignals.getCount(), 6u);

    ASSERT_EQ(clientSignals[4].getDomainSignal(), clientSignals[5]);
    ASSERT_TRUE(!clientSignals[5].getDomainSignal().assigned());

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
    ASSERT_EQ(clientSignals.getCount(), 2u);
}
