#include <websocket_streaming/streaming_client.h>
#include <websocket_streaming/streaming_server.h>
#include <opendaq/context_factory.h>

#include <gtest/gtest.h>
#include <chrono>
#include "streaming_test_helpers.h"


using namespace daq;
using namespace daq::websocket_streaming;

class StreamingTest : public testing::Test
{
public:
    const uint16_t StreamingPort = daq::streaming_protocol::WEBSOCKET_LISTENING_PORT;
    const std::string StreamingTarget = "/";
    const uint16_t ControlPort = daq::streaming_protocol::HTTP_CONTROL_PORT;
    SignalPtr testDoubleSignal;
    SignalPtr testConstantSignal;
    SignalPtr testDomainSignal;
    ContextPtr context;

    Int delta;
    Int packetOffset;

    void SetUp() override
    {
        context = NullContext();
        testDomainSignal = streaming_test_helpers::createLinearTimeSignal(context);
        testDoubleSignal = streaming_test_helpers::createExplicitValueSignal(context, "DoubleSignal", testDomainSignal);
        testConstantSignal = streaming_test_helpers::createConstantValueSignal(context, "ConstantSignal", testDomainSignal);

        delta = testDomainSignal.getDescriptor().getRule().getParameters().get("delta");
        packetOffset = testDomainSignal.getDescriptor().getRule().getParameters().get("start");
    }

    void TearDown() override
    {
    }

    DataPacketPtr getNextDomainPacket(size_t sampleCount)
    {
        auto packet = DataPacket(testDomainSignal.getDescriptor(), sampleCount, packetOffset);
        packetOffset += sampleCount * delta;
        return packet;
    }
};

TEST_F(StreamingTest, ConnectAndDisconnect)
{
    auto server = std::make_shared<StreamingServer>(context);
    server->start(StreamingPort, ControlPort);

    auto client = StreamingClient(context, "127.0.0.1", StreamingPort, StreamingTarget);

    ASSERT_FALSE(client.isConnected());
    client.connect();

    ASSERT_TRUE(client.isConnected());

    client.disconnect();
    ASSERT_FALSE(client.isConnected());
}

TEST_F(StreamingTest, StopServer)
{
    auto server = std::make_shared<StreamingServer>(context);
    server->start(StreamingPort, ControlPort);

    auto client = StreamingClient(context, "127.0.0.1", StreamingPort, StreamingTarget);

    client.connect();
    ASSERT_TRUE(client.isConnected());

    server->stop();
    server.reset();
}

TEST_F(StreamingTest, ConnectTimeout)
{
    auto server = std::make_shared<StreamingServer>(context);
    server->start(StreamingPort, ControlPort);

    auto client = StreamingClient(context, "127.0.0.1", 7000, StreamingTarget);

    client.connect();
    ASSERT_FALSE(client.isConnected());
}

TEST_F(StreamingTest, ConnectTwice)
{
    auto server = std::make_shared<StreamingServer>(context);
    server->start(StreamingPort, ControlPort);

    auto client = StreamingClient(context, "127.0.0.1", StreamingPort, StreamingTarget);

    ASSERT_TRUE(client.connect());
    client.disconnect();

    ASSERT_TRUE(client.connect());
    ASSERT_TRUE(client.isConnected());
    client.disconnect();

    ASSERT_FALSE(client.isConnected());
}

TEST_F(StreamingTest, ParseConnectString)
{
    auto client = std::make_shared<StreamingClient>(NullContext(), "daq.lt://127.0.0.1");
    ASSERT_EQ(client->getPort(), daq::streaming_protocol::WEBSOCKET_LISTENING_PORT);
    ASSERT_EQ(client->getHost(), "127.0.0.1");
    ASSERT_EQ(client->getTarget(), "/");

    client = std::make_shared<StreamingClient>(NullContext(), "daq.lt://localhost/path/other");
    ASSERT_EQ(client->getPort(), daq::streaming_protocol::WEBSOCKET_LISTENING_PORT);
    ASSERT_EQ(client->getHost(), "localhost");
    ASSERT_EQ(client->getTarget(), "/path/other");

    client = std::make_shared<StreamingClient>(NullContext(), "daq.lt://localhost:3000/path/other");
    ASSERT_EQ(client->getPort(), 3000u);
    ASSERT_EQ(client->getHost(), "localhost");
    ASSERT_EQ(client->getTarget(), "/path/other");

    client = std::make_shared<StreamingClient>(NullContext(), "daq.ws://127.0.0.1");
    ASSERT_EQ(client->getPort(), daq::streaming_protocol::WEBSOCKET_LISTENING_PORT);
    ASSERT_EQ(client->getHost(), "127.0.0.1");
    ASSERT_EQ(client->getTarget(), "/");
}

TEST_F(StreamingTest, Subscription)
{
    auto server = std::make_shared<StreamingServer>(context);
    server->onAccept([this](const daq::streaming_protocol::StreamWriterPtr& writer) {
        auto signals = List<ISignal>();
        signals.pushBack(testDoubleSignal);
        signals.pushBack(testDoubleSignal.getDomainSignal());
        return signals;
    });
    server->start(StreamingPort, ControlPort);

    auto client = StreamingClient(context, "127.0.0.1", StreamingPort, StreamingTarget);

    std::promise<std::string> subscribeAckPromise;
    std::future<std::string> subscribeAckFuture = subscribeAckPromise.get_future();

    std::promise<std::string> unsubscribeAckPromise;
    std::future<std::string> unsubscribeAckFuture = unsubscribeAckPromise.get_future();

    auto onSubscriptionAck =
        [&subscribeAckPromise, &unsubscribeAckPromise](const std::string& signalId, bool subscribed)
    {
        if (subscribed)
            subscribeAckPromise.set_value(signalId);
        else
            unsubscribeAckPromise.set_value(signalId);
    };

    client.onSubscriptionAck(onSubscriptionAck);
    client.connect();
    ASSERT_TRUE(client.isConnected());

    client.subscribeSignal(testDoubleSignal.getGlobalId());
    ASSERT_EQ(subscribeAckFuture.wait_for(std::chrono::milliseconds(500)), std::future_status::ready);
    ASSERT_EQ(subscribeAckFuture.get(), testDoubleSignal.getGlobalId());

    client.unsubscribeSignal(testDoubleSignal.getGlobalId());
    ASSERT_EQ(unsubscribeAckFuture.wait_for(std::chrono::milliseconds(500)), std::future_status::ready);
    ASSERT_EQ(unsubscribeAckFuture.get(), testDoubleSignal.getGlobalId());
}

// sends explicit value packet after constant value packet
TEST_F(StreamingTest, PacketsCorrectSequence)
{
    std::vector<double> data = {-1.5, -1.0, -0.5, 0, 0.5, 1.0, 1.5};
    auto sampleCount = data.size();

    auto server = std::make_shared<StreamingServer>(context);
    server->onAccept([this](const daq::streaming_protocol::StreamWriterPtr& writer) {
        auto signals = List<ISignal>();
        signals.pushBack(testDomainSignal);
        signals.pushBack(testDoubleSignal);
        signals.pushBack(testConstantSignal);
        return signals;
    });
    server->start(StreamingPort, ControlPort);

    std::vector<PacketPtr> receivedPackets;
    auto client = StreamingClient(context, "127.0.0.1", StreamingPort, StreamingTarget);

    std::map<std::string, std::promise<void>> subscribeAckPromises;
    subscribeAckPromises.emplace(testConstantSignal.getGlobalId(), std::promise<void>());
    subscribeAckPromises.emplace(testDoubleSignal.getGlobalId(), std::promise<void>());

    std::map<std::string, std::future<void>> subscribeAckFutures;
    subscribeAckFutures.emplace(testConstantSignal.getGlobalId(), subscribeAckPromises.at(testConstantSignal.getGlobalId()).get_future());
    subscribeAckFutures.emplace(testDoubleSignal.getGlobalId(), subscribeAckPromises.at(testDoubleSignal.getGlobalId()).get_future());

    auto onSubscriptionAck =
        [&subscribeAckPromises](const std::string& signalId, bool subscribed)
    {
        ASSERT_EQ(subscribeAckPromises.count(signalId), 1u);
        if (subscribed)
            subscribeAckPromises.at(signalId).set_value();
    };

    auto onPacket = [&receivedPackets](const StringPtr& signalId, const PacketPtr& packet)
    {
        receivedPackets.push_back(packet);
    };

    client.onPacket(onPacket);
    client.onSubscriptionAck(onSubscriptionAck);
    client.connect();
    ASSERT_TRUE(client.isConnected());

    client.subscribeSignal(testConstantSignal.getGlobalId());
    client.subscribeSignal(testDoubleSignal.getGlobalId());

    ASSERT_EQ(subscribeAckFutures.at(testConstantSignal.getGlobalId()).wait_for(std::chrono::seconds(5)), std::future_status::ready);
    ASSERT_EQ(subscribeAckFutures.at(testDoubleSignal.getGlobalId()).wait_for(std::chrono::seconds(5)), std::future_status::ready);

    auto domainPacket1 = getNextDomainPacket(sampleCount);
    auto explicitValuePacket1 = DataPacketWithDomain(domainPacket1, testDoubleSignal.getDescriptor(), sampleCount);
    std::memcpy(explicitValuePacket1.getRawData(), data.data(), explicitValuePacket1.getRawDataSize());
    auto constantValuePacket1 = ConstantDataPacketWithDomain<uint64_t>(domainPacket1,
                                                                       testConstantSignal.getDescriptor(),
                                                                       sampleCount,
                                                                       1,
                                                                       {{2, 2}, {4, 4}, {6, 5}});

    server->broadcastPacket(testConstantSignal.getGlobalId(), constantValuePacket1);
    server->broadcastPacket(testDoubleSignal.getGlobalId(), explicitValuePacket1);

    auto domainPacket2 = getNextDomainPacket(sampleCount);
    auto explicitValuePacket2 = DataPacketWithDomain(domainPacket2, testDoubleSignal.getDescriptor(), sampleCount);
    std::memcpy(explicitValuePacket2.getRawData(), data.data(), explicitValuePacket2.getRawDataSize());

    server->broadcastPacket(testDoubleSignal.getGlobalId(), explicitValuePacket2);

    auto domainPacket3 = getNextDomainPacket(sampleCount);
    auto explicitValuePacket3 = DataPacketWithDomain(domainPacket3, testDoubleSignal.getDescriptor(), sampleCount);
    std::memcpy(explicitValuePacket3.getRawData(), data.data(), explicitValuePacket3.getRawDataSize());
    auto constantValuePacket3 = ConstantDataPacketWithDomain<uint64_t>(domainPacket3,
                                                                       testConstantSignal.getDescriptor(),
                                                                       sampleCount,
                                                                       1);

    server->broadcastPacket(testConstantSignal.getGlobalId(), constantValuePacket3);
    server->broadcastPacket(testDoubleSignal.getGlobalId(), explicitValuePacket3);

    std::this_thread::sleep_for(std::chrono::milliseconds(250));

    // 2 event packets + data packets: 3 domain and 6 value packets
    ASSERT_EQ(receivedPackets.size(), 11u);

    ASSERT_TRUE(BaseObjectPtr::Equals(domainPacket1, receivedPackets[2]));
    ASSERT_TRUE(BaseObjectPtr::Equals(explicitValuePacket1, receivedPackets[3]));
    ASSERT_TRUE(BaseObjectPtr::Equals(constantValuePacket1, receivedPackets[4]));

    // packet automatically generated by client
    auto constantValuePacket2 = ConstantDataPacketWithDomain<uint64_t>(domainPacket2,
                                                                       testConstantSignal.getDescriptor(),
                                                                       sampleCount,
                                                                       5);

    ASSERT_TRUE(BaseObjectPtr::Equals(domainPacket2, receivedPackets[5]));
    ASSERT_TRUE(BaseObjectPtr::Equals(explicitValuePacket2, receivedPackets[6]));
    ASSERT_TRUE(BaseObjectPtr::Equals(constantValuePacket2, receivedPackets[7]));

    ASSERT_TRUE(BaseObjectPtr::Equals(domainPacket3, receivedPackets[8]));
    ASSERT_TRUE(BaseObjectPtr::Equals(explicitValuePacket3, receivedPackets[9]));
    ASSERT_TRUE(BaseObjectPtr::Equals(constantValuePacket3, receivedPackets[10]));
}

// sends explicit value packet before constant value packet
// this results in client side constant packets that track the last changes of the constant rule,
// but with a delay equivalent to the size of one packet, and potentially missed intermediate changes.
TEST_F(StreamingTest, PacketsIncorrectSequence)
{
    std::vector<double> data = {-1.5, -1.0, -0.5, 0, 0.5, 1.0, 1.5};
    auto sampleCount = data.size();

    auto server = std::make_shared<StreamingServer>(context);
    server->onAccept([this](const daq::streaming_protocol::StreamWriterPtr& writer) {
        auto signals = List<ISignal>();
        signals.pushBack(testDomainSignal);
        signals.pushBack(testDoubleSignal);
        signals.pushBack(testConstantSignal);
        return signals;
    });
    server->start(StreamingPort, ControlPort);

    std::vector<PacketPtr> receivedPackets;
    auto client = StreamingClient(context, "127.0.0.1", StreamingPort, StreamingTarget);

    std::map<std::string, std::promise<void>> subscribeAckPromises;
    subscribeAckPromises.emplace(testConstantSignal.getGlobalId(), std::promise<void>());
    subscribeAckPromises.emplace(testDoubleSignal.getGlobalId(), std::promise<void>());

    std::map<std::string, std::future<void>> subscribeAckFutures;
    subscribeAckFutures.emplace(testConstantSignal.getGlobalId(), subscribeAckPromises.at(testConstantSignal.getGlobalId()).get_future());
    subscribeAckFutures.emplace(testDoubleSignal.getGlobalId(), subscribeAckPromises.at(testDoubleSignal.getGlobalId()).get_future());

    auto onSubscriptionAck =
        [&subscribeAckPromises](const std::string& signalId, bool subscribed)
    {
        ASSERT_EQ(subscribeAckPromises.count(signalId), 1u);
        if (subscribed)
            subscribeAckPromises.at(signalId).set_value();
    };

    auto onPacket = [&receivedPackets](const StringPtr& signalId, const PacketPtr& packet)
    {
        receivedPackets.push_back(packet);
    };

    client.onPacket(onPacket);
    client.onSubscriptionAck(onSubscriptionAck);
    client.connect();
    ASSERT_TRUE(client.isConnected());

    client.subscribeSignal(testConstantSignal.getGlobalId());
    client.subscribeSignal(testDoubleSignal.getGlobalId());

    ASSERT_EQ(subscribeAckFutures.at(testConstantSignal.getGlobalId()).wait_for(std::chrono::seconds(5)), std::future_status::ready);
    ASSERT_EQ(subscribeAckFutures.at(testDoubleSignal.getGlobalId()).wait_for(std::chrono::seconds(5)), std::future_status::ready);

    auto domainPacket1 = getNextDomainPacket(sampleCount);
    auto explicitValuePacket1 = DataPacketWithDomain(domainPacket1, testDoubleSignal.getDescriptor(), sampleCount);
    std::memcpy(explicitValuePacket1.getRawData(), data.data(), explicitValuePacket1.getRawDataSize());
    auto constantValuePacket1 = ConstantDataPacketWithDomain<uint64_t>(domainPacket1,
                                                                       testConstantSignal.getDescriptor(),
                                                                       sampleCount,
                                                                       1,
                                                                       {{2, 2}, {4, 4}, {6, 5}});

    server->broadcastPacket(testDoubleSignal.getGlobalId(), explicitValuePacket1);
    server->broadcastPacket(testConstantSignal.getGlobalId(), constantValuePacket1);

    auto domainPacket2 = getNextDomainPacket(sampleCount);
    auto explicitValuePacket2 = DataPacketWithDomain(domainPacket2, testDoubleSignal.getDescriptor(), sampleCount);
    std::memcpy(explicitValuePacket2.getRawData(), data.data(), explicitValuePacket2.getRawDataSize());

    server->broadcastPacket(testDoubleSignal.getGlobalId(), explicitValuePacket2);

    auto domainPacket3 = getNextDomainPacket(sampleCount);
    auto explicitValuePacket3 = DataPacketWithDomain(domainPacket3, testDoubleSignal.getDescriptor(), sampleCount);
    std::memcpy(explicitValuePacket3.getRawData(), data.data(), explicitValuePacket3.getRawDataSize());
    auto constantValuePacket3 = ConstantDataPacketWithDomain<uint64_t>(domainPacket3,
                                                                       testConstantSignal.getDescriptor(),
                                                                       sampleCount,
                                                                       1);

    server->broadcastPacket(testDoubleSignal.getGlobalId(), explicitValuePacket3);
    server->broadcastPacket(testConstantSignal.getGlobalId(), constantValuePacket3);

    std::this_thread::sleep_for(std::chrono::milliseconds(250));

    // packets automatically generated by client
    auto constantValuePacket2 = ConstantDataPacketWithDomain<uint64_t>(domainPacket2,
                                                                       testConstantSignal.getDescriptor(),
                                                                       sampleCount,
                                                                       5);

    // packets automatically generated by client
    auto clientConstValuePacket3 = ConstantDataPacketWithDomain<uint64_t>(domainPacket3,
                                                                          testConstantSignal.getDescriptor(),
                                                                          sampleCount,
                                                                          5);

    // 2 event packets + data packets: 3 domain and 6 value packets
    ASSERT_EQ(receivedPackets.size(), 10u);

    ASSERT_TRUE(BaseObjectPtr::Equals(domainPacket1, receivedPackets[2]));
    ASSERT_TRUE(BaseObjectPtr::Equals(explicitValuePacket1, receivedPackets[3]));
    // no constant packet generated since the signal value is unknown

    ASSERT_TRUE(BaseObjectPtr::Equals(domainPacket2, receivedPackets[4]));
    ASSERT_TRUE(BaseObjectPtr::Equals(explicitValuePacket2, receivedPackets[5]));
    // contains only last change of constant
    ASSERT_TRUE(BaseObjectPtr::Equals(constantValuePacket2, receivedPackets[6]));

    ASSERT_TRUE(BaseObjectPtr::Equals(domainPacket3, receivedPackets[7]));
    ASSERT_TRUE(BaseObjectPtr::Equals(explicitValuePacket3, receivedPackets[8]));
    // value update is not yet applied, provides old value
    ASSERT_TRUE(BaseObjectPtr::Equals(clientConstValuePacket3, receivedPackets[9]));
}
