#include "test_helpers/test_helpers.h"
#include "test_helpers/mock_helper_module.h"
#include <iostream>
#include <opendaq/device_info_internal_ptr.h>
#include <coreobjects/authentication_provider_factory.h>

using namespace daq;

enum class StreamingType
{
    WebsocketStreaming = 0,
    NativeStreaming
};

// first param: leaf device streaming type
// second param: gateway device streaming type
class SubDevicesTest : public testing::TestWithParam<std::pair<StreamingType, StreamingType>>
{
public:
    const uint16_t NATIVE_STREAMING_PORT = 7420;
    const uint16_t WEBSOCKET_STREAMING_PORT = 7414;
    const uint16_t OPCUA_PORT = 4840;
    const uint16_t WEBSOCKET_CONTROL_PORT = 7438;

    const uint16_t MIN_CONNECTIONS = 0;
    const uint16_t MIN_HOPS = 1;

    const char* MANUFACTURER = "Manufacturer";
    const char* SERIAL_NUMBER = "SerialNumber";
    const char* ADDRESS = "127.0.0.1";

    StringPtr createConnectionString(uint16_t port)
    {
        return String(fmt::format("daq.opcua://{}:{}/", ADDRESS, port));
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

        auto subdeviceStreamingType = GetParam().first;

        if (subdeviceStreamingType == StreamingType::WebsocketStreaming)
        {
            auto ws_config = instance.getAvailableServerTypes().get("openDAQ LT Streaming").createDefaultConfig();
            ws_config.setPropertyValue("Port", WEBSOCKET_STREAMING_PORT + index);
            ws_config.setPropertyValue("WebsocketControlPort", WEBSOCKET_CONTROL_PORT + index);
            instance.addServer("openDAQ LT Streaming", ws_config);
        }
        else if (subdeviceStreamingType == StreamingType::NativeStreaming)
        {
            auto ns_config = instance.getAvailableServerTypes().get("openDAQ Native Streaming").createDefaultConfig();
            ns_config.setPropertyValue("Port", NATIVE_STREAMING_PORT + index);
            instance.addServer("openDAQ Native Streaming", ns_config);
        }

        auto ua_config = instance.getAvailableServerTypes().get("openDAQ OpcUa").createDefaultConfig();
        ua_config.setPropertyValue("Port", OPCUA_PORT + index);
        instance.addServer("openDAQ OpcUa", ua_config);

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

        for (auto index = 1; index <= 2; index++)
        {
            const auto subDevice = instance.addDevice(createConnectionString(OPCUA_PORT+index));
        }

        auto gatewayStreamingType = GetParam().second;

        if (gatewayStreamingType == StreamingType::WebsocketStreaming)
        {
            auto ws_config = instance.getAvailableServerTypes().get("openDAQ LT Streaming").createDefaultConfig();
            ws_config.setPropertyValue("Port", WEBSOCKET_STREAMING_PORT);
            ws_config.setPropertyValue("WebsocketControlPort", WEBSOCKET_CONTROL_PORT);
            instance.addServer("openDAQ LT Streaming", ws_config);
        }
        else if (gatewayStreamingType == StreamingType::NativeStreaming)
        {
            auto ns_config = instance.getAvailableServerTypes().get("openDAQ Native Streaming").createDefaultConfig();
            ns_config.setPropertyValue("Port", NATIVE_STREAMING_PORT);
            instance.addServer("openDAQ Native Streaming", ns_config);
        }

        auto ua_config = instance.getAvailableServerTypes().get("openDAQ OpcUa").createDefaultConfig();
        ua_config.setPropertyValue("Port", OPCUA_PORT);
        instance.addServer("openDAQ OpcUa", ua_config);

        return instance;
    }

    DeviceInfoPtr CreateDiscoveredDeviceInfo(const DeviceTypePtr& deviceType)
    {
        auto deviceConnectionString = createConnectionString(OPCUA_PORT);

        DeviceInfoConfigPtr deviceInfo = DeviceInfo(deviceConnectionString);
        deviceInfo.setDeviceType(deviceType);
        deviceInfo.setManufacturer(MANUFACTURER);
        deviceInfo.setSerialNumber(SERIAL_NUMBER);

        auto cap = ServerCapability("opendaq_opcua_config", "openDAQ OpcUa", ProtocolType::Configuration);
        cap.addConnectionString(deviceConnectionString);
        cap.setConnectionType("TCP/IP");
        cap.setPrefix("daq.opcua");
        cap.addAddress(ADDRESS);

        deviceInfo.asPtr<IDeviceInfoInternal>().addServerCapability(cap);

        return deviceInfo;
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

        auto deviceType = instance.getAvailableDeviceTypes().get("opendaq_opcua_config");
        auto deviceInfo = CreateDiscoveredDeviceInfo(deviceType);

        const ModulePtr helperModule(
            createWithImplementation<IModule, test_helpers::MockHelperModuleImpl>(
                context,
                Function(
                    [deviceInfo = deviceInfo]() -> ListPtr<IDeviceInfo>
                    {
                        ListPtr<IDeviceInfo> availableDevices = List<IDeviceInfo>();

                        availableDevices.pushBack(deviceInfo);
                        return availableDevices;
                    }
                )
            )
        );
        moduleManager.addModule(helperModule);

        auto config = PropertyObject();

        const auto streamingConnectionHeuristicProp =  SelectionProperty("StreamingConnectionHeuristic",
                                                                        List<IString>("MinConnections",
                                                                                      "MinHops",
                                                                                      "Fallbacks",
                                                                                      "NotConnected"),
                                                                        heuristicValue);
        config.addProperty(streamingConnectionHeuristicProp);

        auto smartConnectionString = fmt::format("daq://{}_{}", MANUFACTURER, SERIAL_NUMBER);
        auto gatewayDevice = instance.addDevice(smartConnectionString, config);

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
        auto mirroredSignalConfigPtr = clientSignals[index].template asPtr<IMirroredSignalConfig>();
        ASSERT_EQ(mirroredSignalConfigPtr.getStreamingSources().getCount(), 1u);
        ASSERT_TRUE(mirroredSignalConfigPtr.getActiveStreamingSource().assigned());
        ASSERT_NE(mirroredSignalConfigPtr.getActiveStreamingSource(),
                  gatewaySignals[index].template asPtr<IMirroredSignalConfig>().getActiveStreamingSource());
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
    StreamReaderPtr reader = daq::StreamReader<double, uint64_t>(clientSignal);

    ASSERT_TRUE(test_helpers::waitForAcknowledgement(gatewaySignalSubscribeFuture));
    ASSERT_TRUE(test_helpers::waitForAcknowledgement(clientSignalSubscribeFuture));
    double samples[100];
    for (int i = 0; i < 10; ++i)
    {
        std::this_thread::sleep_for(100ms);
        daq::SizeT count = 100;
        reader.read(samples, &count);
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
        auto mirroredSignalConfigPtr = clientSignals[index].template asPtr<IMirroredSignalConfig>();
        ASSERT_EQ(mirroredSignalConfigPtr.getStreamingSources().getCount(), 2u);
        ASSERT_TRUE(mirroredSignalConfigPtr.getActiveStreamingSource().assigned());

        ASSERT_EQ(mirroredSignalConfigPtr.getActiveStreamingSource(),
                  gatewaySignals[index].template asPtr<IMirroredSignalConfig>().getActiveStreamingSource());
    }

    auto clientSignal = client.getSignalsRecursive()[0].template asPtr<IMirroredSignalConfig>();
    std::promise<StringPtr> clientSignalSubscribePromise;
    std::future<StringPtr> clientSignalSubscribeFuture;
    test_helpers::setupSubscribeAckHandler(clientSignalSubscribePromise,
                                           clientSignalSubscribeFuture,
                                           clientSignal);

    using namespace std::chrono_literals;
    StreamReaderPtr reader = daq::StreamReader<double, uint64_t>(clientSignal);
    
    ASSERT_TRUE(test_helpers::waitForAcknowledgement(clientSignalSubscribeFuture));
    double samples[100];
    for (int i = 0; i < 10; ++i)
    {
        std::this_thread::sleep_for(100ms);
        daq::SizeT count = 100;
        reader.read(samples, &count);
        EXPECT_GT(count, 0u) << "iteration " << i;
    }
}

TEST_P(SubDevicesTest, LeafStreamingToGatewayAndClient)
{
    SKIP_TEST_MAC_CI;

    if (GetParam().second == StreamingType::WebsocketStreaming)
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
        auto mirroredSignalConfigPtr = clientSignals[index].template asPtr<IMirroredSignalConfig>();
        ASSERT_EQ(mirroredSignalConfigPtr.getStreamingSources().getCount(), 2u);
        ASSERT_TRUE(mirroredSignalConfigPtr.getActiveStreamingSource().assigned());

        ASSERT_EQ(mirroredSignalConfigPtr.getActiveStreamingSource(),
                  gatewaySignals[index].template asPtr<IMirroredSignalConfig>().getActiveStreamingSource());
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
    StreamReaderPtr clientReader = daq::StreamReader<double, uint64_t>(clientSignal);
    StreamReaderPtr gatewayReader = daq::StreamReader<double, uint64_t>(gatewaySignal);

    ASSERT_TRUE(test_helpers::waitForAcknowledgement(gatewaySignalSubscribeFuture));
    ASSERT_TRUE(test_helpers::waitForAcknowledgement(clientSignalSubscribeFuture));

    double clientSamples[100];
    double gatewaySamples[100];
    for (int i = 0; i < 10; ++i)
    {
        std::this_thread::sleep_for(100ms);

        daq::SizeT clientSamplesCount = 100;
        clientReader.read(clientSamples, &clientSamplesCount);
        EXPECT_GT(clientSamplesCount, 0u) << "iteration " << i;

        daq::SizeT gatewaySamplesCount = 100;
        gatewayReader.read(gatewaySamples, &gatewaySamplesCount);
        EXPECT_GT(gatewaySamplesCount, 0u) << "iteration " << i;
    }
}

#if defined(OPENDAQ_ENABLE_NATIVE_STREAMING) && !defined(OPENDAQ_ENABLE_WEBSOCKET_STREAMING)
INSTANTIATE_TEST_SUITE_P(
    SubDevicesTestGroup,
    SubDevicesTest,
    testing::Values(
        std::pair(StreamingType::NativeStreaming, StreamingType::NativeStreaming)
    )
);
#elif !defined(OPENDAQ_ENABLE_NATIVE_STREAMING) && defined(OPENDAQ_ENABLE_WEBSOCKET_STREAMING)
INSTANTIATE_TEST_SUITE_P(
    SubDevicesTestGroup,
    SubDevicesTest,
    testing::Values(
        std::pair(StreamingType::WebsocketStreaming, StreamingType::WebsocketStreaming)
    )
);
#elif defined(OPENDAQ_ENABLE_NATIVE_STREAMING) && defined(OPENDAQ_ENABLE_WEBSOCKET_STREAMING)
INSTANTIATE_TEST_SUITE_P(
    SubDevicesTestGroup,
    SubDevicesTest,
    testing::Values(
        std::pair(StreamingType::NativeStreaming, StreamingType::NativeStreaming),
        //std::pair(StreamingType::WebsocketStreaming, StreamingType::WebsocketStreaming),
        std::pair(StreamingType::NativeStreaming, StreamingType::WebsocketStreaming)
        /// note: next one does not work because websocket streaming does not stream domain signal packets
        /// if subdevice has enabled websocket streaming - the gateway device will not be able to stream-forward
        /// signals thru native streaming which requires domain packets to be streamed explicitly
        // std::pair(StreamingType::WebsocketStreaming, StreamingType::NativeStreaming)
    )
);
#endif
