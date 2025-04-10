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
    static constexpr size_t PacketToGenerate = 9;
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
            case 7:
                return PacketBuffer::createNoReplyRpcRequest(json.c_str(), json.length());
            case 8:
                return PacketBuffer::createConnectionRejectedReply(packetIndex, json.c_str(), json.length());
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
    OnConnectionStatusChangedCallback connectionStatusChangedHandler;

    void setUp()
    {
        ClientAttributesBase::setUp();

        clientHandler = std::make_shared<NativeStreamingClientHandler>(
            clientContext, ClientAttributesBase::createTransportLayerConfig(), ClientAttributesBase::createAuthenticationConfig());

        configProtocolHandler = std::make_shared<TestConfigProtocolInstance>(
            [this](const PacketBuffer& packetBuffer)
            {
                clientHandler->sendConfigRequest(packetBuffer);
            }
        );

        configProtocolPacketHandler = [this](PacketBuffer&& packetBuffer)
        {
            configProtocolHandler->receivePacket(std::move(packetBuffer));
        };

        connectionStatusChangedHandler = [this](const EnumerationPtr& status, const StringPtr& statusMessage)
        {
            connectionStatusPromise.set_value(status);
        };

        clientHandler->setConfigHandlers(configProtocolPacketHandler,
                                         connectionStatusChangedHandler);
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
            [this](SendConfigProtocolPacketCb sendPacketCb, const UserPtr& user, ClientType connectionType)
        {
            this->configProtocolHandler = std::make_shared<TestConfigProtocolInstance>(sendPacketCb);
            ProcessConfigProtocolPacketCb receivePacketCb =
                [this](PacketBuffer&& packetBuffer)
            {
                this->configProtocolHandler->receivePacket(std::move(packetBuffer));
            };
            configProtocolTriggeredPromise.set_value();
            return std::make_pair(receivePacketCb, nullptr);
        };

        client.setUp();
        configProtocolTriggeredPromise = std::promise< void > ();
        configProtocolTriggeredFuture = configProtocolTriggeredPromise.get_future();
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

    void startServer()
    {
        startIoOperations();

        serverHandler = std::make_shared<NativeStreamingServerHandler>(
            serverContext,
            ioContextPtrServer,
            List<ISignal>(),
            [](const SignalPtr&){},
            [](const SignalPtr&){},
            setUpConfigProtocolServerCb,
            [](const std::string&, const std::string&, bool, ClientType, const std::string&){},
            [](const std::string&){}
        );
        serverHandler->startServer(NATIVE_STREAMING_SERVER_PORT);
    }

    void stopServer()
    {
        stopIoOperations();
        serverHandler.reset();
    }

    void triggerConfigProtocolSetUp()
    {
        client.clientHandler->sendConfigRequest(PacketBuffer::createGetProtocolInfoRequest(0));
    }

protected:
    std::shared_ptr<TestConfigProtocolInstance> configProtocolHandler;
    std::promise< void > configProtocolTriggeredPromise;
    std::future< void > configProtocolTriggeredFuture;
    SetUpConfigProtocolServerCb setUpConfigProtocolServerCb;

    ConfigProtocolAttributes client;
};

TEST_P(ConfigPacketsTest, Reconnection)
{
    startServer();

    ASSERT_TRUE(client.clientHandler->connect(SERVER_ADDRESS, NATIVE_STREAMING_LISTENING_PORT));
    triggerConfigProtocolSetUp();
    ASSERT_EQ(configProtocolTriggeredFuture.wait_for(timeout), std::future_status::ready);

    stopServer();

    ASSERT_EQ(client.connectionStatusFuture.wait_for(std::chrono::seconds(5)), std::future_status::ready);
    ASSERT_EQ(client.connectionStatusFuture.get(), "Reconnecting");

    client.connectionStatusPromise = std::promise< EnumerationPtr >();
    client.connectionStatusFuture = client.connectionStatusPromise.get_future();

    configProtocolTriggeredPromise = std::promise< void > ();
    configProtocolTriggeredFuture = configProtocolTriggeredPromise.get_future();

    startServer();

    ASSERT_EQ(client.connectionStatusFuture.wait_for(std::chrono::seconds(5)), std::future_status::ready);
    ASSERT_EQ(client.connectionStatusFuture.get(), "Connected");
    triggerConfigProtocolSetUp();
    ASSERT_EQ(configProtocolTriggeredFuture.wait_for(timeout), std::future_status::ready);

    client.connectionStatusPromise = std::promise< EnumerationPtr >();
    client.connectionStatusFuture = client.connectionStatusPromise.get_future();
}

TEST_P(ConfigPacketsTest, ServerToClientPackets)
{
    startServer();

    ASSERT_TRUE(client.clientHandler->connect(SERVER_ADDRESS, NATIVE_STREAMING_LISTENING_PORT));
    triggerConfigProtocolSetUp();
    ASSERT_EQ(configProtocolTriggeredFuture.wait_for(timeout), std::future_status::ready);

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

    ASSERT_TRUE(client.clientHandler->connect(SERVER_ADDRESS, NATIVE_STREAMING_LISTENING_PORT));

    for (size_t packetCnt = 0; packetCnt < ConfigProtocolPacketGenerator::PacketToGenerate; ++packetCnt)
    {
        auto packet = client.configProtocolHandler->packetGenerator.generatePacket();
        client.configProtocolHandler->sendPacket(packet);
    }

    ASSERT_EQ(configProtocolTriggeredFuture.wait_for(timeout), std::future_status::ready);
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

TEST_P(ConfigPacketsTest, ConfigClientConnectDisconnectCallbacks)
{
    std::string clientId;
    bool clientConnected{false};
    auto clientConnectedHandler =
        [&clientId, &clientConnected](const std::string& id,
                                      const std::string& address,
                                      bool isStreamingConnection,
                                      ClientType /*clientType*/,
                                      const std::string& hostName)
    {
        ASSERT_FALSE(isStreamingConnection);
        ASSERT_NE(address, "");
        ASSERT_NE(hostName, "");
        clientConnected = true;
        clientId = id;
    };
    std::promise<bool> clientDisconnectedPromise;
    std::future<bool> clientDisconnectedFuture = clientDisconnectedPromise.get_future();
    auto clientDisconnectedHandler =
        [&clientId, &clientConnected, &clientDisconnectedPromise](const std::string& id)
    {
        if (clientConnected && id == clientId)
            clientDisconnectedPromise.set_value(true);
    };

    startIoOperations();
    serverHandler = std::make_shared<NativeStreamingServerHandler>(
        serverContext,
        ioContextPtrServer,
        List<ISignal>(),
        [](const SignalPtr&){},
        [](const SignalPtr&){},
        setUpConfigProtocolServerCb,
        clientConnectedHandler,
        clientDisconnectedHandler
    );
    serverHandler->startServer(NATIVE_STREAMING_SERVER_PORT);

    ASSERT_TRUE(client.clientHandler->connect(SERVER_ADDRESS, NATIVE_STREAMING_LISTENING_PORT));
    triggerConfigProtocolSetUp();
    ASSERT_EQ(configProtocolTriggeredFuture.wait_for(timeout), std::future_status::ready);

    ASSERT_TRUE(clientConnected);
    ASSERT_NE(clientId, "");

    client.clientHandler.reset(); // disconnect
    ASSERT_EQ(clientDisconnectedFuture.wait_for(std::chrono::milliseconds(1000)), std::future_status::ready);
    ASSERT_TRUE(clientDisconnectedFuture.get());
}

INSTANTIATE_TEST_SUITE_P(
    ProtocolTestGroup,
    ConfigPacketsTest,
    testing::Values(std::make_tuple(1, true), std::make_tuple(1, false))
);
