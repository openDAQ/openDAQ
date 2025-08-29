#include "test_base.h"
#include <testutils/memcheck_listener.h>

#include <native_streaming_protocol/native_streaming_client_handler.h>
#include <native_streaming_protocol/native_streaming_server_handler.h>

using namespace daq;
using namespace daq::opendaq_native_streaming_protocol;
using namespace daq::packet_streaming;

class ClientAttributes : public ClientAttributesBase
{
public:
    void setUp()
    {
        ClientAttributesBase::setUp();

        clientHandler = std::make_shared<NativeStreamingClientHandler>(
            clientContext, ClientAttributesBase::createTransportLayerConfig(), ClientAttributesBase::createAuthenticationConfig());

        clientHandler->setConfigHandlers([](config_protocol::PacketBuffer&&) {},
                                         [](const EnumerationPtr&, const StringPtr&) {});

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

        streamingInitPromise = std::promise< void >();
        streamingInitFuture = streamingInitPromise.get_future();

        streamingInitDoneHandler = [this]()
        {
            streamingInitPromise.set_value();
        };

        clientHandler->setStreamingHandlers([](const StringPtr&, const StringPtr&) {},
                                            [](const StringPtr&) {},
                                            [](const StringPtr&, const PacketPtr&) {},
                                            [](const StringPtr&, bool) {},
                                            [](const EnumerationPtr&, const StringPtr&) {},
                                            streamingInitDoneHandler);

        clientHandler->setStreamingToDeviceHandlers(signalSubscribedHandler, signalUnsubscribedHandler);
    }

    void tearDown()
    {
        ClientAttributesBase::tearDown();
    }

    std::promise< SignalPtr > signalSubscribedPromise;
    std::future< SignalPtr > signalSubscribedFuture;

    std::promise< SignalPtr > signalUnsubscribedPromise;
    std::future< SignalPtr > signalUnsubscribedFuture;

    std::promise< void > streamingInitPromise;
    std::future< void > streamingInitFuture;

protected:
    OnSignalSubscribedCallback signalSubscribedHandler;
    OnSignalUnsubscribedCallback signalUnsubscribedHandler;
    OnStreamingInitDoneCallback streamingInitDoneHandler;
};

class ClientToDeviceStreamingTest : public ProtocolTestBase
{
public:
    void SetUp() override
    {
        ProtocolTestBase::SetUp();
        setUpConfigProtocolServerCb =
            [this](SendConfigProtocolPacketCb sendPacketCb, const UserPtr& user, ClientType connectionType)
        {
            configProtocolTriggeredPromise.set_value();

            OnPacketBufferReceivedCallback basicStreamingPacketBufferHandler =
                [this](const packet_streaming::PacketBufferPtr& packetBufferPtr)
            {
                basicClientToDeviceStreamingConsumer.addPacketBuffer(packetBufferPtr);
                basicStreamingPacketReceivedPromise.set_value(basicClientToDeviceStreamingConsumer.getNextDaqPacket());
            };

            return std::make_pair(nullptr, basicStreamingPacketBufferHandler);
        };

        client.setUp();
        configProtocolTriggeredPromise = std::promise< void > ();
        configProtocolTriggeredFuture = configProtocolTriggeredPromise.get_future();

        basicStreamingPacketReceivedPromise = std::promise< std::tuple<SignalNumericIdType, PacketPtr> >();
        basicStreamingPacketReceivedFuture = basicStreamingPacketReceivedPromise.get_future();

        signalAvailablePromise = std::promise< std::tuple<StringPtr, StringPtr> >();
        signalAvailableFuture = signalAvailablePromise.get_future();

        signalUnavailablePromise = std::promise< StringPtr >();
        signalUnavailableFuture = signalUnavailablePromise.get_future();

        subscribedAckPromise = std::promise< StringPtr >();
        subscribedAckFuture = subscribedAckPromise.get_future();

        generalizedStreamingPacketReceivedPromise = std::promise< std::tuple<StringPtr, PacketPtr> >();
        generalizedStreamingPacketReceivedFuture = generalizedStreamingPacketReceivedPromise.get_future();

        unsubscribedAckPromise = std::promise< StringPtr >();
        unsubscribedAckFuture = unsubscribedAckPromise.get_future();

        signalAvailableHandler = [this](const StringPtr& signalStringId,
                                        const StringPtr& serializedSignal)
        {
            signalAvailablePromise.set_value({signalStringId, serializedSignal});
        };

        signalUnavailableHandler = [this](const StringPtr& signalStringId)
        {
            signalUnavailablePromise.set_value(signalStringId);
        };

        signalSubscriptionAckHandler = [this](const StringPtr& signalStringId, bool subscribed)
        {
            if (subscribed)
                subscribedAckPromise.set_value(signalStringId);
            else
                unsubscribedAckPromise.set_value(signalStringId);
        };

        generalizedStreamingPacketHandler = [this](const StringPtr& signalStringId, const PacketPtr& packet)
        {
            generalizedStreamingPacketReceivedPromise.set_value({signalStringId, packet});
        };
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
        serverHandler->setStreamingToDeviceHandlers(signalAvailableHandler,
                                                    signalUnavailableHandler,
                                                    generalizedStreamingPacketHandler,
                                                    signalSubscriptionAckHandler);
        serverHandler->startServer(NATIVE_STREAMING_SERVER_PORT);
    }

    void stopServer()
    {
        stopIoOperations();
        serverHandler.reset();
    }

protected:
    std::promise< std::tuple<StringPtr, StringPtr> > signalAvailablePromise;
    std::future< std::tuple<StringPtr, StringPtr> > signalAvailableFuture;

    std::promise< StringPtr > signalUnavailablePromise;
    std::future< StringPtr > signalUnavailableFuture;

    std::promise< StringPtr > subscribedAckPromise;
    std::future< StringPtr > subscribedAckFuture;

    std::promise< StringPtr > unsubscribedAckPromise;
    std::future< StringPtr > unsubscribedAckFuture;

    std::promise< std::tuple<StringPtr, PacketPtr> > generalizedStreamingPacketReceivedPromise;
    std::future< std::tuple<StringPtr, PacketPtr> > generalizedStreamingPacketReceivedFuture;

    OnSignalAvailableCallback signalAvailableHandler;
    OnSignalUnavailableCallback signalUnavailableHandler;
    OnSignalSubscriptionAckCallback signalSubscriptionAckHandler;
    OnPacketCallback generalizedStreamingPacketHandler;

    std::promise< void > configProtocolTriggeredPromise;
    std::future< void > configProtocolTriggeredFuture;
    SetUpConfigProtocolServerCb setUpConfigProtocolServerCb;

    ClientAttributes client;

    std::promise< std::tuple<SignalNumericIdType, PacketPtr> > basicStreamingPacketReceivedPromise;
    std::future< std::tuple<SignalNumericIdType, PacketPtr> > basicStreamingPacketReceivedFuture;
    packet_streaming::PacketStreamingClient basicClientToDeviceStreamingConsumer;
};

TEST_P(ClientToDeviceStreamingTest, SendPacketBasicC2DStreaming)
{
    const auto valueDescriptor = DataDescriptorBuilder().setSampleType(SampleType::Float32).build();
    auto eventPacket = DataDescriptorChangedEventPacket(valueDescriptor, nullptr);
    auto clientSignal = SignalWithDescriptor(client.clientContext, valueDescriptor, nullptr, "signal");

    startServer();

    ASSERT_TRUE(client.clientHandler->connect(SERVER_ADDRESS, NATIVE_STREAMING_LISTENING_PORT));
    client.clientHandler->sendConfigRequest(config_protocol::PacketBuffer::createGetProtocolInfoRequest(0));
    ASSERT_EQ(configProtocolTriggeredFuture.wait_for(timeout), std::future_status::ready);

    client.clientHandler->sendStreamingPacket(1u, eventPacket);

    ASSERT_EQ(basicStreamingPacketReceivedFuture.wait_for(timeout), std::future_status::ready);

    auto received = basicStreamingPacketReceivedFuture.get();
    ASSERT_EQ(std::get<0>(received), 1u);
    ASSERT_EQ(std::get<1>(received), eventPacket);
}

TEST_P(ClientToDeviceStreamingTest, GeneralizedC2DStreaming)
{
    // create signal to stream
    const auto valueDescriptor = DataDescriptorBuilder().setSampleType(SampleType::Float32).build();
    auto eventPacket = DataDescriptorChangedEventPacket(valueDescriptor, nullptr);
    auto clientSignal = SignalWithDescriptor(client.clientContext, valueDescriptor, nullptr, "signal");

    // create and start server
    startServer();

    // connect client to server
    ASSERT_TRUE(client.clientHandler->connect(SERVER_ADDRESS, NATIVE_STREAMING_LISTENING_PORT));

    // initiate streaming
    client.clientHandler->sendStreamingRequest();
    ASSERT_EQ(client.streamingInitFuture.wait_for(timeout), std::future_status::ready);

    // make signal available via C2D streaming
    client.clientHandler->addClientSignal(clientSignal);
    ASSERT_EQ(signalAvailableFuture.wait_for(timeout), std::future_status::ready);
    auto [clientSignalStringId, _] = signalAvailableFuture.get();
    ASSERT_EQ(clientSignalStringId, clientSignal.getGlobalId());

    // subscribe signal
    serverHandler->doSubscribeSignal(clientSignal.getGlobalId(), true);
    ASSERT_EQ(subscribedAckFuture.wait_for(timeout), std::future_status::ready);
    ASSERT_EQ(subscribedAckFuture.get(), clientSignal.getGlobalId());
    ASSERT_EQ(client.signalSubscribedFuture.wait_for(timeout), std::future_status::ready);
    ASSERT_EQ(client.signalSubscribedFuture.get(), clientSignal);

    // send one packet
    client.clientHandler->sendPacket(clientSignal.getGlobalId().toStdString(), eventPacket);
    ASSERT_EQ(generalizedStreamingPacketReceivedFuture.wait_for(timeout), std::future_status::ready);
    auto received = generalizedStreamingPacketReceivedFuture.get();
    ASSERT_EQ(std::get<0>(received), clientSignal.getGlobalId());
    ASSERT_EQ(std::get<1>(received), eventPacket);

    // unsubscribe signal
    serverHandler->doSubscribeSignal(clientSignal.getGlobalId(), false);
    ASSERT_EQ(unsubscribedAckFuture.wait_for(timeout), std::future_status::ready);
    ASSERT_EQ(unsubscribedAckFuture.get(), clientSignal.getGlobalId());
    ASSERT_EQ(client.signalUnsubscribedFuture.wait_for(timeout), std::future_status::ready);
    ASSERT_EQ(client.signalUnsubscribedFuture.get(), clientSignal);

    // make signal unavailable via C2D streaming
    client.clientHandler->removeClientSignal(clientSignal);
    ASSERT_EQ(signalUnavailableFuture.wait_for(timeout), std::future_status::ready);
    ASSERT_EQ(signalUnavailableFuture.get(), clientSignal.getGlobalId());
}

INSTANTIATE_TEST_SUITE_P(
    ProtocolTestGroup,
    ClientToDeviceStreamingTest,
    testing::Values(std::make_tuple(1, true))
);
