#include "test_helpers.h"
#include <opendaq/event_packet_params.h>
#include <opendaq/mock/mock_device_module.h>
#include <opendaq/custom_log.h>

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
        serverInstance = CreateServerInstance();
        clientInstance = CreateClientInstance();

        logger = Logger();
        loggerComponent = logger.getOrAddComponent("StreamingTest");
    }

    void TearDown() override
    {
    }

    void generatePackets(size_t packetCount)
    {
        auto devices = serverInstance.getDevices();

        for (const auto& device : devices)
        {
            auto name = device.getInfo().getName();
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

    static ListPtr<IPacket> tryReadPackets(const PacketReaderPtr& reader,
                                    size_t packetCount,
                                    std::chrono::seconds timeout = std::chrono::seconds(60))
    {
        auto allPackets = List<IPacket>();
        auto startPoint = std::chrono::system_clock::now();

        while (allPackets.getCount() < packetCount)
        {
            if (reader.getAvailableCount() == 0)
            {
                auto now = std::chrono::system_clock::now();
                auto timeElapsed = now - startPoint;
                if (timeElapsed > timeout)
                {
                    break;
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(20));
                continue;
            }

            auto packets = reader.readAll();

            for (const auto& packet : packets)
                allPackets.pushBack(packet);
        }

        return allPackets;
    }

    bool packetsEqual(const ListPtr<IPacket>& listA, const ListPtr<IPacket>& listB, bool skipEventPackets = false)
    {
        bool result = true;
        if (listA.getCount() != listB.getCount())
        {
            LOG_E("Compared packets count differs: A {}, B {}", listA.getCount(), listB.getCount());
            result = false;
        }

        auto count = std::min(listA.getCount(), listB.getCount());

        for (SizeT i = 0; i < count; i++)
        {
            if (listA.getItemAt(i).getType() == PacketType::Event &&
                listB.getItemAt(i).getType() == PacketType::Event)
            {
                if (skipEventPackets)
                    continue;
                auto eventPacketA = listA.getItemAt(i).asPtr<IEventPacket>(true);
                auto eventPacketB = listB.getItemAt(i).asPtr<IEventPacket>(true);

                if (eventPacketA.getEventId() != eventPacketB.getEventId())
                {
                    LOG_E("Event id of packets at index {} differs: A - \"{}\", B - \"{}\"",
                          i, eventPacketA.getEventId(), eventPacketB.getEventId());
                }
                else if(eventPacketA.getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED &&
                         eventPacketB.getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED)
                {
                    const DataDescriptorPtr valueDataDescA = eventPacketA.getParameters().get(event_packet_param::DATA_DESCRIPTOR);
                    const DataDescriptorPtr domainDataDescA = eventPacketA.getParameters().get(event_packet_param::DOMAIN_DATA_DESCRIPTOR);

                    const DataDescriptorPtr valueDataDescB = eventPacketB.getParameters().get(event_packet_param::DATA_DESCRIPTOR);
                    const DataDescriptorPtr domainDataDescB = eventPacketB.getParameters().get(event_packet_param::DOMAIN_DATA_DESCRIPTOR);

                    LOG_E("Event parameters of packets at index {} differs:", i);
                    LOG_E("packet A - \nvalue:\n\"{}\"\ndomain:\n\"{}\"",
                          valueDataDescA.assigned() ? valueDataDescA.toString() : "null",
                          domainDataDescA.assigned() ? domainDataDescA.toString() : "null");
                    LOG_E("packet B - \nvalue:\n\"{}\"\ndomain:\n\"{}\"",
                          valueDataDescB.assigned() ? valueDataDescB.toString() : "null",
                          domainDataDescB.assigned() ? domainDataDescB.toString() : "null");
                }
                else
                {
                    LOG_E("Event packets at index {} differs: A - \"{}\", B - \"{}\"",
                          i, listA.getItemAt(i).toString(), listB.getItemAt(i).toString());
                }
            }
            else
            {
                LOG_E("Data packets at index {} differs: A - \"{}\", B - \"{}\"",
                      i, listA.getItemAt(i).toString(), listB.getItemAt(i).toString());
            }
            result = false;
        }

        return result;
    }

protected:
    virtual InstancePtr CreateServerInstance()
    {
        auto logger = Logger();
        auto scheduler = Scheduler(logger);
        auto moduleManager = ModuleManager("");
        auto typeManager = TypeManager();
        auto context = Context(scheduler, logger, typeManager, moduleManager);

        const ModulePtr deviceModule(MockDeviceModule_Create(context));
        moduleManager.addModule(deviceModule);

        auto instance = InstanceCustom(context, "local");

        const auto mockDevice = instance.addDevice("mock_phys_device");

        auto streamingServer = std::get<0>(GetParam());
        instance.addServer(streamingServer, nullptr);
        // streaming server added first, so registered device streaming options is published over opcua
        instance.addServer("openDAQ OpcUa", nullptr);

        return instance;
    }

    InstancePtr CreateClientInstance()
    {
        auto instance = Instance();
        auto connectionString = std::get<2>(GetParam());
        if (connectionString.find("daq.opcua") == 0)
        {
            auto config = instance.getAvailableDeviceTypes().get("opendaq_opcua_config").createDefaultConfig();
            config.setPropertyValue("AllowedStreamingProtocols", List<IString>("opendaq_native_streaming", "opendaq_lt_streaming"));
            config.setPropertyValue("PrimaryStreamingProtocol", std::get<1>(GetParam()));
            auto device = instance.addDevice(connectionString, config);
        }
        else
        {
            auto device = instance.addDevice(connectionString);
        }
        return instance;
    }

    InstancePtr serverInstance;
    InstancePtr clientInstance;
    LoggerPtr logger;
    LoggerComponentPtr loggerComponent;
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
    auto serverReceivedPackets = tryReadPackets(serverReader, packetsToRead);
    auto clientReceivedPackets = tryReadPackets(clientReader, packetsToRead);
    EXPECT_EQ(serverReceivedPackets.getCount(), packetsToRead);
    EXPECT_EQ(clientReceivedPackets.getCount(), packetsToRead);
    EXPECT_TRUE(packetsEqual(serverReceivedPackets,
                             clientReceivedPackets,
                             std::get<0>(GetParam()) == "openDAQ LT Streaming"));

    // recreate client reader and test initial event packet
    clientReader = createClientReader(clientSignal.getDescriptor().getName());
    clientReceivedPackets = tryReadPackets(clientReader, 1);

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

    auto serverReceivedPackets = tryReadPackets(serverReader, packetsToRead);
    auto clientReceivedPackets = tryReadPackets(clientReader, packetsToRead);

    EXPECT_EQ(serverReceivedPackets.getCount(), packetsToRead);
    EXPECT_EQ(clientReceivedPackets.getCount(), packetsToRead);
    EXPECT_TRUE(packetsEqual(serverReceivedPackets, clientReceivedPackets));
}

TEST_P(StreamingTest, ChangedDataDescriptorBeforeSubscribe)
{
    SKIP_TEST_MAC_CI;
    SignalConfigPtr serverSignalPtr = getSignal(serverInstance, "ByteStep");
    MirroredSignalConfigPtr clientSignalPtr = getSignal(clientInstance, "ByteStep");
    MirroredSignalConfigPtr clientDomainSignalPtr = clientSignalPtr.getDomainSignal();

    bool usingNativePseudoDevice = std::get<1>(GetParam()) == "opendaq_native_streaming" && std::get<2>(GetParam()) == "daq.ns://127.0.0.1/";
    bool usingWSPseudoDevice = std::get<1>(GetParam()) == "opendaq_lt_streaming" && std::get<2>(GetParam()) == "daq.lt://127.0.0.1/";
    bool usingNativeStreaming = std::get<1>(GetParam()) == "opendaq_native_streaming";

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
            auto clientReceivedPackets = tryReadPackets(clientReader, packetsToRead + 2);
            ASSERT_EQ(clientReceivedPackets.getCount(), packetsToRead + 2);

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
            auto clientReceivedPackets = tryReadPackets(clientReader, packetsToRead + 3);
            ASSERT_EQ(clientReceivedPackets.getCount(), packetsToRead + 3);

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
            auto clientReceivedPackets = tryReadPackets(clientReader, packetsToRead + 1);
            ASSERT_EQ(clientReceivedPackets.getCount(), packetsToRead + 1);
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
        std::make_tuple("openDAQ Native Streaming", "opendaq_native_streaming", "daq.ns://127.0.0.1/"),
        std::make_tuple("openDAQ Native Streaming", "opendaq_native_streaming", "daq.opcua://127.0.0.1/"),
        std::make_tuple("openDAQ LT Streaming", "opendaq_lt_streaming", "daq.lt://127.0.0.1/"),
        std::make_tuple("openDAQ LT Streaming", "opendaq_lt_streaming", "daq.opcua://127.0.0.1/")
    )
);
#elif defined(OPENDAQ_ENABLE_NATIVE_STREAMING) && !defined(OPENDAQ_ENABLE_WEBSOCKET_STREAMING)
INSTANTIATE_TEST_SUITE_P(
    StreamingTestGroup,
    StreamingTest,
    testing::Values(
        std::make_tuple("openDAQ Native Streaming", "opendaq_native_streaming", "daq.ns://127.0.0.1/"),
        std::make_tuple("openDAQ Native Streaming", "opendaq_native_streaming", "daq.opcua://127.0.0.1/")
    )
);
#elif !defined(OPENDAQ_ENABLE_NATIVE_STREAMING) && defined(OPENDAQ_ENABLE_WEBSOCKET_STREAMING)
INSTANTIATE_TEST_SUITE_P(
    StreamingTestGroup,
    StreamingTest,
    testing::Values(
        std::make_tuple("openDAQ LT Streaming", "opendaq_lt_streaming", "daq.lt://127.0.0.1/"),
        std::make_tuple("openDAQ LT Streaming", "opendaq_lt_streaming", "daq.opcua://127.0.0.1/")
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
        auto context = Context(scheduler, logger, typeManager, moduleManager);

        const ModulePtr deviceModule(MockDeviceModule_Create(context));
        moduleManager.addModule(deviceModule);

        auto instance = InstanceCustom(context, "local");

        const auto mockDevice = instance.addDevice("mock_phys_device");

        const auto statisticsFb = instance.addFunctionBlock("ref_fb_module_statistics");
        statisticsFb.setPropertyValue("DomainSignalType", 1);  // 1 - Explicit
        statisticsFb.getInputPorts()[0].connect(getSignal(instance, "ByteStep"));

        auto streamingServer = std::get<0>(GetParam());
        instance.addServer(streamingServer, nullptr);
        // streaming server added first, so registered device streaming options is published over opcua
        instance.addServer("openDAQ OpcUa", nullptr);

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

    auto serverReceivedPackets = tryReadPackets(serverReader, packetsToRead + 1);
    auto clientReceivedPackets = tryReadPackets(clientReader, packetsToRead + 1);

    EXPECT_EQ(serverReceivedPackets.getCount(), packetsToRead + 1);
    EXPECT_EQ(clientReceivedPackets.getCount(), packetsToRead + 1);
    EXPECT_TRUE(packetsEqual(serverReceivedPackets, clientReceivedPackets));
}

INSTANTIATE_TEST_SUITE_P(
    StreamingAsyncSignalTestGroup,
    StreamingAsyncSignalTest,
    testing::Values(
        std::make_tuple("openDAQ Native Streaming", "opendaq_native_streaming", "daq.ns://127.0.0.1/"),
        std::make_tuple("openDAQ Native Streaming", "opendaq_native_streaming", "daq.opcua://127.0.0.1/")
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
        auto context = Context(scheduler, logger, typeManager, moduleManager);

        const ModulePtr deviceModule(MockDeviceModule_Create(context));
        moduleManager.addModule(deviceModule);

        auto instance = InstanceCustom(context, "local");

        const auto mockDevice = instance.addDevice("mock_phys_device");

        auto streamingServerName = std::get<0>(GetParam());
        streamingServer = instance.addServer(streamingServerName, nullptr);
        // streaming server added first, so registered device streaming options is published over opcua
        instance.addServer("openDAQ OpcUa", nullptr);

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
    auto clientReceivedPackets = tryReadPackets(clientReader, 1);
    EXPECT_EQ(clientReceivedPackets.getCount(), 1u);

    auto serverReceivedPackets = tryReadPackets(serverReader, 1);
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

    serverReceivedPackets = tryReadPackets(serverReader, packetsToGenerate);
    clientReceivedPackets = tryReadPackets(clientReader, packetsToGenerate);

    EXPECT_EQ(serverReceivedPackets.getCount(), packetsToGenerate);
    EXPECT_EQ(clientReceivedPackets.getCount(), packetsToGenerate);
    EXPECT_TRUE(packetsEqual(serverReceivedPackets, clientReceivedPackets));
}

INSTANTIATE_TEST_SUITE_P(
    StreamingReconnectionTestGroup,
    StreamingReconnectionTest,
    testing::Values(
        std::make_tuple("openDAQ Native Streaming", "opendaq_native_streaming", "daq.ns://127.0.0.1/"),
        std::make_tuple("openDAQ Native Streaming", "opendaq_native_streaming", "daq.opcua://127.0.0.1/")
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
    serverInstance.setRootDevice("mock_phys_device");
    serverInstance.addServer("openDAQ Native Streaming", nullptr);

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

        auto clientReceivedPackets = StreamingTest::tryReadPackets(clientReader, packetsToRead + 1);
        ASSERT_EQ(clientReceivedPackets.getCount(), packetsToRead + 1);
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
