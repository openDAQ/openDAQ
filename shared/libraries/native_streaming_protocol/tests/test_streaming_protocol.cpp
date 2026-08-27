#include "test_base.h"
#include <testutils/memcheck_listener.h>

#include <opendaq/opendaq.h>
#include <opendaq/deserialize_component_ptr.h>
#include <opendaq/component_deserialize_context_factory.h>

#include <memory>
#include <future>
#include <thread>
#include <chrono>

using namespace daq;
using namespace daq::opendaq_native_streaming_protocol;

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

class StreamingProtocolAttributes : public ClientAttributesBase
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

    std::promise< void > streamingInitPromise;
    std::future< void > streamingInitFuture;

    OnSignalAvailableCallback signalAvailableHandler;
    OnSignalAvailableCallback signalWithDomainAvailableHandler;
    OnSignalUnavailableCallback signalUnavailableHandler;
    OnPacketCallback packetHandler;
    OnSignalSubscriptionAckCallback signalSubscriptionAckHandler;
    OnConnectionStatusChangedCallback connectionStatusChangedHandler;
    OnStreamingInitDoneCallback streamingInitDoneHandler;

    void setUp()
    {
        ClientAttributesBase::setUp();

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

        streamingInitPromise = std::promise< void >();
        streamingInitFuture = streamingInitPromise.get_future();

        streamingInitDoneHandler = [this]()
        {
            streamingInitPromise.set_value();
        };

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

        connectionStatusChangedHandler = [this](const EnumerationPtr& status, const StringPtr& statusMessage)
        {
            connectionStatusPromise.set_value(status);
        };
    }

    void tearDown()
    {
        ClientAttributesBase::tearDown();
    }
};

class StreamingProtocolTest : public ProtocolTestBase
{
public:
    void SetUp() override
    {
        ProtocolTestBase::SetUp();

        signalSubscribedPromise = std::promise< SignalPtr >();
        signalSubscribedFuture = signalSubscribedPromise.get_future();
        signalSubscribedHandler =
            [this](const SignalPtr& signal)
        {
            // called only when signal first time subscribed by any client
            if (initialEventPacket.assigned())
                serverHandler->sendPacket(signal.getGlobalId().toStdString(), initialEventPacket);
            signalSubscribedPromise.set_value(signal);
        };

        signalUnsubscribedPromise = std::promise< SignalPtr >();
        signalUnsubscribedFuture = signalUnsubscribedPromise.get_future();
        signalUnsubscribedHandler =
            [this](const SignalPtr& signal)
        {
            signalUnsubscribedPromise.set_value(signal);
        };

        setUpConfigProtocolServerCb = [](SendConfigProtocolPacketCb sendPacketCb, const UserPtr& user, ClientType connectionType)
        {
            return std::make_pair(nullptr, nullptr);
        };

        clientsCount = std::get<0>(GetParam());
        clients = std::vector<StreamingProtocolAttributes>(clientsCount);
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
        ProtocolTestBase::TearDown();
    }

    std::shared_ptr<NativeStreamingClientHandler> createClient(StreamingProtocolAttributes& client,
                                                               OnSignalAvailableCallback signalAvailableHandler)
    {
        auto clientHandler = std::make_shared<NativeStreamingClientHandler>(
            client.clientContext, ClientAttributesBase::createTransportLayerConfig(), ClientAttributesBase::createAuthenticationConfig());

        clientHandler->setStreamingHandlers(signalAvailableHandler,
                                            client.signalUnavailableHandler,
                                            client.packetHandler,
                                            client.signalSubscriptionAckHandler,
                                            client.connectionStatusChangedHandler,
                                            client.streamingInitDoneHandler);

        return clientHandler;
    }

    void startServer(const ListPtr<ISignal>& signalsList, const EventPacketPtr& eventPacket = nullptr)
    {
        initialEventPacket = eventPacket;
        startIoOperations();

        auto config = NativeStreamingServerHandler::createDefaultConfig();
        // maxAllowedConfigConnections = 1 is used here to verify that the limit does not impact streaming connections
        config.setPropertyValue("MaxAllowedConfigConnections", 1);

        serverHandler = std::make_shared<NativeStreamingServerHandler>(
            serverContext,
            ioContextPtrServer,
            signalsList,
            signalSubscribedHandler,
            signalUnsubscribedHandler,
            setUpConfigProtocolServerCb,
            [](const std::string&, const std::string&, bool, ClientType, const std::string&){},
            [](const std::string&){},
            config
        );
        serverHandler->startServer(NATIVE_STREAMING_SERVER_PORT);
    }

    void stopServer()
    {
        stopIoOperations();
        serverHandler.reset();
    }

protected:
    std::vector<StreamingProtocolAttributes> clients;

    EventPacketPtr initialEventPacket;
    OnSignalSubscribedCallback signalSubscribedHandler;
    OnSignalUnsubscribedCallback signalUnsubscribedHandler;

    std::promise< SignalPtr > signalSubscribedPromise;
    std::future< SignalPtr > signalSubscribedFuture;

    std::promise< SignalPtr > signalUnsubscribedPromise;
    std::future< SignalPtr > signalUnsubscribedFuture;

    SetUpConfigProtocolServerCb setUpConfigProtocolServerCb;
};

TEST_P(StreamingProtocolTest, CreateServerNoSignals)
{
    serverHandler = std::make_shared<NativeStreamingServerHandler>(
        serverContext,
        ioContextPtrServer,
        List<ISignal>(),
        signalSubscribedHandler,
        signalUnsubscribedHandler,
        setUpConfigProtocolServerCb,
        [](const std::string&, const std::string&, bool, ClientType, const std::string&){},
        [](const std::string&){}
    );
}

TEST_P(StreamingProtocolTest, CreateClient)
{
    for (auto& client : clients)
    {
        client.clientHandler = createClient(client, client.signalAvailableHandler);
    }
}

TEST_P(StreamingProtocolTest, ClientConnectFailed)
{
    for (auto& client : clients)
    {
        client.clientHandler = createClient(client, client.signalAvailableHandler);
        ASSERT_FALSE(client.clientHandler->connect(SERVER_ADDRESS, NATIVE_STREAMING_LISTENING_PORT));
    }
}

TEST_P(StreamingProtocolTest, ConnectDisconnectNoSignals)
{
    startServer(List<ISignal>());

    for (auto& client : clients)
    {
        client.clientHandler = createClient(client, client.signalAvailableHandler);
        ASSERT_TRUE(client.clientHandler->connect(SERVER_ADDRESS, NATIVE_STREAMING_LISTENING_PORT));
        client.clientHandler->sendStreamingRequest();
        ASSERT_EQ(client.streamingInitFuture.wait_for(timeout), std::future_status::ready);
    }
}

TEST_P(StreamingProtocolTest, StreamingClientConnectDisconnectCallbacks)
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
        ASSERT_TRUE(isStreamingConnection);
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
        signalSubscribedHandler,
        signalUnsubscribedHandler,
        setUpConfigProtocolServerCb,
        clientConnectedHandler,
        clientDisconnectedHandler
    );
    serverHandler->startServer(NATIVE_STREAMING_SERVER_PORT);

    auto& client = clients[0];
    client.clientHandler = createClient(client, client.signalAvailableHandler);
    ASSERT_TRUE(client.clientHandler->connect(SERVER_ADDRESS, NATIVE_STREAMING_LISTENING_PORT));
    client.clientHandler->sendStreamingRequest();
    ASSERT_EQ(client.streamingInitFuture.wait_for(timeout), std::future_status::ready);

    ASSERT_TRUE(clientConnected);
    ASSERT_NE(clientId, "");

    client.clientHandler.reset(); // disconnect
    ASSERT_EQ(clientDisconnectedFuture.wait_for(std::chrono::milliseconds(1000)), std::future_status::ready);
    ASSERT_TRUE(clientDisconnectedFuture.get());
}

TEST_P(StreamingProtocolTest, Reconnection)
{
    startServer(List<ISignal>());

    for (auto& client : clients)
    {
        client.clientHandler = createClient(client, client.signalAvailableHandler);
        ASSERT_TRUE(client.clientHandler->connect(SERVER_ADDRESS, NATIVE_STREAMING_LISTENING_PORT));

        client.clientHandler->sendStreamingRequest();
        ASSERT_EQ(client.streamingInitFuture.wait_for(timeout), std::future_status::ready);

        client.streamingInitPromise = std::promise< void >();
        client.streamingInitFuture = client.streamingInitPromise.get_future();
    }

    stopServer();

    for (auto& client : clients)
    {
        ASSERT_EQ(client.connectionStatusFuture.wait_for(std::chrono::seconds(5)), std::future_status::ready);
        ASSERT_EQ(client.connectionStatusFuture.get(), "Reconnecting");

        client.connectionStatusPromise = std::promise< EnumerationPtr >();
        client.connectionStatusFuture = client.connectionStatusPromise.get_future();
    }

    startServer(List<ISignal>());

    for (auto& client : clients)
    {
        ASSERT_EQ(client.connectionStatusFuture.wait_for(std::chrono::seconds(5)), std::future_status::ready);
        ASSERT_EQ(client.connectionStatusFuture.get(), "Connected");

        client.connectionStatusPromise = std::promise< EnumerationPtr >();
        client.connectionStatusFuture = client.connectionStatusPromise.get_future();

        client.clientHandler->sendStreamingRequest();
        ASSERT_EQ(client.streamingInitFuture.wait_for(timeout), std::future_status::ready);
    }
}

TEST_P(StreamingProtocolTest, ConnectDisconnectWithSignalDomainUnassigned)
{
    auto serverSignal = SignalWithDescriptor(serverContext, DataDescriptorBuilder().setSampleType(SampleType::Undefined).build(), nullptr, "signal");
    serverSignal.setName("signalName");
    serverSignal.setDescription("signalDescription");

    startServer(List<ISignal>(serverSignal));

    for (auto& client : clients)
    {
        client.clientHandler = createClient(client, client.signalWithDomainAvailableHandler);
        ASSERT_TRUE(client.clientHandler->connect(SERVER_ADDRESS, NATIVE_STREAMING_LISTENING_PORT));
        client.clientHandler->sendStreamingRequest();
        ASSERT_EQ(client.streamingInitFuture.wait_for(timeout), std::future_status::ready);

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

TEST_P(StreamingProtocolTest, ConnectDisconnectWithSignalDomainAssigned)
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
        client.clientHandler->sendStreamingRequest();
        ASSERT_EQ(client.streamingInitFuture.wait_for(timeout), std::future_status::ready);

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

TEST_P(StreamingProtocolTest, AddSignal)
{
    startServer(List<ISignal>());

    for (auto& client : clients)
    {
        client.clientHandler = createClient(client, client.signalAvailableHandler);
        ASSERT_TRUE(client.clientHandler->connect(SERVER_ADDRESS, NATIVE_STREAMING_LISTENING_PORT));
        client.clientHandler->sendStreamingRequest();
        ASSERT_EQ(client.streamingInitFuture.wait_for(timeout), std::future_status::ready);
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

TEST_P(StreamingProtocolTest, RemoveSignal)
{
    auto serverSignal =
        SignalWithDescriptor(serverContext, DataDescriptorBuilder().setSampleType(SampleType::Undefined).build(), nullptr, "signal");

    startServer(List<ISignal>(serverSignal));

    for (auto& client : clients)
    {
        client.clientHandler = createClient(client, client.signalAvailableHandler);
        ASSERT_TRUE(client.clientHandler->connect(SERVER_ADDRESS, NATIVE_STREAMING_LISTENING_PORT));
        client.clientHandler->sendStreamingRequest();
        ASSERT_EQ(client.streamingInitFuture.wait_for(timeout), std::future_status::ready);

        ASSERT_EQ(client.signalAvailableFuture.wait_for(timeout), std::future_status::ready);
        auto [clientSignalStringId, serializedSignal] =
            client.signalAvailableFuture.get();
        ASSERT_EQ(clientSignalStringId, serverSignal.getGlobalId());
    }

    serverHandler->removeComponentSignals(serverSignal.getGlobalId());

    for (auto& client : clients)
    {
        ASSERT_EQ(client.signalUnavailableFuture.wait_for(timeout), std::future_status::ready);
        auto clientSignalStringId = client.signalUnavailableFuture.get();
        ASSERT_EQ(clientSignalStringId, serverSignal.getGlobalId());
    }
}

TEST_P(StreamingProtocolTest, RemoveSignalOfParent)
{
    const auto folder = Folder(serverContext, nullptr, "folder");
    auto serverSignal =
        SignalWithDescriptor(serverContext, DataDescriptorBuilder().setSampleType(SampleType::Undefined).build(), folder, "signal");

    startServer(List<ISignal>(serverSignal));

    for (auto& client : clients)
    {
        client.clientHandler = createClient(client, client.signalAvailableHandler);
        ASSERT_TRUE(client.clientHandler->connect(SERVER_ADDRESS, NATIVE_STREAMING_LISTENING_PORT));
        client.clientHandler->sendStreamingRequest();
        ASSERT_EQ(client.streamingInitFuture.wait_for(timeout), std::future_status::ready);

        ASSERT_EQ(client.signalAvailableFuture.wait_for(timeout), std::future_status::ready);
        auto [clientSignalStringId, serializedSignal] =
            client.signalAvailableFuture.get();
        ASSERT_EQ(clientSignalStringId, serverSignal.getGlobalId());
    }

    serverHandler->removeComponentSignals(folder.getGlobalId());

    for (auto& client : clients)
    {
        ASSERT_EQ(client.signalUnavailableFuture.wait_for(timeout), std::future_status::ready);
        auto clientSignalStringId = client.signalUnavailableFuture.get();
        ASSERT_EQ(clientSignalStringId, serverSignal.getGlobalId());
    }
}

TEST_P(StreamingProtocolTest, SignalSubscribeUnsubscribe)
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
        client.clientHandler->sendStreamingRequest();
        ASSERT_EQ(client.streamingInitFuture.wait_for(timeout), std::future_status::ready);

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

TEST_P(StreamingProtocolTest, RemoveSubscribedSignal)
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
        client.clientHandler->sendStreamingRequest();
        ASSERT_EQ(client.streamingInitFuture.wait_for(timeout), std::future_status::ready);

        ASSERT_EQ(client.signalAvailableFuture.wait_for(timeout), std::future_status::ready);
        std::tie(clientSignalStringId, serializedSignal) = client.signalAvailableFuture.get();
        client.clientHandler->subscribeSignal(clientSignalStringId);
    }

    ASSERT_EQ(signalSubscribedFuture.wait_for(timeout), std::future_status::ready);
    ASSERT_EQ(signalSubscribedFuture.get(), serverSignal);

    serverHandler->removeComponentSignals(serverSignal.getGlobalId());

    ASSERT_EQ(signalUnsubscribedFuture.wait_for(timeout), std::future_status::ready);
    ASSERT_EQ(signalUnsubscribedFuture.get(), serverSignal);
}

TEST_P(StreamingProtocolTest, InitialEventPacketOnSubscribe)
{
    const auto valueDescriptor = DataDescriptorBuilder().setSampleType(SampleType::Float32).build();
    auto firstEventPacket = DataDescriptorChangedEventPacket(valueDescriptor, NullDataDescriptor());
    auto serverSignal = SignalWithDescriptor(serverContext, valueDescriptor, nullptr, "signal");

    StringPtr clientSignalStringId;

    // send event packet on first subscribe request
    startServer(List<ISignal>(serverSignal), firstEventPacket);

    // connect all clients
    for (auto& client : clients)
    {
        client.clientHandler = createClient(client, client.signalAvailableHandler);
        ASSERT_TRUE(client.clientHandler->connect(SERVER_ADDRESS, NATIVE_STREAMING_LISTENING_PORT));
        client.clientHandler->sendStreamingRequest();
        ASSERT_EQ(client.streamingInitFuture.wait_for(timeout), std::future_status::ready);

        ASSERT_EQ(client.signalAvailableFuture.wait_for(timeout), std::future_status::ready);
        clientSignalStringId = std::get<0>(client.signalAvailableFuture.get());
    }

    // subscribe all clients to signal
    for (auto& client : clients)
    {
        client.clientHandler->subscribeSignal(clientSignalStringId);
    }

    ASSERT_EQ(signalSubscribedFuture.wait_for(timeout), std::future_status::ready);

    // wait for ack
    for (auto& client : clients)
    {
        ASSERT_EQ(client.subscribedAckFuture.wait_for(timeout), std::future_status::ready);
    }

    // check that all clients received event packet
    for (auto& client : clients)
    {
        ASSERT_EQ(client.packetReceivedFuture.wait_for(timeout), std::future_status::ready);
        auto [signalId, packet] = client.packetReceivedFuture.get();
        ASSERT_EQ(signalId, serverSignal.getGlobalId());
        ASSERT_EQ(packet, firstEventPacket);
    }
}

TEST_P(StreamingProtocolTest, InitialEventPacketPostSubscribe)
{
    const auto valueDescriptor = DataDescriptorBuilder().setSampleType(SampleType::Float32).build();
    auto firstEventPacket = DataDescriptorChangedEventPacket(valueDescriptor, NullDataDescriptor());
    auto serverSignal = SignalWithDescriptor(serverContext, valueDescriptor, nullptr, "signal");

    // do not send event packet on first subscribe request
    startServer(List<ISignal>(serverSignal), nullptr);

    for (auto& client : clients)
    {
        client.clientHandler = createClient(client, client.signalAvailableHandler);
        ASSERT_TRUE(client.clientHandler->connect(SERVER_ADDRESS, NATIVE_STREAMING_LISTENING_PORT));
        client.clientHandler->sendStreamingRequest();
        ASSERT_EQ(client.streamingInitFuture.wait_for(timeout), std::future_status::ready);

        ASSERT_EQ(client.signalAvailableFuture.wait_for(timeout), std::future_status::ready);
        auto clientSignalStringId = std::get<0>(client.signalAvailableFuture.get());

        client.clientHandler->subscribeSignal(clientSignalStringId);
        ASSERT_EQ(client.subscribedAckFuture.wait_for(timeout), std::future_status::ready);
    }

    ASSERT_EQ(signalSubscribedFuture.wait_for(timeout), std::future_status::ready);

    serverHandler->sendPacket(serverSignal.getGlobalId().toStdString(), firstEventPacket);
    for (auto& client : clients)
    {
        ASSERT_EQ(client.packetReceivedFuture.wait_for(timeout), std::future_status::ready);
        auto [signalId, packet] = client.packetReceivedFuture.get();
        ASSERT_EQ(signalId, serverSignal.getGlobalId());
        ASSERT_EQ(packet, firstEventPacket);
    }
}

TEST_P(StreamingProtocolTest, SendEventPacket)
{
    const auto valueDescriptor = DataDescriptorBuilder().setSampleType(SampleType::Float32).build();
    auto firstEventPacket = DataDescriptorChangedEventPacket(valueDescriptor, NullDataDescriptor());
    auto serverSignal = SignalWithDescriptor(serverContext, valueDescriptor, nullptr, "signal");

    startServer(List<ISignal>(serverSignal), firstEventPacket);

    for (auto& client : clients)
    {
        client.clientHandler = createClient(client, client.signalAvailableHandler);
        ASSERT_TRUE(client.clientHandler->connect(SERVER_ADDRESS, NATIVE_STREAMING_LISTENING_PORT));
        client.clientHandler->sendStreamingRequest();
        ASSERT_EQ(client.streamingInitFuture.wait_for(timeout), std::future_status::ready);

        ASSERT_EQ(client.signalAvailableFuture.wait_for(timeout), std::future_status::ready);
        auto clientSignalStringId = std::get<0>(client.signalAvailableFuture.get());

        client.clientHandler->subscribeSignal(clientSignalStringId);
        ASSERT_EQ(client.subscribedAckFuture.wait_for(timeout), std::future_status::ready);
    }

    ASSERT_EQ(signalSubscribedFuture.wait_for(timeout), std::future_status::ready);

    serverHandler->sendPacket(serverSignal.getGlobalId().toStdString(), firstEventPacket);
    for (auto& client : clients)
    {
        ASSERT_EQ(client.packetReceivedFuture.wait_for(timeout), std::future_status::ready);
        auto [signalId, packet] = client.packetReceivedFuture.get();
        ASSERT_EQ(signalId, serverSignal.getGlobalId());
        ASSERT_EQ(packet, firstEventPacket);

        // reset packet future / promise
        client.packetReceivedPromise = std::promise< std::tuple<StringPtr, PacketPtr> >();
        client.packetReceivedFuture = client.packetReceivedPromise.get_future();
    }

    const auto newValueDescriptor = DataDescriptorBuilder().setSampleType(SampleType::Binary).build();
    auto secondEventPacket = DataDescriptorChangedEventPacket(newValueDescriptor, nullptr);
    serverHandler->sendPacket(serverSignal.getGlobalId().toStdString(), secondEventPacket);

    for (auto& client : clients)
    {
        ASSERT_EQ(client.packetReceivedFuture.wait_for(timeout), std::future_status::ready);
        auto [signalId, packet] = client.packetReceivedFuture.get();
        ASSERT_EQ(signalId, serverSignal.getGlobalId());
        ASSERT_EQ(packet, secondEventPacket);
    }
}

TEST_P(StreamingProtocolTest, SendPacketsNoSubscribers)
{
    const auto valueDescriptor = DataDescriptorBuilder().setSampleType(SampleType::Float32).build();
    auto serverSignal = SignalWithDescriptor(serverContext, valueDescriptor, nullptr, "signal");
    auto serverEventPacket = DataDescriptorChangedEventPacket(valueDescriptor, NullDataDescriptor());

    startServer(List<ISignal>(serverSignal), serverEventPacket);

    for (auto& client : clients)
    {
        client.clientHandler = createClient(client, client.signalAvailableHandler);
        ASSERT_TRUE(client.clientHandler->connect(SERVER_ADDRESS, NATIVE_STREAMING_LISTENING_PORT));
        client.clientHandler->sendStreamingRequest();
        ASSERT_EQ(client.streamingInitFuture.wait_for(timeout), std::future_status::ready);
    }

    auto serverDataPacket = DataPacket(valueDescriptor, 100);
    serverHandler->sendPacket(serverSignal.getGlobalId().toStdString(), serverDataPacket);

    // no subscribers - so packets wont be sent to clients
    for (auto& client : clients)
    {
        ASSERT_EQ(client.packetReceivedFuture.wait_for(timeout), std::future_status::timeout);
    }
}

TEST_P(StreamingProtocolTest, SendDataPacket)
{
    const auto valueDescriptor = DataDescriptorBuilder().setSampleType(SampleType::Float32).build();
    auto serverEventPacket = DataDescriptorChangedEventPacket(valueDescriptor, NullDataDescriptor());
    auto serverDataPacket = DataPacket(valueDescriptor, 100);
    auto serverSignal = SignalWithDescriptor(serverContext, valueDescriptor, nullptr, "signal");

    startServer(List<ISignal>(serverSignal), serverEventPacket);

    for (auto& client : clients)
    {
        client.clientHandler = createClient(client, client.signalAvailableHandler);
        ASSERT_TRUE(client.clientHandler->connect(SERVER_ADDRESS, NATIVE_STREAMING_LISTENING_PORT));
        client.clientHandler->sendStreamingRequest();
        ASSERT_EQ(client.streamingInitFuture.wait_for(timeout), std::future_status::ready);

        ASSERT_EQ(client.signalAvailableFuture.wait_for(timeout), std::future_status::ready);
        auto clientSignalStringId = std::get<0>(client.signalAvailableFuture.get());

        client.clientHandler->subscribeSignal(clientSignalStringId);
        ASSERT_EQ(client.subscribedAckFuture.wait_for(timeout), std::future_status::ready);
    }

    ASSERT_EQ(signalSubscribedFuture.wait_for(timeout), std::future_status::ready);

    for (auto& client : clients)
    {
        // wait for event packet
        ASSERT_EQ(client.packetReceivedFuture.wait_for(timeout), std::future_status::ready);
        auto [signalId, packet] = client.packetReceivedFuture.get();
        ASSERT_EQ(signalId, serverSignal.getGlobalId());
        ASSERT_EQ(packet, serverEventPacket);
        // reset packet future / promise
        client.packetReceivedPromise = std::promise< std::tuple<StringPtr, PacketPtr> >();
        client.packetReceivedFuture = client.packetReceivedPromise.get_future();
    }

    serverHandler->sendPacket(serverSignal.getGlobalId().toStdString(), serverDataPacket);
    for (auto& client : clients)
    {
        // wait for data packet
        ASSERT_EQ(client.packetReceivedFuture.wait_for(timeout), std::future_status::ready);
        auto [signalId, packet] = client.packetReceivedFuture.get();
        ASSERT_EQ(signalId, serverSignal.getGlobalId());
        ASSERT_EQ(packet, serverDataPacket);
    }
}

TEST_P(StreamingProtocolTest, SendMultipleDataPackets)
{
    const auto valueDescriptor = DataDescriptorBuilder().setSampleType(SampleType::Float32).build();
    auto serverEventPacket = DataDescriptorChangedEventPacket(valueDescriptor, NullDataDescriptor());
    auto serverDataPackets = List<IPacket>(DataPacket(valueDescriptor, 100, 100),
                                           DataPacket(valueDescriptor, 100, 200),
                                           DataPacket(valueDescriptor, 100, 300),
                                           DataPacket(valueDescriptor, 100, 400));
    auto serverSignal = SignalWithDescriptor(serverContext, valueDescriptor, nullptr, "signal");

    auto sentDataPackets = List<IPacket>();
    using ClientReceivedPackets = std::tuple<StringPtr, ListPtr<IPacket>>;
    using ClientReceivedPacketsPromise = std::promise<ClientReceivedPackets>;
    using ClientReceivedPacketsFuture = std::future<ClientReceivedPackets>;
    std::vector<std::shared_ptr<ClientReceivedPacketsPromise>> receivedPacketPromises;
    std::vector<std::shared_ptr<ClientReceivedPacketsFuture>> receivedPacketFutures;

    startServer(List<ISignal>(serverSignal), serverEventPacket);

    for (auto& client : clients)
    {
        client.clientHandler = createClient(client, client.signalAvailableHandler);
        ASSERT_TRUE(client.clientHandler->connect(SERVER_ADDRESS, NATIVE_STREAMING_LISTENING_PORT));
        client.clientHandler->sendStreamingRequest();
        ASSERT_EQ(client.streamingInitFuture.wait_for(timeout), std::future_status::ready);

        ASSERT_EQ(client.signalAvailableFuture.wait_for(timeout), std::future_status::ready);
        auto clientSignalStringId = std::get<0>(client.signalAvailableFuture.get());

        client.clientHandler->subscribeSignal(clientSignalStringId);
        ASSERT_EQ(client.subscribedAckFuture.wait_for(timeout), std::future_status::ready);
    }

    ASSERT_EQ(signalSubscribedFuture.wait_for(timeout), std::future_status::ready);

    for (auto& client : clients)
    {
        // wait for event packet
        ASSERT_EQ(client.packetReceivedFuture.wait_for(timeout), std::future_status::ready);
        auto [signalId, packet] = client.packetReceivedFuture.get();
        ASSERT_EQ(signalId, serverSignal.getGlobalId());
        ASSERT_EQ(packet, serverEventPacket);

        ListPtr<IPacket> receivedDataPackets = List<IPacket>();
        // create received packets future / promise
        auto packetsReceivedPromise = std::make_shared< ClientReceivedPacketsPromise >();
        auto packetsReceivedFuture = std::make_shared< ClientReceivedPacketsFuture >(packetsReceivedPromise->get_future());
        receivedPacketFutures.push_back(packetsReceivedFuture);

        client.packetHandler =
            [packetsReceivedPromise = packetsReceivedPromise, receivedDataPackets = receivedDataPackets]
            (const StringPtr& signalStringId, const PacketPtr& packet) mutable
        {
            receivedDataPackets.pushBack(packet);
            if (receivedDataPackets.getCount() == 4)
                packetsReceivedPromise->set_value({signalStringId, receivedDataPackets});
        };

        client.clientHandler->setStreamingHandlers(
            client.signalAvailableHandler,
            client.signalUnavailableHandler,
            client.packetHandler,
            client.signalSubscriptionAckHandler,
            client.connectionStatusChangedHandler,
            client.streamingInitDoneHandler
        );
    }

    // process and then send all data packets within a single transport operation
    
    std::vector<IPacket*> packetBuf;
    tsl::ordered_map<std::string, opendaq_native_streaming_protocol::PacketBufferData> packetIndices;
    packetBuf.resize(serverDataPackets.getCount());

    for (size_t i = 0; i < serverDataPackets.getCount(); ++i)
    {
        packetBuf[i] = serverDataPackets[i].detach();
    }

    auto packetBufferData = PacketBufferData();
    packetBufferData.index = 0;
    packetBufferData.count = static_cast<int>(packetBuf.size());
    packetIndices.insert(std::make_pair(serverSignal.getGlobalId().toStdString(), packetBufferData));

    serverHandler->processStreamingPackets(packetIndices, packetBuf);
    serverHandler->sendAvailableStreamingPackets();

    for (size_t i = 0; i < clients.size(); ++i)
    {
        // wait for all data packets received
        ASSERT_EQ(receivedPacketFutures[i]->wait_for(timeout), std::future_status::ready);
        auto [signalId, packets] = receivedPacketFutures[i]->get();
        ASSERT_EQ(signalId, serverSignal.getGlobalId());
        ASSERT_EQ(serverDataPackets, packets);
    }
}

TEST_P(StreamingProtocolTest, ConstantValueReplayedToLateSubscriber)
{
    if (clients.size() < 2)
        return;

    const auto valueDescriptor = DataDescriptorBuilder().setSampleType(SampleType::Int64).setRule(ConstantDataRule()).build();
    auto serverEventPacket = DataDescriptorChangedEventPacket(valueDescriptor, NullDataDescriptor());
    auto serverDataPacket = ConstantDataPacket(valueDescriptor, int64_t{7});
    auto serverSignal = SignalWithDescriptor(serverContext, valueDescriptor, nullptr, "signal");

    startServer(List<ISignal>(serverSignal), serverEventPacket);

    // the first subscriber gets the descriptor and then the value the signal produced
    auto& firstClient = clients[0];
    firstClient.clientHandler = createClient(firstClient, firstClient.signalAvailableHandler);
    ASSERT_TRUE(firstClient.clientHandler->connect(SERVER_ADDRESS, NATIVE_STREAMING_LISTENING_PORT));
    firstClient.clientHandler->sendStreamingRequest();
    ASSERT_EQ(firstClient.streamingInitFuture.wait_for(timeout), std::future_status::ready);
    ASSERT_EQ(firstClient.signalAvailableFuture.wait_for(timeout), std::future_status::ready);
    const auto firstClientSignalId = std::get<0>(firstClient.signalAvailableFuture.get());

    firstClient.clientHandler->subscribeSignal(firstClientSignalId);
    ASSERT_EQ(firstClient.subscribedAckFuture.wait_for(timeout), std::future_status::ready);
    ASSERT_EQ(signalSubscribedFuture.wait_for(timeout), std::future_status::ready);

    ASSERT_EQ(firstClient.packetReceivedFuture.wait_for(timeout), std::future_status::ready);
    ASSERT_EQ(std::get<1>(firstClient.packetReceivedFuture.get()), serverEventPacket);
    firstClient.packetReceivedPromise = std::promise<std::tuple<StringPtr, PacketPtr>>();
    firstClient.packetReceivedFuture = firstClient.packetReceivedPromise.get_future();

    serverHandler->sendPacket(serverSignal.getGlobalId().toStdString(), serverDataPacket);
    ASSERT_EQ(firstClient.packetReceivedFuture.wait_for(timeout), std::future_status::ready);
    ASSERT_EQ(std::get<1>(firstClient.packetReceivedFuture.get()), serverDataPacket);

    // every later subscriber gets the descriptor and the value without the server sending anything new
    for (size_t i = 1; i < clients.size(); ++i)
    {
        auto& client = clients[i];
        client.clientHandler = createClient(client, client.signalAvailableHandler);
        ASSERT_TRUE(client.clientHandler->connect(SERVER_ADDRESS, NATIVE_STREAMING_LISTENING_PORT));
        client.clientHandler->sendStreamingRequest();
        ASSERT_EQ(client.streamingInitFuture.wait_for(timeout), std::future_status::ready);
        ASSERT_EQ(client.signalAvailableFuture.wait_for(timeout), std::future_status::ready);
        const auto clientSignalId = std::get<0>(client.signalAvailableFuture.get());

        // both packets arrive back to back, so collect them instead of resetting a promise in between
        auto packetsPromise = std::make_shared<std::promise<ListPtr<IPacket>>>();
        auto packetsFuture = packetsPromise->get_future();
        auto receivedPackets = List<IPacket>();
        client.packetHandler =
            [packetsPromise = packetsPromise, receivedPackets = receivedPackets](const StringPtr&, const PacketPtr& packet) mutable
        {
            receivedPackets.pushBack(packet);
            if (receivedPackets.getCount() == 2)
                packetsPromise->set_value(receivedPackets);
        };
        client.clientHandler->setStreamingHandlers(client.signalAvailableHandler,
                                                   client.signalUnavailableHandler,
                                                   client.packetHandler,
                                                   client.signalSubscriptionAckHandler,
                                                   client.connectionStatusChangedHandler,
                                                   client.streamingInitDoneHandler);

        client.clientHandler->subscribeSignal(clientSignalId);
        ASSERT_EQ(client.subscribedAckFuture.wait_for(timeout), std::future_status::ready);

        ASSERT_EQ(packetsFuture.wait_for(timeout), std::future_status::ready);
        const auto packets = packetsFuture.get();
        ASSERT_EQ(packets[0].getType(), PacketType::Event);

        const auto dataPacket = packets[1].asPtr<IDataPacket>();
        ASSERT_EQ(dataPacket.getSampleCount(), 1u);
        ASSERT_FALSE(dataPacket.getDomainPacket().assigned());
        ASSERT_EQ(dataPacket.getLastValue(), 7);
    }
}

TEST_P(StreamingProtocolTest, ConstantValueNotReplayedAfterDescriptorChanged)
{
    if (clients.size() < 2)
        return;

    const auto valueDescriptor = DataDescriptorBuilder().setSampleType(SampleType::Int64).setRule(ConstantDataRule()).build();
    auto serverEventPacket = DataDescriptorChangedEventPacket(valueDescriptor, NullDataDescriptor());
    auto serverDataPacket = ConstantDataPacket(valueDescriptor, int64_t{7});
    auto serverSignal = SignalWithDescriptor(serverContext, valueDescriptor, nullptr, "signal");

    const auto newValueDescriptor = DataDescriptorBuilder().setSampleType(SampleType::Int32).setRule(ConstantDataRule()).build();
    auto newEventPacket = DataDescriptorChangedEventPacket(newValueDescriptor, NullDataDescriptor());

    startServer(List<ISignal>(serverSignal), serverEventPacket);

    auto& firstClient = clients[0];
    firstClient.clientHandler = createClient(firstClient, firstClient.signalAvailableHandler);
    ASSERT_TRUE(firstClient.clientHandler->connect(SERVER_ADDRESS, NATIVE_STREAMING_LISTENING_PORT));
    firstClient.clientHandler->sendStreamingRequest();
    ASSERT_EQ(firstClient.streamingInitFuture.wait_for(timeout), std::future_status::ready);
    ASSERT_EQ(firstClient.signalAvailableFuture.wait_for(timeout), std::future_status::ready);
    const auto firstClientSignalId = std::get<0>(firstClient.signalAvailableFuture.get());

    firstClient.clientHandler->subscribeSignal(firstClientSignalId);
    ASSERT_EQ(firstClient.subscribedAckFuture.wait_for(timeout), std::future_status::ready);
    ASSERT_EQ(signalSubscribedFuture.wait_for(timeout), std::future_status::ready);
    ASSERT_EQ(firstClient.packetReceivedFuture.wait_for(timeout), std::future_status::ready);

    // this client receives two more packets below, and its default handler fulfills a one-shot promise
    firstClient.packetHandler = [](const StringPtr&, const PacketPtr&) {};
    firstClient.clientHandler->setStreamingHandlers(firstClient.signalAvailableHandler,
                                                    firstClient.signalUnavailableHandler,
                                                    firstClient.packetHandler,
                                                    firstClient.signalSubscriptionAckHandler,
                                                    firstClient.connectionStatusChangedHandler,
                                                    firstClient.streamingInitDoneHandler);

    // a value, and then a descriptor the value does not belong to
    serverHandler->sendPacket(serverSignal.getGlobalId().toStdString(), serverDataPacket);
    serverHandler->sendPacket(serverSignal.getGlobalId().toStdString(), newEventPacket);

    // the late subscriber gets the descriptor only - replaying the stale value would contradict it
    auto& lateClient = clients[1];
    lateClient.clientHandler = createClient(lateClient, lateClient.signalAvailableHandler);
    ASSERT_TRUE(lateClient.clientHandler->connect(SERVER_ADDRESS, NATIVE_STREAMING_LISTENING_PORT));
    lateClient.clientHandler->sendStreamingRequest();
    ASSERT_EQ(lateClient.streamingInitFuture.wait_for(timeout), std::future_status::ready);
    ASSERT_EQ(lateClient.signalAvailableFuture.wait_for(timeout), std::future_status::ready);
    const auto lateClientSignalId = std::get<0>(lateClient.signalAvailableFuture.get());

    auto firstPacketPromise = std::make_shared<std::promise<void>>();
    auto firstPacketFuture = firstPacketPromise->get_future();
    auto receivedPackets = List<IPacket>();
    lateClient.packetHandler =
        [firstPacketPromise = firstPacketPromise, receivedPackets = receivedPackets](const StringPtr&, const PacketPtr& packet) mutable
    {
        receivedPackets.pushBack(packet);
        if (receivedPackets.getCount() == 1)
            firstPacketPromise->set_value();
    };
    lateClient.clientHandler->setStreamingHandlers(lateClient.signalAvailableHandler,
                                                   lateClient.signalUnavailableHandler,
                                                   lateClient.packetHandler,
                                                   lateClient.signalSubscriptionAckHandler,
                                                   lateClient.connectionStatusChangedHandler,
                                                   lateClient.streamingInitDoneHandler);

    lateClient.clientHandler->subscribeSignal(lateClientSignalId);
    ASSERT_EQ(lateClient.subscribedAckFuture.wait_for(timeout), std::future_status::ready);
    ASSERT_EQ(firstPacketFuture.wait_for(timeout), std::future_status::ready);

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    ASSERT_EQ(receivedPackets.getCount(), 1u);
    ASSERT_EQ(receivedPackets[0].getType(), PacketType::Event);
}

TEST_P(StreamingProtocolTest, AddNotPublicSignal)
{
    startServer(List<ISignal>());

    for (auto& client : clients)
    {
        client.clientHandler = createClient(client, client.signalAvailableHandler);
        ASSERT_TRUE(client.clientHandler->connect(SERVER_ADDRESS, NATIVE_STREAMING_LISTENING_PORT));
        client.clientHandler->sendStreamingRequest();
        ASSERT_EQ(client.streamingInitFuture.wait_for(timeout), std::future_status::ready);
    }

    auto serverSignal =
        SignalWithDescriptor(serverContext, DataDescriptorBuilder().setSampleType(SampleType::Undefined).build(), nullptr, "signal");
    serverSignal.setName("signalName");
    serverSignal.setDescription("signalDescription");
    serverSignal.setPublic(false);
    serverHandler->addSignal(serverSignal);

    for (auto& client : clients)
    {
        ASSERT_EQ(client.signalAvailableFuture.wait_for(std::chrono::milliseconds(100)), std::future_status::timeout);
    }
}

TEST_P(StreamingProtocolTest, AddNotPublicSignalInConstructor)
{
    auto serverSignal =
        SignalWithDescriptor(serverContext, DataDescriptorBuilder().setSampleType(SampleType::Undefined).build(), nullptr, "signal");
    serverSignal.setName("signalName");
    serverSignal.setDescription("signalDescription");
    serverSignal.setPublic(false);

    startServer(List<ISignal>(serverSignal));

    for (auto& client : clients)
    {
        client.clientHandler = createClient(client, client.signalAvailableHandler);
        ASSERT_TRUE(client.clientHandler->connect(SERVER_ADDRESS, NATIVE_STREAMING_LISTENING_PORT));
        client.clientHandler->sendStreamingRequest();
        ASSERT_EQ(client.streamingInitFuture.wait_for(timeout), std::future_status::ready);

        ASSERT_EQ(client.signalAvailableFuture.wait_for(std::chrono::milliseconds(100)), std::future_status::timeout);
    }
}

INSTANTIATE_TEST_SUITE_P(
    ProtocolTestGroup,
    StreamingProtocolTest,
    testing::Values(std::make_tuple(1, true),
                    std::make_tuple(1, false),
                    std::make_tuple(4, true),
                    std::make_tuple(4, false))
);
