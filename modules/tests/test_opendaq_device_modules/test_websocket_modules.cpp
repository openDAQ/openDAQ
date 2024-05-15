#include "test_helpers/test_helpers.h"
#include "../../../core/opendaq/opendaq/tests/test_config_provider.h"
#include <coreobjects/authentication_provider_factory.h>

using WebsocketModulesTest = testing::Test;
using namespace test_config_provider_helpers;
using WebsocketModuleTestConfig = ConfigProviderTest;
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

TEST_F(WebsocketModuleTestConfig, PopulateDefaultConfigFromProvider)
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
                    "Name": "streaming lt server",
                    "Manufacturer": "test",
                    "Model": "test device",
                    "SerialNumber": "test_serial_number",
                    "ServicePath": "/some/path"
                }
            }
        }
    )";
    createConfigFile(filename, json);

    auto provider = JsonConfigProvider(filename);
    auto instance = InstanceBuilder().addConfigProvider(provider).build();
    auto serverConfig = instance.getAvailableServerTypes().get("openDAQ LT Streaming").createDefaultConfig();

    ASSERT_TRUE(serverConfig.getPropertyValue("ServiceDiscoverable").asPtr<IBoolean>());
    ASSERT_EQ(serverConfig.getPropertyValue("Port").asPtr<IInteger>(), 1234);
    ASSERT_EQ(serverConfig.getPropertyValue("Name").asPtr<IString>(), "streaming lt server");
    ASSERT_EQ(serverConfig.getPropertyValue("Manufacturer").asPtr<IString>(), "test");
    ASSERT_EQ(serverConfig.getPropertyValue("Model").asPtr<IString>(), "test device");
    ASSERT_EQ(serverConfig.getPropertyValue("SerialNumber").asPtr<IString>(), "test_serial_number");
    ASSERT_EQ(serverConfig.getPropertyValue("ServicePath").asPtr<IString>(), "/some/path");
}

TEST_F(WebsocketModulesTest, DiscoveringServer)
{
    auto server = InstanceBuilder().setDefaultRootDeviceLocalId("local").build();
    const auto statistics = server.addFunctionBlock("ref_fb_module_statistics");
    const auto refDevice = server.addDevice("daqref://device1");
    statistics.getInputPorts()[0].connect(refDevice.getSignals(search::Recursive(search::Visible()))[0]);

    auto serverConfig = server.getAvailableServerTypes().get("openDAQ LT Streaming").createDefaultConfig();
    auto serialNumber = "WebsocketModulesTest_DiscoveringServer_" + test_helpers::getHostname() + "_" + serverConfig.getPropertyValue("Port").toString();
    serverConfig.setPropertyValue("SerialNumber", serialNumber);
    serverConfig.setPropertyValue("ServiceDiscoverable", true);
    server.addServer("openDAQ LT Streaming", serverConfig);

    auto client = Instance();
    DevicePtr device;
    for (const auto & deviceInfo : client.getAvailableDevices())
    {
        if (deviceInfo.getSerialNumber() != serialNumber)
            continue;
        for (const auto & capability : deviceInfo.getServerCapabilities())
        {
            if (capability.getProtocolName() == "openDAQ LT Streaming")
            {
                device = client.addDevice(deviceInfo.getConnectionString(), nullptr);
                break;
            }
        }
    }
    ASSERT_TRUE(device.assigned());
}


TEST_F(WebsocketModuleTestConfig, checkDeviceInfoPopulatedWithProvider)
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
                    "Name": "streaming lt server",
                    "Manufacturer": "test",
                    "Model": "test device",
                    "SerialNumber": "streaming_lt_test_serial_number",
                    "ServicePath": "/some/path"
                }
            }
        }
    )";
    createConfigFile(filename, json);

    auto provider = JsonConfigProvider(filename);
    auto instance = InstanceBuilder().addConfigProvider(provider).build();
    auto serverConfig = instance.getAvailableServerTypes().get("openDAQ LT Streaming").createDefaultConfig();
    instance.addServer("openDAQ LT Streaming", serverConfig);

    auto client = Instance();
    DevicePtr device;
    for (const auto & deviceInfo : client.getAvailableDevices())
    {
        if (deviceInfo.getSerialNumber() != "streaming_lt_test_serial_number")
            continue;

        ASSERT_EQ(deviceInfo.getServerCapabilities().getCount(), 1u);
        device = client.addDevice(deviceInfo.getConnectionString(), nullptr);

        ASSERT_EQ(deviceInfo.getName(), "streaming lt server");
        ASSERT_EQ(deviceInfo.getManufacturer(), "test");
        ASSERT_EQ(deviceInfo.getModel(), "test device");
        ASSERT_EQ(deviceInfo.getSerialNumber(), "streaming_lt_test_serial_number");

        std::string connectionString = deviceInfo.getConnectionString();    
        std::string servicePath = "/some/path";
        std::string connectionPath = connectionString.substr(connectionString.size() - servicePath.size());
        ASSERT_EQ(connectionPath, servicePath);
    }

    ASSERT_TRUE(device.assigned());
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
