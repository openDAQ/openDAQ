#include "test_helpers/test_helpers.h"
#include <iostream>
#include <opendaq/device_info_internal_ptr.h>
#include <coreobjects/authentication_provider_factory.h>

using namespace daq;

enum class StreamingProtocolType
{
    WebsocketStreaming = 0,
    NativeStreaming
};

enum class StructureProtocolType
{
    OpcUa = 0,
    Native
};

// 1-st param: device configuration/structure protocol type
// 2-nd param: leaf device streaming type
// 3-rd param: gateway device streaming type
class SubDevicesTest : public testing::TestWithParam<std::tuple<StructureProtocolType, StreamingProtocolType, StreamingProtocolType>>
{
public:
    const uint16_t NATIVE_PORT = 7420;
    const uint16_t WEBSOCKET_STREAMING_PORT = 7414;
    const uint16_t OPCUA_PORT = 4840;
    const uint16_t WEBSOCKET_CONTROL_PORT = 7438;

    const uint16_t MIN_CONNECTIONS = 0;
    const uint16_t MIN_HOPS = 1;

    const char* ADDRESS = "127.0.0.1";

    PropertyObjectPtr createDeviceConfig(const InstancePtr& instance,
                                         ListPtr<IString> prioritizedStreamingProtocols,
                                         const IntegerPtr& heuristicValue)
    {
        auto config = instance.createDefaultAddDeviceConfig();
        PropertyObjectPtr general = config.getPropertyValue("General");
        general.setPropertyValue("PrioritizedStreamingProtocols", prioritizedStreamingProtocols);
        general.setPropertyValue("StreamingConnectionHeuristic", heuristicValue);

        return config;
    }

    StringPtr createStructureDeviceConnectionString(uint16_t portOffset)
    {
        auto structureProtocolType = std::get<0>(GetParam());
        uint16_t port =
            (structureProtocolType == StructureProtocolType::Native ? NATIVE_PORT : OPCUA_PORT) + portOffset;
        std::string prefix =
            structureProtocolType == StructureProtocolType::Native ? "daq.nd://" : "daq.opcua://";

        return String(fmt::format("{}{}:{}/", prefix, ADDRESS, port));
    }

    InstancePtr CreateSubdeviceInstance(uint16_t index)
    {
        auto logger = Logger();
        auto scheduler = Scheduler(logger);
        auto moduleManager = ModuleManager("");
        auto typeManager = TypeManager();
        auto authenticationProvider = AuthenticationProvider();
        auto context = Context(scheduler, logger, typeManager, moduleManager, authenticationProvider);
        auto instance = InstanceCustom(context, fmt::format("subdevice{}", index));
        const auto refDevice = instance.addDevice("daqref://device0");

        auto structureProtocolType = std::get<0>(GetParam());

        {
            auto ws_config = instance.getAvailableServerTypes().get("OpenDAQLTStreaming").createDefaultConfig();
            ws_config.setPropertyValue("WebsocketStreamingPort", WEBSOCKET_STREAMING_PORT + index);
            ws_config.setPropertyValue("WebsocketControlPort", WEBSOCKET_CONTROL_PORT + index);
            instance.addServer("OpenDAQLTStreaming", ws_config);
        }
        {
            auto ns_config = instance.getAvailableServerTypes().get("OpenDAQNativeStreaming").createDefaultConfig();
            ns_config.setPropertyValue("NativeStreamingPort", NATIVE_PORT + index);
            instance.addServer("OpenDAQNativeStreaming", ns_config);
        }

        if (structureProtocolType == StructureProtocolType::OpcUa)
        {
            auto ua_config = instance.getAvailableServerTypes().get("OpenDAQOPCUA").createDefaultConfig();
            ua_config.setPropertyValue("Port", OPCUA_PORT + index);
            instance.addServer("OpenDAQOPCUA", ua_config);
        }

        return instance;
    }

    InstancePtr CreateGatewayInstance()
    {
        auto logger = Logger();
        auto scheduler = Scheduler(logger);
        auto moduleManager = ModuleManager("");
        auto typeManager = TypeManager();
        auto authenticationProvider = AuthenticationProvider();
        auto context = Context(scheduler, logger, typeManager, moduleManager, authenticationProvider);

        auto instance = InstanceCustom(context, "gateway");

        auto structureProtocolType = std::get<0>(GetParam());
        auto subdeviceStreamingType = std::get<1>(GetParam());

        for (auto index = 1; index <= 2; index++)
        {
            auto streamingProtocolIds = (subdeviceStreamingType == StreamingProtocolType::NativeStreaming)
                                            ? List<IString>("OpenDAQNativeStreaming", "OpenDAQLTStreaming")
                                            : List<IString>("OpenDAQLTStreaming", "OpenDAQNativeStreaming");
            const auto config = createDeviceConfig(instance, streamingProtocolIds, MIN_CONNECTIONS);
            const auto subDevice = instance.addDevice(createStructureDeviceConnectionString(index), config);
        }

        {
            auto ws_config = instance.getAvailableServerTypes().get("OpenDAQLTStreaming").createDefaultConfig();
            ws_config.setPropertyValue("WebsocketStreamingPort", WEBSOCKET_STREAMING_PORT);
            ws_config.setPropertyValue("WebsocketControlPort", WEBSOCKET_CONTROL_PORT);
            instance.addServer("OpenDAQLTStreaming", ws_config);
        }
        {
            auto ns_config = instance.getAvailableServerTypes().get("OpenDAQNativeStreaming").createDefaultConfig();
            ns_config.setPropertyValue("NativeStreamingPort", NATIVE_PORT);
            instance.addServer("OpenDAQNativeStreaming", ns_config);
        }

        if (structureProtocolType == StructureProtocolType::OpcUa)
        {
            auto ua_config = instance.getAvailableServerTypes().get("OpenDAQOPCUA").createDefaultConfig();
            ua_config.setPropertyValue("Port", OPCUA_PORT);
            instance.addServer("OpenDAQOPCUA", ua_config);
        }

        return instance;
    }

    InstancePtr CreateClientInstance(const IntegerPtr& heuristicValue)
    {
        auto logger = Logger();
        auto scheduler = Scheduler(logger);
        auto moduleManager = ModuleManager("");
        auto typeManager = TypeManager();
        auto authenticationProvider = AuthenticationProvider();
        auto context = Context(scheduler, logger, typeManager, moduleManager, authenticationProvider);
        auto instance = InstanceCustom(context, "client");

        auto gatewayStreamingType = std::get<2>(GetParam());

        auto streamingProtocolIds = (gatewayStreamingType == StreamingProtocolType::NativeStreaming)
                                        ? List<IString>("OpenDAQNativeStreaming", "OpenDAQLTStreaming")
                                        : List<IString>("OpenDAQLTStreaming", "OpenDAQNativeStreaming");
        auto config = createDeviceConfig(instance, streamingProtocolIds, heuristicValue);
        auto gatewayDevice = instance.addDevice(createStructureDeviceConnectionString(0), config);

        return instance;
    }
};

TEST_P(SubDevicesTest, RootStreamingToClient)
{
    SKIP_TEST_MAC_CI;
    auto subdevice1 = CreateSubdeviceInstance(1u);
    auto subdevice2 = CreateSubdeviceInstance(2u);
    auto gateway = CreateGatewayInstance();
    auto client = CreateClientInstance(MIN_CONNECTIONS);

    auto clientSignals = client.getSignals(search::Recursive(search::Visible()));
    auto gatewaySignals = gateway.getSignals(search::Recursive(search::Visible()));
    ASSERT_EQ(clientSignals.getCount(), gatewaySignals.getCount());

    for (size_t index = 0; index < clientSignals.getCount(); ++index)
    {
        auto clientSignal = clientSignals[index].template asPtr<IMirroredSignalConfig>();
        auto gatewaySignal = gatewaySignals[index].template asPtr<IMirroredSignalConfig>();
        ASSERT_EQ(clientSignal.getStreamingSources().getCount(), 2u);
        ASSERT_EQ(gatewaySignal.getStreamingSources().getCount(), 2u);
        ASSERT_TRUE(clientSignal.getActiveStreamingSource().assigned());
        ASSERT_NE(clientSignal.getActiveStreamingSource(), gatewaySignal.getStreamingSources()[0]);
        ASSERT_NE(clientSignal.getActiveStreamingSource(), gatewaySignal.getStreamingSources()[1]);
    }

    auto clientSignal = client.getSignalsRecursive()[0].template asPtr<IMirroredSignalConfig>();
    std::promise<StringPtr> clientSignalSubscribePromise;
    std::future<StringPtr> clientSignalSubscribeFuture;
    test_helpers::setupSubscribeAckHandler(clientSignalSubscribePromise,
                                           clientSignalSubscribeFuture,
                                           clientSignal);

    auto gatewaySignal = gateway.getSignalsRecursive()[0].template asPtr<IMirroredSignalConfig>();
    std::promise<StringPtr> gatewaySignalSubscribePromise;
    std::future<StringPtr> gatewaySignalSubscribeFuture;
    test_helpers::setupSubscribeAckHandler(gatewaySignalSubscribePromise,
                                           gatewaySignalSubscribeFuture,
                                           gatewaySignal);

    using namespace std::chrono_literals;
    StreamReaderPtr reader = daq::StreamReader<double, uint64_t>(clientSignal, ReadTimeoutType::Any);

    ASSERT_TRUE(test_helpers::waitForAcknowledgement(gatewaySignalSubscribeFuture));
    ASSERT_TRUE(test_helpers::waitForAcknowledgement(clientSignalSubscribeFuture));

    {
        daq::SizeT count = 0;
        reader.read(nullptr, &count, 100);
    }

    double samples[100];
    for (int i = 0; i < 10; ++i)
    {
        daq::SizeT count = 100;
        reader.read(samples, &count, 100);
        EXPECT_GT(count, 0u) << "iteration " << i;
    }
}

TEST_P(SubDevicesTest, LeafStreamingToClient)
{
    SKIP_TEST_MAC_CI;
    auto subdevice1 = CreateSubdeviceInstance(1u);
    auto subdevice2 = CreateSubdeviceInstance(2u);
    auto gateway = CreateGatewayInstance();
    auto client = CreateClientInstance(MIN_HOPS);    

    auto clientSignals = client.getSignals(search::Recursive(search::Visible()));
    auto gatewaySignals = gateway.getSignals(search::Recursive(search::Visible()));
    ASSERT_EQ(clientSignals.getCount(), gatewaySignals.getCount());

    for (size_t index = 0; index < clientSignals.getCount(); ++index)
    {
        auto clientSignal = clientSignals[index].template asPtr<IMirroredSignalConfig>();
        auto gatewaySignal = gatewaySignals[index].template asPtr<IMirroredSignalConfig>();
        ASSERT_EQ(clientSignal.getStreamingSources().getCount(), 4u);
        ASSERT_EQ(gatewaySignal.getStreamingSources().getCount(), 2u);
        ASSERT_TRUE(clientSignal.getActiveStreamingSource().assigned());
        ASSERT_TRUE(clientSignal.getActiveStreamingSource() == gatewaySignal.getStreamingSources()[0] ||
                    clientSignal.getActiveStreamingSource() == gatewaySignal.getStreamingSources()[1]);
    }

    auto clientSignal = client.getSignalsRecursive()[0].template asPtr<IMirroredSignalConfig>();
    std::promise<StringPtr> clientSignalSubscribePromise;
    std::future<StringPtr> clientSignalSubscribeFuture;
    test_helpers::setupSubscribeAckHandler(clientSignalSubscribePromise,
                                           clientSignalSubscribeFuture,
                                           clientSignal);

    using namespace std::chrono_literals;
    StreamReaderPtr reader = daq::StreamReader<double, uint64_t>(clientSignal, ReadTimeoutType::Any);
    
    ASSERT_TRUE(test_helpers::waitForAcknowledgement(clientSignalSubscribeFuture));

    {
        daq::SizeT count = 0;
        reader.read(nullptr, &count, 100);
    }

    double samples[100];
    for (int i = 0; i < 10; ++i)
    {
        daq::SizeT count = 100;
        reader.read(samples, &count, 100);
        EXPECT_GT(count, 0u) << "iteration " << i;
    }
}

TEST_P(SubDevicesTest, LeafStreamingToGatewayAndClient)
{
    SKIP_TEST_MAC_CI;

    if (std::get<2>(GetParam()) == StreamingProtocolType::WebsocketStreaming)
    {
        // skip test
        return;
    }

    auto subdevice1 = CreateSubdeviceInstance(1u);
    auto subdevice2 = CreateSubdeviceInstance(2u);
    auto gateway = CreateGatewayInstance();
    auto client = CreateClientInstance(MIN_HOPS);

    auto clientSignals = client.getSignalsRecursive();
    auto gatewaySignals = gateway.getSignalsRecursive();
    ASSERT_EQ(clientSignals.getCount(), gatewaySignals.getCount());

    for (size_t index = 0; index < clientSignals.getCount(); ++index)
    {
        auto clientSignal = clientSignals[index].template asPtr<IMirroredSignalConfig>();
        auto gatewaySignal = gatewaySignals[index].template asPtr<IMirroredSignalConfig>();
        ASSERT_EQ(clientSignal.getStreamingSources().getCount(), 4u);
        ASSERT_EQ(gatewaySignal.getStreamingSources().getCount(), 2u);
        ASSERT_TRUE(clientSignal.getActiveStreamingSource().assigned());
        ASSERT_TRUE(clientSignal.getActiveStreamingSource() == gatewaySignal.getStreamingSources()[0] ||
                    clientSignal.getActiveStreamingSource() == gatewaySignal.getStreamingSources()[1]);
    }

    auto clientSignal = client.getSignalsRecursive()[0].template asPtr<IMirroredSignalConfig>();
    std::promise<StringPtr> clientSignalSubscribePromise;
    std::future<StringPtr> clientSignalSubscribeFuture;
    test_helpers::setupSubscribeAckHandler(clientSignalSubscribePromise,
                                           clientSignalSubscribeFuture,
                                           clientSignal);

    auto gatewaySignal = gateway.getSignalsRecursive()[0].template asPtr<IMirroredSignalConfig>();
    std::promise<StringPtr> gatewaySignalSubscribePromise;
    std::future<StringPtr> gatewaySignalSubscribeFuture;
    test_helpers::setupSubscribeAckHandler(gatewaySignalSubscribePromise,
                                           gatewaySignalSubscribeFuture,
                                           gatewaySignal);

    using namespace std::chrono_literals;
    StreamReaderPtr clientReader = daq::StreamReader<double, uint64_t>(clientSignal, ReadTimeoutType::Any);
    StreamReaderPtr gatewayReader = daq::StreamReader<double, uint64_t>(gatewaySignal, ReadTimeoutType::Any);

    ASSERT_TRUE(test_helpers::waitForAcknowledgement(gatewaySignalSubscribeFuture));
    ASSERT_TRUE(test_helpers::waitForAcknowledgement(clientSignalSubscribeFuture));

    {
        daq::SizeT count = 0;
        clientReader.read(nullptr, &count, 100);
        gatewayReader.read(nullptr, &count, 100);
    }

    double clientSamples[100];
    double gatewaySamples[100];
    for (int i = 0; i < 10; ++i)
    {
        daq::SizeT clientSamplesCount = 100;
        clientReader.read(clientSamples, &clientSamplesCount, 100);
        EXPECT_GT(clientSamplesCount, 0u) << "iteration " << i;
        
        daq::SizeT gatewaySamplesCount = 100;
        gatewayReader.read(gatewaySamples, &gatewaySamplesCount, 100);
        EXPECT_GT(gatewaySamplesCount, 0u) << "iteration " << i;
    }
}

INSTANTIATE_TEST_SUITE_P(
    SubDevicesTestGroup,
    SubDevicesTest,
    testing::Values(
        std::make_tuple(StructureProtocolType::OpcUa, StreamingProtocolType::NativeStreaming, StreamingProtocolType::NativeStreaming),
        std::make_tuple(StructureProtocolType::OpcUa, StreamingProtocolType::WebsocketStreaming, StreamingProtocolType::WebsocketStreaming),
        std::make_tuple(StructureProtocolType::OpcUa, StreamingProtocolType::NativeStreaming, StreamingProtocolType::WebsocketStreaming),
        std::make_tuple(StructureProtocolType::OpcUa, StreamingProtocolType::WebsocketStreaming, StreamingProtocolType::NativeStreaming),
        std::make_tuple(StructureProtocolType::Native, StreamingProtocolType::NativeStreaming, StreamingProtocolType::NativeStreaming),
        std::make_tuple(StructureProtocolType::Native, StreamingProtocolType::WebsocketStreaming, StreamingProtocolType::WebsocketStreaming),
        std::make_tuple(StructureProtocolType::Native, StreamingProtocolType::NativeStreaming, StreamingProtocolType::WebsocketStreaming),
        std::make_tuple(StructureProtocolType::Native, StreamingProtocolType::WebsocketStreaming, StreamingProtocolType::NativeStreaming)
    )
);
