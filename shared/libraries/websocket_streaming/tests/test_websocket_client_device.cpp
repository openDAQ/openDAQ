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

class WebsocketClientDeviceTestP : public WebsocketClientDeviceTest, public testing::WithParamInterface<bool>
{
public:
    bool addSignals(const ListPtr<ISignal>& signals,
                    const StreamingServerPtr& server,
                    const ContextPtr& context)
    {
        SizeT addedSigCount = 0;
        std::promise<void> addSigPromise;
        std::future<void> addSigFuture = addSigPromise.get_future();

        auto eventHandler = [&](const ComponentPtr& comp, const CoreEventArgsPtr& args)
        {
            auto params = args.getParameters();
            if (static_cast<CoreEventId>(args.getEventId()) == CoreEventId::ComponentAdded)
            {
                ComponentPtr component = params.get("Component");
                if (component.asPtrOrNull<ISignal>().assigned())
                {
                    addedSigCount++;
                    if (addedSigCount == signals.getCount())
                        addSigPromise.set_value();
                }
            }
        };

        context.getOnCoreEvent() += eventHandler;

        server->addSignals(signals);

        bool result = (addSigFuture.wait_for(std::chrono::seconds(5)) == std::future_status::ready);

        context.getOnCoreEvent() -= eventHandler;
        return result;
    }

    bool removeSignals(const ListPtr<ISignal>& signals,
                       const StreamingServerPtr& server,
                       const ContextPtr& context)
    {
        SizeT removedSigCount = 0;
        std::promise<void> rmSigPromise;
        std::future<void> rmSigFuture = rmSigPromise.get_future();

        auto eventHandler = [&](const ComponentPtr& comp, const CoreEventArgsPtr& args)
        {
            if (static_cast<CoreEventId>(args.getEventId()) == CoreEventId::ComponentRemoved)
            {
                removedSigCount++;
                if (removedSigCount == signals.getCount())
                    rmSigPromise.set_value();
            }
        };

        context.getOnCoreEvent() += eventHandler;

        for (const auto& signal : signals)
            server->removeComponentSignals(signal.getGlobalId());

        bool result = (rmSigFuture.wait_for(std::chrono::seconds(5)) == std::future_status::ready);

        context.getOnCoreEvent() -= eventHandler;
        return result;
    }
};

TEST_P(WebsocketClientDeviceTestP, SignalWithDomain)
{
    const bool signalsAddedAfterConnect = GetParam();

    // Create server signals
    auto testDomainSignal = streaming_test_helpers::createLinearTimeSignal(context);
    auto testValueSignal = streaming_test_helpers::createExplicitValueSignal(context, "TestName", testDomainSignal);
    auto signals = List<ISignal>(testValueSignal, testDomainSignal);

    // Setup and start server which will publish created signal
    auto server = std::make_shared<StreamingServer>(context);
    server->onAccept([&](const daq::streaming_protocol::StreamWriterPtr& writer) {
        if (signalsAddedAfterConnect)
            return List<ISignal>();
        else
            return signals;
    });
    server->start(STREAMING_PORT, CONTROL_PORT);

    // Create the client device
    auto clientDevice = WebsocketClientDevice(NullContext(), nullptr, "device", HOST);
    clientDevice.asPtr<daq::IPropertyObjectInternal>().enableCoreEventTrigger();

    if (signalsAddedAfterConnect)
    {
        ASSERT_TRUE(addSignals(signals, server, clientDevice.getContext()));
    }

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
    server->broadcastPacket(signalId, DataDescriptorChangedEventPacket(descriptor, testValueSignal.getDomainSignal().getDescriptor()));

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

    ASSERT_TRUE(removeSignals({testDomainSignal}, server, clientDevice.getContext()));
    ASSERT_EQ(clientDevice.getSignals().getCount(), 1u);
    ASSERT_FALSE(clientDevice.getSignals()[0].getDomainSignal().assigned());

    ASSERT_TRUE(removeSignals({testValueSignal}, server, clientDevice.getContext()));
    ASSERT_EQ(clientDevice.getSignals().getCount(), 0u);
}

TEST_P(WebsocketClientDeviceTestP, SingleDomainSignal)
{
    const bool signalsAddedAfterConnect = GetParam();

    // Create server signal
    auto testSignal = streaming_test_helpers::createLinearTimeSignal(context);
    auto signals = List<ISignal>(testSignal);

    // Setup and start server which will publish created signal
    auto server = std::make_shared<StreamingServer>(context);
    server->onAccept([&](const daq::streaming_protocol::StreamWriterPtr& writer) {
        if (signalsAddedAfterConnect)
            return List<ISignal>();
        else
            return signals;
    });
    server->start(STREAMING_PORT, CONTROL_PORT);

    // Create the client device
    auto clientDevice = WebsocketClientDevice(NullContext(), nullptr, "device", HOST);
    clientDevice.asPtr<daq::IPropertyObjectInternal>().enableCoreEventTrigger();

    if (signalsAddedAfterConnect)
    {
        ASSERT_TRUE(addSignals(signals, server, clientDevice.getContext()));
    }

    // The mirrored signal exists and has descriptor
    ASSERT_EQ(clientDevice.getSignals().getCount(), 1u);
    ASSERT_TRUE(clientDevice.getSignals()[0].getDescriptor().assigned());
    ASSERT_FALSE(clientDevice.getSignals()[0].getDomainSignal().assigned());

    ASSERT_TRUE(removeSignals(signals, server, clientDevice.getContext()));
    ASSERT_EQ(clientDevice.getSignals().getCount(), 0u);
}

TEST_P(WebsocketClientDeviceTestP, SingleUnsupportedSignal)
{
    const bool signalsAddedAfterConnect = GetParam();

    // Create server signal
    auto testSignal = streaming_test_helpers::createExplicitValueSignal(context, "TestSignal", nullptr);
    auto signals = List<ISignal>(testSignal);

    // Setup and start server which will publish created signal
    auto server = std::make_shared<StreamingServer>(context);
    server->onAccept([&](const daq::streaming_protocol::StreamWriterPtr& writer) {
        if (signalsAddedAfterConnect)
            return List<ISignal>();
        else
            return signals;
    });
    server->start(STREAMING_PORT, CONTROL_PORT);

    // Create the client device
    auto clientDevice = WebsocketClientDevice(NullContext(), nullptr, "device", HOST);
    clientDevice.asPtr<daq::IPropertyObjectInternal>().enableCoreEventTrigger();

    if (signalsAddedAfterConnect)
    {
        ASSERT_TRUE(addSignals(signals, server, clientDevice.getContext()));
    }

    // The mirrored signal exists but does not have descriptor
    ASSERT_EQ(clientDevice.getSignals().getCount(), 1u);
    ASSERT_FALSE(clientDevice.getSignals()[0].getDescriptor().assigned());
    ASSERT_FALSE(clientDevice.getSignals()[0].getDomainSignal().assigned());

    ASSERT_TRUE(removeSignals(signals, server, clientDevice.getContext()));
    ASSERT_EQ(clientDevice.getSignals().getCount(), 0u);
}

TEST_P(WebsocketClientDeviceTestP, SignalsWithSharedDomain)
{
    const bool signalsAddedAfterConnect = GetParam();

    // Create server signals
    auto timeSignal = streaming_test_helpers::createLinearTimeSignal(context);
    auto dataSignal1 = streaming_test_helpers::createExplicitValueSignal(context, "Data1", timeSignal);
    auto dataSignal2 = streaming_test_helpers::createExplicitValueSignal(context, "Data2", timeSignal);
    auto signals = List<ISignal>(dataSignal1, timeSignal, dataSignal2);

    auto server = std::make_shared<StreamingServer>(context);
    server->onAccept([&](const daq::streaming_protocol::StreamWriterPtr& writer) {
        if (signalsAddedAfterConnect)
            return List<ISignal>();
        else
            return signals;
    });
    server->start(STREAMING_PORT, CONTROL_PORT);

    // Create the client device
    auto clientDevice = WebsocketClientDevice(NullContext(), nullptr, "device", HOST);
    clientDevice.asPtr<daq::IPropertyObjectInternal>().enableCoreEventTrigger();

    if (signalsAddedAfterConnect)
    {
        ASSERT_TRUE(addSignals(signals, server, clientDevice.getContext()));
    }

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

    ASSERT_TRUE(removeSignals({timeSignal}, server, clientDevice.getContext()));
    ASSERT_EQ(clientDevice.getSignals().getCount(), 2u);
    ASSERT_FALSE(clientDevice.getSignals()[0].getDomainSignal().assigned());
    ASSERT_FALSE(clientDevice.getSignals()[1].getDomainSignal().assigned());

    ASSERT_TRUE(removeSignals({dataSignal1, dataSignal2}, server, clientDevice.getContext()));
    ASSERT_EQ(clientDevice.getSignals().getCount(), 0u);
}

INSTANTIATE_TEST_SUITE_P(SignalsAddedAfterConnect, WebsocketClientDeviceTestP, testing::Values(true, false));

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
