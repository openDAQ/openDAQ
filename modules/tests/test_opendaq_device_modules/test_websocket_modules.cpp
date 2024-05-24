#include "test_helpers/test_helpers.h"
#include <coreobjects/authentication_provider_factory.h>

using WebsocketModulesTest = testing::Test;
using namespace daq;

static InstancePtr CreateServerInstance()
{
    auto logger = Logger();
    auto scheduler = Scheduler(logger);
    auto moduleManager = ModuleManager("");
    auto typeManager = TypeManager();
    auto authenticationProvider = AuthenticationProvider();
    auto context = Context(scheduler, logger, typeManager, moduleManager, authenticationProvider);

    auto instance = InstanceCustom(context, "local");

    const auto statistics = instance.addFunctionBlock("ref_fb_module_statistics");
    const auto refDevice = instance.addDevice("daqref://device1");
    statistics.getInputPorts()[0].connect(refDevice.getSignals(search::Recursive(search::Visible()))[0]);

    instance.addServer("openDAQ LT Streaming", nullptr);

    return instance;
}

static InstancePtr CreateClientInstance()
{
    auto instance = Instance();
    auto refDevice = instance.addDevice("daq.lt://127.0.0.1/");
    return instance;
}

TEST_F(WebsocketModulesTest, ConnectFail)
{
    ASSERT_THROW(CreateClientInstance(), NotFoundException);
}

TEST_F(WebsocketModulesTest, ConnectAndDisconnect)
{
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();
}

TEST_F(WebsocketModulesTest, ConnectAndDisconnectBackwardCompatibility)
{
    auto server = CreateServerInstance();
    auto client = Instance();
    client.addDevice("daq.ws://127.0.0.1/", nullptr);
}

TEST_F(WebsocketModulesTest, ConnectViaIpv6)
{
    if (test_helpers::Ipv6IsDisabled())
        return;

    auto server = CreateServerInstance();
    auto client = Instance();
    client.addDevice("daq.lt://[::1]", nullptr);
}

TEST_F(WebsocketModulesTest, PopulateDefaultConfigFromProvider)
{
    std::string filename = "populateDefaultConfig.json";
    std::string json = R"(
        {
            "Modules":
            {
                "StreamingLtServer":
                {
                    "ServiceDiscoverable": true,
                    "Port": 1234,
                    "ServicePath": "/some/path"
                }
            }
        }
    )";
    auto finally = test_helpers::CreateConfigFile(filename, json);

    auto provider = JsonConfigProvider(filename);
    auto instance = InstanceBuilder().addConfigProvider(provider).build();
    auto serverConfig = instance.getAvailableServerTypes().get("openDAQ LT Streaming").createDefaultConfig();

    ASSERT_TRUE(serverConfig.getPropertyValue("ServiceDiscoverable").asPtr<IBoolean>());
    ASSERT_EQ(serverConfig.getPropertyValue("Port").asPtr<IInteger>(), 1234);
    ASSERT_EQ(serverConfig.getPropertyValue("ServicePath").asPtr<IString>(), "/some/path");
}

TEST_F(WebsocketModulesTest, DiscoveringServer)
{
    auto server = InstanceBuilder().addDiscoveryService("mdns").setDefaultRootDeviceLocalId("local").build();
    server.addDevice("daqref://device1");

    auto serverConfig = server.getAvailableServerTypes().get("openDAQ LT Streaming").createDefaultConfig();
    auto servicePath = "/test/streaming_lt/discovery/";
    serverConfig.setPropertyValue("ServiceDiscoverable", true);
    serverConfig.setPropertyValue("ServicePath", servicePath);
    server.addServer("openDAQ LT Streaming", serverConfig);

    auto client = Instance();
    DevicePtr device;
    for (const auto & deviceInfo : client.getAvailableDevices())
    {
        for (const auto & capability : deviceInfo.getServerCapabilities())
        {
            if (!test_helpers::isSufix(deviceInfo.getConnectionString(), servicePath))
            {
                break;
            }
            if (capability.getProtocolName() == "openDAQ LT Streaming")
            {
                device = client.addDevice(deviceInfo.getConnectionString(), nullptr);
                return;
            }
        }
    }
    ASSERT_TRUE(false);
}


TEST_F(WebsocketModulesTest, checkDeviceInfoPopulatedWithProvider)
{
    std::string filename = "populateDefaultConfig.json";
    std::string json = R"(
        {
            "Modules":
            {
                "StreamingLtServer":
                {
                    "ServiceDiscoverable": true,
                    "Port": 1234,
                    "ServicePath": "/test/streaming_lt/checkDeviceInfoPopulated"
                }
            }
        }
    )";
    auto servicePath = "/test/streaming_lt/checkDeviceInfoPopulated";
    auto finally = test_helpers::CreateConfigFile(filename, json);

    auto rootInfo = DeviceInfo("");
    rootInfo.setName("WebsocketModulesTest::checkDeviceInfoPopulatedWithProvider");
    rootInfo.setManufacturer("Manufacturer");
    rootInfo.setModel("Model");
    rootInfo.setSerialNumber("SerialNumber");

    auto provider = JsonConfigProvider(filename);
    auto instance = InstanceBuilder().addDiscoveryService("mdns")
                                     .addConfigProvider(provider)
                                     .setDefaultRootDeviceInfo(rootInfo)
                                     .build();
    instance.addDevice("daqref://device1");
    auto serverConfig = instance.getAvailableServerTypes().get("openDAQ LT Streaming").createDefaultConfig();
    instance.addServer("openDAQ LT Streaming", serverConfig);

    auto client = Instance();

    for (const auto & deviceInfo : client.getAvailableDevices())
    {
        if (deviceInfo.getSerialNumber() != rootInfo.getSerialNumber())
            continue;

        for (const auto & capability : deviceInfo.getServerCapabilities())
        {
            if (capability.getProtocolName() == "openDAQ LT Streaming")
            {
                client.addDevice(deviceInfo.getConnectionString(), nullptr);

                ASSERT_EQ(deviceInfo.getName(), rootInfo.getName());
                ASSERT_EQ(deviceInfo.getManufacturer(), rootInfo.getManufacturer());
                ASSERT_EQ(deviceInfo.getModel(), rootInfo.getModel());
                ASSERT_EQ(deviceInfo.getSerialNumber(), rootInfo.getSerialNumber());
                ASSERT_TRUE(test_helpers::isSufix(capability.getConnectionString(), servicePath));
                return;
            }
        }      
    }

    ASSERT_TRUE(false);
}

TEST_F(WebsocketModulesTest, GetRemoteDeviceObjects)
{
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    ASSERT_EQ(client.getDevices().getCount(), 1u);
    auto signals = client.getSignals(search::Recursive(search::Visible()));
    ASSERT_EQ(signals.getCount(), 7u);
}

TEST_F(WebsocketModulesTest, SignalConfig_Server)
{
    const std::string newSignalName{"some new name"};

    auto server = CreateServerInstance();

    auto serverSignal = server.getSignals(search::Recursive(search::Visible()))[0].asPtr<ISignalConfig>();
    auto serverSignalDataDescriptor = DataDescriptorBuilderCopy(serverSignal.getDescriptor()).setName(newSignalName).build();
    serverSignal.setDescriptor(serverSignalDataDescriptor);

    auto client = CreateClientInstance();

    auto clientSignals = client.getDevices()[0].getSignals(search::Recursive(search::Visible()));
    auto clientSignal = clientSignals[0].asPtr<ISignalConfig>();

    auto clientSignalDataDescriptor = DataDescriptorBuilderCopy(clientSignal.getDescriptor()).build();

    ASSERT_EQ(serverSignal.getDescriptor().getName(), newSignalName);
    ASSERT_EQ(serverSignal.getDescriptor().getName(), clientSignal.getDescriptor().getName());
}

TEST_F(WebsocketModulesTest, DataDescriptor)
{
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    DataDescriptorPtr dataDescriptor = client.getSignals(search::Recursive(search::Visible()))[0].getDescriptor();
    DataDescriptorPtr serverDataDescriptor = server.getSignals(search::Recursive(search::Visible()))[0].getDescriptor();

    DataDescriptorPtr domainDataDescriptor = client.getSignals(search::Recursive(search::Visible()))[0].getDomainSignal().getDescriptor();
    DataDescriptorPtr serverDomainDataDescriptor = server.getSignals(search::Recursive(search::Visible()))[0].getDomainSignal().getDescriptor();

    ASSERT_EQ(dataDescriptor, serverDataDescriptor);

    ASSERT_EQ(domainDataDescriptor.getRule().getParameters(), serverDomainDataDescriptor.getRule().getParameters());
    ASSERT_EQ(domainDataDescriptor.getOrigin(), serverDomainDataDescriptor.getOrigin());
    ASSERT_EQ(domainDataDescriptor.getTickResolution(), serverDomainDataDescriptor.getTickResolution());
}

TEST_F(WebsocketModulesTest, SubscribeReadUnsubscribe)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    auto signal = client.getSignalsRecursive()[0].template asPtr<IMirroredSignalConfig>();

    StringPtr streamingSource = signal.getActiveStreamingSource();

    std::promise<StringPtr> signalSubscribePromise;
    std::future<StringPtr> signalSubscribeFuture;
    test_helpers::setupSubscribeAckHandler(signalSubscribePromise, signalSubscribeFuture, signal);

    std::promise<StringPtr> signalUnsubscribePromise;
    std::future<StringPtr> signalUnsubscribeFuture;
    test_helpers::setupUnsubscribeAckHandler(signalUnsubscribePromise, signalUnsubscribeFuture, signal);

    using namespace std::chrono_literals;
    StreamReaderPtr reader = daq::StreamReader<double, uint64_t>(signal);

    ASSERT_TRUE(test_helpers::waitForAcknowledgement(signalSubscribeFuture));
    ASSERT_EQ(signalSubscribeFuture.get(), streamingSource);

    double samples[100];
    for (int i = 0; i < 10; ++i)
    {
        std::this_thread::sleep_for(100ms);
        daq::SizeT count = 100;
        reader.read(samples, &count);
        EXPECT_GT(count, 0u) << "iteration " << i;
    }

    reader.release();

    ASSERT_TRUE(test_helpers::waitForAcknowledgement(signalUnsubscribeFuture));
    ASSERT_EQ(signalUnsubscribeFuture.get(), streamingSource);
}

TEST_F(WebsocketModulesTest, DISABLED_RenderSignal)
{
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    auto signals = client.getSignals(search::Recursive(search::Visible()));
    const auto renderer = client.addFunctionBlock("ref_fb_module_renderer");
    renderer.getInputPorts()[0].connect(signals[0]);

    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
}
