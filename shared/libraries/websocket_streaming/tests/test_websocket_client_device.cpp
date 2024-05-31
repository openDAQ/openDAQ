#include <opendaq/instance_factory.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/unit_factory.h>
#include <gtest/gtest.h>
#include <websocket_streaming/websocket_client_device_factory.h>
#include <websocket_streaming/websocket_streaming_server.h>
#include <opendaq/search_filter_factory.h>
#include "streaming_test_helpers.h"
#include <opendaq/mirrored_signal_config_ptr.h>

using namespace daq;
using namespace std::chrono_literals;
using namespace daq::websocket_streaming;

class WebsocketClientDeviceTest : public testing::Test
{
public:
    const uint16_t STREAMING_PORT = daq::streaming_protocol::WEBSOCKET_LISTENING_PORT;
    const uint16_t CONTROL_PORT = daq::streaming_protocol::HTTP_CONTROL_PORT;
    const std::string HOST = "127.0.0.1";
    ContextPtr context;

    void SetUp() override
    {
        context = NullContext();
    }

    void TearDown() override
    {
    }
};

TEST_F(WebsocketClientDeviceTest, CreateWithInvalidParameters)
{
    ASSERT_THROW(WebsocketClientDevice(context, nullptr, "device", nullptr), ArgumentNullException);
}

TEST_F(WebsocketClientDeviceTest, CreateSuccess)
{
    // Create server side device
    auto serverInstance = streaming_test_helpers::createServerInstance();

    // Setup and start streaming server
    auto server = WebsocketStreamingServer(serverInstance);
    server.setStreamingPort(STREAMING_PORT);
    server.setControlPort(CONTROL_PORT);
    server.start();

    DevicePtr clientDevice;
    ASSERT_NO_THROW(clientDevice = WebsocketClientDevice(NullContext(), nullptr, "device", HOST));
}

TEST_F(WebsocketClientDeviceTest, DeviceInfo)
{
    // Create server side device
    auto serverInstance = streaming_test_helpers::createServerInstance();

    // Setup and start streaming server
    auto server = WebsocketStreamingServer(serverInstance);
    server.setStreamingPort(STREAMING_PORT);
    server.setControlPort(CONTROL_PORT);
    server.start();

    // Create the client device
    auto clientDevice = WebsocketClientDevice(NullContext(), nullptr, "device", HOST);

    // get DeviceInfo and check fields
    DeviceInfoPtr clientDeviceInfo;
    ASSERT_NO_THROW(clientDeviceInfo = clientDevice.getInfo());
    ASSERT_EQ(clientDeviceInfo.getName(), "WebsocketClientPseudoDevice");
    ASSERT_EQ(clientDeviceInfo.getConnectionString(), HOST);
}

TEST_F(WebsocketClientDeviceTest, SignalWithDomain)
{
    // Create server signals
    auto testDomainSignal = streaming_test_helpers::createLinearTimeSignal(context);
    auto testValueSignal = streaming_test_helpers::createExplicitValueSignal(context, "TestName", testDomainSignal);

    // Setup and start server which will publish created signal
    auto server = std::make_shared<StreamingServer>(context);
    server->onAccept([&](const daq::streaming_protocol::StreamWriterPtr& writer) {
        auto signals = List<ISignal>();
        signals.pushBack(testValueSignal);
        signals.pushBack(testDomainSignal);
        return signals;
    });
    server->start(STREAMING_PORT, CONTROL_PORT);

    // Create the client device
    auto clientDevice = WebsocketClientDevice(NullContext(), nullptr, "device", HOST);

    // Check the mirrored signal
    ASSERT_EQ(clientDevice.getSignals().getCount(), 2u);
    ASSERT_TRUE(clientDevice.getSignals()[0].getDescriptor().assigned());
    ASSERT_TRUE(clientDevice.getSignals()[0].getDomainSignal().assigned());

    ASSERT_FALSE(clientDevice.getSignals()[1].getDomainSignal().assigned());
    ASSERT_EQ(clientDevice.getSignals()[0].getDomainSignal(), clientDevice.getSignals()[1]);

    ASSERT_TRUE(BaseObjectPtr::Equals(clientDevice.getSignals()[0].getDescriptor(),
                                      testValueSignal.getDescriptor()));
    ASSERT_TRUE(BaseObjectPtr::Equals(clientDevice.getSignals()[0].getDomainSignal().getDescriptor(),
                                      testValueSignal.getDomainSignal().getDescriptor()));

    ASSERT_EQ(clientDevice.getSignals()[0].getName(), "TestName");
    ASSERT_EQ(clientDevice.getSignals()[0].getDescription(), "TestDescription");

    std::promise<StringPtr> acknowledgementPromise;
    std::future<StringPtr> acknowledgementFuture = acknowledgementPromise.get_future();
    auto signal = clientDevice.getSignals()[0].asPtr<IMirroredSignalConfig>();
    signal.getOnSubscribeComplete() +=
        [&acknowledgementPromise](MirroredSignalConfigPtr&, SubscriptionEventArgsPtr& args)
    {
        acknowledgementPromise.set_value(args.getStreamingConnectionString());
    };

    auto reader = PacketReader(clientDevice.getSignals()[0]);
    ASSERT_EQ(acknowledgementFuture.wait_for(std::chrono::milliseconds(500)), std::future_status::ready);

    // Publish signal changes
    auto descriptor = DataDescriptorBuilderCopy(testValueSignal.getDescriptor()).build();
    std::string signalId = testValueSignal.getGlobalId();
    server->sendPacketToSubscribers(signalId, DataDescriptorChangedEventPacket(descriptor, testValueSignal.getDomainSignal().getDescriptor()));

    std::this_thread::sleep_for(std::chrono::milliseconds(250));

    testValueSignal.asPtr<IComponentPrivate>().unlockAllAttributes();
    testValueSignal.setName("NewName");
    testValueSignal.setDescription("NewDescription");
    std::this_thread::sleep_for(std::chrono::milliseconds(250));

    ASSERT_EQ(clientDevice.getSignals()[0].getName(), "NewName");
    ASSERT_EQ(clientDevice.getSignals()[0].getDescription(), "NewDescription");

    ASSERT_NO_THROW(clientDevice.getSignals()[0].setName("ClientName"));

    // Check if the mirrored signal changed
    ASSERT_TRUE(BaseObjectPtr::Equals(clientDevice.getSignals()[0].getDescriptor(),
                                      descriptor));
    ASSERT_TRUE(BaseObjectPtr::Equals(clientDevice.getSignals()[0].getDomainSignal().getDescriptor(),
                                      testValueSignal.getDomainSignal().getDescriptor()));
}

TEST_F(WebsocketClientDeviceTest, SingleDomainSignal)
{
    // Create server signal
    auto testSignal = streaming_test_helpers::createLinearTimeSignal(context);

    // Setup and start server which will publish created signal
    auto server = std::make_shared<StreamingServer>(context);
    server->onAccept([&testSignal](const daq::streaming_protocol::StreamWriterPtr& writer) {
        auto signals = List<ISignal>();
        signals.pushBack(testSignal);
        return signals;
    });
    server->start(STREAMING_PORT, CONTROL_PORT);

    // Create the client device
    auto clientDevice = WebsocketClientDevice(NullContext(), nullptr, "device", HOST);

    // The mirrored signal exists and has descriptor
    ASSERT_EQ(clientDevice.getSignals().getCount(), 1u);
    ASSERT_TRUE(clientDevice.getSignals()[0].getDescriptor().assigned());
    ASSERT_FALSE(clientDevice.getSignals()[0].getDomainSignal().assigned());
}

TEST_F(WebsocketClientDeviceTest, SingleUnsupportedSignal)
{
    // Create server signal
    auto testSignal = streaming_test_helpers::createExplicitValueSignal(context, "TestSignal", nullptr);

    // Setup and start server which will publish created signal
    auto server = std::make_shared<StreamingServer>(context);
    server->onAccept([&testSignal](const daq::streaming_protocol::StreamWriterPtr& writer) {
        auto signals = List<ISignal>();
        signals.pushBack(testSignal);
        return signals;
    });
    server->start(STREAMING_PORT, CONTROL_PORT);

    // Create the client device
    auto clientDevice = WebsocketClientDevice(NullContext(), nullptr, "device", HOST);

    // The mirrored signal exists but does not have descriptor
    ASSERT_EQ(clientDevice.getSignals().getCount(), 1u);
    ASSERT_FALSE(clientDevice.getSignals()[0].getDescriptor().assigned());
    ASSERT_FALSE(clientDevice.getSignals()[0].getDomainSignal().assigned());
}

TEST_F(WebsocketClientDeviceTest, DeviceWithMultipleSignals)
{
    // Create server side device
    auto serverInstance = streaming_test_helpers::createServerInstance();

    // Setup and start streaming server
    auto server = WebsocketStreamingServer(serverInstance);
    server.setStreamingPort(STREAMING_PORT);
    server.setControlPort(CONTROL_PORT);
    server.start();

    // Create the client device
    auto clientDevice = WebsocketClientDevice(NullContext(), nullptr, "device", HOST);

    // There should not be any difference if we get signals recursively or not,
    // since client device doesn't know anything about hierarchy
    size_t expectedSignalCount = 0;
    for (const auto& signal : serverInstance.getSignals(search::Recursive(search::Visible())))
        expectedSignalCount += signal.getPublic();

    ListPtr<ISignal> signals;
    ASSERT_NO_THROW(signals = clientDevice.getSignals());
    ASSERT_EQ(signals.getCount(), expectedSignalCount);
    ASSERT_NO_THROW(signals = clientDevice.getSignals(search::Recursive(search::Visible())));
    ASSERT_EQ(signals.getCount(), expectedSignalCount);
}

TEST_F(WebsocketClientDeviceTest, SignalsWithSharedDomain)
{
    // Create server signals
    auto timeSignal = streaming_test_helpers::createLinearTimeSignal(context);
    auto dataSignal1 = streaming_test_helpers::createExplicitValueSignal(context, "Data1", timeSignal);
    auto dataSignal2 = streaming_test_helpers::createExplicitValueSignal(context, "Data2", timeSignal);

    auto server = std::make_shared<StreamingServer>(context);
    server->onAccept([&](const daq::streaming_protocol::StreamWriterPtr& writer) {
        auto signals = List<ISignal>();
        signals.pushBack(dataSignal1);
        signals.pushBack(timeSignal);
        signals.pushBack(dataSignal2);
        return signals;
    });
    server->start(STREAMING_PORT, CONTROL_PORT);

    // Create the client device
    auto clientDevice = WebsocketClientDevice(NullContext(), nullptr, "device", HOST);

    ASSERT_EQ(clientDevice.getSignals().getCount(), 3u);

    // Check the mirrored signals
    ASSERT_TRUE(clientDevice.getSignals()[0].getDescriptor().assigned());
    ASSERT_TRUE(clientDevice.getSignals()[0].getDomainSignal().assigned());
    ASSERT_TRUE(BaseObjectPtr::Equals(clientDevice.getSignals()[0].getDescriptor(),
                                      dataSignal1.getDescriptor()));
    ASSERT_TRUE(BaseObjectPtr::Equals(clientDevice.getSignals()[0].getDomainSignal().getDescriptor(),
                                      dataSignal1.getDomainSignal().getDescriptor()));
    ASSERT_EQ(clientDevice.getSignals()[0].getName(), "Data1");

    ASSERT_TRUE(clientDevice.getSignals()[1].getDescriptor().assigned());
    ASSERT_TRUE(!clientDevice.getSignals()[1].getDomainSignal().assigned());
    ASSERT_TRUE(BaseObjectPtr::Equals(clientDevice.getSignals()[1].getDescriptor(),
                                      timeSignal.getDescriptor()));
    ASSERT_EQ(clientDevice.getSignals()[1].getName(), "Time");

    ASSERT_TRUE(clientDevice.getSignals()[2].getDescriptor().assigned());
    ASSERT_TRUE(clientDevice.getSignals()[2].getDomainSignal().assigned());
    ASSERT_TRUE(BaseObjectPtr::Equals(clientDevice.getSignals()[2].getDescriptor(),
                                      dataSignal2.getDescriptor()));
    ASSERT_TRUE(BaseObjectPtr::Equals(clientDevice.getSignals()[2].getDomainSignal().getDescriptor(),
                                      dataSignal2.getDomainSignal().getDescriptor()));
    ASSERT_EQ(clientDevice.getSignals()[2].getName(), "Data2");

    ASSERT_EQ(clientDevice.getSignals()[2].getDomainSignal(), clientDevice.getSignals()[1]);
    ASSERT_EQ(clientDevice.getSignals()[0].getDomainSignal(), clientDevice.getSignals()[1]);
}
