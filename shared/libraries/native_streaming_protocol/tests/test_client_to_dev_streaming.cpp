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
    }

    void tearDown()
    {
        ClientAttributesBase::tearDown();
    }
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

            OnPacketBufferReceivedCallback receiveStreamingPacketBuffer =
                [this](const packet_streaming::PacketBufferPtr& packetBufferPtr)
            {
                clientToDeviceStreamingConsumer.addPacketBuffer(packetBufferPtr);
                streamingPacketReceivedPromise.set_value(clientToDeviceStreamingConsumer.getNextDaqPacket());
            };

            return std::make_pair(nullptr, receiveStreamingPacketBuffer);
        };

        client.setUp();
        configProtocolTriggeredPromise = std::promise< void > ();
        configProtocolTriggeredFuture = configProtocolTriggeredPromise.get_future();

        streamingPacketReceivedPromise = std::promise< std::tuple<SignalNumericIdType, PacketPtr> >();
        streamingPacketReceivedFuture = streamingPacketReceivedPromise.get_future();
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

protected:
    std::promise< void > configProtocolTriggeredPromise;
    std::future< void > configProtocolTriggeredFuture;
    SetUpConfigProtocolServerCb setUpConfigProtocolServerCb;

    ClientAttributes client;

    std::promise< std::tuple<SignalNumericIdType, PacketPtr> > streamingPacketReceivedPromise;
    std::future< std::tuple<SignalNumericIdType, PacketPtr> > streamingPacketReceivedFuture;
    packet_streaming::PacketStreamingClient clientToDeviceStreamingConsumer;
};

TEST_P(ClientToDeviceStreamingTest, EventPacket)
{
    const auto valueDescriptor = DataDescriptorBuilder().setSampleType(SampleType::Float32).build();
    auto eventPacket = DataDescriptorChangedEventPacket(valueDescriptor, nullptr);
    auto clientSignal = SignalWithDescriptor(client.clientContext, valueDescriptor, nullptr, "signal");

    startServer();

    ASSERT_TRUE(client.clientHandler->connect(SERVER_ADDRESS, NATIVE_STREAMING_LISTENING_PORT));
    client.clientHandler->sendConfigRequest(config_protocol::PacketBuffer::createGetProtocolInfoRequest(0));
    ASSERT_EQ(configProtocolTriggeredFuture.wait_for(timeout), std::future_status::ready);

    client.clientHandler->sendStreamingPacket(1u, eventPacket);

    ASSERT_EQ(streamingPacketReceivedFuture.wait_for(timeout), std::future_status::ready);

    auto received = streamingPacketReceivedFuture.get();
    ASSERT_EQ(std::get<0>(received), 1u);
    ASSERT_EQ(std::get<1>(received), eventPacket);
}

INSTANTIATE_TEST_SUITE_P(
    ProtocolTestGroup,
    ClientToDeviceStreamingTest,
    testing::Values(std::make_tuple(1, true))
);
