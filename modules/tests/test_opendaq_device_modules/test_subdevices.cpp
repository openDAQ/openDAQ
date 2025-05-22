#include "test_helpers/test_helpers.h"
#include <opendaq/device_info_internal_ptr.h>
#include <coreobjects/authentication_provider_factory.h>
#include <testutils/test_helpers.h>


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

    virtual void addLtServer(InstancePtr& instance, uint16_t streamingPort, uint16_t controlPort)
    {
        auto ws_config = instance.getAvailableServerTypes().get("OpenDAQLTStreaming").createDefaultConfig();
        ws_config.setPropertyValue("WebsocketStreamingPort", streamingPort);
        ws_config.setPropertyValue("WebsocketControlPort", controlPort);
        instance.addServer("OpenDAQLTStreaming", ws_config);
    }

    virtual void addNativeServer(InstancePtr& instance, uint16_t port)
    {
        auto ns_config = instance.getAvailableServerTypes().get("OpenDAQNativeStreaming").createDefaultConfig();
        ns_config.setPropertyValue("NativeStreamingPort", port);
        instance.addServer("OpenDAQNativeStreaming", ns_config);
    }

    virtual void addOpcuaServer(InstancePtr& instance, uint16_t port)
    {
        auto ua_config = instance.getAvailableServerTypes().get("OpenDAQOPCUA").createDefaultConfig();
        ua_config.setPropertyValue("Port", port);
        instance.addServer("OpenDAQOPCUA", ua_config);
    }

    void testStreamClientSignalFromLeaf(const InstancePtr& client)
    {
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
            // TODO: needed becuase there are two descriptor changes,
            // due to reference domain "stuff" only being fully supported over Native
            // can be deleted once full support is added
            reader.read(nullptr, &count, 100);
        }

        double samples[100];
        for (int i = 0; i < 5; ++i)
        {
            daq::SizeT count = 100;
            reader.read(samples, &count, 1000);
            EXPECT_GT(count, 0u) << "iteration " << i;
        }
    }

    void testStreamGatewayAndClientSignals(const InstancePtr& client, const InstancePtr& gateway)
    {
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
            // TODO: needed becuase there are two descriptor changes,
            // due to reference domain "stuff" only being fully supported over Native
            // can be deleted once full support is added
            clientReader.read(nullptr, &count, 100);
            gatewayReader.read(nullptr, &count, 100);
        }

        double clientSamples[100];
        double gatewaySamples[100];
        for (int i = 0; i < 5; ++i)
        {
            daq::SizeT clientSamplesCount = 100;
            clientReader.read(clientSamples, &clientSamplesCount, 1000);
            EXPECT_GT(clientSamplesCount, 0u) << "iteration " << i;

            daq::SizeT gatewaySamplesCount = 100;
            gatewayReader.read(gatewaySamples, &gatewaySamplesCount, 1000);
            EXPECT_GT(gatewaySamplesCount, 0u) << "iteration " << i;
        }
    }

    void testStreamClientSignalViaGateway(const InstancePtr& client, const InstancePtr& gateway)
    {
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
            // TODO: needed becuase there are two descriptor changes,
            // due to reference domain "stuff" only being fully supported over Native
            // can be deleted once full support is added
            reader.read(nullptr, &count, 100);
        }

        double samples[100];
        for (int i = 0; i < 5; ++i)
        {
            daq::SizeT count = 100;
            reader.read(samples, &count, 1000);
            EXPECT_GT(count, 0u) << "iteration " << i;
        }
    }

    InstancePtr CreateLeafDeviceInstance(uint16_t leafDeviceIndex)
    {
        auto logger = Logger();
        auto scheduler = Scheduler(logger);
        auto moduleManager = ModuleManager("");
        auto typeManager = TypeManager();
        auto authenticationProvider = AuthenticationProvider();
        auto context = Context(scheduler, logger, typeManager, moduleManager, authenticationProvider);
        auto instance = InstanceCustom(context, fmt::format("subdevice{}", leafDeviceIndex));
        const auto refDevice = instance.addDevice("daqref://device0", test_helpers::createRefDeviceConfigWithRandomSerialNumber());

        auto structureProtocolType = std::get<0>(GetParam());

        addLtServer(instance, WEBSOCKET_STREAMING_PORT + leafDeviceIndex, WEBSOCKET_CONTROL_PORT + leafDeviceIndex);
        addNativeServer(instance, NATIVE_PORT + leafDeviceIndex);
        if (structureProtocolType == StructureProtocolType::OpcUa)
            addOpcuaServer(instance, OPCUA_PORT + leafDeviceIndex);

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
        ConnectLeafDeviceToGateway(instance, 1u);

        addLtServer(instance, WEBSOCKET_STREAMING_PORT, WEBSOCKET_CONTROL_PORT);
        addNativeServer(instance, NATIVE_PORT);
        if (std::get<0>(GetParam()) == StructureProtocolType::OpcUa)
            addOpcuaServer(instance, OPCUA_PORT);

        return instance;
    }

    void ConnectLeafDeviceToGateway(const InstancePtr& gatewayInstance, uint16_t leafDeviceIndex)
    {
        auto subdeviceStreamingType = std::get<1>(GetParam());

        auto streamingProtocolIds = (subdeviceStreamingType == StreamingProtocolType::NativeStreaming)
                                        ? List<IString>("OpenDAQNativeStreaming", "OpenDAQLTStreaming")
                                        : List<IString>("OpenDAQLTStreaming", "OpenDAQNativeStreaming");
        const auto config = createDeviceConfig(gatewayInstance, streamingProtocolIds, MIN_CONNECTIONS);
        gatewayInstance.addDevice(createStructureDeviceConnectionString(leafDeviceIndex), config);
    }

    InstancePtr addSecondLeafDevice(const InstancePtr& gatewayInstance, const InstancePtr& clientInstance, bool& success)
    {
        InstancePtr secondLeafDevice = nullptr;

        // dynamically added leaf device connections handled via core events
        if (std::get<0>(GetParam()) == StructureProtocolType::Native)
        {
            secondLeafDevice = CreateLeafDeviceInstance(2u);

            std::promise<void> leafDevAddedPromise;
            std::future<void> leafDevAddedFuture = leafDevAddedPromise.get_future();

            clientInstance.getContext().getOnCoreEvent() +=
                [&](const ComponentPtr& /*comp*/, const CoreEventArgsPtr& args)
            {
                auto params = args.getParameters();
                if (static_cast<CoreEventId>(args.getEventId()) == CoreEventId::ComponentAdded)
                {
                    ComponentPtr component = params.get("Component");
                    if (component.getLocalId() == "subdevice2")
                        leafDevAddedPromise.set_value();
                }
            };

            ConnectLeafDeviceToGateway(gatewayInstance, 2u);
            success = leafDevAddedFuture.wait_for(std::chrono::seconds(5)) == std::future_status::ready;
        }
        else
        {
            success = true;
        }
        return secondLeafDevice;
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
    auto firstLeafDevice = CreateLeafDeviceInstance(1u);
    InstancePtr secondLeafDevice;
    auto gateway = CreateGatewayInstance();
    auto client = CreateClientInstance(MIN_CONNECTIONS);

    bool success{false};
    secondLeafDevice = addSecondLeafDevice(gateway, client, success);
    ASSERT_TRUE(success);

    auto clientSignals = client.getSignals(search::Recursive(search::Visible()));
    auto gatewaySignals = gateway.getSignals(search::Recursive(search::Visible()));
    ASSERT_EQ(clientSignals.getCount(), gatewaySignals.getCount());

    for (size_t index = 0; index < clientSignals.getCount(); ++index)
    {
        auto clientSignal = clientSignals[index].template asPtr<IMirroredSignalConfig>();
        auto gatewaySignal = gatewaySignals[index].template asPtr<IMirroredSignalConfig>();
        ASSERT_EQ(clientSignal.getStreamingSources().getCount(), 2u) << clientSignal.getGlobalId();
        ASSERT_EQ(gatewaySignal.getStreamingSources().getCount(), 2u) << gatewaySignal.getGlobalId();
        ASSERT_TRUE(clientSignal.getActiveStreamingSource().assigned());
        ASSERT_NE(clientSignal.getActiveStreamingSource(), gatewaySignal.getStreamingSources()[0])
            << "client sig " << clientSignal.getGlobalId()
            << " streaming source " << clientSignal.getActiveStreamingSource();
        ASSERT_NE(clientSignal.getActiveStreamingSource(), gatewaySignal.getStreamingSources()[1])
            << "client sig " << clientSignal.getGlobalId()
            << " streaming source " << clientSignal.getActiveStreamingSource();
    }

#ifdef OPENDAQ_ENABLE_OPTIONAL_TESTS
#ifndef OPENDAQ_SKIP_UNSTABLE_TESTS
    testStreamClientSignalViaGateway(client, gateway);
#endif
#endif
}

TEST_P(SubDevicesTest, LeafStreamingToClient)
{
    SKIP_TEST_MAC_CI;
    auto firstLeafDevice = CreateLeafDeviceInstance(1u);
    InstancePtr secondLeafDevice;
    auto gateway = CreateGatewayInstance();
    auto client = CreateClientInstance(MIN_HOPS);

    bool success{false};
    secondLeafDevice = addSecondLeafDevice(gateway, client, success);
    ASSERT_TRUE(success);

    auto clientSignals = client.getSignals(search::Recursive(search::Visible()));
    auto gatewaySignals = gateway.getSignals(search::Recursive(search::Visible()));
    ASSERT_EQ(clientSignals.getCount(), gatewaySignals.getCount());

    for (size_t index = 0; index < clientSignals.getCount(); ++index)
    {
        auto clientSignal = clientSignals[index].template asPtr<IMirroredSignalConfig>();
        auto gatewaySignal = gatewaySignals[index].template asPtr<IMirroredSignalConfig>();
        ASSERT_EQ(clientSignal.getStreamingSources().getCount(), 4u) << clientSignal.getGlobalId();
        ASSERT_EQ(gatewaySignal.getStreamingSources().getCount(), 2u) << gatewaySignal.getGlobalId();
        ASSERT_TRUE(clientSignal.getActiveStreamingSource().assigned());
        ASSERT_TRUE(clientSignal.getActiveStreamingSource() == gatewaySignal.getStreamingSources()[0] ||
                    clientSignal.getActiveStreamingSource() == gatewaySignal.getStreamingSources()[1])
            << "client sig " << clientSignal.getGlobalId()
            << " streaming source " << clientSignal.getActiveStreamingSource();
    }

#ifdef OPENDAQ_ENABLE_OPTIONAL_TESTS
    testStreamClientSignalFromLeaf(client);
#endif
}

TEST_P(SubDevicesTest, LeafStreamingToGatewayAndClient)
{
    SKIP_TEST_MAC_CI;
    auto firstLeafDevice = CreateLeafDeviceInstance(1u);
    InstancePtr secondLeafDevice;
    auto gateway = CreateGatewayInstance();
    auto client = CreateClientInstance(MIN_HOPS);

    bool success{false};
    secondLeafDevice = addSecondLeafDevice(gateway, client, success);
    ASSERT_TRUE(success);

    auto clientSignals = client.getSignalsRecursive();
    auto gatewaySignals = gateway.getSignalsRecursive();
    ASSERT_EQ(clientSignals.getCount(), gatewaySignals.getCount());

    for (size_t index = 0; index < clientSignals.getCount(); ++index)
    {
        auto clientSignal = clientSignals[index].template asPtr<IMirroredSignalConfig>();
        auto gatewaySignal = gatewaySignals[index].template asPtr<IMirroredSignalConfig>();
        ASSERT_EQ(clientSignal.getStreamingSources().getCount(), 4u) << clientSignal.getGlobalId();
        ASSERT_EQ(gatewaySignal.getStreamingSources().getCount(), 2u) << gatewaySignal.getGlobalId();
        ASSERT_TRUE(clientSignal.getActiveStreamingSource().assigned());
        ASSERT_TRUE(clientSignal.getActiveStreamingSource() == gatewaySignal.getStreamingSources()[0] ||
                    clientSignal.getActiveStreamingSource() == gatewaySignal.getStreamingSources()[1])
            << "client sig " << clientSignal.getGlobalId()
            << " streaming source " << clientSignal.getActiveStreamingSource();
    }

#ifdef OPENDAQ_ENABLE_OPTIONAL_TESTS
    if (std::get<2>(GetParam()) == StreamingProtocolType::WebsocketStreaming)
    {
        GTEST_SKIP();
    }
    testStreamGatewayAndClientSignals(client, gateway);
#endif
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

class SubDevicesReconnectionTest : public SubDevicesTest
{
};

TEST_P(SubDevicesReconnectionTest, LeafStreamingToClientAfterReconnect)
{
    SKIP_TEST_MAC_CI;
    auto firstLeafDevice = CreateLeafDeviceInstance(1u);
    auto gateway = CreateGatewayInstance();
    auto client = CreateClientInstance(MIN_HOPS);

    ASSERT_EQ(client.getDevices()[0].getConnectionStatusContainer().getStatus("ConfigurationStatus"), "Connected");

    auto clientSignals = client.getSignals(search::Recursive(search::Visible()));
    auto gatewaySignals = gateway.getSignals(search::Recursive(search::Visible()));
    ASSERT_EQ(clientSignals.getCount(), gatewaySignals.getCount());

    for (size_t index = 0; index < clientSignals.getCount(); ++index)
    {
        auto clientSignal = clientSignals[index].template asPtr<IMirroredSignalConfig>();
        auto gatewaySignal = gatewaySignals[index].template asPtr<IMirroredSignalConfig>();
        ASSERT_EQ(clientSignal.getStreamingSources().getCount(), 4u) << clientSignal.getGlobalId();
        ASSERT_EQ(gatewaySignal.getStreamingSources().getCount(), 2u) << gatewaySignal.getGlobalId();
        ASSERT_TRUE(clientSignal.getActiveStreamingSource().assigned());
        ASSERT_TRUE(clientSignal.getActiveStreamingSource() == gatewaySignal.getStreamingSources()[0] ||
                    clientSignal.getActiveStreamingSource() == gatewaySignal.getStreamingSources()[1])
            << "client sig " << clientSignal.getGlobalId()
            << " streaming source " << clientSignal.getActiveStreamingSource();
    }
    gatewaySignals.clear();

    std::promise<StringPtr> reconnectionStatusPromise;
    std::future<StringPtr> reconnectionStatusFuture = reconnectionStatusPromise.get_future();
    client.getDevices()[0].getOnComponentCoreEvent() += [&](ComponentPtr& /*comp*/, CoreEventArgsPtr& args)
    {
        auto params = args.getParameters();
        if (static_cast<CoreEventId>(args.getEventId()) == CoreEventId::ConnectionStatusChanged &&
            args.getParameters().get("StatusName") == "ConfigurationStatus")
            reconnectionStatusPromise.set_value(args.getParameters().get("StatusValue").toString());
    };

    gateway.release();

    ASSERT_TRUE(reconnectionStatusFuture.wait_for(std::chrono::seconds(5)) == std::future_status::ready);
    ASSERT_EQ(reconnectionStatusFuture.get(), "Reconnecting");
    ASSERT_EQ(client.getDevices()[0].getConnectionStatusContainer().getStatus("ConfigurationStatus"), "Reconnecting");

    // reset future / promise
    reconnectionStatusPromise = std::promise<StringPtr>();
    reconnectionStatusFuture = reconnectionStatusPromise.get_future();

    // recreate gateway and auto-reconnect
    gateway = CreateGatewayInstance();
    ASSERT_TRUE(reconnectionStatusFuture.wait_for(std::chrono::seconds(5)) == std::future_status::ready);
    ASSERT_EQ(reconnectionStatusFuture.get(), "Connected");
    ASSERT_EQ(client.getDevices()[0].getConnectionStatusContainer().getStatus("ConfigurationStatus"), "Connected");

    // test streaming sources again
    gatewaySignals = gateway.getSignals(search::Recursive(search::Visible()));
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
}

INSTANTIATE_TEST_SUITE_P(
    SubDevicesReconnectionTestGroup,
    SubDevicesReconnectionTest,
    testing::Values(
        std::make_tuple(StructureProtocolType::Native, StreamingProtocolType::NativeStreaming, StreamingProtocolType::NativeStreaming)
    )
);
