#include "test_helpers/test_helpers.h"
#include <opendaq/mock/mock_device_module.h>
#include <coreobjects/authentication_provider_factory.h>

using namespace daq;
using namespace std::chrono_literals;

// first param: streaming server type
// second param: primary streaming protocol
// third param: client device connection string
class StreamingTest : public testing::TestWithParam<std::tuple<std::string, std::string, std::string>>
{
public:
    void SetUp() override
    {
        auto connectionString = std::get<2>(GetParam());
        bool connectStringIpv6 = connectionString.find('[') != std::string::npos && connectionString.find(']') != std::string::npos;
        if (connectStringIpv6 && test_helpers::Ipv6IsDisabled())
        {
            GTEST_SKIP() << "Ipv6 is disabled";
        }

        serverInstance = CreateServerInstance();
        clientInstance = CreateClientInstance();
    }

    void TearDown() override
    {
    }

    void generatePackets(size_t packetCount)
    {
        auto devices = serverInstance.getDevices();

        for (const auto& device : devices)
        {
            auto name = device.getName();
            if (name == "MockPhysicalDevice")
                device.setPropertyValue("GeneratePackets", packetCount);
        }
    }

    SignalPtr getSignal(const DevicePtr& device, const std::string& signalName)
    {
        auto signals = device.getSignals(search::Recursive(search::Visible()));

        for (const auto& signal : signals)
        {
            const auto descriptor = signal.getDescriptor();
            if (descriptor.assigned() && descriptor.getName() == signalName)
            {
                return signal;
            }
        }

        throw NotFoundException();
    }

    PacketReaderPtr createServerReader(const std::string& signalName)
    {
        return PacketReader(getSignal(serverInstance, signalName));
    }

    PacketReaderPtr createClientReader(const std::string& signalName)
    {
        PacketReaderPtr reader = PacketReader(getSignal(clientInstance, signalName));
        return reader;
    }

protected:
    virtual InstancePtr CreateServerInstance()
    {
        auto logger = Logger();
        auto scheduler = Scheduler(logger);
        auto moduleManager = ModuleManager("");
        auto typeManager = TypeManager();
        auto authenticationProvider = AuthenticationProvider();
        auto context = Context(scheduler, logger, typeManager, moduleManager, authenticationProvider);

        const ModulePtr deviceModule(MockDeviceModule_Create(context));
        moduleManager.addModule(deviceModule);

        auto instance = InstanceCustom(context, "local");

        const auto mockDevice = instance.addDevice("daqmock://phys_device");

        auto streamingServer = std::get<0>(GetParam());
        instance.addServer(streamingServer, nullptr);
        // streaming server added first, so registered device streaming options is published over opcua
        instance.addServer("OpenDAQOPCUA", nullptr);

        return instance;
    }

    InstancePtr CreateClientInstance()
    {
        auto instance = Instance();
        auto connectionString = std::get<2>(GetParam());
        auto device = instance.addDevice(connectionString);
        return instance;
    }

    InstancePtr serverInstance;
    InstancePtr clientInstance;
};

TEST_P(StreamingTest, SignalDescriptorEvents)
{
    const size_t packetsToGenerate = 5;
    const size_t initialEventPackets = 1;
    const size_t packetsPerChange = 2;  // one triggered by data signal and one trigegred by domain signal
    const size_t packetsToRead = initialEventPackets + packetsToGenerate + (packetsToGenerate - 1) * packetsPerChange;

    auto serverSignal = getSignal(serverInstance, "ChangingSignal");
    auto clientSignal = getSignal(clientInstance, "ChangingSignal");

    auto mirroredSignalPtr = clientSignal.template asPtr<IMirroredSignalConfig>();
    std::promise<StringPtr> subscribeCompletePromise;
    std::future<StringPtr> subscribeCompleteFuture;
    test_helpers::setupSubscribeAckHandler(subscribeCompletePromise, subscribeCompleteFuture, mirroredSignalPtr);

    auto serverReader = createServerReader("ChangingSignal");
    auto clientReader = createClientReader("ChangingSignal");

    ASSERT_TRUE(test_helpers::waitForAcknowledgement(subscribeCompleteFuture));

    generatePackets(packetsToGenerate); 

    // TODO event packets for websocket streaming are not compared
    // websocket streaming does not recreate half assigned data descriptor changed event packet on client side
    // both: value and domain descriptors are always assigned in event packet
    // while on server side one descriptor can be assigned only
    auto serverReceivedPackets = test_helpers::tryReadPackets(serverReader, packetsToRead);
    auto clientReceivedPackets = test_helpers::tryReadPackets(clientReader, packetsToRead);
    EXPECT_EQ(serverReceivedPackets.getCount(), packetsToRead);
    EXPECT_EQ(clientReceivedPackets.getCount(), packetsToRead);
    EXPECT_TRUE(test_helpers::packetsEqual(serverReceivedPackets,
                                           clientReceivedPackets,
                                           std::get<0>(GetParam()) == "OpenDAQLTStreaming"));

    // recreate client reader and test initial event packet
    clientReader = createClientReader(clientSignal.getDescriptor().getName());
    clientReceivedPackets = test_helpers::tryReadPackets(clientReader, 1);

    ASSERT_EQ(clientReceivedPackets.getCount(), 1u);

    EventPacketPtr eventPacket = clientReceivedPackets[0].asPtrOrNull<IEventPacket>();
    ASSERT_TRUE(eventPacket.assigned());
    ASSERT_EQ(eventPacket.getEventId(), event_packet_id::DATA_DESCRIPTOR_CHANGED);

    DataDescriptorPtr dataDescriptor = eventPacket.getParameters().get(event_packet_param::DATA_DESCRIPTOR);
    DataDescriptorPtr domainDescriptor = eventPacket.getParameters().get(event_packet_param::DOMAIN_DATA_DESCRIPTOR);

    ASSERT_TRUE(dataDescriptor.assigned());
    ASSERT_TRUE(domainDescriptor.assigned());

    EXPECT_EQ(dataDescriptor, serverSignal.getDescriptor());
    EXPECT_EQ(dataDescriptor, clientSignal.getDescriptor());
    EXPECT_EQ(domainDescriptor, serverSignal.getDomainSignal().getDescriptor());
    EXPECT_EQ(domainDescriptor, clientSignal.getDomainSignal().getDescriptor());
}

TEST_P(StreamingTest, DataPackets)
{
    const size_t packetsToGenerate = 10;

    // Expect to receive all data packets,
    // +1 signal initial descriptor changed event packet
    const size_t packetsToRead = packetsToGenerate + 1;

    auto mirroredSignalPtr = getSignal(clientInstance, "ByteStep").template asPtr<IMirroredSignalConfig>();
    std::promise<StringPtr> subscribeCompletePromise;
    std::future<StringPtr> subscribeCompleteFuture;
    test_helpers::setupSubscribeAckHandler(subscribeCompletePromise, subscribeCompleteFuture, mirroredSignalPtr);

    auto serverReader = createServerReader("ByteStep");
    auto clientReader = createClientReader("ByteStep");

    ASSERT_TRUE(test_helpers::waitForAcknowledgement(subscribeCompleteFuture));

    generatePackets(packetsToGenerate);

    auto serverReceivedPackets = test_helpers::tryReadPackets(serverReader, packetsToRead);
    auto clientReceivedPackets = test_helpers::tryReadPackets(clientReader, packetsToRead);

    EXPECT_EQ(serverReceivedPackets.getCount(), packetsToRead);
    EXPECT_EQ(clientReceivedPackets.getCount(), packetsToRead);
    EXPECT_TRUE(test_helpers::packetsEqual(serverReceivedPackets, clientReceivedPackets));
}

TEST_P(StreamingTest, ChangedDataDescriptorBeforeSubscribe)
{
    SKIP_TEST_MAC_CI;
    SignalConfigPtr serverSignalPtr = getSignal(serverInstance, "ByteStep");
    MirroredSignalConfigPtr clientSignalPtr = getSignal(clientInstance, "ByteStep");
    MirroredSignalConfigPtr clientDomainSignalPtr = clientSignalPtr.getDomainSignal();

    bool usingNativePseudoDevice = std::get<1>(GetParam()) == "OpenDAQNativeStreaming" && (std::get<2>(GetParam()).find("daq.ns://") == 0);
    bool usingWSPseudoDevice = std::get<1>(GetParam()) == "OpenDAQLTStreaming" && (std::get<2>(GetParam()).find("daq.lt://") == 0);
    bool usingNativeStreaming = std::get<1>(GetParam()) == "OpenDAQNativeStreaming";

    for (int i = 0; i < 5; ++i)
    {
        const auto oldValueDataDesc = serverSignalPtr.getDescriptor();
        const auto oldDomainDataDesc = serverSignalPtr.getDomainSignal().getDescriptor();

        const auto valueDataDesc = DataDescriptorBuilderCopy(oldValueDataDesc).setName("test" + std::to_string(i)).build();
        const auto domainDataDesc = DataDescriptorBuilderCopy(oldDomainDataDesc).setName("test_domain" + std::to_string(i)).build();

        serverSignalPtr.setDescriptor(valueDataDesc);
        serverSignalPtr.getDomainSignal().asPtr<ISignalConfig>().setDescriptor(domainDataDesc);

        std::promise<StringPtr> subscribeCompletePromise;
        std::future<StringPtr> subscribeCompleteFuture;
        test_helpers::setupSubscribeAckHandler(subscribeCompletePromise, subscribeCompleteFuture, clientSignalPtr);
        
        std::promise<StringPtr> unsubscribeCompletePromise;
        std::future<StringPtr> unsubscribeCompleteFuture;
        test_helpers::setupUnsubscribeAckHandler(unsubscribeCompletePromise, unsubscribeCompleteFuture, clientSignalPtr);

        std::promise<StringPtr> subscribeDomainCompletePromise;
        std::future<StringPtr> subscribeDomainCompleteFuture;
        
        std::promise<StringPtr> unsubscribeDomainCompletePromise;
        std::future<StringPtr> unsubscribeDomainCompleteFuture;

        if (usingNativeStreaming)
        {
            test_helpers::setupSubscribeAckHandler(subscribeDomainCompletePromise, subscribeDomainCompleteFuture, clientDomainSignalPtr);
            test_helpers::setupUnsubscribeAckHandler(unsubscribeDomainCompletePromise, unsubscribeDomainCompleteFuture, clientDomainSignalPtr);
        }

        auto clientReader = PacketReader(clientSignalPtr);

        ASSERT_TRUE(test_helpers::waitForAcknowledgement(subscribeCompleteFuture));
        if (usingNativeStreaming)
            ASSERT_TRUE(test_helpers::waitForAcknowledgement(subscribeDomainCompleteFuture)); 

        const int packetsToRead = i + 3;
        generatePackets(packetsToRead);

        if (usingNativePseudoDevice)
        {
            auto clientReceivedPackets = test_helpers::tryReadPackets(clientReader, packetsToRead + 2u);
            ASSERT_EQ(clientReceivedPackets.getCount(), packetsToRead + 2u);

            for (int j = 0; j < 2; ++j)
            {
                const auto packet = clientReceivedPackets[j];
                const auto eventPacket = packet.asPtrOrNull<IEventPacket>();
                ASSERT_TRUE(eventPacket.assigned());
                ASSERT_EQ(eventPacket.getEventId(), event_packet_id::DATA_DESCRIPTOR_CHANGED);

                const auto valueDataDescClient = eventPacket.getParameters().get(event_packet_param::DATA_DESCRIPTOR);
                const auto domainDataDescClient = eventPacket.getParameters().get(event_packet_param::DOMAIN_DATA_DESCRIPTOR);

                if (j == 0)
                {
                    EXPECT_EQ(oldValueDataDesc, valueDataDescClient);
                    EXPECT_EQ(oldDomainDataDesc, domainDataDescClient);
                }
                else
                {
                    EXPECT_EQ(valueDataDesc, valueDataDescClient);
                    EXPECT_EQ(domainDataDesc, domainDataDescClient);
                }
            }
        }
        else if (usingWSPseudoDevice)
        {
            auto clientReceivedPackets = test_helpers::tryReadPackets(clientReader, packetsToRead + 3u);
            ASSERT_EQ(clientReceivedPackets.getCount(), packetsToRead + 3u);

            for (int j = 0; j < 3; ++j)
            {
                const auto packet = clientReceivedPackets[j];
                const auto eventPacket = packet.asPtrOrNull<IEventPacket>();
                ASSERT_TRUE(eventPacket.assigned());
                ASSERT_EQ(eventPacket.getEventId(), event_packet_id::DATA_DESCRIPTOR_CHANGED);

                const auto valueDataDescClient = eventPacket.getParameters().get(event_packet_param::DATA_DESCRIPTOR);
                const auto domainDataDescClient = eventPacket.getParameters().get(event_packet_param::DOMAIN_DATA_DESCRIPTOR);

                if (j == 0)
                {
                    EXPECT_EQ(oldValueDataDesc, valueDataDescClient);
                    EXPECT_EQ(oldDomainDataDesc, domainDataDescClient);
                }
                else if (j == 1)
                {
                    EXPECT_EQ(valueDataDesc, valueDataDescClient);
                    EXPECT_EQ(nullptr, domainDataDescClient);
                }
                else
                {
                    EXPECT_EQ(nullptr, valueDataDescClient);
                    EXPECT_EQ(domainDataDesc, domainDataDescClient);
                }
            }
        }
        else
        {
            auto clientReceivedPackets = test_helpers::tryReadPackets(clientReader, packetsToRead + 1u);
            ASSERT_EQ(clientReceivedPackets.getCount(), packetsToRead + 1u);
            const auto packet = clientReceivedPackets[0];
            const auto eventPacket = packet.asPtrOrNull<IEventPacket>();
            ASSERT_TRUE(eventPacket.assigned());
            ASSERT_EQ(eventPacket.getEventId(), event_packet_id::DATA_DESCRIPTOR_CHANGED);

            const auto valueDataDescClient = eventPacket.getParameters().get(event_packet_param::DATA_DESCRIPTOR);
            const auto domainDataDescClient = eventPacket.getParameters().get(event_packet_param::DOMAIN_DATA_DESCRIPTOR);

            EXPECT_EQ(valueDataDesc, valueDataDescClient);
            EXPECT_EQ(domainDataDesc, domainDataDescClient);
        }

        clientReader.release();
        
        ASSERT_TRUE(test_helpers::waitForAcknowledgement(unsubscribeCompleteFuture));
        if (usingNativeStreaming)
        {
            ASSERT_TRUE(test_helpers::waitForAcknowledgement(unsubscribeDomainCompleteFuture));
            IEvent* domainSub = clientDomainSignalPtr.getOnSubscribeComplete();
            IEvent* domainUnSub = clientDomainSignalPtr.getOnUnsubscribeComplete();
            domainSub->clear();
            domainUnSub->clear();
        }

        IEvent* evSub = clientSignalPtr.getOnSubscribeComplete();
        IEvent* evUnsub = clientSignalPtr.getOnUnsubscribeComplete();
        evSub->clear();
        evUnsub->clear();
    }
}

#if defined(OPENDAQ_ENABLE_NATIVE_STREAMING) && defined(OPENDAQ_ENABLE_WEBSOCKET_STREAMING)
INSTANTIATE_TEST_SUITE_P(
    StreamingTestGroup,
    StreamingTest,
    testing::Values(
        std::make_tuple("OpenDAQNativeStreaming", "OpenDAQNativeStreaming", "daq.ns://127.0.0.1/"),
        std::make_tuple("OpenDAQNativeStreaming", "OpenDAQNativeStreaming", "daq.opcua://127.0.0.1/"),
        std::make_tuple("OpenDAQLTStreaming", "OpenDAQLTStreaming", "daq.lt://127.0.0.1/"),
        std::make_tuple("OpenDAQLTStreaming", "OpenDAQLTStreaming", "daq.opcua://127.0.0.1/"),
        std::make_tuple("OpenDAQNativeStreaming", "OpenDAQNativeStreaming", "daq.ns://[::1]/"),
        std::make_tuple("OpenDAQNativeStreaming", "OpenDAQNativeStreaming", "daq.opcua://[::1]/"),
        std::make_tuple("OpenDAQLTStreaming", "OpenDAQLTStreaming", "daq.lt://[::1]/"),
        std::make_tuple("OpenDAQLTStreaming", "OpenDAQLTStreaming", "daq.opcua://[::1]/")
    )
);
#elif defined(OPENDAQ_ENABLE_NATIVE_STREAMING) && !defined(OPENDAQ_ENABLE_WEBSOCKET_STREAMING)
INSTANTIATE_TEST_SUITE_P(
    StreamingTestGroup,
    StreamingTest,
    testing::Values(
        std::make_tuple("OpenDAQNativeStreaming", "OpenDAQNativeStreaming", "daq.ns://127.0.0.1/"),
        std::make_tuple("OpenDAQNativeStreaming", "OpenDAQNativeStreaming", "daq.opcua://127.0.0.1/"),
        std::make_tuple("OpenDAQNativeStreaming", "OpenDAQNativeStreaming", "daq.ns://[::1]/"),
        std::make_tuple("OpenDAQNativeStreaming", "OpenDAQNativeStreaming", "daq.opcua://[::1]/")
    )
);
#elif !defined(OPENDAQ_ENABLE_NATIVE_STREAMING) && defined(OPENDAQ_ENABLE_WEBSOCKET_STREAMING)
INSTANTIATE_TEST_SUITE_P(
    StreamingTestGroup,
    StreamingTest,
    testing::Values(
        std::make_tuple("OpenDAQLTStreaming", "OpenDAQLTStreaming", "daq.lt://127.0.0.1/"),
        std::make_tuple("OpenDAQLTStreaming", "OpenDAQLTStreaming", "daq.opcua://127.0.0.1/"),
        std::make_tuple("OpenDAQLTStreaming", "OpenDAQLTStreaming", "daq.lt://[::1]/"),
        std::make_tuple("OpenDAQLTStreaming", "OpenDAQLTStreaming", "daq.opcua://[::1]/")
    )
);
#endif

#if defined(OPENDAQ_ENABLE_NATIVE_STREAMING)
class StreamingAsyncSignalTest : public StreamingTest
{
protected:
    InstancePtr CreateServerInstance() override
    {
        auto logger = Logger();
        auto scheduler = Scheduler(logger);
        auto moduleManager = ModuleManager("");
        auto typeManager = TypeManager();
        auto authenticationProvider = AuthenticationProvider();
        auto context = Context(scheduler, logger, typeManager, moduleManager, authenticationProvider);

        const ModulePtr deviceModule(MockDeviceModule_Create(context));
        moduleManager.addModule(deviceModule);

        auto instance = InstanceCustom(context, "local");

        const auto mockDevice = instance.addDevice("daqmock://phys_device");

        const auto statisticsFb = instance.addFunctionBlock("RefFBModuleStatistics");
        statisticsFb.setPropertyValue("DomainSignalType", 1);  // 1 - Explicit
        statisticsFb.getInputPorts()[0].connect(getSignal(instance, "ByteStep"));

        auto streamingServer = std::get<0>(GetParam());
        instance.addServer(streamingServer, nullptr);
        // streaming server added first, so registered device streaming options is published over opcua
        instance.addServer("OpenDAQOPCUA", nullptr);

        return instance;
    }
};

TEST_P(StreamingAsyncSignalTest, SigWithExplicitDomain)
{
    const size_t packetsToRead = 10;

    auto mirroredSignalPtr = getSignal(clientInstance, "ByteStep/Avg").template asPtr<IMirroredSignalConfig>();
    std::promise<StringPtr> subscribeCompletePromise;
    std::future<StringPtr> subscribeCompleteFuture;
    test_helpers::setupSubscribeAckHandler(subscribeCompletePromise, subscribeCompleteFuture, mirroredSignalPtr);

    auto serverReader = createServerReader("ByteStep/Avg");
    auto clientReader = createClientReader("ByteStep/Avg");

    ASSERT_TRUE(test_helpers::waitForAcknowledgement(subscribeCompleteFuture));

    generatePackets(packetsToRead);

    auto serverReceivedPackets = test_helpers::tryReadPackets(serverReader, packetsToRead + 1);
    auto clientReceivedPackets = test_helpers::tryReadPackets(clientReader, packetsToRead + 1);

    EXPECT_EQ(serverReceivedPackets.getCount(), packetsToRead + 1);
    EXPECT_EQ(clientReceivedPackets.getCount(), packetsToRead + 1);
    EXPECT_TRUE(test_helpers::packetsEqual(serverReceivedPackets, clientReceivedPackets));
}

INSTANTIATE_TEST_SUITE_P(
    StreamingAsyncSignalTestGroup,
    StreamingAsyncSignalTest,
    testing::Values(
        std::make_tuple("OpenDAQNativeStreaming", "OpenDAQNativeStreaming", "daq.ns://127.0.0.1/"),
        std::make_tuple("OpenDAQNativeStreaming", "OpenDAQNativeStreaming", "daq.opcua://127.0.0.1/"),
        std::make_tuple("OpenDAQNativeStreaming", "OpenDAQNativeStreaming", "daq.ns://[::1]/"),
        std::make_tuple("OpenDAQNativeStreaming", "OpenDAQNativeStreaming", "daq.opcua://[::1]/")
    )
);

class StreamingReconnectionTest : public StreamingTest
{
protected:
    InstancePtr CreateServerInstance() override
    {
        auto logger = Logger();
        auto scheduler = Scheduler(logger);
        auto moduleManager = ModuleManager("");
        auto typeManager = TypeManager();
        auto authenticationProvider = AuthenticationProvider();
        auto context = Context(scheduler, logger, typeManager, moduleManager, authenticationProvider);

        const ModulePtr deviceModule(MockDeviceModule_Create(context));
        moduleManager.addModule(deviceModule);

        auto instance = InstanceCustom(context, "local");

        const auto mockDevice = instance.addDevice("daqmock://phys_device");

        auto streamingServerName = std::get<0>(GetParam());
        streamingServer = instance.addServer(streamingServerName, nullptr);
        // streaming server added first, so registered device streaming options is published over opcua
        instance.addServer("OpenDAQOPCUA", nullptr);

        return instance;
    }

    void removeStreamingServer()
    {
        serverInstance.removeServer(streamingServer);
    }

    void restoreStreamingServer()
    {
        auto streamingServerName = std::get<0>(GetParam());
        streamingServer = serverInstance.addServer(streamingServerName, nullptr);
    }

    ServerPtr streamingServer;
};

TEST_P(StreamingReconnectionTest, Reconnection)
{
    auto mirroredSignalPtr = getSignal(clientInstance, "ByteStep").template asPtr<IMirroredSignalConfig>();
    std::promise<StringPtr> subscribeCompletePromise;
    std::future<StringPtr> subscribeCompleteFuture;

    test_helpers::setupSubscribeAckHandler(subscribeCompletePromise, subscribeCompleteFuture, mirroredSignalPtr);

    auto serverReader = createServerReader("ByteStep");
    auto clientReader = createClientReader("ByteStep");

    ASSERT_TRUE(test_helpers::waitForAcknowledgement(subscribeCompleteFuture));

    // read initial event packets
    auto clientReceivedPackets = test_helpers::tryReadPackets(clientReader, 1);
    EXPECT_EQ(clientReceivedPackets.getCount(), 1u);

    auto serverReceivedPackets = test_helpers::tryReadPackets(serverReader, 1);
    EXPECT_EQ(serverReceivedPackets.getCount(), 1u);

    // remove streaming server to emulate disconnection
    removeStreamingServer();
    // TODO test disconnected status
    subscribeCompletePromise = std::promise<StringPtr>();
    subscribeCompleteFuture = subscribeCompletePromise.get_future();
    // add streaming server back to enable reconnection
    restoreStreamingServer();
    // TODO test reconnected status

    ASSERT_TRUE(test_helpers::waitForAcknowledgement(subscribeCompleteFuture, 5s));
    
    // Expect to receive all data packets,
    const size_t packetsToGenerate = 10;

    generatePackets(packetsToGenerate);

    serverReceivedPackets = test_helpers::tryReadPackets(serverReader, packetsToGenerate);
    clientReceivedPackets = test_helpers::tryReadPackets(clientReader, packetsToGenerate);

    EXPECT_EQ(serverReceivedPackets.getCount(), packetsToGenerate);
    EXPECT_EQ(clientReceivedPackets.getCount(), packetsToGenerate);
    EXPECT_TRUE(test_helpers::packetsEqual(serverReceivedPackets, clientReceivedPackets));
}

INSTANTIATE_TEST_SUITE_P(
    StreamingReconnectionTestGroup,
    StreamingReconnectionTest,
    testing::Values(
        std::make_tuple("OpenDAQNativeStreaming", "OpenDAQNativeStreaming", "daq.ns://127.0.0.1/"),
        std::make_tuple("OpenDAQNativeStreaming", "OpenDAQNativeStreaming", "daq.opcua://127.0.0.1/"),
        std::make_tuple("OpenDAQNativeStreaming", "OpenDAQNativeStreaming", "daq.ns://[::1]/"),
        std::make_tuple("OpenDAQNativeStreaming", "OpenDAQNativeStreaming", "daq.opcua://[::1]/")
    )
);

class NativeDeviceStreamingTest : public testing::Test
{};

TEST_F(NativeDeviceStreamingTest, ChangedDataDescriptorBeforeSubscribeNativeDevice)
{
    SKIP_TEST_MAC_CI;
    const auto moduleManager = ModuleManager("");
    auto serverInstance = InstanceBuilder().setModuleManager(moduleManager).build();
    const ModulePtr deviceModule(MockDeviceModule_Create(serverInstance.getContext()));
    moduleManager.addModule(deviceModule);
    serverInstance.setRootDevice("daqmock://phys_device");
    serverInstance.addServer("OpenDAQNativeStreaming", nullptr);

    const auto channels = serverInstance.getChannelsRecursive();
    Int sigCount = 0;
    for (const auto& ch : channels)
        sigCount += ch.getSignalsRecursive().getCount();

    const auto clientInstance = Instance();
    clientInstance.addDevice("daq.nd://127.0.0.1");


    int callCount = 0;
    clientInstance.getContext().getOnCoreEvent() +=
        [&](const ComponentPtr& /*comp*/, const CoreEventArgsPtr& args)
        {
            if (args.getEventId() == static_cast<Int>(CoreEventId::DataDescriptorChanged))  
                callCount++;
        };

    SignalConfigPtr serverSignalPtr = serverInstance.getSignalsRecursive(search::LocalId("ByteStep"))[0];
    MirroredSignalConfigPtr clientSignalPtr = clientInstance.getSignalsRecursive(search::LocalId("ByteStep"))[0];
    MirroredSignalConfigPtr clientDomainSignal = clientSignalPtr.getDomainSignal();

    for (int i = 1; i < 5; ++i)
    {
        callCount = 0;
        clientInstance.getDevices()[0].setPropertyValue("ChangeDescriptors", i % 2);
        ASSERT_EQ(callCount, sigCount);
            
        const DataDescriptorPtr valueDataDesc = serverSignalPtr.getDescriptor();
        const DataDescriptorPtr domainDataDesc = serverSignalPtr.getDomainSignal().getDescriptor();

        std::promise<StringPtr> subscribeCompletePromise;
        std::future<StringPtr> subscribeCompleteFuture;
        test_helpers::setupSubscribeAckHandler(subscribeCompletePromise, subscribeCompleteFuture, clientSignalPtr);
        
        std::promise<StringPtr> subscribeDomainCompletePromise;
        std::future<StringPtr> subscribeDomainCompleteFuture;
        test_helpers::setupSubscribeAckHandler(subscribeDomainCompletePromise, subscribeDomainCompleteFuture, clientDomainSignal);

        std::promise<StringPtr> unsubscribeCompletePromise;
        std::future<StringPtr> unsubscribeCompleteFuture;
        test_helpers::setupUnsubscribeAckHandler(unsubscribeCompletePromise, unsubscribeCompleteFuture, clientSignalPtr);
        
        std::promise<StringPtr> unsubscribeDomainCompletePromise;
        std::future<StringPtr> unsubscribeDomainCompleteFuture;
        test_helpers::setupUnsubscribeAckHandler(unsubscribeDomainCompletePromise, unsubscribeDomainCompleteFuture, clientDomainSignal);

        auto clientReader = PacketReader(clientSignalPtr);
        
        ASSERT_TRUE(test_helpers::waitForAcknowledgement(subscribeCompleteFuture));
        ASSERT_TRUE(test_helpers::waitForAcknowledgement(subscribeDomainCompleteFuture));

        const int packetsToRead = i + 3;
        serverInstance.setPropertyValue("GeneratePackets", packetsToRead);

        auto clientReceivedPackets = test_helpers::tryReadPackets(clientReader, packetsToRead + 1u);
        ASSERT_EQ(clientReceivedPackets.getCount(), packetsToRead + 1u);
        const auto packet = clientReceivedPackets[0];
        const auto eventPacket = packet.asPtrOrNull<IEventPacket>();
        ASSERT_TRUE(eventPacket.assigned());
        ASSERT_EQ(eventPacket.getEventId(), event_packet_id::DATA_DESCRIPTOR_CHANGED);

        const auto valueDataDescClient = eventPacket.getParameters().get(event_packet_param::DATA_DESCRIPTOR);
        const auto domainDataDescClient = eventPacket.getParameters().get(event_packet_param::DOMAIN_DATA_DESCRIPTOR);

        EXPECT_EQ(valueDataDesc, valueDataDescClient);
        EXPECT_EQ(domainDataDesc, domainDataDescClient);

        clientReader.release();

        test_helpers::waitForAcknowledgement(unsubscribeCompleteFuture);
        test_helpers::waitForAcknowledgement(unsubscribeDomainCompleteFuture);

        
        IEvent* evSub = clientSignalPtr.getOnSubscribeComplete();
        IEvent* evSubDomain = clientDomainSignal.getOnSubscribeComplete();
        IEvent* evUnsub = clientSignalPtr.getOnUnsubscribeComplete();
        IEvent* evUnsubDomain = clientDomainSignal.getOnUnsubscribeComplete();
        evSub->clear();
        evSubDomain->clear();
        evUnsub->clear();
        evUnsubDomain->clear();
    }
}

#endif
