#include "test_helpers.h"

using NativeStreamingModulesTest = testing::Test;

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
    auto refDevice = instance.addDevice("daq.nsd://127.0.0.1/");
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

TEST_F(NativeStreamingModulesTest, GetRemoteDeviceObjects)
{
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    ASSERT_EQ(client.getDevices().getCount(), 1u);
    auto clientSignals = client.getSignals(search::Recursive(search::Any()));
    ASSERT_EQ(clientSignals.getCount(), 4u);

    ASSERT_EQ(clientSignals[0].getDomainSignal(), clientSignals[1]);
    ASSERT_TRUE(!clientSignals[1].getDomainSignal().assigned());
    ASSERT_EQ(clientSignals[2].getDomainSignal(), clientSignals[3]);
    ASSERT_TRUE(!clientSignals[3].getDomainSignal().assigned());

    auto serverSignals = server.getSignals(search::Recursive(search::Any()));

    for (size_t i = 0; i < serverSignals.getCount(); ++i)
    {
        auto serverDataDescriptor = serverSignals[i].getDescriptor();
        auto clientDataDescriptor = clientSignals[i].getDescriptor();

        ASSERT_EQ(clientDataDescriptor, serverDataDescriptor);

        ASSERT_EQ(serverSignals[i].getName(), clientSignals[i].getName());
        ASSERT_EQ(serverSignals[i].getDescription(), clientSignals[i].getDescription());
    }

    DeviceInfoPtr info;
    ASSERT_NO_THROW(info = client.getDevices()[0].getInfo());
    ASSERT_TRUE(info.assigned());
    ASSERT_EQ(info.getConnectionString(), "daq.nsd://127.0.0.1/");
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
        EXPECT_GT(count, 0) << "iteration " << i;
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

    auto clientSignalsBeforeDisconnection = client.getSignals(search::Recursive(search::Any()));

    // destroy server to emulate disconnection
    server.release();
    // TODO check for disconnected status

    // re-create server to enable reconnection
    server = CreateServerInstance();
    auto serverSignals = server.getSignals(search::Recursive(search::Any()));

    // TODO check for reconnected status

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

        ASSERT_EQ(serverSignals[i].getName(), clientSignalsAfterReconnection[i].getName());
        ASSERT_EQ(serverSignals[i].getDescription(), clientSignalsAfterReconnection[i].getDescription());

    }
}

TEST_F(NativeStreamingModulesTest, ReconnectWhileRead)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    auto signal = client.getSignals(search::Recursive(search::Any()))[0].template asPtr<IMirroredSignalConfig>();
    auto domainSignal = signal.getDomainSignal().template asPtr<IMirroredSignalConfig>();

    std::promise<StringPtr> signalSubscribePromise[2];
    std::future<StringPtr> signalSubscribeFuture[2];
    std::promise<StringPtr> domainSubscribePromise[2];
    std::future<StringPtr> domainSubscribeFuture[2];

    test_helpers::setupSubscribeAckHandler(signalSubscribePromise[0], signalSubscribeFuture[0], signal);
    test_helpers::setupSubscribeAckHandler(domainSubscribePromise[0], domainSubscribeFuture[0], domainSignal);

    using namespace std::chrono_literals;
    StreamReaderPtr reader = daq::StreamReader<double, uint64_t>(signal);

    // wait for subscribe ack before read
    ASSERT_TRUE(test_helpers::waitForAcknowledgement(signalSubscribeFuture[0]));
    ASSERT_TRUE(test_helpers::waitForAcknowledgement(domainSubscribeFuture[0]));

    // destroy server to emulate disconnection
    server.release();
    // TODO check for disconnected status

    {
        double samples[1000];

        // read all data received from server before disconnection
        daq::SizeT count = 1000;
        reader.read(samples, &count);

        count = 1000;
        // and test there is no more data to read
        std::this_thread::sleep_for(100ms);
        reader.read(samples, &count);
        EXPECT_EQ(count, 0);
    }

    test_helpers::setupSubscribeAckHandler(signalSubscribePromise[1], signalSubscribeFuture[1], signal);
    test_helpers::setupSubscribeAckHandler(domainSubscribePromise[1], domainSubscribeFuture[1], domainSignal);

    // re-create server to enable reconnection
    server = CreateServerInstance();

    // TODO check for reconnected status

    // wait for new subscribe ack before further reading
    ASSERT_TRUE(test_helpers::waitForAcknowledgement(signalSubscribeFuture[1], 5s));
    ASSERT_TRUE(test_helpers::waitForAcknowledgement(domainSubscribeFuture[1], 5s));

    // read data received from server after reconnection
    for (int i = 0; i < 10; ++i)
    {
        std::this_thread::sleep_for(100ms);
        daq::SizeT count = 100;
        double samples[100];
        reader.read(samples, &count);
        EXPECT_GT(count, 0) << "iteration " << i;
    }
}
