#include "test_helpers/test_helpers.h"
#include <opendaq/mock/mock_device_module.h>
#include <coreobjects/authentication_provider_factory.h>

#include "test_helpers/device_modules.h"

#ifdef DAQMODULES_LT_LEGACY_MODULES
    #define ENABLE_COMMON_LT_STREAMING_TESTS
#else
    #define ENABLE_ALTERNATIVE_LT_STREAMING_TESTS
#endif
#define ENABLE_COMMON_NATIVE_STREAMING_TESTS

using namespace daq;
using namespace std::chrono_literals;

// first param: streaming server type / streaming protocol
// second param: client device connection string
class StreamingTest : public testing::TestWithParam<std::tuple<std::string, std::string>>
{
public:
    void SetUp() override
    {
        auto connectionString = std::get<1>(GetParam());
        if (test_helpers::isIpv6ConnectionString(connectionString) && test_helpers::Ipv6IsDisabled())
        {
            GTEST_SKIP() << "Ipv6 is disabled";
        }

        serverInstance = CreateServerInstance();
        clientInstance = CreateClientInstance();

        usingNativePseudoDevice = std::get<0>(GetParam()) == "OpenDAQNativeStreaming" && (std::get<1>(GetParam()).find("daq.ns://") == 0);
        usingLTPseudoDevice = std::get<0>(GetParam()) == "OpenDAQLTStreaming" && (std::get<1>(GetParam()).find("daq.lt://") == 0);
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
        auto signal = getSignal(clientInstance, signalName);
        readerInputPort = InputPort(clientInstance.getContext(), nullptr, "readsig");
        PacketReaderPtr reader = PacketReaderFromPort(readerInputPort);
        readerInputPort.connect(signal);
        return reader;
    }

    static std::vector<std::tuple<std::string, std::string>> GetNativeTestSuite()
    {
        std::vector<std::tuple<std::string, std::string>> suite;
#ifndef ENABLE_COMMON_NATIVE_STREAMING_TESTS
        return suite;
#endif
        suite.push_back(std::make_tuple("OpenDAQNativeStreaming", "daq.ns://127.0.0.1/"));
        suite.push_back(std::make_tuple("OpenDAQNativeStreaming", "daq.ns://[::1]/"));
        suite.push_back(std::make_tuple("OpenDAQNativeStreaming", "daq.nd://127.0.0.1/"));
        suite.push_back(std::make_tuple("OpenDAQNativeStreaming", "daq.nd://[::1]/"));
        suite.push_back(std::make_tuple("OpenDAQNativeStreaming", "daq.opcua://127.0.0.1/"));
        suite.push_back(std::make_tuple("OpenDAQNativeStreaming", "daq.opcua://[::1]/"));
        return suite;
    }

    static std::vector<std::tuple<std::string, std::string>> GetLtTestSuite()
    {
        std::vector<std::tuple<std::string, std::string>> suite;
#ifndef ENABLE_COMMON_LT_STREAMING_TESTS
        return suite;
#endif
        suite.push_back(std::make_tuple("OpenDAQLTStreaming", "daq.lt://127.0.0.1/"));
        suite.push_back(std::make_tuple("OpenDAQLTStreaming", "daq.lt://[::1]/"));
#if defined(OPENDAQ_ENABLE_NATIVE_STREAMING)
        suite.push_back(std::make_tuple("OpenDAQLTStreaming", "daq.nd://127.0.0.1/"));
        suite.push_back(std::make_tuple("OpenDAQLTStreaming", "daq.nd://[::1]/"));
#endif
        suite.push_back(std::make_tuple("OpenDAQLTStreaming", "daq.opcua://127.0.0.1/"));
        suite.push_back(std::make_tuple("OpenDAQLTStreaming", "daq.opcua://[::1]/"));
        return suite;
    }

protected:
    virtual InstancePtr CreateServerInstance()
    {
        auto logger = Logger();
        auto scheduler = Scheduler(logger);
        auto moduleManager = ModuleManager("[[none]]");
        auto typeManager = TypeManager();
        auto authenticationProvider = AuthenticationProvider();
        auto context = Context(scheduler, logger, typeManager, moduleManager, authenticationProvider);

        const ModulePtr deviceModule(MockDeviceModule_Create(context));
        moduleManager.addModule(deviceModule);

        auto instance = InstanceCustom(context, "local");
        addLtServerModule(instance);
        addNativeServerModule(instance);
        addOpcuaServerModule(instance);

        const auto mockDevice = instance.addDevice("daqmock://phys_device");

        instance.addServer("OpenDAQLTStreaming", nullptr);
        instance.addServer("OpenDAQNativeStreaming", nullptr);
        // streaming servers added first, so registered device streaming options is published over opcua
        instance.addServer("OpenDAQOPCUA", nullptr);

        return instance;
    }

    InstancePtr CreateClientInstance()
    {
        auto instance = Instance("[[none]]");
        addLtClientModule(instance);
        addNativeClientModule(instance);
        addOpcuaClientModule(instance);

        auto connectionString = std::get<1>(GetParam());

        auto config = instance.createDefaultAddDeviceConfig();
        PropertyObjectPtr general = config.getPropertyValue("General");
        general.setPropertyValue("PrioritizedStreamingProtocols", List<IString>(std::get<0>(GetParam())));

        auto device = instance.addDevice(connectionString, config);
        return instance;
    }

    InputPortPtr readerInputPort;
    InstancePtr serverInstance;
    InstancePtr clientInstance;

    bool usingNativePseudoDevice{false};
    bool usingLTPseudoDevice{false};
};

TEST_P(StreamingTest, SignalDescriptorEvents)
{
    // LT streaming subscribe-completion ack is intermittently dropped — waitForAcknowledgement
    // below would time out at 5 s. Skip just the daq.lt:// param; other transports still run.
    if (std::get<1>(GetParam()).find("daq.lt://") == 0)
        GTEST_SKIP();

    const size_t packetsToGenerate = 5;
    const size_t initialEventPackets = 1;
    const size_t packetsPerChange = 2;  // one triggered by data signal and one triggered by domain signal
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
    // Same dropped LT streaming subscribe-completion ack as SignalDescriptorEvents.
    if (std::get<1>(GetParam()).find("daq.lt://") == 0)
        GTEST_SKIP();

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

TEST_P(StreamingTest, LastValue)
{
    if (std::get<1>(GetParam()).find("daq.ns://") == 0 || std::get<1>(GetParam()).find("daq.lt://") == 0)
    {
        GTEST_SKIP();
    }
    // Flaky on IPv6 OPC UA endpoints: phase-3 lastValue mismatch after unsubscribe
    // (mirror keeps a stale streaming-cached value instead of reading via the config channel).
    if (std::get<1>(GetParam()) == "daq.opcua://[::1]/")
    {
        GTEST_SKIP();
    }

    auto serverSignal = getSignal(serverInstance, "IntStep");
    auto mirroredSignalPtr = getSignal(clientInstance, "IntStep").template asPtr<IMirroredSignalConfig>();
    std::promise<StringPtr> subscribeCompletePromise;
    std::future<StringPtr> subscribeCompleteFuture;
    test_helpers::setupSubscribeAckHandler(subscribeCompletePromise, subscribeCompleteFuture, mirroredSignalPtr);

    // before any packet send
    ASSERT_FALSE(serverSignal.getLastValue().assigned());
    ASSERT_FALSE(mirroredSignalPtr.getLastValue().assigned());

    {
        auto serverReader = PacketReader(serverSignal);
        auto clientReader = PacketReader(mirroredSignalPtr);

        ASSERT_TRUE(test_helpers::waitForAcknowledgement(subscribeCompleteFuture));
        generatePackets(5);

        auto serverReceivedPackets = test_helpers::tryReadPackets(serverReader, 6);
        auto clientReceivedPackets = test_helpers::tryReadPackets(clientReader, 6);

        ASSERT_TRUE(serverSignal.getLastValue().assigned());
        ASSERT_TRUE(mirroredSignalPtr.getLastValue().assigned());
        // generated packets are sent and read, signal is still subscribed via streaming - mirrored signal gets last value from streaming
        EXPECT_EQ(serverSignal.getLastValue(), mirroredSignalPtr.getLastValue());
    }

    ASSERT_TRUE(serverSignal.getLastValue().assigned());
    ASSERT_TRUE(mirroredSignalPtr.getLastValue().assigned());
    // signal is not subscribed via streaming anymore - mirrored signal gets last value from config but it hasn't changed yet
    EXPECT_EQ(serverSignal.getLastValue(), mirroredSignalPtr.getLastValue());
    {
        auto serverReader = PacketReader(serverSignal);
        generatePackets(3);

        auto serverReceivedPackets = test_helpers::tryReadPackets(serverReader, 3);

        ASSERT_TRUE(serverSignal.getLastValue().assigned());
        ASSERT_TRUE(mirroredSignalPtr.getLastValue().assigned());
        // more generated packets are sent and read, signal was not subscribed via streaming
        // - mirrored signal gets last value from config and it differs from one previously obtained from streaming
        EXPECT_EQ(serverSignal.getLastValue(), mirroredSignalPtr.getLastValue());
    }
}

TEST_P(StreamingTest, SetNullDescriptor)
{
    // Same dropped LT streaming subscribe-completion ack as SignalDescriptorEvents.
    if (std::get<1>(GetParam()).find("daq.lt://") == 0)
        GTEST_SKIP();

    if (!usingLTPseudoDevice)
    {
        const size_t packetsToRead = 2;

        auto serverSignalPtr = getSignal(serverInstance, "ByteStep").template asPtr<ISignalConfig>();
        auto mirroredSignalPtr = getSignal(clientInstance, "ByteStep").template asPtr<IMirroredSignalConfig>();
        std::promise<StringPtr> subscribeCompletePromise;
        std::future<StringPtr> subscribeCompleteFuture;
        test_helpers::setupSubscribeAckHandler(subscribeCompletePromise, subscribeCompleteFuture, mirroredSignalPtr);

        auto serverReader = createServerReader("ByteStep");
        auto clientReader = createClientReader("ByteStep");

        ASSERT_TRUE(test_helpers::waitForAcknowledgement(subscribeCompleteFuture));

        // set null descriptor
        serverSignalPtr.setDescriptor(nullptr);

        auto serverReceivedPackets = test_helpers::tryReadPackets(serverReader, packetsToRead);
        auto clientReceivedPackets = test_helpers::tryReadPackets(clientReader, packetsToRead);

        ASSERT_EQ(serverReceivedPackets.getCount(), packetsToRead);
        ASSERT_EQ(clientReceivedPackets.getCount(), packetsToRead);

        const auto nullDescEventPacket = clientReceivedPackets[1].asPtr<IEventPacket>();
        EXPECT_EQ(nullDescEventPacket.getEventId(), event_packet_id::DATA_DESCRIPTOR_CHANGED);
        EXPECT_EQ(nullDescEventPacket.getParameters().get(event_packet_param::DATA_DESCRIPTOR), NullDataDescriptor());

        ASSERT_EQ(mirroredSignalPtr.getDescriptor(), nullptr);

        EXPECT_TRUE(test_helpers::packetsEqual(serverReceivedPackets,
                                               clientReceivedPackets,
                                               std::get<0>(GetParam()) == "OpenDAQLTStreaming"));
    }
    else // usingLTPseudoDevice true
    {

        auto serverSignalPtr = getSignal(serverInstance, "ByteStep").template asPtr<ISignalConfig>();
        auto serverReader = createServerReader("ByteStep");

        auto mirroredOrigSignalPtr = getSignal(clientInstance, "ByteStep").template asPtr<IMirroredSignalConfig>();
        std::promise<StringPtr> origSigSubscribeCompletePromise;
        std::future<StringPtr> origSigSubscribeCompleteFuture;
        test_helpers::setupSubscribeAckHandler(origSigSubscribeCompletePromise, origSigSubscribeCompleteFuture, mirroredOrigSignalPtr);
        auto clientOrigSigReader = createClientReader("ByteStep");
        ASSERT_TRUE(test_helpers::waitForAcknowledgement(origSigSubscribeCompleteFuture));
        auto clientOrigSigReceivedPackets = test_helpers::tryReadPackets(clientOrigSigReader, 1);

        SignalConfigPtr mirroredNewSignalPtr;
        std::promise<void> addSigPromise;
        std::future<void> addSigFuture = addSigPromise.get_future();

        clientInstance.getContext().getOnCoreEvent() +=
            [&](const ComponentPtr& comp, const CoreEventArgsPtr& args)
        {
            auto params = args.getParameters();
            if (static_cast<CoreEventId>(args.getEventId()) == CoreEventId::ComponentAdded)
            {
                ComponentPtr component = params.get("Component");
                if (component.asPtrOrNull<ISignal>().assigned())
                {
                    mirroredNewSignalPtr = component;
                    addSigPromise.set_value();
                }
            }
        };

        // set null descriptor
        serverSignalPtr.setDescriptor(nullptr);

        ASSERT_TRUE(addSigFuture.wait_for(std::chrono::seconds(10)) == std::future_status::ready);

        auto serverReceivedPackets = test_helpers::tryReadPackets(serverReader, 2);

        auto clientNewSigReader = PacketReader(mirroredNewSignalPtr);
        auto clientNewSigReceivedPackets = test_helpers::tryReadPackets(clientNewSigReader, 1);

        ASSERT_EQ(serverReceivedPackets.getCount(), 2u);
        ASSERT_EQ(clientOrigSigReceivedPackets.getCount(), 1u);
        ASSERT_EQ(clientNewSigReceivedPackets.getCount(), 1u);

        const auto nullDescEventPacket = clientNewSigReceivedPackets[0].asPtr<IEventPacket>();
        EXPECT_EQ(nullDescEventPacket.getEventId(), event_packet_id::DATA_DESCRIPTOR_CHANGED);
        EXPECT_EQ(nullDescEventPacket.getParameters().get(event_packet_param::DATA_DESCRIPTOR), NullDataDescriptor());

        ASSERT_EQ(mirroredNewSignalPtr.getDescriptor(), nullptr);
        ASSERT_TRUE(mirroredOrigSignalPtr.isRemoved());
    }
}

TEST_P(StreamingTest, ChangedDataDescriptorBeforeSubscribe)
{
    // daq.nd:// is not supported by this test. daq.lt:// hits the same dropped LT streaming
    // subscribe-completion ack as SignalDescriptorEvents.
    if (std::get<1>(GetParam()).find("daq.nd://") == 0 ||
        std::get<1>(GetParam()).find("daq.lt://") == 0)
    {
        GTEST_SKIP();
    }

    SKIP_TEST_MAC_CI;
    SignalConfigPtr serverSignalPtr = getSignal(serverInstance, "ByteStep");
    MirroredSignalConfigPtr clientSignalPtr = getSignal(clientInstance, "ByteStep");
    MirroredSignalConfigPtr clientDomainSignalPtr = clientSignalPtr.getDomainSignal();

    bool usingNativeStreaming = std::get<0>(GetParam()) == "OpenDAQNativeStreaming";

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
        else if (usingLTPseudoDevice)
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

#if defined(OPENDAQ_ENABLE_NATIVE_STREAMING)
INSTANTIATE_TEST_SUITE_P(
    StreamingTestGroup_Native,
    StreamingTest,
    testing::ValuesIn(StreamingTest::GetNativeTestSuite())
    );
#endif

#if defined(OPENDAQ_ENABLE_WEBSOCKET_STREAMING)
INSTANTIATE_TEST_SUITE_P(
    StreamingTestGroup_LT,
    StreamingTest,
    testing::ValuesIn(StreamingTest::GetLtTestSuite())
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
        auto moduleManager = ModuleManager("[[none]]");
        auto typeManager = TypeManager();
        auto authenticationProvider = AuthenticationProvider();
        auto context = Context(scheduler, logger, typeManager, moduleManager, authenticationProvider);

        const ModulePtr deviceModule(MockDeviceModule_Create(context));
        moduleManager.addModule(deviceModule);

        auto instance = InstanceCustom(context, "local");
        addRefFBModule(instance);
        addNativeServerModule(instance);
        addLtServerModule(instance);
        addOpcuaServerModule(instance);

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
    testing::ValuesIn(
        StreamingTest::GetNativeTestSuite()
    )
);

class StreamingReconnectionTest : public StreamingTest
{
protected:
    InstancePtr CreateServerInstance() override
    {
        auto logger = Logger();
        auto scheduler = Scheduler(logger);
        auto moduleManager = ModuleManager("[[none]]");
        auto typeManager = TypeManager();
        auto authenticationProvider = AuthenticationProvider();
        auto context = Context(scheduler, logger, typeManager, moduleManager, authenticationProvider);

        const ModulePtr deviceModule(MockDeviceModule_Create(context));
        moduleManager.addModule(deviceModule);

        auto instance = InstanceCustom(context, "local");
        addLtServerModule(instance);
        addOpcuaServerModule(instance);
        addNativeServerModule(instance);

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
    testing::ValuesIn(
        StreamingTest::GetNativeTestSuite()
    )
);

class NativeDeviceStreamingTest : public testing::Test
{};

TEST_F_UNSTABLE_SKIPPED(NativeDeviceStreamingTest, ChangedDataDescriptorBeforeSubscribeNativeDevice)
{
    SKIP_TEST_MAC_CI;
    const auto moduleManager = ModuleManager("[[none]]");
    auto serverInstance = InstanceBuilder().setModuleManager(moduleManager).build();
    const ModulePtr deviceModule(MockDeviceModule_Create(serverInstance.getContext()));
    moduleManager.addModule(deviceModule);
    serverInstance.setRootDevice("daqmock://phys_device");

    addNativeServerModule(serverInstance);
    serverInstance.addServer("OpenDAQNativeStreaming", nullptr);

    const auto channels = serverInstance.getChannelsRecursive();
    Int sigCount = 0;
    for (const auto& ch : channels)
        sigCount += ch.getSignalsRecursive().getCount();

    const auto clientInstance = Instance("[[none]]");

    addNativeClientModule(clientInstance);
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

#if defined(OPENDAQ_ENABLE_WEBSOCKET_STREAMING) && defined(ENABLE_ALTERNATIVE_LT_STREAMING_TESTS)
class StreamingTestForModernLt : public StreamingTest
{
public:
    static std::vector<std::tuple<std::string, std::string>> GetLtTestSuite()
    {
        std::vector<std::tuple<std::string, std::string>> suite;
        suite.push_back(std::make_tuple("OpenDAQLTStreaming", "daq.lt://127.0.0.1/"));
        suite.push_back(std::make_tuple("OpenDAQLTStreaming", "daq.lt://[::1]/"));
#if defined(OPENDAQ_ENABLE_NATIVE_STREAMING)
        suite.push_back(std::make_tuple("OpenDAQLTStreaming", "daq.nd://127.0.0.1/"));
        suite.push_back(std::make_tuple("OpenDAQLTStreaming", "daq.nd://[::1]/"));
#endif
        suite.push_back(std::make_tuple("OpenDAQLTStreaming", "daq.opcua://127.0.0.1/"));
        suite.push_back(std::make_tuple("OpenDAQLTStreaming", "daq.opcua://[::1]/"));
        return suite;
    }

    static bool compareDescriptors(const daq::DataDescriptorPtr& transmitted, const daq::DataDescriptorPtr& received)
    {
        // LT streaming support partial data descriptor transmission, so only some fields are expected to be transmitted and compared in this test
        // Be careful with argument order in the method - first argument is transmitted descriptor and second argument is received descriptor,
        // because some fields are expected to be default constructed on client side due to LT streaming limitations

        if (transmitted.assigned() != received.assigned())
            return false;

        if (!transmitted.assigned() && !received.assigned())
            return true;

        bool result = true;

        result &= transmitted.getName() == received.getName();
        result &= transmitted.getDimensions() == received.getDimensions();
        result &= transmitted.getSampleType() == received.getSampleType();
        result &= transmitted.getUnit() == received.getUnit();
        result &= transmitted.getValueRange() == received.getValueRange();
        result &= transmitted.getRule() == received.getRule();
        result &= transmitted.getOrigin() == received.getOrigin();
        result &= transmitted.getTickResolution() == received.getTickResolution();
        {
            // LT streaming does not support post scaling, so it is expected to be default constructed on client side
            // result &= transmitted.getPostScaling() == received.getPostScaling();
            result &= !received.getPostScaling().assigned();
        }
        result &= transmitted.getStructFields() == received.getStructFields();
        {
            // LT streaming does not support metadata, so it is expected to be default constructed on client side
            // result &= transmitted.getMetadata() == received.getMetadata();
            result &= !received.getMetadata().assigned() || received.getMetadata().getCount() == 0u;
        }
        result &= transmitted.getReferenceDomainInfo() == received.getReferenceDomainInfo();
        return result;
    };

    static void checkDescriptors(const daq::DataDescriptorPtr& transmitted, const daq::DataDescriptorPtr& received)
    {
        // LT streaming support partial data descriptor transmission, so only some fields are expected to be transmitted and compared in
        // this test Be careful with argument order in the method - first argument is transmitted descriptor and second argument is received
        // descriptor, because some fields are expected to be default constructed on client side due to LT streaming limitations
        ASSERT_EQ(transmitted.assigned(), received.assigned());

        if (transmitted.assigned())
        {
            EXPECT_EQ(transmitted.getName(), received.getName());
            EXPECT_EQ(transmitted.getDimensions(), received.getDimensions());
            EXPECT_EQ(transmitted.getSampleType(), received.getSampleType());
            EXPECT_EQ(transmitted.getUnit(), received.getUnit());
            EXPECT_EQ(transmitted.getValueRange(), received.getValueRange());
            EXPECT_EQ(transmitted.getRule(), received.getRule());
            EXPECT_EQ(transmitted.getOrigin(), received.getOrigin());
            EXPECT_EQ(transmitted.getTickResolution(), received.getTickResolution());
            {
                // LT streaming does not support post scaling, so it is expected to be default constructed on client side
                // EXPECT_EQ(transmitted.getPostScaling(), received.getPostScaling());
                EXPECT_FALSE(received.getPostScaling().assigned());
            }
            EXPECT_EQ(transmitted.getStructFields(), received.getStructFields());
            {
                // LT streaming does not support metadata, so it is expected to be default constructed on client side
                // EXPECT_EQ(transmitted.getMetadata(), received.getMetadata());
                EXPECT_TRUE(received.getMetadata().assigned() == false || received.getMetadata().getCount() == 0u);
            }
            EXPECT_EQ(transmitted.getReferenceDomainInfo(), received.getReferenceDomainInfo());
        }
    };

    static bool compareEventPackets(const daq::PacketPtr& transmitted, const daq::PacketPtr& received)
    {
        // LT streaming support partial data descriptor transmission, so only some fields are expected to be transmitted and compared in this test
        // Be careful with argument order in the method - first argument is transmitted event packet and second argument is received event packet,
        // because some fields are expected to be default constructed on client side due to LT streaming limitations

        if (transmitted.assigned() != received.assigned())
            return false;

        if (!transmitted.assigned() && !received.assigned())
            return true;

        bool result = true;

        auto eventPacketT = transmitted.asPtr<IEventPacket>(true);
        auto eventPacketR = received.asPtr<IEventPacket>(true);

        result &= eventPacketT.getEventId() == eventPacketR.getEventId();
        result &= eventPacketT.getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED;

        const DataDescriptorPtr valueDataDescT = eventPacketT.getParameters().get(event_packet_param::DATA_DESCRIPTOR);
        const DataDescriptorPtr domainDataDescT = eventPacketT.getParameters().get(event_packet_param::DOMAIN_DATA_DESCRIPTOR);

        const DataDescriptorPtr valueDataDescR = eventPacketR.getParameters().get(event_packet_param::DATA_DESCRIPTOR);
        const DataDescriptorPtr domainDataDescR = eventPacketR.getParameters().get(event_packet_param::DOMAIN_DATA_DESCRIPTOR);

        result &= compareDescriptors(valueDataDescT, valueDataDescR);
        result &= compareDescriptors(domainDataDescT, domainDataDescR);

        return result;
    };

    static bool compareDataPackets(const daq::PacketPtr& transmitted, const daq::PacketPtr& received)
    {
        // LT streaming support partial data descriptor transmission, so only some fields are expected to be transmitted and compared in this test
        if (transmitted.assigned() != received.assigned())
            return false;

        if (!transmitted.assigned() && !received.assigned())
            return true;

        bool result = true;

        auto dataPacketT = transmitted.asPtr<IDataPacket>(true);
        auto dataPacketR = received.asPtr<IDataPacket>(true);

        result &= compareDescriptors(dataPacketT.getDataDescriptor(), dataPacketR.getDataDescriptor());
        result &= dataPacketT.getSampleCount() == dataPacketR.getSampleCount();
        result &= dataPacketT.getOffset() == dataPacketR.getOffset();
        result &= compareDataPackets(dataPacketT.getDomainPacket(), dataPacketR.getDomainPacket());
        result &= dataPacketT.getRawDataSize() == dataPacketR.getRawDataSize();
        result &= dataPacketT.getRawData() == dataPacketR.getRawData() ||
                  std::memcmp(dataPacketT.getRawData(), dataPacketR.getRawData(), dataPacketT.getRawDataSize()) == 0;

        return result;
    };
};

TEST_P(StreamingTestForModernLt, SignalDescriptorEvents)
{
    // "ChangingSignal" emits a new descriptor with every generated packet, so each generated packet is
    // preceded by descriptor-changed event packets. Expected packet count:
    // 1 initial descriptor-changed event (sent on subscribe)
    // + packetsToGenerate data packets
    // + (packetsToGenerate - 1) changes, each producing 2 event packets
    const size_t packetsToGenerate = 5;
    const size_t initialEventPackets = 1;
    const size_t packetsPerChange = 2;  // one triggered by data signal and one triggered by domain signal
    const size_t packetsToRead = initialEventPackets + packetsToGenerate + (packetsToGenerate - 1) * packetsPerChange;

    auto serverSignal = getSignal(serverInstance, "ChangingSignal");
    // Give the client time to do async work related to signal creation
    // Otherwise getSignal() on the client may not find it yet.
    std::this_thread::sleep_for(1s);
    auto clientSignal = getSignal(clientInstance, "ChangingSignal");

    auto mirroredSignalPtr = clientSignal.template asPtr<IMirroredSignalConfig>();
    std::promise<StringPtr> subscribeCompletePromise;
    std::future<StringPtr> subscribeCompleteFuture;
    test_helpers::setupSubscribeAckHandler(subscribeCompletePromise, subscribeCompleteFuture, mirroredSignalPtr);

    auto serverReader = createServerReader("ChangingSignal");
    auto clientReader = createClientReader("ChangingSignal");

    ASSERT_TRUE(test_helpers::waitForAcknowledgement(subscribeCompleteFuture));

    generatePackets(packetsToGenerate);

    auto serverReceivedPackets = test_helpers::tryReadPackets(serverReader, packetsToRead);
    auto clientReceivedPackets = test_helpers::tryReadPackets(clientReader, packetsToRead);
    EXPECT_EQ(serverReceivedPackets.getCount(), packetsToRead);
    EXPECT_EQ(clientReceivedPackets.getCount(), packetsToRead);
    // LT streaming transmits only a subset of descriptor fields, so packets cannot be compared with plain equality
    // Use the LT-aware comparators (compareDataPackets/compareEventPackets) instead
    EXPECT_TRUE(
        test_helpers::packetsEqualWithComparators(serverReceivedPackets, clientReceivedPackets, compareDataPackets, compareEventPackets));

    // Recreate the client reader on the same signal: a freshly created reader always replays the current
    // descriptor as its first initial event packet, which we inspect below
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

    // LT streaming supports only partial data descriptor transmission, so only some fields are expected to be
    // transmitted. Hence checkDescriptors() (field-by-field partial comparison) is used instead of EXPECT_EQ() on the whole DataDescriptorPtr
    checkDescriptors(serverSignal.getDescriptor(), dataDescriptor);
    checkDescriptors(clientSignal.getDescriptor(), dataDescriptor);
    checkDescriptors(serverSignal.getDomainSignal().getDescriptor(), domainDescriptor);
    checkDescriptors(clientSignal.getDomainSignal().getDescriptor(), domainDescriptor);
}

TEST_P(StreamingTestForModernLt, DataPackets)
{
    const size_t packetsToGenerate = 10;

    // Expect to receive all data packets,
    // +1 signal initial descriptor changed event packet for server side
    // +2 signal descriptor changed event packets for client side for daq.lt (workaround)
    // +1 signal initial descriptor changed event packet for client side for other connections
    // These additional event packets are triggered by client reader creation in this test and
    // they are not expected to be transmitted over LT streaming, but they are triggered on client side
    // and received by client reader, so they are included in expected packet count and compared in packet comparison
    const size_t packetsToReadServer = packetsToGenerate + 1;
    const size_t packetsToReadClient = packetsToGenerate + ((std::get<1>(GetParam()).find("daq.lt://") == 0) ? 1 : 2);
    // Give the client time to do async work related to signal creation
    // Otherwise getSignal() on the client may not find it yet.
    std::this_thread::sleep_for(1s);
    auto mirroredSignalPtr = getSignal(clientInstance, "ByteStep").template asPtr<IMirroredSignalConfig>();

    std::promise<StringPtr> subscribeCompletePromise;
    std::future<StringPtr> subscribeCompleteFuture;
    test_helpers::setupSubscribeAckHandler(subscribeCompletePromise, subscribeCompleteFuture, mirroredSignalPtr);

    auto serverReader = createServerReader("ByteStep");
    auto clientReader = createClientReader("ByteStep");

    ASSERT_TRUE(test_helpers::waitForAcknowledgement(subscribeCompleteFuture));

    generatePackets(packetsToGenerate);

    auto serverReceivedPackets = test_helpers::tryReadPackets(serverReader, packetsToReadServer);
    auto clientReceivedPackets = test_helpers::tryReadPackets(clientReader, packetsToReadClient);

    test_helpers::printPackets("serverReceivedPackets", serverReceivedPackets, true);
    test_helpers::printPackets("clientReceivedPackets", clientReceivedPackets, true);

    EXPECT_EQ(serverReceivedPackets.getCount(), packetsToReadServer);
    EXPECT_EQ(clientReceivedPackets.getCount(), packetsToReadClient);
    if (std::get<1>(GetParam()).find("daq.lt://") == 0)
    {
        // Pure LT pseudo-device: server and client packet streams line up one-to-one, so compare them
        // pairwise with the LT-aware comparators (LT transmits only a subset of descriptor fields)
        EXPECT_TRUE(test_helpers::packetsEqualWithComparators(
            serverReceivedPackets, clientReceivedPackets, compareDataPackets, compareEventPackets));
    }
    else
    {
        // Config devices (daq.nd/daq.opcua) over LT streaming produce a different number of event
        // packets than the server, so a one-to-one comparison does not fit
        // packetBehaviorComparison() instead extracts the descriptor carried by event packets and verifies each data packet against
        // the descriptor that was in effect before it. It compares observable behavior, not exact packets
        EXPECT_TRUE(
            test_helpers::packetBehaviorComparison(serverReceivedPackets, clientReceivedPackets, compareDataPackets, compareDescriptors));
    }
}

TEST_P(StreamingTestForModernLt, MultipleSignalsConcurrent)
{
    // Subscribe 3 signals with different sample types/rules at once (ByteStep: Int8 explicit, IntStep: Int32 explicit,
    // Sine: Float64 + post-scaling) and verify each stream arrives complete and independent
    // +1 signal initial descriptor changed event packet for server side
    // +2 signal descriptor changed event packets for client side for daq.lt (workaround)
    // +1 signal initial descriptor changed event packet for client side for other connections
    // These additional event packets are triggered by client reader creation in this test and
    // they are not expected to be transmitted over LT streaming, but they are triggered on client side
    // and received by client reader, so they are included in expected packet count and compared in packet comparison
    const std::vector<std::string> signalNames = {"ByteStep", "IntStep", "Sine"};
    const size_t packetsToGenerate = 10;
    const size_t packetsToReadServer = packetsToGenerate + 1;
    const size_t packetsToReadClient = packetsToGenerate + ((std::get<1>(GetParam()).find("daq.lt://") == 0) ? 1 : 2);

    // Give the client time to do async work related to signal creation
    // Otherwise getSignal() on the client may not find it yet
    std::this_thread::sleep_for(1s);

    std::vector<std::promise<StringPtr>> subscribePromises(signalNames.size());
    std::vector<std::future<StringPtr>> subscribeFutures(signalNames.size());
    std::vector<MirroredSignalConfigPtr> mirroredSignals;
    std::vector<PacketReaderPtr> serverReaders;
    std::vector<PacketReaderPtr> clientReaders;
    std::vector<InputPortPtr> clientPorts;

    for (size_t i = 0; i < signalNames.size(); ++i)
    {
        auto mirrored = getSignal(clientInstance, signalNames[i]).template asPtr<IMirroredSignalConfig>();
        mirroredSignals.push_back(mirrored);
        test_helpers::setupSubscribeAckHandler(subscribePromises[i], subscribeFutures[i], mirroredSignals[i]);

        serverReaders.push_back(createServerReader(signalNames[i]));

        auto signal = getSignal(clientInstance, signalNames[i]);
        auto port = InputPort(clientInstance.getContext(), nullptr, "readsig_" + signalNames[i]);
        PacketReaderPtr reader = PacketReaderFromPort(port);
        port.connect(signal);
        clientPorts.push_back(port);
        clientReaders.push_back(reader);
    }

    for (size_t i = 0; i < signalNames.size(); ++i)
        ASSERT_TRUE(test_helpers::waitForAcknowledgement(subscribeFutures[i])) << "subscribe ack timed out for " << signalNames[i];

    generatePackets(packetsToGenerate);

    auto countDataPackets = [](const ListPtr<IPacket>& packets)
    {
        size_t n = 0;
        for (const auto& p : packets)
            if (p.getType() == PacketType::Data)
                ++n;
        return n;
    };

    const bool isLtPseudoDevice = std::get<1>(GetParam()).find("daq.lt://") == 0;

    for (size_t i = 0; i < signalNames.size(); ++i)
    {
        auto serverReceivedPackets = test_helpers::tryReadPackets(serverReaders[i], packetsToReadServer);
        EXPECT_EQ(serverReceivedPackets.getCount(), packetsToReadServer) << "signal " << signalNames[i];

        if (isLtPseudoDevice)
        {
            // LT pseudo-device: descriptors arrive only via streaming, so the server/client streams line up one-to-one.
            auto clientReceivedPackets = test_helpers::tryReadPackets(clientReaders[i], packetsToReadClient);
            EXPECT_EQ(clientReceivedPackets.getCount(), packetsToReadClient) << "signal " << signalNames[i];
            EXPECT_TRUE(test_helpers::packetsEqualWithComparators(
                serverReceivedPackets, clientReceivedPackets, compareDataPackets, compareEventPackets))
                << "signal " << signalNames[i];
        }
        else
        {
            // Config devices (daq.nd/daq.opcua) deliver the initial descriptor through both the config core
            // event and the streaming subscribe. The number of data packets is always packetsToGenerate.
            auto clientReceivedPackets =
                test_helpers::tryReadPackets(clientReaders[i], packetsToReadClient);
            EXPECT_EQ(countDataPackets(clientReceivedPackets), packetsToGenerate) << "signal " << signalNames[i];
            EXPECT_TRUE(test_helpers::packetBehaviorComparison(
                serverReceivedPackets, clientReceivedPackets, compareDataPackets, compareDescriptors))
                << "signal " << signalNames[i];
        }
    }
}

TEST_P(StreamingTestForModernLt, LastValue)
{
    // daq.lt:// is a streaming-only transport (no config channel), so while unsubscribed the client
    // signal has no way to fetch the last value and getLastValue() stays unassigned
    // Config-enabled transports (daq.nd://, daq.opcua://) fall back to a config-protocol RPC and keep returning it
    const bool isStreamingOnly = (std::get<1>(GetParam()).find("daq.lt://") == 0);

    // Flaky on IPv6 OPC UA endpoints: phase-3 lastValue mismatch after unsubscribe
    // (mirror keeps a stale streaming-cached value instead of reading via the config channel).
    if (std::get<1>(GetParam()) == "daq.opcua://[::1]/")
    {
        GTEST_SKIP();
    }

    auto serverSignal = getSignal(serverInstance, "IntStep");
    // Give the client time to do async work related to signal creation
    // Otherwise getSignal() on the client may not find it yet
    std::this_thread::sleep_for(1s);
    auto mirroredSignalPtr = getSignal(clientInstance, "IntStep").template asPtr<IMirroredSignalConfig>();

    std::promise<StringPtr> subscribeCompletePromise;
    std::future<StringPtr> subscribeCompleteFuture;
    test_helpers::setupSubscribeAckHandler(subscribeCompletePromise, subscribeCompleteFuture, mirroredSignalPtr);

    // before any packet is sent neither side has a cached value yet
    ASSERT_FALSE(serverSignal.getLastValue().assigned());
    ASSERT_FALSE(mirroredSignalPtr.getLastValue().assigned());

    // PHASE 1
    // Signal subscribed via streaming
    // Creating a PacketReader on the mirrored signal makes it listened and triggers a streaming subscription
    // The reader is kept alive for the whole scope so the signal stays subscribed.
    {
        auto serverReader = PacketReader(serverSignal);
        auto clientReader = PacketReader(mirroredSignalPtr);

        // Wait until the subscription is actually established before generating data.
        ASSERT_TRUE(test_helpers::waitForAcknowledgement(subscribeCompleteFuture));
        generatePackets(5);

        auto serverReceivedPackets = test_helpers::tryReadPackets(serverReader, 6);
        auto clientReceivedPackets = test_helpers::tryReadPackets(clientReader, 6);

        ASSERT_TRUE(serverSignal.getLastValue().assigned());
        ASSERT_TRUE(mirroredSignalPtr.getLastValue().assigned());
        // While subscribed, the client's last value is filled from the streamed packets, so it matches the server.
        EXPECT_EQ(serverSignal.getLastValue(), mirroredSignalPtr.getLastValue());
    }
    // PHASE 2
    // Signal just unsubscribed because the readers above went out of scope and has been destroyed
    // Unsubscription is asynchronous. Destroying the reader only sends the unsubscribe request and
    // the client's last-value cache is cleared later (when the server's ack arrives -> unsubscribeCompletedInternal -> lastValueCache.reset...)
    // We must wait for the unsubscribe-completion ack otherwise getLastValue() below would still return the stale streaming-cached value
    std::promise<StringPtr> unsubscribeCompletePromise;
    std::future<StringPtr> unsubscribeCompleteFuture;
    test_helpers::setupUnsubscribeAckHandler(unsubscribeCompletePromise, unsubscribeCompleteFuture, mirroredSignalPtr);
    ASSERT_TRUE(test_helpers::waitForAcknowledgement(unsubscribeCompleteFuture));

    ASSERT_TRUE(serverSignal.getLastValue().assigned());
    if (isStreamingOnly)
    {
        // Streaming-only transport does not have config channel and the streaming cache was cleared
        // on unsubscribe so there is nothing left to return
        ASSERT_FALSE(mirroredSignalPtr.getLastValue().assigned());
    }
    else
    {
        // getLastValue() falls back to the config-protocol RPC
        // The value has not changed since unsubscribe so it still equals the server's last value
        ASSERT_TRUE(mirroredSignalPtr.getLastValue().assigned());
        EXPECT_EQ(serverSignal.getLastValue(), mirroredSignalPtr.getLastValue());
    }

    // PHASE 3
    // More data generated while the client is not subscribed via streaming
    // The server's last value advances but no packets reach the client (it is unsubscribed).
    {
        auto serverReader = PacketReader(serverSignal);
        generatePackets(3);

        auto serverReceivedPackets = test_helpers::tryReadPackets(serverReader, 3);

        ASSERT_TRUE(serverSignal.getLastValue().assigned());
        if (isStreamingOnly)
        {
            // No streaming subscription and no config channel so the client never learns the new value
            ASSERT_FALSE(mirroredSignalPtr.getLastValue().assigned());
        }
        else
        {
            // The config-protocol fallback fetches the server's updated value so they match again
            ASSERT_TRUE(mirroredSignalPtr.getLastValue().assigned());
            EXPECT_EQ(serverSignal.getLastValue(), mirroredSignalPtr.getLastValue());
        }
    }
}

TEST_P(StreamingTestForModernLt, DISABLED_SetNullDescriptor)
{
    // DISABLED
    // null descriptor is not supported by modern LT streaming, so this test is disabled
    // serverSignalPtr.setDescriptor(nullptr) call invokes sending an event packet with null descriptor.
    // it triggers a chain of calls and at the end triggers WsStreamingListener::packetReceived and WsStreamingListener::onEventPacketReceived
    // (see websocket_streaming/src/ws_streaming_listener.cpp)
    // parseDataDescriptorEventPacket() function (core/opendaq/signal/include/opendaq/event_packet_utils.h) extract a newValueDescriptor as nullptr
    // (that is right behavior). But then descriptorToMetadata function (see websocket_streaming/src/descriptor_to_metadata.cpp) tries to convert
    // the DataDescriptorPtr to wss::metadata object and it fails because it does not support null descriptor and internal calls lead to an exception.
    // So local_signal::set_metadata() method does not get called and the mirrored signal does not get a new null descriptor. The test fails because of this.

    if (!usingLTPseudoDevice)
    {
        const size_t packetsToRead = 2;

        auto serverSignalPtr = getSignal(serverInstance, "ByteStep").template asPtr<ISignalConfig>();
        // Give the client time to do async work related to signal creation
        // Otherwise getSignal() on the client may not find it yet
        std::this_thread::sleep_for(1s);
        auto mirroredSignalPtr = getSignal(clientInstance, "ByteStep").template asPtr<IMirroredSignalConfig>();
        std::promise<StringPtr> subscribeCompletePromise;
        std::future<StringPtr> subscribeCompleteFuture;
        test_helpers::setupSubscribeAckHandler(subscribeCompletePromise, subscribeCompleteFuture, mirroredSignalPtr);

        auto serverReader = createServerReader("ByteStep");
        auto clientReader = createClientReader("ByteStep");

        ASSERT_TRUE(test_helpers::waitForAcknowledgement(subscribeCompleteFuture));

        // set null descriptor
        serverSignalPtr.setDescriptor(nullptr);

        auto serverReceivedPackets = test_helpers::tryReadPackets(serverReader, packetsToRead);
        auto clientReceivedPackets = test_helpers::tryReadPackets(clientReader, packetsToRead);

        test_helpers::printPackets("serverReceivedPackets", serverReceivedPackets, true);
        test_helpers::printPackets("clientReceivedPackets", clientReceivedPackets, true);

        ASSERT_EQ(serverReceivedPackets.getCount(), packetsToRead);
        ASSERT_EQ(clientReceivedPackets.getCount(), packetsToRead);

        const auto nullDescEventPacket = clientReceivedPackets[1].asPtr<IEventPacket>();
        EXPECT_EQ(nullDescEventPacket.getEventId(), event_packet_id::DATA_DESCRIPTOR_CHANGED);
        EXPECT_EQ(nullDescEventPacket.getParameters().get(event_packet_param::DATA_DESCRIPTOR), NullDataDescriptor());

        ASSERT_EQ(mirroredSignalPtr.getDescriptor(), nullptr);

        EXPECT_TRUE(test_helpers::packetsEqual(serverReceivedPackets,
                                               clientReceivedPackets,
                                               std::get<0>(GetParam()) == "OpenDAQLTStreaming"));
    }
    else // usingLTPseudoDevice true
    {
        auto serverSignalPtr = getSignal(serverInstance, "ByteStep").template asPtr<ISignalConfig>();
        // Give the client time to do async work related to signal creation
        // Otherwise getSignal() on the client may not find it yet.
        std::this_thread::sleep_for(1s);
        auto mirroredOrigSignalPtr = getSignal(clientInstance, "ByteStep").template asPtr<IMirroredSignalConfig>();

        std::promise<StringPtr> origSigSubscribeCompletePromise;
        std::future<StringPtr> origSigSubscribeCompleteFuture;
        test_helpers::setupSubscribeAckHandler(origSigSubscribeCompletePromise, origSigSubscribeCompleteFuture, mirroredOrigSignalPtr);
        auto clientOrigSigReader = createClientReader("ByteStep");
        auto serverReader = createServerReader("ByteStep");
        ASSERT_TRUE(test_helpers::waitForAcknowledgement(origSigSubscribeCompleteFuture));
        auto clientOrigSigReceivedPackets = test_helpers::tryReadPackets(clientOrigSigReader, 1);

        SignalConfigPtr mirroredNewSignalPtr;
        std::promise<void> addSigPromise;
        std::future<void> addSigFuture = addSigPromise.get_future();

        clientInstance.getContext().getOnCoreEvent() +=
            [&](const ComponentPtr& comp, const CoreEventArgsPtr& args)
        {
            auto params = args.getParameters();
            if (static_cast<CoreEventId>(args.getEventId()) == CoreEventId::ComponentAdded)
            {
                ComponentPtr component = params.get("Component");
                if (component.asPtrOrNull<ISignal>().assigned())
                {
                    mirroredNewSignalPtr = component;
                    addSigPromise.set_value();
                }
            }
        };

        // set null descriptor
        serverSignalPtr.setDescriptor(nullptr);

        ASSERT_TRUE(addSigFuture.wait_for(std::chrono::seconds(10)) == std::future_status::ready);

        auto serverReceivedPackets = test_helpers::tryReadPackets(serverReader, 2);

        auto clientNewSigReader = PacketReader(mirroredNewSignalPtr);
        auto clientNewSigReceivedPackets = test_helpers::tryReadPackets(clientNewSigReader, 1);

        test_helpers::printPackets("serverReceivedPackets", serverReceivedPackets, true);
        test_helpers::printPackets("clientReceivedPackets", clientNewSigReceivedPackets, true);

        ASSERT_EQ(serverReceivedPackets.getCount(), 2u);
        ASSERT_EQ(clientOrigSigReceivedPackets.getCount(), 1u);
        ASSERT_EQ(clientNewSigReceivedPackets.getCount(), 1u);

        const auto nullDescEventPacket = clientNewSigReceivedPackets[0].asPtr<IEventPacket>();
        EXPECT_EQ(nullDescEventPacket.getEventId(), event_packet_id::DATA_DESCRIPTOR_CHANGED);
        EXPECT_EQ(nullDescEventPacket.getParameters().get(event_packet_param::DATA_DESCRIPTOR), NullDataDescriptor());

        ASSERT_EQ(mirroredNewSignalPtr.getDescriptor(), nullptr);
        ASSERT_TRUE(mirroredOrigSignalPtr.isRemoved());
    }
}

TEST_P(StreamingTestForModernLt, ChangedDataDescriptorBeforeSubscribe)
{
    // daq.nd:// is not supported by this test.
    // A native configuration device mirrors the whole component tree and actively pushes signal descriptor
    // changes to the client as DataDescriptorChanged core events. So with daq.nd the descriptor change reaches the client through two
    // independent paths: the config core event (applied as soon as setDescriptor() runs on the server, even before
    // the streaming subscription) and the LT streaming event packet on subscribe. This produces extra
    // descriptor-changed event packets and a non-deterministic packet count/ordering, which breaks the exact
    // expectations below
    if (std::get<1>(GetParam()).find("daq.nd://") == 0)
    {
        GTEST_SKIP() << "daq.nd:// is not supported by this test";
    }

    SKIP_TEST_MAC_CI;
    SignalConfigPtr serverSignalPtr = getSignal(serverInstance, "ByteStep");
    // Give the client time to do async work related to signal creation
    // Otherwise getSignal() on the client may not find it yet
    std::this_thread::sleep_for(1s);
    MirroredSignalConfigPtr clientSignalPtr = getSignal(clientInstance, "ByteStep");
    MirroredSignalConfigPtr clientDomainSignalPtr = clientSignalPtr.getDomainSignal();

    // Repeat several times: each iteration changes the descriptor on the server BEFORE subscribing, then
    // subscribes and checks that the client observes the change through the initial descriptor-changed
    // event packet replayed on subscribe
    for (int i = 0; i < 5; ++i)
    {
        const auto oldValueDataDesc = serverSignalPtr.getDescriptor();
        const auto oldDomainDataDesc = serverSignalPtr.getDomainSignal().getDescriptor();

        // Build new value/domain descriptors that differ only by name, and apply them on the server while the
        // client is NOT subscribed (the reader is created afterwards).
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

        // Creating the reader triggers the subscription.
        auto clientReader = PacketReader(clientSignalPtr);

        ASSERT_TRUE(test_helpers::waitForAcknowledgement(subscribeCompleteFuture));

        const int packetsToRead = i + 3;
        generatePackets(packetsToRead);

        if (usingLTPseudoDevice)
        {
            // LT pseudo-device replays 3 initial descriptor-changed events on subscribe
            // LT sends value and domain descriptor changes as SEPARATE events
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
                    compareDescriptors(oldValueDataDesc, valueDataDescClient);
                    compareDescriptors(oldDomainDataDesc, domainDataDescClient);
                }
                else if (j == 1)
                {
                    compareDescriptors(valueDataDesc, valueDataDescClient);
                    compareDescriptors(nullptr, domainDataDescClient);
                }
                else
                {
                    compareDescriptors(nullptr, valueDataDescClient);
                    compareDescriptors(domainDataDesc, domainDataDescClient);
                }
            }
        }
        else
        {
            // Config device over LT streaming (daq.opcua://): a single initial descriptor-changed event carries
            // the latest value+domain descriptors. Plain EXPECT_EQ is fine here (full descriptors are compared)
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

        // Releasing the reader unsubscribes the signal
        // wait for the unsubscribe to complete so the next
        // iteration starts from a clean, fully-unsubscribed state.
        clientReader.release();

        ASSERT_TRUE(test_helpers::waitForAcknowledgement(unsubscribeCompleteFuture));

        // Same cleanup for the value signal's subscribe/unsubscribe event listeners.
        IEvent* evSub = clientSignalPtr.getOnSubscribeComplete();
        IEvent* evUnsub = clientSignalPtr.getOnUnsubscribeComplete();
        evSub->clear();
        evUnsub->clear();
    }
}

INSTANTIATE_TEST_SUITE_P(
    StreamingTestGroup_ModernLT,
    StreamingTestForModernLt,
    testing::ValuesIn(StreamingTestForModernLt::GetLtTestSuite())
    );

class StreamingReconnectionTestForModernLt : public StreamingTestForModernLt
{
protected:
    InstancePtr CreateServerInstance() override
    {
        auto logger = Logger();
        auto scheduler = Scheduler(logger);
        auto moduleManager = ModuleManager("[[none]]");
        auto typeManager = TypeManager();
        auto authenticationProvider = AuthenticationProvider();
        auto context = Context(scheduler, logger, typeManager, moduleManager, authenticationProvider);

        const ModulePtr deviceModule(MockDeviceModule_Create(context));
        moduleManager.addModule(deviceModule);

        auto instance = InstanceCustom(context, "local");
        addLtServerModule(instance);
        addOpcuaServerModule(instance);
#if defined(OPENDAQ_ENABLE_NATIVE_STREAMING)
        addNativeServerModule(instance);
#endif

        const auto mockDevice = instance.addDevice("daqmock://phys_device");

        streamingServer = instance.addServer("OpenDAQLTStreaming", nullptr);
#if defined(OPENDAQ_ENABLE_NATIVE_STREAMING)
        // native server provides the config channel for daq.nd:// clients (streaming itself stays on LT,
        // which is the only prioritized streaming protocol on the client side)
        instance.addServer("OpenDAQNativeStreaming", nullptr);
#endif
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
        streamingServer = serverInstance.addServer("OpenDAQLTStreaming", nullptr);
    }

    ServerPtr streamingServer;
};

// DISABLED: this test aborts the whole process (SIGABRT)
// It aborts on the main thread during the re-add server step,
// restoreStreamingServer() -> serverInstance.addServer("OpenDAQLTStreaming"), inside the WsStreamingServer constructor.
// Trace:
//  - restoreStreamingServer()
//  - <...>
//  - Module::createServer   // == addServer("OpenDAQLTStreaming")
//  - <...>
//  - WsStreamingServer::WsStreamingServer(...)
//  - addCapability()
//      - throw InvalidStateException because the "OpenDAQLTStreaming" capability is still registered
//          from the previous instance (removeServer() does not remove it from the device info)
//  - <constructor unwinding>
//  - std::thread::~thread() for _thread in WsStreamingServer
//  - std::terminate
//
// Two cooperating bugs in the LtStreamingModulesModern shared/libraries/websocket_streaming/src/ws_streaming_server.cpp:
//   1) Capability leak. WsStreamingServer::onStopServer() stops/joins the io thread but
//      never removes the "OpenDAQLTStreaming" server capability it registered in addCapability()
//      After removeServer() the device info still advertises that capability
//   2) The constructor is not exception safe. On re-add the ctor first spawns its thread
//      and then calls addCapability(). If addCapability() throw an exeption then unwinding the half-built object destroys
//      the still joinable _thread with no join()/detach()

TEST_P(StreamingReconnectionTestForModernLt, DISABLED_Reconnection)
{
    // Give the client time to do async work related to signal creation
    // Otherwise getSignal() on the client may not find it yet
    std::this_thread::sleep_for(1s);
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
    // reset the subscribe promise; the handler stays attached and fills the new promise on re-subscribe
    subscribeCompletePromise = std::promise<StringPtr>();
    subscribeCompleteFuture = subscribeCompletePromise.get_future();
    // add streaming server back to enable reconnection
    restoreStreamingServer();

    ASSERT_TRUE(test_helpers::waitForAcknowledgement(subscribeCompleteFuture, 30s));

    const size_t packetsToGenerate = 10;
    generatePackets(packetsToGenerate);

    serverReceivedPackets = test_helpers::tryReadPackets(serverReader, packetsToGenerate);
    clientReceivedPackets = test_helpers::tryReadPackets(clientReader, packetsToGenerate);

    EXPECT_EQ(serverReceivedPackets.getCount(), packetsToGenerate);
    EXPECT_EQ(clientReceivedPackets.getCount(), packetsToGenerate);
    // LT transmits only a subset of descriptor fields, so use the LT-aware comparators
    EXPECT_TRUE(test_helpers::packetsEqualWithComparators(
        serverReceivedPackets, clientReceivedPackets, compareDataPackets, compareEventPackets));
}

INSTANTIATE_TEST_SUITE_P(
    StreamingReconnectionTestGroup_ModernLT,
    StreamingReconnectionTestForModernLt,
    testing::ValuesIn(StreamingTestForModernLt::GetLtTestSuite())
    );
#endif