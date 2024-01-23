#include <gtest/gtest.h>
#include <testutils/memcheck_listener.h>

#include <opendaq/opendaq.h>

#include <native_streaming_protocol/native_streaming_client_handler.h>
#include <native_streaming_protocol/native_streaming_server_handler.h>

#include <memory>
#include <future>

using namespace daq;
using namespace daq::opendaq_native_streaming_protocol;

using ClientCountType = size_t;
using WorkGuardType = boost::asio::executor_work_guard<boost::asio::io_context::executor_type>;

class ClientAttributes
{
public:
    std::promise< std::tuple<StringPtr, DataDescriptorPtr, StringPtr, StringPtr> > signalAvailablePromise;
    std::future< std::tuple<StringPtr, DataDescriptorPtr, StringPtr, StringPtr> > signalAvailableFuture;

    std::promise< std::tuple<StringPtr, StringPtr, DataDescriptorPtr, StringPtr, StringPtr> > signalWithDomainAvailablePromise;
    std::future< std::tuple<StringPtr, StringPtr, DataDescriptorPtr, StringPtr, StringPtr> > signalWithDomainAvailableFuture;

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

    /// async operations handler
    std::shared_ptr<boost::asio::io_context> ioContextPtrClient;

    void setUp()
    {
        clientContext = NullContext(Logger(nullptr, LogLevel::Trace));

        signalAvailablePromise = std::promise< std::tuple<StringPtr, DataDescriptorPtr, StringPtr, StringPtr> >();
        signalAvailableFuture = signalAvailablePromise.get_future();

        signalWithDomainAvailablePromise = std::promise< std::tuple<StringPtr, StringPtr, DataDescriptorPtr, StringPtr, StringPtr> >();
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
                                        const StringPtr& domainSignalStringId,
                                        const DataDescriptorPtr& signalDescriptor,
                                        const StringPtr& name,
                                        const StringPtr& description)
        {
            signalAvailablePromise.set_value({signalStringId, signalDescriptor, name, description});
        };

        signalWithDomainAvailableHandler = [this](const StringPtr& signalStringId,
                                                  const StringPtr& domainSignalStringId,
                                                  const DataDescriptorPtr& signalDescriptor,
                                                  const StringPtr& name,
                                                  const StringPtr& description)
        {
            signalWithDomainAvailablePromise.set_value({signalStringId, domainSignalStringId, signalDescriptor, name, description});
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
        return std::make_shared<NativeStreamingClientHandler>(client.clientContext,
                                                              client.ioContextPtrClient,
                                                              signalAvailableHandler,
                                                              client.signalUnavailableHandler,
                                                              client.packetHandler,
                                                              client.signalSubscriptionAckHandler,
                                                              client.reconnectionStatusChangedHandler);
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
                                                                       signalUnsubscribedHandler);
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
                                                                   signalUnsubscribedHandler);
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
        auto [clientSignalStringId, clientDomainSignalStringId, clientSignalDescriptor, clientSignalName, clientSignalDescription] =
            client.signalWithDomainAvailableFuture.get();
        ASSERT_EQ(clientSignalStringId, serverSignal.getGlobalId());
        ASSERT_TRUE(!clientDomainSignalStringId.assigned());
        ASSERT_EQ(clientSignalDescriptor, serverSignal.getDescriptor());
        ASSERT_EQ(clientSignalName, serverSignal.getName());
        ASSERT_EQ(clientSignalDescription, serverSignal.getDescription());
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
        auto [clientSignalStringId, clientDomainSignalStringId, clientSignalDescriptor, clientSignalName, clientSignalDescription] =
            client.signalWithDomainAvailableFuture.get();
        ASSERT_EQ(clientSignalStringId, serverSignal.getGlobalId());
        ASSERT_EQ(clientSignalDescriptor, serverSignal.getDescriptor());
        ASSERT_EQ(clientDomainSignalStringId, serverDomainSignal.getGlobalId());
        ASSERT_EQ(clientSignalName, serverSignal.getName());
        ASSERT_EQ(clientSignalDescription, serverSignal.getDescription());
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
        auto [clientSignalStringId, clientSignalDescriptor, clientSignalName, clientSignalDescription] =
            client.signalAvailableFuture.get();
        ASSERT_EQ(clientSignalStringId, serverSignal.getGlobalId());
        ASSERT_EQ(clientSignalDescriptor, serverSignal.getDescriptor());
        ASSERT_EQ(clientSignalName, serverSignal.getName());
        ASSERT_EQ(clientSignalDescription, serverSignal.getDescription());
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
        auto [clientSignalStringId, clientSignalDescriptor, clientSignalName, clientSignalDescription] =
            client.signalAvailableFuture.get();
        ASSERT_EQ(clientSignalStringId, serverSignal.getGlobalId());
        ASSERT_EQ(clientSignalDescriptor, serverSignal.getDescriptor());
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
    StringPtr clientSignalStringId, clientSignalName, clientSignalDescription;
    DataDescriptorPtr clientSignalDescriptor;

    auto serverSignal =
        SignalWithDescriptor(serverContext, DataDescriptorBuilder().setSampleType(SampleType::Undefined).build(), nullptr, "signal");
    startServer(List<ISignal>(serverSignal));

    for (auto& client : clients)
    {
        client.clientHandler = createClient(client, client.signalAvailableHandler);
        ASSERT_TRUE(client.clientHandler->connect(SERVER_ADDRESS, NATIVE_STREAMING_LISTENING_PORT));

        ASSERT_EQ(client.signalAvailableFuture.wait_for(timeout), std::future_status::ready);
        std::tie(clientSignalStringId, clientSignalDescriptor, clientSignalName, clientSignalDescription) =
            client.signalAvailableFuture.get();
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
        auto [clientSignalStringId, clientSignalDescriptor, clientSignalName, clientSignalDescription] =
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
        auto [clientSignalStringId, clientSignalDescriptor, clientSignalName, clientSignalDescription] =
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

INSTANTIATE_TEST_SUITE_P(
    ProtocolTestGroup,
    ProtocolTest,
    testing::Combine(
        testing::Range<ClientCountType>(1, 5),
        testing::Values(true, false)
    )
);
