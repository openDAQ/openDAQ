#include <gtest/gtest.h>
#include <testutils/testutils.h>
#include <gmock/gmock.h>
#include <config_protocol/config_protocol_streaming_producer.h>
#include <opendaq/gmock/signal.h>
#include <opendaq/context_factory.h>
#include <opendaq/gmock/component.h>
#include <opendaq/packet_factory.h>
#include <future>
#include <queue>
#include "test_utils.h"

#include <coreobjects/user_factory.h>
#include <opendaq/event_packet_utils.h>

#include <config_protocol/config_protocol_server.h>
#include <config_protocol/config_protocol_client.h>
#include <config_protocol/config_client_device_impl.h>

using namespace daq;
using namespace config_protocol;
using namespace testing;

class StreamingProducerTest : public Test
{
public:
    void SetUp() override
    {
        dummySendDaqPacketCb = [](const PacketPtr&, SignalNumericIdType) {};
        context = NullContext();
        testDevice = test_utils::createTestDevice();
        signals = testDevice.getDevices()[0].getChannels()[0].getSignals();
    }

protected:
    ContextPtr context;
    DevicePtr testDevice;
    ListPtr<ISignal> signals;
    SendDaqPacketCallback dummySendDaqPacketCb;
    std::vector<SignalNumericIdType> dummyUnusedSignals;
};

TEST_F(StreamingProducerTest, Instantiate)
{
    ConfigProtocolStreamingProducer streamingProducer(context, dummySendDaqPacketCb);
}

TEST_F(StreamingProducerTest, NoSignalsRegistered)
{
    ConfigProtocolStreamingProducer streamingProducer(context, dummySendDaqPacketCb);
    ASSERT_EQ(streamingProducer.findRegisteredSignal("Signal"), nullptr);
}

TEST_F(StreamingProducerTest, RegisterAndFindSignal)
{
    auto signal = signals[0];

    ConfigProtocolStreamingProducer streamingProducer(context, dummySendDaqPacketCb);

    SignalNumericIdType registeredNumericId;
    ASSERT_NO_THROW(registeredNumericId = streamingProducer.registerOrUpdateSignal(signal));
    ASSERT_EQ(registeredNumericId, 1u);

    ASSERT_EQ(streamingProducer.findRegisteredSignal(signal.getGlobalId()), signal);
}

TEST_F(StreamingProducerTest, RegisterMultipleSignals)
{
    auto signal1 = signals[0];
    auto signal2 = signals[1];

    ConfigProtocolStreamingProducer streamingProducer(context, dummySendDaqPacketCb);

    SignalNumericIdType registeredNumericId;
    ASSERT_NO_THROW(registeredNumericId = streamingProducer.registerOrUpdateSignal(signal1));
    ASSERT_EQ(registeredNumericId, 1u);
    ASSERT_NO_THROW(registeredNumericId = streamingProducer.registerOrUpdateSignal(signal2));
    ASSERT_EQ(registeredNumericId, 2u);

    ASSERT_EQ(streamingProducer.findRegisteredSignal(signal1.getGlobalId()), signal1);
    ASSERT_EQ(streamingProducer.findRegisteredSignal(signal2.getGlobalId()), signal2);
}

TEST_F(StreamingProducerTest, ConnectDisconnectNotRegisteredSignal)
{
    auto signal = signals[0];

    ConfigProtocolStreamingProducer streamingProducer(context, dummySendDaqPacketCb);

    ASSERT_ANY_THROW(streamingProducer.addConnection(signal, "TestInputPortId"));
    ASSERT_ANY_THROW(streamingProducer.removeConnection(signal, "TestInputPortId", dummyUnusedSignals));
}

TEST_F(StreamingProducerTest, ConnectDisconnectRegisteredSignal)
{
    auto valueSignal = signals[0];
    auto domainSignal = valueSignal.getDomainSignal();

    auto eventPacketValueSignal =
        DataDescriptorChangedEventPacket(descriptorToEventPacketParam(valueSignal.getDescriptor()),
                                         descriptorToEventPacketParam(domainSignal.getDescriptor()));
    auto eventPacketDomainSignal =
        DataDescriptorChangedEventPacket(descriptorToEventPacketParam(domainSignal.getDescriptor()),
                                         NullDataDescriptor());

    std::unordered_map<SignalNumericIdType, std::tuple<PacketPtr, std::promise<void>, std::future<void>>> streamingData;
    auto sendDaqPacketLambda =
        [&streamingData](const PacketPtr& packet, SignalNumericIdType signalNumericId)
    {
        std::get<0>(streamingData.at(signalNumericId)) = packet;
        std::get<1>(streamingData.at(signalNumericId)).set_value();
    };

    ConfigProtocolStreamingProducer streamingProducer(context, sendDaqPacketLambda);
    {
        std::promise<void> promise;
        std::future<void> future = promise.get_future();
        auto valueSignalNumericId = streamingProducer.registerOrUpdateSignal(valueSignal);
        ASSERT_EQ(valueSignalNumericId, 1u);
        streamingData.insert({valueSignalNumericId, std::make_tuple(nullptr, std::move(promise), std::move(future))});
    }
    {
        std::promise<void> promise;
        std::future<void> future = promise.get_future();
        auto domainSignalNumericId = streamingProducer.registerOrUpdateSignal(domainSignal);
        ASSERT_EQ(domainSignalNumericId, 2u);
        streamingData.insert({domainSignalNumericId, std::make_tuple(nullptr, std::move(promise), std::move(future))});
    }

    ASSERT_NO_THROW(streamingProducer.addConnection(valueSignal, "TestInputPortId"));
    ASSERT_EQ(valueSignal.getConnections().getCount(), 1u);
    ASSERT_EQ(domainSignal.getConnections().getCount(), 1u);

    ASSERT_EQ(std::get<2>(streamingData.at(1)).wait_for(std::chrono::seconds(1)), std::future_status::ready);
    ASSERT_EQ(std::get<2>(streamingData.at(2)).wait_for(std::chrono::seconds(1)), std::future_status::ready);
    ASSERT_EQ(std::get<0>(streamingData.at(1)), eventPacketValueSignal);
    ASSERT_EQ(std::get<0>(streamingData.at(2)), eventPacketDomainSignal);

    std::vector<SignalNumericIdType> unusedSignals;
    ASSERT_NO_THROW(streamingProducer.removeConnection(valueSignal, "TestInputPortId", unusedSignals));
    ASSERT_EQ(valueSignal.getConnections().getCount(), 0u);
    ASSERT_EQ(domainSignal.getConnections().getCount(), 0u);

    // connect again to restart producer read thread
    {
        std::promise<void> promise;
        std::future<void> future = promise.get_future();
        auto valueSignalNumericId = streamingProducer.registerOrUpdateSignal(valueSignal);
        ASSERT_EQ(valueSignalNumericId, 1u);
        streamingData.insert_or_assign(valueSignalNumericId, std::make_tuple(nullptr, std::move(promise), std::move(future)));
    }
    {
        std::promise<void> promise;
        std::future<void> future = promise.get_future();
        auto domainSignalNumericId = streamingProducer.registerOrUpdateSignal(domainSignal);
        ASSERT_EQ(domainSignalNumericId, 2u);
        streamingData.insert_or_assign(domainSignalNumericId, std::make_tuple(nullptr, std::move(promise), std::move(future)));
    }

    ASSERT_NO_THROW(streamingProducer.addConnection(valueSignal, "TestInputPortId"));
    ASSERT_EQ(valueSignal.getConnections().getCount(), 1u);
    ASSERT_EQ(domainSignal.getConnections().getCount(), 1u);

    ASSERT_EQ(std::get<2>(streamingData.at(1)).wait_for(std::chrono::seconds(1)), std::future_status::ready);
    ASSERT_EQ(std::get<2>(streamingData.at(2)).wait_for(std::chrono::seconds(1)), std::future_status::ready);
    ASSERT_EQ(std::get<0>(streamingData.at(1)), eventPacketValueSignal);
    ASSERT_EQ(std::get<0>(streamingData.at(2)), eventPacketDomainSignal);
}

TEST_F(StreamingProducerTest, SignalMultipleConnections)
{
    auto valueSignal = signals[0];
    auto domainSignal = valueSignal.getDomainSignal();

    ConfigProtocolStreamingProducer streamingProducer(context, dummySendDaqPacketCb);
    auto valueSignalNumericId = streamingProducer.registerOrUpdateSignal(valueSignal);
    ASSERT_EQ(valueSignalNumericId, 1u);
    auto domainSignalNumericId = streamingProducer.registerOrUpdateSignal(domainSignal);
    ASSERT_EQ(domainSignalNumericId, 2u);

    ASSERT_NO_THROW(streamingProducer.addConnection(valueSignal, "TestInputPortId1"));
    ASSERT_EQ(valueSignal.getConnections().getCount(), 1u);
    ASSERT_EQ(domainSignal.getConnections().getCount(), 1u);

    ASSERT_NO_THROW(streamingProducer.addConnection(valueSignal, "TestInputPortId2"));
    ASSERT_EQ(valueSignal.getConnections().getCount(), 1u);
    ASSERT_EQ(domainSignal.getConnections().getCount(), 1u);

    ASSERT_NO_THROW(streamingProducer.addConnection(domainSignal, "TestInputPortId3"));
    ASSERT_EQ(valueSignal.getConnections().getCount(), 1u);
    ASSERT_EQ(domainSignal.getConnections().getCount(), 1u);

    {
        std::vector<SignalNumericIdType> unusedSignals;
        ASSERT_NO_THROW(streamingProducer.removeConnection(valueSignal, "TestInputPortId1", unusedSignals));
        ASSERT_EQ(valueSignal.getConnections().getCount(), 1u); // reader still exists
        ASSERT_EQ(domainSignal.getConnections().getCount(), 1u); // reader still exists
        ASSERT_EQ(unusedSignals.size(), 0u);
    }
    {
        std::vector<SignalNumericIdType> unusedSignals;
        ASSERT_NO_THROW(streamingProducer.removeConnection(valueSignal, "TestInputPortId2", unusedSignals));
        ASSERT_EQ(valueSignal.getConnections().getCount(), 0u); // reader removed
        ASSERT_EQ(domainSignal.getConnections().getCount(), 1u); // reader still exists
        ASSERT_EQ(unusedSignals.size(), 1u);
        ASSERT_EQ(unusedSignals[0], valueSignalNumericId);
    }
    {
        std::vector<SignalNumericIdType> unusedSignals;
        ASSERT_NO_THROW(streamingProducer.removeConnection(domainSignal, "TestInputPortId3", unusedSignals));
        ASSERT_EQ(valueSignal.getConnections().getCount(), 0u); // reader removed
        ASSERT_EQ(domainSignal.getConnections().getCount(), 0u); // reader removed
        ASSERT_EQ(unusedSignals.size(), 1u);
        ASSERT_EQ(unusedSignals[0], domainSignalNumericId);
    }
}

using ConfigServerPtr = std::unique_ptr<ConfigProtocolServer>;
using ConfigClientPtr = std::unique_ptr<ConfigProtocolClient<ConfigClientDeviceImpl>>;

class AsyncRpcHelper
{
public:
    explicit AsyncRpcHelper(ConfigServerPtr& serverPtr)
        : running(true)
        , serverPtr(serverPtr)
    {
        workerThread = std::thread([this]() { threadFunc(); });
    }

    ~AsyncRpcHelper()
    {
        running = false;
        workerThread.join();
    }

    void sendNoReplyReqToServer(const PacketBuffer& packetBuffer)
    {
        std::scoped_lock lock(sync);
        packetsQueue.push(std::make_shared<PacketBuffer>(packetBuffer.getBuffer(), true));
    }

private:
    using PacketBufferPtr = std::shared_ptr<PacketBuffer>;
    void threadFunc()
    {
        while(running)
        {
            bool hasPacket = false;
            PacketBufferPtr packetPtr;
            {
                std::scoped_lock lock(sync);
                hasPacket = !packetsQueue.empty();
                if (hasPacket)
                {
                    packetPtr = std::move(packetsQueue.front());
                    packetsQueue.pop();
                }
            }
            if (hasPacket)
            {
                assert(packetPtr->getPacketType() == config_protocol::PacketType::NoReplyRpc);
                serverPtr->processNoReplyRequest(*packetPtr);
            }
        }
    }

    std::thread workerThread;
    bool running;
    ConfigServerPtr& serverPtr;
    std::queue<PacketBufferPtr> packetsQueue;
    std::mutex sync;
};

class ClientToDeviceStreamingTest : public Test
{
public:
    ClientToDeviceStreamingTest()
        : anonymousUser(User("", ""))
        , helper1(std::ref(server1))
        , helper2(std::ref(server2))
    {
    }

    ConfigServerPtr createServer(ConfigClientPtr& clientPtr)
    {
        return std::make_unique<ConfigProtocolServer>(
            serverDevice,
            std::bind(&ClientToDeviceStreamingTest::serverNotificationReady, this, std::ref(clientPtr), std::placeholders::_1),
            anonymousUser,
            serverExtSigFolder);
    }

    void createAndConnectClient(const std::string& rootDevId, ConfigClientPtr& clientPtr, ConfigServerPtr& serverPtr, AsyncRpcHelper& helper, DevicePtr& clientRootDevice, DevicePtr& clientMirroredDevice, FolderPtr& clientExtSigFolder)
    {
        clientPtr = std::make_unique<ConfigProtocolClient<ConfigClientDeviceImpl>>(
            NullContext(),
            std::bind(&ClientToDeviceStreamingTest::sendRequestAndGetReply, this, std::ref(serverPtr), std::placeholders::_1),
            std::bind(&ClientToDeviceStreamingTest::sendNoReplyRequest, this, std::ref(helper), std::placeholders::_1),
            nullptr,
            nullptr);

        clientRootDevice = test_utils::createTestDevice(rootDevId);
        clientRootDevice.asPtr<IPropertyObjectInternal>().enableCoreEventTrigger();

        clientMirroredDevice = clientPtr->connect(clientRootDevice.getItem("Dev"));
        clientMirroredDevice.asPtr<IPropertyObjectInternal>().enableCoreEventTrigger();

        clientExtSigFolder = clientMirroredDevice.getServers()[0].getItem("Sig");
    }

    void SetUp() override
    {
        serverDevice = test_utils::createTestDevice("server");
        serverExtSigFolder = serverDevice.getServers()[0].getItem("Sig");
        serverDevice.asPtr<IPropertyObjectInternal>().enableCoreEventTrigger();

        server1 = createServer(client1);
        createAndConnectClient("client1", client1, server1, helper1, client1RootDevice, client1MirroredDevice, client1ExtSigFolder);

        server2 = createServer(client2);
        createAndConnectClient("client2", client2, server2, helper2, client2RootDevice, client2MirroredDevice, client2ExtSigFolder);

        chIpServer = serverDevice.getChannels()[0].getInputPorts()[0];
        chIpClient1 = client1MirroredDevice.getChannels()[0].getInputPorts()[0];
        chIpClient2 = client2MirroredDevice.getChannels()[0].getInputPorts()[0];

        fbIpServer = serverDevice.getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0];
        fbIpClient1 = client1MirroredDevice.getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0];
        fbIpClient2 = client2MirroredDevice.getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0];

        serverExtSigFolder.getOnComponentCoreEvent() += event(this, &ClientToDeviceStreamingTest::onServerSignalRemoved);
        client1ExtSigFolder.getOnComponentCoreEvent() += event(this, &ClientToDeviceStreamingTest::onClient1SignalRemoved);
        client2ExtSigFolder.getOnComponentCoreEvent() += event(this, &ClientToDeviceStreamingTest::onClient2SignalRemoved);
    }

    void TearDown() override
    {
        serverExtSigFolder.getOnComponentCoreEvent() -= event(this, &ClientToDeviceStreamingTest::onServerSignalRemoved);
        client1ExtSigFolder.getOnComponentCoreEvent() -= event(this, &ClientToDeviceStreamingTest::onClient1SignalRemoved);
        client2ExtSigFolder.getOnComponentCoreEvent() -= event(this, &ClientToDeviceStreamingTest::onClient2SignalRemoved);
    }

    void setExternalSignalRemovedExpectation(size_t serverRemovedSignalsCount, size_t client1RemovedSignalsCount, size_t client2RemovedSignalsCount)
    {
        serverExtSigToBeRemoved = serverRemovedSignalsCount;
        client1ExtSigToBeRemoved = client1RemovedSignalsCount;
        client2ExtSigToBeRemoved = client2RemovedSignalsCount;

        serverExtSigRemovedPromise = std::promise<void>();
        serverExtSigRemovedFuture = serverExtSigRemovedPromise.get_future();
        client1ExtSigRemovedPromise = std::promise<void>();
        client1ExtSigRemovedFuture = client1ExtSigRemovedPromise.get_future();
        client2ExtSigRemovedPromise = std::promise<void>();
        client2ExtSigRemovedFuture = client2ExtSigRemovedPromise.get_future();
    }

    bool isExternalMirroredSignalsRemoved()
    {
        bool serverSignalsRemoved = serverExtSigToBeRemoved == 0 || serverExtSigRemovedFuture.wait_for(std::chrono::seconds(5)) == std::future_status::ready;
        bool client1SignalsRemoved = client1ExtSigToBeRemoved == 0 || client1ExtSigRemovedFuture.wait_for(std::chrono::seconds(5)) == std::future_status::ready;
        bool client2SignalsRemoved = client2ExtSigToBeRemoved == 0 || client2ExtSigRemovedFuture.wait_for(std::chrono::seconds(5)) == std::future_status::ready;
        return serverSignalsRemoved && client1SignalsRemoved && client2SignalsRemoved;
    }

    bool testDomainSignal(const SignalPtr& signal, const SignalPtr& domainSignal)
    {
        return signal.getDomainSignal().assigned() && signal.getDomainSignal() == domainSignal;
    }

protected:
    const UserPtr anonymousUser;
    LoggerComponentPtr loggerComponent;
    DevicePtr serverDevice;

    DevicePtr client1MirroredDevice;
    DevicePtr client2MirroredDevice;
    DevicePtr client1RootDevice;
    DevicePtr client2RootDevice;

    ConfigServerPtr server1;
    ConfigClientPtr client1;
    AsyncRpcHelper helper1;

    ConfigServerPtr server2;
    ConfigClientPtr client2;
    AsyncRpcHelper helper2;

    FolderPtr serverExtSigFolder;
    FolderPtr client1ExtSigFolder;
    FolderPtr client2ExtSigFolder;

    InputPortPtr chIpServer;
    InputPortPtr chIpClient1;
    InputPortPtr chIpClient2;

    InputPortPtr fbIpServer;
    InputPortPtr fbIpClient1;
    InputPortPtr fbIpClient2;

    size_t serverExtSigToBeRemoved{0};
    size_t client1ExtSigToBeRemoved{0};
    size_t client2ExtSigToBeRemoved{0};

    std::promise<void> serverExtSigRemovedPromise;
    std::promise<void> client1ExtSigRemovedPromise;
    std::promise<void> client2ExtSigRemovedPromise;

    std::future<void> serverExtSigRemovedFuture;
    std::future<void> client1ExtSigRemovedFuture;
    std::future<void> client2ExtSigRemovedFuture;

    // server handling
    void serverNotificationReady(ConfigClientPtr& clientPtr, const PacketBuffer& notificationPacket) const
    {
        clientPtr->triggerNotificationPacket(notificationPacket);
    }

    // client handling
    PacketBuffer sendRequestAndGetReply(ConfigServerPtr& serverPtr, const PacketBuffer& requestPacket) const
    {
        return serverPtr->processRequestAndGetReply(requestPacket);
    }

    void sendNoReplyRequest(AsyncRpcHelper& helper, const PacketBuffer& requestPacket) const
    {
        helper.sendNoReplyReqToServer(requestPacket);
    }

    static StringPtr remoteId(const SignalPtr& signal)
    {
        return signal.asPtr<IMirroredSignalConfig>().getRemoteId();
    }

    void onServerSignalRemoved(ComponentPtr& comp, CoreEventArgsPtr& eventArgs)
    {
        onSignalRemoved(serverExtSigToBeRemoved, serverExtSigRemovedPromise, comp, eventArgs);
    };
    void onClient1SignalRemoved(ComponentPtr& comp, CoreEventArgsPtr& eventArgs)
    {
        onSignalRemoved(client1ExtSigToBeRemoved, client1ExtSigRemovedPromise, comp, eventArgs);
    };
    void onClient2SignalRemoved(ComponentPtr& comp, CoreEventArgsPtr& eventArgs)
    {
        onSignalRemoved(client2ExtSigToBeRemoved, client2ExtSigRemovedPromise, comp, eventArgs);
    };

    void onSignalRemoved(size_t& signalCounter, std::promise<void>& promise, const ComponentPtr& comp, const CoreEventArgsPtr& eventArgs)
    {
        if (static_cast<CoreEventId>(eventArgs.getEventId()) == CoreEventId::ComponentRemoved)
        {
            if (signalCounter == 0)
            {
                EXPECT_FALSE(true) << "Unexpected removed signal " << eventArgs.getParameters().get("Id");
            }
            else if (--signalCounter == 0)
            {
                promise.set_value();
            }
        }
    }
};

TEST_F(ClientToDeviceStreamingTest, NoneSignalsConnected)
{
    // none external signals connected - none mirrored signals created
    EXPECT_EQ(serverExtSigFolder.getItems().getCount(), 0u);
    EXPECT_EQ(client1ExtSigFolder.getItems().getCount(), 0u);
    EXPECT_EQ(client2ExtSigFolder.getItems().getCount(), 0u);
}

TEST_F(ClientToDeviceStreamingTest, ConnectExternalSignalFailure)
{
    SignalPtr localRootSigClient1 = client1RootDevice.getSignals()[0];
    SignalPtr localRootSigClient2 = client2RootDevice.getSignals()[0];

    EXPECT_ANY_THROW(chIpClient2.connect(localRootSigClient1));
    EXPECT_ANY_THROW(chIpClient1.connect(localRootSigClient2));
}

TEST_F(ClientToDeviceStreamingTest, SignalWithoutDomainConnectOnly)
{
    SignalPtr localRootSigClient1 = client1RootDevice.getSignals()[0];
    SignalPtr localRootSigClient2 = client2RootDevice.getSignals()[0];

    // connect local signal of 1-st client to mirrored channel input port
    chIpClient1.connect(localRootSigClient1);
    // one external signal connected - one mirrored signal created
    EXPECT_EQ(serverExtSigFolder.getItems().getCount(), 1u);
    EXPECT_EQ(client1ExtSigFolder.getItems().getCount(), 0u);
    EXPECT_EQ(client2ExtSigFolder.getItems().getCount(), 1u);
    // verify connected signals
    EXPECT_EQ(chIpClient1.getSignal(), localRootSigClient1);
    EXPECT_EQ(remoteId(chIpServer.getSignal()), localRootSigClient1.getGlobalId());
    EXPECT_EQ(remoteId(chIpClient2.getSignal()), chIpServer.getSignal().getGlobalId());

    // connect local signal of 2-nd client to mirrored fb input port
    fbIpClient2.connect(localRootSigClient2);
    // two external signals connected - two mirrored signals created
    EXPECT_EQ(serverExtSigFolder.getItems().getCount(), 2u);
    EXPECT_EQ(client1ExtSigFolder.getItems().getCount(), 1u);
    EXPECT_EQ(client2ExtSigFolder.getItems().getCount(), 1u);
    // verify connected signals
    EXPECT_EQ(fbIpClient2.getSignal(), localRootSigClient2);
    EXPECT_EQ(remoteId(fbIpServer.getSignal()), localRootSigClient2.getGlobalId());
    EXPECT_EQ(remoteId(fbIpClient1.getSignal()), fbIpServer.getSignal().getGlobalId());
}

TEST_F(ClientToDeviceStreamingTest, SignalWithoutDomainConnectDisconnect)
{
    SignalPtr localRootSigClient1 = client1RootDevice.getSignals()[0];
    SignalPtr localRootSigClient2 = client2RootDevice.getSignals()[0];

    // connect local signal of 1-st client to mirrored channel input port
    chIpClient1.connect(localRootSigClient1);
    // connect local signal of 2-nd client to mirrored fb input port
    fbIpClient2.connect(localRootSigClient2);

    setExternalSignalRemovedExpectation(1, 0, 1);
    chIpClient1.disconnect();
    EXPECT_TRUE(isExternalMirroredSignalsRemoved());
    // one external signal connected - one mirrored signal remains
    EXPECT_EQ(serverExtSigFolder.getItems().getCount(), 1u);
    EXPECT_EQ(client1ExtSigFolder.getItems().getCount(), 1u);
    EXPECT_EQ(client2ExtSigFolder.getItems().getCount(), 0u);
    // verify that signal disconnected
    EXPECT_FALSE(chIpClient1.getSignal().assigned());
    EXPECT_FALSE(chIpServer.getSignal().assigned());
    EXPECT_FALSE(chIpClient2.getSignal().assigned());

    setExternalSignalRemovedExpectation(1, 1, 0);
    fbIpClient2.disconnect();
    EXPECT_TRUE(isExternalMirroredSignalsRemoved());
    // none signals connected - none mirrored signals existing
    EXPECT_EQ(serverExtSigFolder.getItems().getCount(), 0u);
    EXPECT_EQ(client1ExtSigFolder.getItems().getCount(), 0u);
    EXPECT_EQ(client2ExtSigFolder.getItems().getCount(), 0u);
    // verify that signal disconnected
    EXPECT_FALSE(fbIpClient1.getSignal().assigned());
    EXPECT_FALSE(fbIpServer.getSignal().assigned());
    EXPECT_FALSE(fbIpClient2.getSignal().assigned());
}

TEST_F_UNSTABLE_SKIPPED(ClientToDeviceStreamingTest, ReplaceConnectedSignalWithServerSignal)
{
    SignalPtr localRootSigClient1 = client1RootDevice.getSignals()[0];
    SignalPtr localRootSigClient2 = client2RootDevice.getSignals()[0];

    // connect local signal of 1-st client to mirrored channel input port
    chIpClient1.connect(localRootSigClient1);
    // connect local signal of 2-nd client to mirrored fb input port
    fbIpClient2.connect(localRootSigClient2);

    setExternalSignalRemovedExpectation(1, 0, 1);
    chIpClient1.connect(client1MirroredDevice.getDevices()[0].getSignals()[0]);
    EXPECT_TRUE(isExternalMirroredSignalsRemoved());
    // one external signal connected - one mirrored signal remains
    EXPECT_EQ(serverExtSigFolder.getItems().getCount(), 1u);
    EXPECT_EQ(client1ExtSigFolder.getItems().getCount(), 1u);
    EXPECT_EQ(client2ExtSigFolder.getItems().getCount(), 0u);
    // verify that signal disconnected
    EXPECT_TRUE(chIpClient1.getSignal().assigned());
    EXPECT_TRUE(chIpServer.getSignal().assigned());
    EXPECT_TRUE(chIpClient2.getSignal().assigned());

    setExternalSignalRemovedExpectation(1, 1, 0);
    fbIpClient2.connect(client2MirroredDevice.getDevices()[0].getSignals()[0]);
    EXPECT_TRUE(isExternalMirroredSignalsRemoved());
    // none signals connected - none mirrored signals existing
    EXPECT_EQ(serverExtSigFolder.getItems().getCount(), 0u);
    EXPECT_EQ(client1ExtSigFolder.getItems().getCount(), 0u);
    EXPECT_EQ(client2ExtSigFolder.getItems().getCount(), 0u);
    // verify that signal disconnected
    EXPECT_TRUE(fbIpClient1.getSignal().assigned());
    EXPECT_TRUE(fbIpServer.getSignal().assigned());
    EXPECT_TRUE(fbIpClient2.getSignal().assigned());
}

TEST_F(ClientToDeviceStreamingTest, ReplaceConnectedSignalWithAnotherExternal)
{
    SignalPtr localRootSigClient1 = client1RootDevice.getSignals()[0];
    SignalPtr localRootSigClient2 = client2RootDevice.getSignals()[0];

    SignalPtr localSigClient1 = client1RootDevice.getDevices()[0].getSignals()[0];
    SignalPtr localSigClient2 = client2RootDevice.getDevices()[0].getSignals()[0];

    // connect local signal of 1-st client to mirrored channel input port
    chIpClient1.connect(localRootSigClient1);
    // connect local signal of 2-nd client to mirrored fb input port
    fbIpClient2.connect(localRootSigClient2);

    setExternalSignalRemovedExpectation(1, 0, 1);
    chIpClient1.connect(localSigClient1);
    EXPECT_TRUE(isExternalMirroredSignalsRemoved());
    // two external signal still connected - two mirrored signal remains
    EXPECT_EQ(serverExtSigFolder.getItems().getCount(), 2u);
    EXPECT_EQ(client1ExtSigFolder.getItems().getCount(), 1u);
    EXPECT_EQ(client2ExtSigFolder.getItems().getCount(), 1u);
    // verify that signal disconnected
    EXPECT_TRUE(chIpClient1.getSignal().assigned());
    EXPECT_TRUE(chIpServer.getSignal().assigned());
    EXPECT_TRUE(chIpClient2.getSignal().assigned());

    setExternalSignalRemovedExpectation(1, 1, 0);
    fbIpClient2.connect(localSigClient2);
    EXPECT_TRUE(isExternalMirroredSignalsRemoved());
    // two external signal still connected - two mirrored signal remains
    EXPECT_EQ(serverExtSigFolder.getItems().getCount(), 2u);
    EXPECT_EQ(client1ExtSigFolder.getItems().getCount(), 1u);
    EXPECT_EQ(client2ExtSigFolder.getItems().getCount(), 1u);
    // verify that signal disconnected
    EXPECT_TRUE(fbIpClient1.getSignal().assigned());
    EXPECT_TRUE(fbIpServer.getSignal().assigned());
    EXPECT_TRUE(fbIpClient2.getSignal().assigned());
}

TEST_F(ClientToDeviceStreamingTest, SignalWithoutDomainPortRemove)
{
    SignalPtr localRootSigClient1 = client1RootDevice.getSignals()[0];
    SignalPtr localRootSigClient2 = client2RootDevice.getSignals()[0];

    // connect local signal of 2-nd client to mirrored fb input port
    fbIpClient2.connect(localRootSigClient2);

    setExternalSignalRemovedExpectation(1, 1, 0);
    auto fbToRemove = serverDevice.getDevices()[0].getFunctionBlocks()[0];
    serverDevice.getDevices()[0].removeFunctionBlock(fbToRemove);
    EXPECT_TRUE(isExternalMirroredSignalsRemoved());
    // none signals connected - none mirrored signals existing
    EXPECT_EQ(serverExtSigFolder.getItems().getCount(), 0u);
    EXPECT_EQ(client1ExtSigFolder.getItems().getCount(), 0u);
    EXPECT_EQ(client2ExtSigFolder.getItems().getCount(), 0u);
    // verify that signal disconnected
    EXPECT_TRUE(fbIpClient1.isRemoved());
    EXPECT_TRUE(fbIpServer.isRemoved());
    EXPECT_TRUE(fbIpClient2.isRemoved());
}

TEST_F(ClientToDeviceStreamingTest, ConnectMirroredExternalSignalFailure)
{
    SignalPtr localRootSigClient1 = client1RootDevice.getSignals()[0];

    // connect local signal of 1-st client to mirrored channel input port
    chIpClient1.connect(localRootSigClient1);

    SignalPtr mirroredExternalSignal = client2ExtSigFolder.getItems()[0];
    EXPECT_ANY_THROW(fbIpClient2.connect(mirroredExternalSignal));
}

TEST_F(ClientToDeviceStreamingTest, SignalWithoutDomainConnectTwice)
{
    SignalPtr localRootSigClient1 = client1RootDevice.getSignals()[0];

    // connect local signal of 1-st client to mirrored channel input port
    chIpClient1.connect(localRootSigClient1);
    // one external signal connected - one mirrored signal created
    EXPECT_EQ(serverExtSigFolder.getItems().getCount(), 1u);
    EXPECT_EQ(client1ExtSigFolder.getItems().getCount(), 0u);
    EXPECT_EQ(client2ExtSigFolder.getItems().getCount(), 1u);
    // verify connected signals
    EXPECT_EQ(chIpClient1.getSignal(), localRootSigClient1);
    EXPECT_EQ(remoteId(chIpServer.getSignal()), localRootSigClient1.getGlobalId());
    EXPECT_EQ(remoteId(chIpClient2.getSignal()), chIpServer.getSignal().getGlobalId());

    // connect local signal of 1-st client to mirrored fb input port
    fbIpClient1.connect(localRootSigClient1);
    // one external signal connected - one mirrored signal created
    EXPECT_EQ(serverExtSigFolder.getItems().getCount(), 1u);
    EXPECT_EQ(client1ExtSigFolder.getItems().getCount(), 0u);
    EXPECT_EQ(client2ExtSigFolder.getItems().getCount(), 1u);
    // verify connected signals
    EXPECT_EQ(fbIpClient1.getSignal(), localRootSigClient1);
    EXPECT_EQ(remoteId(fbIpServer.getSignal()), localRootSigClient1.getGlobalId());
    EXPECT_EQ(remoteId(fbIpClient2.getSignal()), fbIpServer.getSignal().getGlobalId());
}

TEST_F(ClientToDeviceStreamingTest, SignalWithoutDomainConnectDisconnectTwice)
{
    SignalPtr localRootSigClient1 = client1RootDevice.getSignals()[0];

    // connect local signal of 1-st client to mirrored channel input port
    chIpClient1.connect(localRootSigClient1);
    // connect local signal of 1-st client to mirrored fb input port
    fbIpClient1.connect(localRootSigClient1);

    setExternalSignalRemovedExpectation(0, 0, 0);
    chIpClient1.disconnect();
    EXPECT_TRUE(isExternalMirroredSignalsRemoved());
    // one external signal connected - one mirrored signal remains
    EXPECT_EQ(serverExtSigFolder.getItems().getCount(), 1u);
    EXPECT_EQ(client1ExtSigFolder.getItems().getCount(), 0u);
    EXPECT_EQ(client2ExtSigFolder.getItems().getCount(), 1u);
    // verify that signal disconnected
    EXPECT_FALSE(chIpClient1.getSignal().assigned());
    EXPECT_FALSE(chIpServer.getSignal().assigned());
    EXPECT_FALSE(chIpClient2.getSignal().assigned());

    setExternalSignalRemovedExpectation(1, 0, 1);
    fbIpClient1.disconnect();
    EXPECT_TRUE(isExternalMirroredSignalsRemoved());
    // none signals connected - none mirrored signals existing
    EXPECT_EQ(serverExtSigFolder.getItems().getCount(), 0u);
    EXPECT_EQ(client1ExtSigFolder.getItems().getCount(), 0u);
    EXPECT_EQ(client2ExtSigFolder.getItems().getCount(), 0u);
    // verify that signal disconnected
    EXPECT_FALSE(fbIpClient1.getSignal().assigned());
    EXPECT_FALSE(fbIpServer.getSignal().assigned());
    EXPECT_FALSE(fbIpClient2.getSignal().assigned());
}

TEST_F(ClientToDeviceStreamingTest, SignalWithDomainConnectOnly)
{
    SignalPtr localChSigClient1 = client1RootDevice.getDevices()[0].getChannels()[0].getSignals()[0];
    SignalPtr localChSigClient2 = client2RootDevice.getDevices()[0].getChannels()[0].getSignals()[0];

    // connect local signal of 1-st client to mirrored channel input port
    chIpClient1.connect(localChSigClient1);
    // one external signal with domain connected - two mirrored signals created
    EXPECT_EQ(serverExtSigFolder.getItems().getCount(), 2u);
    EXPECT_EQ(client1ExtSigFolder.getItems().getCount(), 0u);
    EXPECT_EQ(client2ExtSigFolder.getItems().getCount(), 2u);
    EXPECT_TRUE(testDomainSignal(serverExtSigFolder.getItems()[1], serverExtSigFolder.getItems()[0]));
    EXPECT_TRUE(testDomainSignal(client2ExtSigFolder.getItems()[1], client2ExtSigFolder.getItems()[0]));
    // verify connected signals
    EXPECT_EQ(chIpClient1.getSignal(), localChSigClient1);
    EXPECT_EQ(remoteId(chIpServer.getSignal()), localChSigClient1.getGlobalId());
    EXPECT_EQ(remoteId(chIpClient2.getSignal()), chIpServer.getSignal().getGlobalId());

    // connect local signal of 2-nd client to mirrored fb input port
    fbIpClient2.connect(localChSigClient2);
    // two external signals with domain connected - four mirrored signals created
    EXPECT_EQ(serverExtSigFolder.getItems().getCount(), 4u);
    EXPECT_EQ(client1ExtSigFolder.getItems().getCount(), 2u);
    EXPECT_EQ(client2ExtSigFolder.getItems().getCount(), 2u);
    EXPECT_TRUE(testDomainSignal(serverExtSigFolder.getItems()[3], serverExtSigFolder.getItems()[2]));
    EXPECT_TRUE(testDomainSignal(client1ExtSigFolder.getItems()[1], client1ExtSigFolder.getItems()[0]));
    // verify connected signals
    EXPECT_EQ(fbIpClient2.getSignal(), localChSigClient2);
    EXPECT_EQ(remoteId(fbIpServer.getSignal()), localChSigClient2.getGlobalId());
    EXPECT_EQ(remoteId(fbIpClient1.getSignal()), fbIpServer.getSignal().getGlobalId());
}

TEST_F(ClientToDeviceStreamingTest, SignalWithDomainConnectDisconnect)
{
    SignalPtr localChSigClient1 = client1RootDevice.getDevices()[0].getChannels()[0].getSignals()[0];
    SignalPtr localChSigClient2 = client2RootDevice.getDevices()[0].getChannels()[0].getSignals()[0];

    // connect local signal of 1-st client to mirrored channel input port
    chIpClient1.connect(localChSigClient1);
    // connect local signal of 2-nd client to mirrored fb input port
    fbIpClient2.connect(localChSigClient2);

    setExternalSignalRemovedExpectation(2, 0, 2);
    chIpClient1.disconnect();
    EXPECT_TRUE(isExternalMirroredSignalsRemoved());
    // one external signal with domain connected - two mirrored signals remains
    EXPECT_EQ(serverExtSigFolder.getItems().getCount(), 2u);
    EXPECT_EQ(client1ExtSigFolder.getItems().getCount(), 2u);
    EXPECT_EQ(client2ExtSigFolder.getItems().getCount(), 0u);
    EXPECT_TRUE(testDomainSignal(serverExtSigFolder.getItems()[1], serverExtSigFolder.getItems()[0]));
    EXPECT_TRUE(testDomainSignal(client1ExtSigFolder.getItems()[1], client1ExtSigFolder.getItems()[0]));
    // verify that signal disconnected
    EXPECT_FALSE(chIpClient1.getSignal().assigned());
    EXPECT_FALSE(chIpServer.getSignal().assigned());
    EXPECT_FALSE(chIpClient2.getSignal().assigned());

    setExternalSignalRemovedExpectation(2, 2, 0);
    fbIpClient2.disconnect();
    EXPECT_TRUE(isExternalMirroredSignalsRemoved());
    // none signals connected - none mirrored signals existing
    EXPECT_EQ(serverExtSigFolder.getItems().getCount(), 0u);
    EXPECT_EQ(client1ExtSigFolder.getItems().getCount(), 0u);
    EXPECT_EQ(client2ExtSigFolder.getItems().getCount(), 0u);
    // verify that signal disconnected
    EXPECT_FALSE(fbIpClient1.getSignal().assigned());
    EXPECT_FALSE(fbIpServer.getSignal().assigned());
    EXPECT_FALSE(fbIpClient2.getSignal().assigned());
}

TEST_F(ClientToDeviceStreamingTest, SignalsWithSharedDomainConnectOnly)
{
    SignalPtr localFbSig1Client1 = client1RootDevice.getDevices()[0].getFunctionBlocks()[0].getSignals()[0];
    SignalPtr localFbSig2Client1 = client1RootDevice.getDevices()[0].getFunctionBlocks()[0].getSignals()[1];

    // connect local signal of 1-st client to mirrored channel input port
    chIpClient1.connect(localFbSig1Client1);
    // one external signal with domain connected - two mirrored signals created
    EXPECT_EQ(serverExtSigFolder.getItems().getCount(), 2u);
    EXPECT_EQ(client1ExtSigFolder.getItems().getCount(), 0u);
    EXPECT_EQ(client2ExtSigFolder.getItems().getCount(), 2u);
    EXPECT_TRUE(testDomainSignal(serverExtSigFolder.getItems()[1], serverExtSigFolder.getItems()[0]));
    EXPECT_TRUE(testDomainSignal(client2ExtSigFolder.getItems()[1], client2ExtSigFolder.getItems()[0]));
    // verify connected signals
    EXPECT_EQ(chIpClient1.getSignal(), localFbSig1Client1);
    EXPECT_EQ(remoteId(chIpServer.getSignal()), localFbSig1Client1.getGlobalId());
    EXPECT_EQ(remoteId(chIpClient2.getSignal()), chIpServer.getSignal().getGlobalId());

    // connect local signal of 1-st client to mirrored fb input port
    fbIpClient1.connect(localFbSig2Client1);
    // two external signals with shared domain connected - 3 mirrored signals created
    EXPECT_EQ(serverExtSigFolder.getItems().getCount(), 3u);
    EXPECT_EQ(client1ExtSigFolder.getItems().getCount(), 0u);
    EXPECT_EQ(client2ExtSigFolder.getItems().getCount(), 3u);
    EXPECT_TRUE(testDomainSignal(serverExtSigFolder.getItems()[2], serverExtSigFolder.getItems()[0]));
    EXPECT_TRUE(testDomainSignal(client2ExtSigFolder.getItems()[2], client2ExtSigFolder.getItems()[0]));
    // verify connected signals
    EXPECT_EQ(fbIpClient1.getSignal(), localFbSig2Client1);
    EXPECT_EQ(remoteId(fbIpServer.getSignal()), localFbSig2Client1.getGlobalId());
    EXPECT_EQ(remoteId(fbIpClient2.getSignal()), fbIpServer.getSignal().getGlobalId());
}

TEST_F(ClientToDeviceStreamingTest, SignalsWithSharedDomainConnectDisconnect)
{
    SignalPtr localFbSig1Client1 = client1RootDevice.getDevices()[0].getFunctionBlocks()[0].getSignals()[0];
    SignalPtr localFbSig2Client1 = client1RootDevice.getDevices()[0].getFunctionBlocks()[0].getSignals()[1];

    // connect local signal of 1-st client to mirrored channel input port
    chIpClient1.connect(localFbSig1Client1);
    // connect local signal of 1-st client to mirrored fb input port
    fbIpClient1.connect(localFbSig2Client1);

    setExternalSignalRemovedExpectation(1, 0, 1);
    chIpClient1.disconnect();
    EXPECT_TRUE(isExternalMirroredSignalsRemoved());
    // one external signal with domain connected - two mirrored signals remains
    EXPECT_EQ(serverExtSigFolder.getItems().getCount(), 2u);
    EXPECT_EQ(client1ExtSigFolder.getItems().getCount(), 0u);
    EXPECT_EQ(client2ExtSigFolder.getItems().getCount(), 2u);
    EXPECT_TRUE(testDomainSignal(serverExtSigFolder.getItems()[1], serverExtSigFolder.getItems()[0]));
    EXPECT_TRUE(testDomainSignal(client2ExtSigFolder.getItems()[1], client2ExtSigFolder.getItems()[0]));
    // verify that signal disconnected
    EXPECT_FALSE(chIpClient1.getSignal().assigned());
    EXPECT_FALSE(chIpServer.getSignal().assigned());
    EXPECT_FALSE(chIpClient2.getSignal().assigned());

    setExternalSignalRemovedExpectation(2, 0, 2);
    fbIpClient1.disconnect();
    EXPECT_TRUE(isExternalMirroredSignalsRemoved());
    // none signals connected - none mirrored signals existing
    EXPECT_EQ(serverExtSigFolder.getItems().getCount(), 0u);
    EXPECT_EQ(client1ExtSigFolder.getItems().getCount(), 0u);
    EXPECT_EQ(client2ExtSigFolder.getItems().getCount(), 0u);
    // verify that signal disconnected
    EXPECT_FALSE(fbIpClient1.getSignal().assigned());
    EXPECT_FALSE(fbIpServer.getSignal().assigned());
    EXPECT_FALSE(fbIpClient2.getSignal().assigned());
}

TEST_F(ClientToDeviceStreamingTest, SignalAndDomainConnectOnly)
{
    SignalPtr localChValSigClient1 = client1RootDevice.getDevices()[0].getChannels()[0].getSignals()[0];
    SignalPtr localChDomSigClient1 = client1RootDevice.getDevices()[0].getChannels()[0].getSignals()[1];

    // connect value local signal of 1-st client to mirrored channel input port
    chIpClient1.connect(localChValSigClient1);
    // one external signal with domain connected - two mirrored signals created
    EXPECT_EQ(serverExtSigFolder.getItems().getCount(), 2u);
    EXPECT_EQ(client1ExtSigFolder.getItems().getCount(), 0u);
    EXPECT_EQ(client2ExtSigFolder.getItems().getCount(), 2u);
    EXPECT_TRUE(testDomainSignal(serverExtSigFolder.getItems()[1], serverExtSigFolder.getItems()[0]));
    EXPECT_TRUE(testDomainSignal(client2ExtSigFolder.getItems()[1], client2ExtSigFolder.getItems()[0]));
    // verify connected signals
    EXPECT_EQ(chIpClient1.getSignal(), localChValSigClient1);
    EXPECT_EQ(remoteId(chIpServer.getSignal()), localChValSigClient1.getGlobalId());
    EXPECT_EQ(remoteId(chIpClient2.getSignal()), chIpServer.getSignal().getGlobalId());

    // connect domain local signal of 1-st client to mirrored fb input port
    fbIpClient1.connect(localChDomSigClient1);
    // external value and domain signals connected - 2 mirrored signals created
    EXPECT_EQ(serverExtSigFolder.getItems().getCount(), 2u);
    EXPECT_EQ(client1ExtSigFolder.getItems().getCount(), 0u);
    EXPECT_EQ(client2ExtSigFolder.getItems().getCount(), 2u);
    EXPECT_TRUE(testDomainSignal(serverExtSigFolder.getItems()[1], serverExtSigFolder.getItems()[0]));
    EXPECT_TRUE(testDomainSignal(client2ExtSigFolder.getItems()[1], client2ExtSigFolder.getItems()[0]));
    // verify connected signals
    EXPECT_EQ(fbIpClient1.getSignal(), localChDomSigClient1);
    EXPECT_EQ(remoteId(fbIpServer.getSignal()), localChDomSigClient1.getGlobalId());
    EXPECT_EQ(remoteId(fbIpClient2.getSignal()), fbIpServer.getSignal().getGlobalId());
}

TEST_F(ClientToDeviceStreamingTest, SignalAndDomainConnectDisconnect)
{
    SignalPtr localChValSigClient1 = client1RootDevice.getDevices()[0].getChannels()[0].getSignals()[0];
    SignalPtr localChDomSigClient1 = client1RootDevice.getDevices()[0].getChannels()[0].getSignals()[1];

    // connect value local signal of 1-st client to mirrored channel input port
    chIpClient1.connect(localChValSigClient1);
    // connect domain local signal of 1-st client to mirrored fb input port
    fbIpClient1.connect(localChDomSigClient1);

    setExternalSignalRemovedExpectation(1, 0, 1);
    chIpClient1.disconnect();
    EXPECT_TRUE(isExternalMirroredSignalsRemoved());
    // external domain signal connected - one mirrored signals remains
    EXPECT_EQ(serverExtSigFolder.getItems().getCount(), 1u);
    EXPECT_EQ(client1ExtSigFolder.getItems().getCount(), 0u);
    EXPECT_EQ(client2ExtSigFolder.getItems().getCount(), 1u);
    // verify that signal disconnected
    EXPECT_FALSE(chIpClient1.getSignal().assigned());
    EXPECT_FALSE(chIpServer.getSignal().assigned());
    EXPECT_FALSE(chIpClient2.getSignal().assigned());

    setExternalSignalRemovedExpectation(1, 0, 1);
    fbIpClient1.disconnect();
    EXPECT_TRUE(isExternalMirroredSignalsRemoved());
    // none signals connected - none mirrored signals existing
    EXPECT_EQ(serverExtSigFolder.getItems().getCount(), 0u);
    EXPECT_EQ(client1ExtSigFolder.getItems().getCount(), 0u);
    EXPECT_EQ(client2ExtSigFolder.getItems().getCount(), 0u);
    // verify that signal disconnected
    EXPECT_FALSE(fbIpClient1.getSignal().assigned());
    EXPECT_FALSE(fbIpServer.getSignal().assigned());
    EXPECT_FALSE(fbIpClient2.getSignal().assigned());
}

TEST_F(ClientToDeviceStreamingTest, ConnectNewClientWhenStreamingActive)
{
    SignalPtr localChSigClient1 = client1RootDevice.getDevices()[0].getChannels()[0].getSignals()[0];

    // connect local signal of 1-st client to mirrored channel input port
    chIpClient1.connect(localChSigClient1);

    ConfigClientPtr client3;
    ConfigServerPtr newServer = createServer(client3);
    AsyncRpcHelper newHelper(std::ref(newServer));
    DevicePtr client3RootDevice, client3MirroredDevice;
    FolderPtr client3ExtSigFolder;
    createAndConnectClient("client3", client3, newServer, newHelper, client3RootDevice, client3MirroredDevice, client3ExtSigFolder);
    auto chIpClient3 = client3MirroredDevice.getChannels()[0].getInputPorts()[0];

    // one external signal with domain connected - two mirrored signals created
    EXPECT_EQ(client3ExtSigFolder.getItems().getCount(), 2u);
    EXPECT_TRUE(testDomainSignal(client3ExtSigFolder.getItems()[1], client3ExtSigFolder.getItems()[0]));

    // verify connected signals
    EXPECT_TRUE(chIpClient3.getSignal().assigned());
    EXPECT_EQ(chIpClient3.getSignal(), client3ExtSigFolder.getItems()[1]);
    EXPECT_EQ(remoteId(chIpClient3.getSignal()), chIpServer.getSignal().getGlobalId());
}
