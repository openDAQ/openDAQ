#include <gtest/gtest.h>
#include <testutils/memcheck_listener.h>

#include <opendaq/opendaq.h>
#include <opendaq/deserialize_component_ptr.h>
#include <opendaq/component_deserialize_context_factory.h>

#include <native_streaming_protocol/native_streaming_client_handler.h>
#include <native_streaming_protocol/native_streaming_server_handler.h>

#include <memory>
#include <future>

using namespace daq;
using namespace daq::opendaq_native_streaming_protocol;

using ClientCountType = size_t;
using WorkGuardType = boost::asio::executor_work_guard<boost::asio::io_context::executor_type>;

class DummyConfigProtocolInstance
{
public:
    DummyConfigProtocolInstance()
        : sendPacketCb(nullptr)
    {};

    void receivePacket(const config_protocol::PacketBuffer& packetBuffer)
    {
        receivedPackets.push_back(config_protocol::PacketBuffer(packetBuffer.getBuffer(), true));
    };

    void sendPacket(const config_protocol::PacketBuffer& packetBuffer)
    {
        sentPackets.push_back(config_protocol::PacketBuffer(packetBuffer.getBuffer(), true));
        //sendPacketCb(packetBuffer);
    };

    static void testPacketsEquality(const config_protocol::PacketBuffer& packetBuffer1,
                                    const config_protocol::PacketBuffer& packetBuffer2)
    {
        ASSERT_TRUE(packetBuffer1.getPacketType() == packetBuffer2.getPacketType());
        ASSERT_TRUE(packetBuffer1.getId() == packetBuffer2.getId());
        ASSERT_TRUE(packetBuffer1.getPayloadSize() == packetBuffer2.getPayloadSize());
        ASSERT_TRUE(packetBuffer1.getLength() == packetBuffer2.getLength());
        ASSERT_TRUE(std::memcmp(packetBuffer1.getPayload(), packetBuffer2.getPayload(), packetBuffer1.getPayloadSize()) == 0);
        ASSERT_TRUE(std::memcmp(packetBuffer1.getBuffer(), packetBuffer2.getBuffer(), packetBuffer1.getLength()) == 0);
    };

    std::vector<config_protocol::PacketBuffer> receivedPackets;
    std::vector<config_protocol::PacketBuffer> sentPackets;

    ConfigProtocolPacketCb sendPacketCb;
};
#include <opendaq/signal_impl.h>
static SignalPtr deserializeSignal(const ContextPtr& context, const StringPtr& serializedSignal)
{
    const auto deserializer = JsonDeserializer();
    const auto deserializeContext = ComponentDeserializeContext(context, nullptr, nullptr, "sig");

    const SignalPtr signal = deserializer.deserialize(serializedSignal, deserializeContext, nullptr);
    return signal;
}

static StringPtr getDomainSignalId(const SignalPtr& signal)
{
    return signal.asPtr<IDeserializeComponent>(true).getDeserializedParameter("domainSignalId");
}

class ClientAttributes
{
public:
    std::promise< std::tuple<StringPtr, StringPtr> > signalAvailablePromise;
    std::future< std::tuple<StringPtr, StringPtr> > signalAvailableFuture;

    std::promise< std::tuple<StringPtr, StringPtr> > signalWithDomainAvailablePromise;
    std::future< std::tuple<StringPtr, StringPtr> > signalWithDomainAvailableFuture;

    std::promise< StringPtr > signalUnavailablePromise;
    std::future< StringPtr > signalUnavailableFuture;

    std::promise< std::tuple<StringPtr, PacketPtr> > packetReceivedPromise;
    std::future< std::tuple<StringPtr, PacketPtr> > packetReceivedFuture;

    std::promise< StringPtr > subscribedAckPromise;
    std::future< StringPtr > subscribedAckFuture;

    std::promise< StringPtr > unsubscribedAckPromise;
    std::future< StringPtr > unsubscribedAckFuture;

    std::promise< ClientReconnectionStatus > reconnectionStatusPromise;
    std::future< ClientReconnectionStatus > reconnectionStatusFuture;

    std::shared_ptr<NativeStreamingClientHandler> clientHandler;
    ContextPtr clientContext;

    OnSignalAvailableCallback signalAvailableHandler;
    OnSignalAvailableCallback signalWithDomainAvailableHandler;
    OnSignalUnavailableCallback signalUnavailableHandler;
    OnPacketCallback packetHandler;
    OnSignalSubscriptionAckCallback signalSubscriptionAckHandler;
    OnReconnectionStatusChangedCallback reconnectionStatusChangedHandler;

    DummyConfigProtocolInstance configProtocolHandler;
    ConfigProtocolPacketCb configProtocolPacketHandler;

    /// async operations handler
    std::shared_ptr<boost::asio::io_context> ioContextPtrClient;

    void setUp()
    {
        clientContext = NullContext(Logger(nullptr, LogLevel::Trace));

        signalAvailablePromise = std::promise< std::tuple<StringPtr, StringPtr> >();
        signalAvailableFuture = signalAvailablePromise.get_future();

        signalWithDomainAvailablePromise = std::promise< std::tuple<StringPtr, StringPtr> >();
        signalWithDomainAvailableFuture = signalWithDomainAvailablePromise.get_future();

        signalUnavailablePromise = std::promise< StringPtr >();
        signalUnavailableFuture = signalUnavailablePromise.get_future();

        packetReceivedPromise = std::promise< std::tuple<StringPtr, PacketPtr> >();
        packetReceivedFuture = packetReceivedPromise.get_future();

        subscribedAckPromise = std::promise< StringPtr >();
        subscribedAckFuture = subscribedAckPromise.get_future();

        unsubscribedAckPromise = std::promise< StringPtr >();
        unsubscribedAckFuture = unsubscribedAckPromise.get_future();

        reconnectionStatusPromise = std::promise< ClientReconnectionStatus >();
        reconnectionStatusFuture = reconnectionStatusPromise.get_future();

        signalAvailableHandler = [this](const StringPtr& signalStringId,
                                        const StringPtr& serializedSignal)
        {
            signalAvailablePromise.set_value({signalStringId, serializedSignal});
        };

        signalWithDomainAvailableHandler = [this](const StringPtr& signalStringId,
                                                  const StringPtr& serializedSignal)
        {
            signalWithDomainAvailablePromise.set_value({signalStringId, serializedSignal});
        };

        signalUnavailableHandler = [this](const StringPtr& signalStringId)
        {
            signalUnavailablePromise.set_value(signalStringId);
        };

        packetHandler = [this](const StringPtr& signalStringId, const PacketPtr& packet)
        {
            packetReceivedPromise.set_value({signalStringId, packet});
        };

        signalSubscriptionAckHandler = [this](const StringPtr& signalStringId, bool subscribed)
        {
            if (subscribed)
                subscribedAckPromise.set_value(signalStringId);
            else
                unsubscribedAckPromise.set_value(signalStringId);
        };

        reconnectionStatusChangedHandler = [this](ClientReconnectionStatus status)
        {
            reconnectionStatusPromise.set_value(status);
        };

        configProtocolPacketHandler = [this](const config_protocol::PacketBuffer& packetBuffer)
        {
            configProtocolHandler.receivePacket(packetBuffer);
        };

        ioContextPtrClient = std::make_shared<boost::asio::io_context>();
        workGuardClient = std::make_unique<WorkGuardType>(ioContextPtrClient->get_executor());
        execThreadClient = std::thread([this]() { ioContextPtrClient->run(); });
    }

    void tearDown()
    {
        ioContextPtrClient->stop();
        if (execThreadClient.joinable())
        {
            execThreadClient.join();
        }
        workGuardClient.reset();
        ioContextPtrClient.reset();
        clientHandler.reset();
    }

private:
    /// prevents boost::asio::io_context::run() from returning when there is no more async operations pending
    std::unique_ptr<WorkGuardType> workGuardClient;
    /// async operations runner thread
    std::thread execThreadClient;
};

class ProtocolTest : public testing::TestWithParam<std::tuple<ClientCountType, bool>>
{
public:
    const uint16_t NATIVE_STREAMING_SERVER_PORT = 7420;
    const std::string NATIVE_STREAMING_LISTENING_PORT = "7420";
    const std::string SERVER_ADDRESS = "127.0.0.1";
    const std::chrono::milliseconds timeout = std::chrono::milliseconds(500);

    void SetUp() override
    {
        serverContext = NullContext(Logger(nullptr, LogLevel::Trace));

        signalSubscribedPromise = std::promise< SignalPtr >();
        signalSubscribedFuture = signalSubscribedPromise.get_future();
        signalSubscribedHandler =
            [this](const SignalPtr& signal)
        {
            signalSubscribedPromise.set_value(signal);
        };

        signalUnsubscribedPromise = std::promise< SignalPtr >();
        signalUnsubscribedFuture = signalUnsubscribedPromise.get_future();
        signalUnsubscribedHandler =
            [this](const SignalPtr& signal)
        {
            signalUnsubscribedPromise.set_value(signal);
        };

        setUpConfigProtocolServerCb =
            [this](ConfigProtocolPacketCb sendPacketCb)
        {
            auto configProtocolHandler = std::make_shared<DummyConfigProtocolInstance>();
            ConfigProtocolPacketCb receivePacketCb =
                [configProtocolHandler](const config_protocol::PacketBuffer& packetBuffer)
            {
                configProtocolHandler->receivePacket(packetBuffer);
            };
            configProtocolHandlers.push_back(configProtocolHandler);
            return receivePacketCb;
        };

        clientsCount = std::get<0>(GetParam());
        clients = std::vector<ClientAttributes>(clientsCount);
        for (size_t i = 0; i < clients.size(); ++i)
        {
            clients[i].setUp();
        }
    }

    void TearDown() override
    {
        if (std::get<1>(GetParam())) // stop server first
        {
            stopServer();
            for (auto& client : clients)
                client.tearDown();
        }
        else // stop clients first
        {
            for (auto& client : clients)
                client.tearDown();
            stopServer();
        }
    }

    std::shared_ptr<NativeStreamingClientHandler> createClient(const ClientAttributes& client,
                                                               OnSignalAvailableCallback signalAvailableHandler)
    {
        auto clientHandler = std::make_shared<NativeStreamingClientHandler>(client.clientContext);
        clientHandler->setIoContext(client.ioContextPtrClient);
        clientHandler->setSignalAvailableHandler(signalAvailableHandler);
        clientHandler->setSignalUnavailableHandler(client.signalUnavailableHandler);
        clientHandler->setPacketHandler(client.packetHandler);
        clientHandler->setSignalSubscriptionAckCallback(client.signalSubscriptionAckHandler);
        clientHandler->setReconnectionStatusChangedCb(client.reconnectionStatusChangedHandler);
        clientHandler->setConfigPacketHandler(client.configProtocolPacketHandler);
        return clientHandler;
    }

    void startServer(const ListPtr<ISignal>& signalsList)
    {
        ioContextPtrServer = std::make_shared<boost::asio::io_context>();
        workGuardServer = std::make_unique<WorkGuardType>(ioContextPtrServer->get_executor());
        execThreadServer = std::thread([this]() { ioContextPtrServer->run(); });

        serverHandler = std::make_shared<NativeStreamingServerHandler>(serverContext,
                                                                       ioContextPtrServer,
                                                                       signalsList,
                                                                       signalSubscribedHandler,
                                                                       signalUnsubscribedHandler,
                                                                       setUpConfigProtocolServerCb);
        serverHandler->startServer(NATIVE_STREAMING_SERVER_PORT);
    }

    void stopServer()
    {
        if (ioContextPtrServer)
            ioContextPtrServer->stop();
        if (execThreadServer.joinable())
            execThreadServer.join();
        workGuardServer.reset();
        ioContextPtrServer.reset();
        serverHandler.reset();
    }

protected:
    ContextPtr serverContext;

    ClientCountType clientsCount;
    std::vector<ClientAttributes> clients;

    OnSignalSubscribedCallback signalSubscribedHandler;
    OnSignalUnsubscribedCallback signalUnsubscribedHandler;

    std::vector<std::shared_ptr<DummyConfigProtocolInstance>> configProtocolHandlers;
    SetUpConfigProtocolServerCb setUpConfigProtocolServerCb;

    std::promise< SignalPtr > signalSubscribedPromise;
    std::future< SignalPtr > signalSubscribedFuture;

    std::promise< SignalPtr > signalUnsubscribedPromise;
    std::future< SignalPtr > signalUnsubscribedFuture;

    /// async operations handler
    std::shared_ptr<boost::asio::io_context> ioContextPtrServer;
    /// prevents boost::asio::io_context::run() from returning when there is no more async operations pending
    std::unique_ptr<WorkGuardType> workGuardServer;
    /// async operations runner thread
    std::thread execThreadServer;

    std::shared_ptr<NativeStreamingServerHandler> serverHandler;
};

TEST_P(ProtocolTest, CreateServerNoSignals)
{
    serverHandler = std::make_shared<NativeStreamingServerHandler>(serverContext,
                                                                   ioContextPtrServer,
                                                                   List<ISignal>(),
                                                                   signalSubscribedHandler,
                                                                   signalUnsubscribedHandler,
                                                                   setUpConfigProtocolServerCb);
}

TEST_P(ProtocolTest, CreateClient)
{
    for (auto& client : clients)
    {
        client.clientHandler = createClient(client, client.signalAvailableHandler);
    }
}

TEST_P(ProtocolTest, ClientConnectFailed)
{
    for (auto& client : clients)
    {
        client.clientHandler = createClient(client, client.signalAvailableHandler);
        ASSERT_FALSE(client.clientHandler->connect(SERVER_ADDRESS, NATIVE_STREAMING_LISTENING_PORT));
    }
}

TEST_P(ProtocolTest, ConnectDisconnectNoSignals)
{
    startServer(List<ISignal>());

    for (auto& client : clients)
    {
        client.clientHandler = createClient(client, client.signalAvailableHandler);
        ASSERT_TRUE(client.clientHandler->connect(SERVER_ADDRESS, NATIVE_STREAMING_LISTENING_PORT));
    }
}

TEST_P(ProtocolTest, Reconnection)
{
    startServer(List<ISignal>());

    for (auto& client : clients)
    {
        client.clientHandler = createClient(client, client.signalAvailableHandler);
        ASSERT_TRUE(client.clientHandler->connect(SERVER_ADDRESS, NATIVE_STREAMING_LISTENING_PORT));
    }

    stopServer();

    for (auto& client : clients)
    {
        ASSERT_EQ(client.reconnectionStatusFuture.wait_for(std::chrono::seconds(5)), std::future_status::ready);
        ASSERT_EQ(client.reconnectionStatusFuture.get(), ClientReconnectionStatus::Reconnecting);

        client.reconnectionStatusPromise = std::promise< ClientReconnectionStatus >();
        client.reconnectionStatusFuture = client.reconnectionStatusPromise.get_future();
    }

    startServer(List<ISignal>());

    for (auto& client : clients)
    {
        ASSERT_EQ(client.reconnectionStatusFuture.wait_for(std::chrono::seconds(5)), std::future_status::ready);
        ASSERT_EQ(client.reconnectionStatusFuture.get(), ClientReconnectionStatus::Restored);

        client.reconnectionStatusPromise = std::promise< ClientReconnectionStatus >();
        client.reconnectionStatusFuture = client.reconnectionStatusPromise.get_future();
    }
}

TEST_P(ProtocolTest, ConnectDisconnectWithSignalDomainUnassigned)
{
    auto serverSignal = SignalWithDescriptor(serverContext, DataDescriptorBuilder().setSampleType(SampleType::Undefined).build(), nullptr, "signal");
    serverSignal.setName("signalName");
    serverSignal.setDescription("signalDescription");

    startServer(List<ISignal>(serverSignal));

    for (auto& client : clients)
    {
        client.clientHandler = createClient(client, client.signalWithDomainAvailableHandler);
        ASSERT_TRUE(client.clientHandler->connect(SERVER_ADDRESS, NATIVE_STREAMING_LISTENING_PORT));

        ASSERT_EQ(client.signalWithDomainAvailableFuture.wait_for(timeout), std::future_status::ready);
        auto [clientSignalStringId, serializedSignal] = client.signalWithDomainAvailableFuture.get();
        SignalPtr clientSignal = deserializeSignal(client.clientContext, serializedSignal);
        ASSERT_EQ(clientSignalStringId, serverSignal.getGlobalId());
        ASSERT_TRUE(!getDomainSignalId(clientSignal).assigned());
        ASSERT_EQ(clientSignal.getDescriptor(), serverSignal.getDescriptor());
        ASSERT_EQ(clientSignal.getName(), serverSignal.getName());
        ASSERT_EQ(clientSignal.getDescription(), serverSignal.getDescription());
    }
}

TEST_P(ProtocolTest, ConnectDisconnectWithSignalDomainAssigned)
{
    auto serverDomainSignal =
        SignalWithDescriptor(serverContext, DataDescriptorBuilder().setSampleType(SampleType::Undefined).build(), nullptr, "domainSignal");
    auto serverSignal =
        SignalWithDescriptor(serverContext, DataDescriptorBuilder().setSampleType(SampleType::Undefined).build(), nullptr, "signal");
    serverSignal.setDomainSignal(serverDomainSignal);
    serverSignal.setName("signalName");
    serverSignal.setDescription("signalDescription");

    startServer(List<ISignal>(serverSignal));

    for (auto& client : clients)
    {
        client.clientHandler = createClient(client, client.signalWithDomainAvailableHandler);
        ASSERT_TRUE(client.clientHandler->connect(SERVER_ADDRESS, NATIVE_STREAMING_LISTENING_PORT));

        ASSERT_EQ(client.signalWithDomainAvailableFuture.wait_for(timeout), std::future_status::ready);
        auto [clientSignalStringId, serializedSignal] = client.signalWithDomainAvailableFuture.get();
        SignalPtr clientSignal = deserializeSignal(client.clientContext, serializedSignal);
        ASSERT_EQ(clientSignalStringId, serverSignal.getGlobalId());
        ASSERT_EQ(clientSignal.getDescriptor(), serverSignal.getDescriptor());
        ASSERT_EQ(getDomainSignalId(clientSignal), serverDomainSignal.getGlobalId());
        ASSERT_EQ(clientSignal.getName(), serverSignal.getName());
        ASSERT_EQ(clientSignal.getDescription(), serverSignal.getDescription());
    }
}

TEST_P(ProtocolTest, AddSignal)
{
    startServer(List<ISignal>());

    for (auto& client : clients)
    {
        client.clientHandler = createClient(client, client.signalAvailableHandler);
        ASSERT_TRUE(client.clientHandler->connect(SERVER_ADDRESS, NATIVE_STREAMING_LISTENING_PORT));
    }

    auto serverSignal =
        SignalWithDescriptor(serverContext, DataDescriptorBuilder().setSampleType(SampleType::Undefined).build(), nullptr, "signal");
    serverSignal.setName("signalName");
    serverSignal.setDescription("signalDescription");
    serverHandler->addSignal(serverSignal);

    for (auto& client : clients)
    {
        ASSERT_EQ(client.signalAvailableFuture.wait_for(timeout), std::future_status::ready);
        auto [clientSignalStringId, serializedSignal] = client.signalAvailableFuture.get();
        SignalPtr clientSignal = deserializeSignal(client.clientContext, serializedSignal);
        ASSERT_EQ(clientSignalStringId, serverSignal.getGlobalId());
        ASSERT_EQ(clientSignal.getDescriptor(), serverSignal.getDescriptor());
        ASSERT_EQ(clientSignal.getName(), serverSignal.getName());
        ASSERT_EQ(clientSignal.getDescription(), serverSignal.getDescription());
    }
}

TEST_P(ProtocolTest, RemoveSignal)
{
    auto serverSignal =
        SignalWithDescriptor(serverContext, DataDescriptorBuilder().setSampleType(SampleType::Undefined).build(), nullptr, "signal");

    startServer(List<ISignal>(serverSignal));

    for (auto& client : clients)
    {
        client.clientHandler = createClient(client, client.signalAvailableHandler);
        ASSERT_TRUE(client.clientHandler->connect(SERVER_ADDRESS, NATIVE_STREAMING_LISTENING_PORT));

        ASSERT_EQ(client.signalAvailableFuture.wait_for(timeout), std::future_status::ready);
        auto [clientSignalStringId, serializedSignal] =
            client.signalAvailableFuture.get();
        ASSERT_EQ(clientSignalStringId, serverSignal.getGlobalId());
    }

    serverHandler->removeSignal(serverSignal);

    for (auto& client : clients)
    {
        ASSERT_EQ(client.signalUnavailableFuture.wait_for(timeout), std::future_status::ready);
        auto clientSignalStringId = client.signalUnavailableFuture.get();
        ASSERT_EQ(clientSignalStringId, serverSignal.getGlobalId());
    }
}

TEST_P(ProtocolTest, SignalSubscribeUnsubscribe)
{
    StringPtr clientSignalStringId, serializedSignal;
    DataDescriptorPtr clientSignalDescriptor;

    auto serverSignal =
        SignalWithDescriptor(serverContext, DataDescriptorBuilder().setSampleType(SampleType::Undefined).build(), nullptr, "signal");
    startServer(List<ISignal>(serverSignal));

    for (auto& client : clients)
    {
        client.clientHandler = createClient(client, client.signalAvailableHandler);
        ASSERT_TRUE(client.clientHandler->connect(SERVER_ADDRESS, NATIVE_STREAMING_LISTENING_PORT));

        ASSERT_EQ(client.signalAvailableFuture.wait_for(timeout), std::future_status::ready);
        std::tie(clientSignalStringId, serializedSignal) = client.signalAvailableFuture.get();
        client.clientHandler->subscribeSignal(clientSignalStringId);
        ASSERT_EQ(client.subscribedAckFuture.wait_for(timeout), std::future_status::ready);
        ASSERT_EQ(client.subscribedAckFuture.get(), clientSignalStringId);
    }

    ASSERT_EQ(signalSubscribedFuture.wait_for(timeout), std::future_status::ready);
    ASSERT_EQ(signalSubscribedFuture.get(), serverSignal);

    for (auto& client : clients)
    {
        client.clientHandler->unsubscribeSignal(clientSignalStringId);
        ASSERT_EQ(client.unsubscribedAckFuture.wait_for(timeout), std::future_status::ready);
        ASSERT_EQ(client.unsubscribedAckFuture.get(), clientSignalStringId);
    }
    ASSERT_EQ(signalUnsubscribedFuture.wait_for(timeout), std::future_status::ready);
    ASSERT_EQ(signalUnsubscribedFuture.get(), serverSignal);
}

TEST_P(ProtocolTest, InitialEventPacket)
{
    const auto valueDescriptor = DataDescriptorBuilder().setSampleType(SampleType::Float32).build();
    auto initialEventPacket = DataDescriptorChangedEventPacket(valueDescriptor, nullptr);
    auto serverSignal = SignalWithDescriptor(serverContext, valueDescriptor, nullptr, "signal");

    startServer(List<ISignal>(serverSignal));

    for (auto& client : clients)
    {
        client.clientHandler = createClient(client, client.signalAvailableHandler);
        ASSERT_TRUE(client.clientHandler->connect(SERVER_ADDRESS, NATIVE_STREAMING_LISTENING_PORT));

        ASSERT_EQ(client.packetReceivedFuture.wait_for(timeout), std::future_status::ready);
        auto [signalId, packet] = client.packetReceivedFuture.get();
        ASSERT_EQ(signalId, serverSignal.getGlobalId());
        ASSERT_EQ(packet, initialEventPacket);
    }
}

TEST_P(ProtocolTest, SendEventPacket)
{
    const auto valueDescriptor = DataDescriptorBuilder().setSampleType(SampleType::Float32).build();
    auto initialEventPacket = DataDescriptorChangedEventPacket(valueDescriptor, nullptr);
    auto serverSignal = SignalWithDescriptor(serverContext, valueDescriptor, nullptr, "signal");

    startServer(List<ISignal>(serverSignal));

    for (auto& client : clients)
    {
        client.clientHandler = createClient(client, client.signalAvailableHandler);
        ASSERT_TRUE(client.clientHandler->connect(SERVER_ADDRESS, NATIVE_STREAMING_LISTENING_PORT));

        ASSERT_EQ(client.signalAvailableFuture.wait_for(timeout), std::future_status::ready);
        auto [clientSignalStringId, serializedSignal] =
            client.signalAvailableFuture.get();

        // wait for initial event packet
        ASSERT_EQ(client.packetReceivedFuture.wait_for(timeout), std::future_status::ready);
        // reset packet future / promise
        client.packetReceivedPromise = std::promise< std::tuple<StringPtr, PacketPtr> >();
        client.packetReceivedFuture = client.packetReceivedPromise.get_future();

        client.clientHandler->subscribeSignal(clientSignalStringId);
        ASSERT_EQ(client.subscribedAckFuture.wait_for(timeout), std::future_status::ready);
    }

    ASSERT_EQ(signalSubscribedFuture.wait_for(timeout), std::future_status::ready);

    const auto newValueDescriptor = DataDescriptorBuilder().setSampleType(SampleType::Binary).build();
    auto serverEventPacket = DataDescriptorChangedEventPacket(newValueDescriptor, nullptr);
    serverHandler->sendPacket(serverSignal, serverEventPacket);

    for (auto& client : clients)
    {
        ASSERT_EQ(client.packetReceivedFuture.wait_for(timeout), std::future_status::ready);
        auto [signalId, packet] = client.packetReceivedFuture.get();
        ASSERT_EQ(signalId, serverSignal.getGlobalId());
        ASSERT_EQ(packet, serverEventPacket);
    }
}

TEST_P(ProtocolTest, SendPacketsNoSubscribers)
{
    const auto valueDescriptor = DataDescriptorBuilder().setSampleType(SampleType::Float32).build();
    auto serverSignal = SignalWithDescriptor(serverContext, valueDescriptor, nullptr, "signal");

    startServer(List<ISignal>(serverSignal));

    for (auto& client : clients)
    {
        client.clientHandler = createClient(client, client.signalAvailableHandler);
        ASSERT_TRUE(client.clientHandler->connect(SERVER_ADDRESS, NATIVE_STREAMING_LISTENING_PORT));

        // wait for initial event packet
        ASSERT_EQ(client.packetReceivedFuture.wait_for(timeout), std::future_status::ready);

        // reset packet future / promise
        client.packetReceivedPromise = std::promise< std::tuple<StringPtr, PacketPtr> >();
        client.packetReceivedFuture = client.packetReceivedPromise.get_future();
    }

    auto serverEventPacket = DataDescriptorChangedEventPacket(valueDescriptor, nullptr);
    auto serverDataPacket = DataPacket(valueDescriptor, 100);

    serverHandler->sendPacket(serverSignal, serverEventPacket);
    serverHandler->sendPacket(serverSignal, serverDataPacket);

    // no subscribers - so packets wont be sent to clients
    for (auto& client : clients)
    {
        ASSERT_EQ(client.packetReceivedFuture.wait_for(timeout), std::future_status::timeout);
    }
}

TEST_P(ProtocolTest, SendDataPacket)
{
    const auto valueDescriptor = DataDescriptorBuilder().setSampleType(SampleType::Float32).build();
    auto serverDataPacket = DataPacket(valueDescriptor, 100);
    auto serverSignal = SignalWithDescriptor(serverContext, valueDescriptor, nullptr, "signal");

    startServer(List<ISignal>(serverSignal));

    for (auto& client : clients)
    {
        client.clientHandler = createClient(client, client.signalAvailableHandler);
        ASSERT_TRUE(client.clientHandler->connect(SERVER_ADDRESS, NATIVE_STREAMING_LISTENING_PORT));

        ASSERT_EQ(client.signalAvailableFuture.wait_for(timeout), std::future_status::ready);
        auto [clientSignalStringId, serializedSignal] =
            client.signalAvailableFuture.get();

        // wait for initial event packet
        ASSERT_EQ(client.packetReceivedFuture.wait_for(timeout), std::future_status::ready);
        // reset packet future / promise
        client.packetReceivedPromise = std::promise< std::tuple<StringPtr, PacketPtr> >();
        client.packetReceivedFuture = client.packetReceivedPromise.get_future();

        client.clientHandler->subscribeSignal(clientSignalStringId);
        ASSERT_EQ(client.subscribedAckFuture.wait_for(timeout), std::future_status::ready);
    }

    ASSERT_EQ(signalSubscribedFuture.wait_for(timeout), std::future_status::ready);

    serverHandler->sendPacket(serverSignal, serverDataPacket);

    for (auto& client : clients)
    {
        ASSERT_EQ(client.packetReceivedFuture.wait_for(timeout), std::future_status::ready);
        auto [signalId, packet] = client.packetReceivedFuture.get();
        ASSERT_EQ(signalId, serverSignal.getGlobalId());
        ASSERT_EQ(packet, serverDataPacket);
    }
}

TEST_P(ProtocolTest, DISABLED_ConfigProtocolPackets)
{
    startServer(List<ISignal>());

    for (auto& client : clients)
    {
        client.clientHandler = createClient(client, client.signalAvailableHandler);
        ASSERT_TRUE(client.clientHandler->connect(SERVER_ADDRESS, NATIVE_STREAMING_LISTENING_PORT));
    }

    ASSERT_EQ(clients.size(), configProtocolHandlers.size());

    for (auto& client : clients)
    {
        auto packet = config_protocol::PacketBuffer::createGetProtocolInfoRequest(1);
        client.clientHandler->sendConfigRequest(packet);
        client.configProtocolHandler.sendPacket(packet);
    }

    std::this_thread::sleep_for(std::chrono::seconds(3));

    for (size_t i = 0; i < clients.size(); ++i)
    {
        auto packetsReceived = configProtocolHandlers[i]->receivedPackets.size();
        ASSERT_EQ(clients[i].configProtocolHandler.sentPackets.size(), packetsReceived);
        for (size_t j = 0; j < packetsReceived; ++j)
            DummyConfigProtocolInstance::testPacketsEquality(
                configProtocolHandlers[i]->receivedPackets[j],
                clients[i].configProtocolHandler.sentPackets[j]
            );
    }
}

INSTANTIATE_TEST_SUITE_P(
    ProtocolTestGroup,
    ProtocolTest,
    testing::Combine(
        testing::Range<ClientCountType>(1, 4),
        testing::Values(true, false)
    )
);
