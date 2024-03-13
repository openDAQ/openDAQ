#include "test_base.h"
#include <testutils/memcheck_listener.h>

#include <native_streaming_protocol/native_streaming_client_handler.h>
#include <native_streaming_protocol/native_streaming_server_handler.h>

using namespace daq;
using namespace daq::opendaq_native_streaming_protocol;
using namespace daq::config_protocol;

class ConfigProtocolPacketGenerator
{
public:
    static constexpr size_t PacketToGenerate = 7;
    ConfigProtocolPacketGenerator()
        : packetIndex(0)
        {};

    PacketBuffer generatePacket()
    {
        switch (packetIndex++)
        {
            case 0:
                return PacketBuffer::createGetProtocolInfoRequest(packetIndex);
            case 1:
                return PacketBuffer::createGetProtocolInfoReply(packetIndex, 2, {0, 1, 2, 3});
            case 2:
                return PacketBuffer::createUpgradeProtocolRequest(packetIndex, 0);
            case 3:
                return PacketBuffer::createUpgradeProtocolReply(packetIndex, false);
            case 4:
                return PacketBuffer::createRpcRequestOrReply(packetIndex, json.c_str(), json.length());
            case 5:
                return PacketBuffer::createServerNotification(json.c_str(), json.length());
            case 6:
                return PacketBuffer::createInvalidRequestReply(packetIndex);
            default:
                return PacketBuffer();
        }
    }

private:
    size_t packetIndex;

    const std::string json{"str"};
};

class TestConfigProtocolInstance
{
public:
    TestConfigProtocolInstance(SendConfigProtocolPacketCb sendPacketCb)
        : promise(std::promise< void >())
        , future(promise.get_future())
        , sendPacketCb(sendPacketCb)
    {};

    void receivePacket(PacketBuffer&& packetBuffer)
    {
        receivedPackets.push_back(std::move(packetBuffer));
        if (receivedPackets.size() == ConfigProtocolPacketGenerator::PacketToGenerate)
            promise.set_value();
    };

    void sendPacket(const PacketBuffer& packetBuffer)
    {
        sentPackets.push_back(PacketBuffer(packetBuffer.getBuffer(), true));
        sendPacketCb(packetBuffer);
    };

    static void testPacketsEquality(const PacketBuffer& packetBuffer1,
                                    const PacketBuffer& packetBuffer2)
    {
        ASSERT_TRUE(packetBuffer1.getPacketType() == packetBuffer2.getPacketType());
        ASSERT_TRUE(packetBuffer1.getId() == packetBuffer2.getId());
        ASSERT_TRUE(packetBuffer1.getPayloadSize() == packetBuffer2.getPayloadSize());
        ASSERT_TRUE(packetBuffer1.getLength() == packetBuffer2.getLength());
        ASSERT_TRUE(std::memcmp(packetBuffer1.getPayload(), packetBuffer2.getPayload(), packetBuffer1.getPayloadSize()) == 0);
        ASSERT_TRUE(std::memcmp(packetBuffer1.getBuffer(), packetBuffer2.getBuffer(), packetBuffer1.getLength()) == 0);
    };

    std::vector<PacketBuffer> receivedPackets;
    std::vector<PacketBuffer> sentPackets;

    ConfigProtocolPacketGenerator packetGenerator;

    std::promise< void > promise;
    std::future< void > future;
    SendConfigProtocolPacketCb sendPacketCb;
};

class ConfigProtocolAttributes : public ClientAttributesBase
{
public:
    std::shared_ptr<TestConfigProtocolInstance> configProtocolHandler;
    ProcessConfigProtocolPacketCb configProtocolPacketHandler;

    void setUp()
    {
        ClientAttributesBase::setUp();
    }

    void tearDown()
    {
        ClientAttributesBase::tearDown();
    }
};

class ConfigPacketsTest : public ProtocolTestBase
{
public:
    void SetUp() override
    {
        ProtocolTestBase::SetUp();
        setUpConfigProtocolServerCb =
            [this](SendConfigProtocolPacketCb sendPacketCb)
        {
            this->configProtocolHandler = std::make_shared<TestConfigProtocolInstance>(sendPacketCb);
            ProcessConfigProtocolPacketCb receivePacketCb =
                [this](PacketBuffer&& packetBuffer)
            {
                this->configProtocolHandler->receivePacket(std::move(packetBuffer));
            };
            clientConnectedPromise.set_value();
            return receivePacketCb;
        };

        client.setUp();
        clientConnectedPromise = std::promise< void > ();
        clientConnectedFuture = clientConnectedPromise.get_future();
    }

    void TearDown() override
    {
        if (std::get<1>(GetParam())) // stop server first
        {
            stopServer();
            client.tearDown();
        }
        else // stop clients first
        {
            client.tearDown();
            stopServer();
        }
        ProtocolTestBase::TearDown();
    }

    std::shared_ptr<NativeStreamingClientHandler> createClient(ConfigProtocolAttributes& client)
    {
        auto clientHandler = std::make_shared<NativeStreamingClientHandler>(
            client.clientContext, ClientAttributesBase::createTransportLayerConfig());

        clientHandler->setIoContext(client.ioContextPtrClient);
        clientHandler->setSignalAvailableHandler([](const StringPtr&, const StringPtr&){});
        clientHandler->setSignalUnavailableHandler([](const StringPtr&){});
        clientHandler->setPacketHandler([](const StringPtr&, const PacketPtr&){});
        clientHandler->setSignalSubscriptionAckCallback([](const StringPtr&, bool){});
        clientHandler->setReconnectionStatusChangedCb([](ClientReconnectionStatus){});
        clientHandler->setStreamingInitDoneCb([](){});

        client.configProtocolHandler = std::make_shared<TestConfigProtocolInstance>(
            [clientHandler](const PacketBuffer& packetBuffer)
            {
                clientHandler->sendConfigRequest(packetBuffer);
            }
        );
        client.configProtocolPacketHandler = [&client](PacketBuffer&& packetBuffer)
        {
            (client.configProtocolHandler)->receivePacket(std::move(packetBuffer));
        };
        clientHandler->setConfigPacketHandler(client.configProtocolPacketHandler);

        return clientHandler;
    }

    void startServer()
    {
        startIoOperations();

        serverHandler = std::make_shared<NativeStreamingServerHandler>(
            serverContext,
            ioContextPtrServer,
            List<ISignal>(),
            [](const SignalPtr&){},
            [](const SignalPtr&){},
            setUpConfigProtocolServerCb
        );
        serverHandler->startServer(NATIVE_STREAMING_SERVER_PORT);
    }

    void stopServer()
    {
        stopIoOperations();
        serverHandler.reset();
    }

protected:
    std::shared_ptr<TestConfigProtocolInstance> configProtocolHandler;
    std::promise< void > clientConnectedPromise;
    std::future< void > clientConnectedFuture;
    SetUpConfigProtocolServerCb setUpConfigProtocolServerCb;

    ConfigProtocolAttributes client;
};

TEST_P(ConfigPacketsTest, ServerToClientPackets)
{
    startServer();

    client.clientHandler = createClient(client);
    ASSERT_TRUE(client.clientHandler->connect(SERVER_ADDRESS, NATIVE_STREAMING_LISTENING_PORT));
    ASSERT_EQ(clientConnectedFuture.wait_for(timeout), std::future_status::ready);

    for (size_t packetCnt = 0; packetCnt < ConfigProtocolPacketGenerator::PacketToGenerate; ++packetCnt)
    {
        auto packet = configProtocolHandler->packetGenerator.generatePacket();
        configProtocolHandler->sendPacket(packet);
    }

    ASSERT_EQ(client.configProtocolHandler->future.wait_for(timeout), std::future_status::ready);

    auto packetsReceived = client.configProtocolHandler->receivedPackets.size();
    ASSERT_EQ(configProtocolHandler->sentPackets.size(), packetsReceived);

    for (size_t j = 0; j < packetsReceived; ++j)
    {
        TestConfigProtocolInstance::testPacketsEquality(
            configProtocolHandler->sentPackets[j],
            client.configProtocolHandler->receivedPackets[j]
        );
    }
}

TEST_P(ConfigPacketsTest, ClientToServerPackets)
{
    startServer();

    client.clientHandler = createClient(client);
    ASSERT_TRUE(client.clientHandler->connect(SERVER_ADDRESS, NATIVE_STREAMING_LISTENING_PORT));
    ASSERT_EQ(clientConnectedFuture.wait_for(timeout), std::future_status::ready);

    for (size_t packetCnt = 0; packetCnt < ConfigProtocolPacketGenerator::PacketToGenerate; ++packetCnt)
    {
        auto packet = client.configProtocolHandler->packetGenerator.generatePacket();
        client.configProtocolHandler->sendPacket(packet);
    }

    ASSERT_EQ(configProtocolHandler->future.wait_for(timeout), std::future_status::ready);

    auto packetsReceived = configProtocolHandler->receivedPackets.size();
    ASSERT_EQ(client.configProtocolHandler->sentPackets.size(), packetsReceived);

    for (size_t j = 0; j < packetsReceived; ++j)
    {
        TestConfigProtocolInstance::testPacketsEquality(
            configProtocolHandler->receivedPackets[j],
            client.configProtocolHandler->sentPackets[j]
        );
    }
}

INSTANTIATE_TEST_SUITE_P(
    ProtocolTestGroup,
    ConfigPacketsTest,
    testing::Values(std::make_tuple(1, true), std::make_tuple(1, false))
);
