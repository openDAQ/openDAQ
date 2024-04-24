#include <gtest/gtest.h>
#include <opendaq/opendaq.h>
#include <thread>
#include "docs_test_helpers.h"

using StreamingConfigTest = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

// Corresponding document: Antora/modules/howto_guides/pages/howto_configure_streaming.adoc
TEST_F(StreamingConfigTest, ModifyConfiguration)
{
    InstancePtr instance = Instance();

    DictPtr<IString, IDeviceType> deviceTypes;
    deviceTypes = instance.getAvailableDeviceTypes();
    ASSERT_TRUE(deviceTypes.hasKey("opendaq_opcua_config"));

    PropertyObjectPtr deviceConfig = deviceTypes.get("opendaq_opcua_config").createDefaultConfig();
    ASSERT_TRUE(deviceConfig.assigned());

    ASSERT_TRUE(deviceConfig.hasProperty("AllowedStreamingProtocols"));
    ASSERT_NO_THROW(
        deviceConfig.setPropertyValue("AllowedStreamingProtocols", List<IString>("opendaq_native_streaming", "opendaq_lt_streaming")));

    ASSERT_TRUE(deviceConfig.hasProperty("PrimaryStreamingProtocol"));
    ASSERT_NO_THROW(deviceConfig.setPropertyValue("PrimaryStreamingProtocol", "opendaq_lt_streaming"));

    ASSERT_TRUE(deviceConfig.hasProperty("StreamingConnectionHeuristic"));
    ASSERT_NO_THROW(deviceConfig.setPropertyValue("StreamingConnectionHeuristic", 0));
}

// Corresponding document: Antora/modules/howto_guides/pages/howto_configure_streaming.adoc
TEST_F(StreamingConfigTest, AddPseudoDevice)
{
    SKIP_TEST_MAC_CI;

    InstancePtr server = docs_test_helpers::setupSimulatorServers();
    InstancePtr instance = Instance();

    DevicePtr device = instance.addDevice("daq.ns://127.0.0.1");
    ASSERT_TRUE(device.assigned());

    ASSERT_EQ(device.getInfo().getName(), "NativeStreamingClientPseudoDevice");
}

// Corresponding document: Antora/modules/howto_guides/pages/howto_configure_streaming.adoc
TEST_F(StreamingConfigTest, AddDeviceWithConfig)
{
    SKIP_TEST_MAC_CI;

    InstancePtr server = docs_test_helpers::setupSimulatorServers();
    InstancePtr instance = Instance();

    PropertyObjectPtr deviceConfig = instance.getAvailableDeviceTypes().get("opendaq_opcua_config").createDefaultConfig();
    deviceConfig.setPropertyValue("AllowedStreamingProtocols", List<IString>("opendaq_native_streaming", "opendaq_lt_streaming"));
    deviceConfig.setPropertyValue("PrimaryStreamingProtocol", "opendaq_lt_streaming");
    deviceConfig.setPropertyValue("StreamingConnectionHeuristic", 0);

    DevicePtr device = instance.addDevice("daq.opcua://127.0.0.1", deviceConfig);
    ASSERT_TRUE(device.assigned());

    ASSERT_EQ(device.getInfo().getName(), "Device 1");
}

// Corresponding document: Antora/modules/howto_guides/pages/howto_configure_streaming.adoc
TEST_F(StreamingConfigTest, StreamingSources)
{
    SKIP_TEST_MAC_CI;

    InstancePtr server = docs_test_helpers::setupSimulatorServers();
    InstancePtr instance = Instance();

    PropertyObjectPtr deviceConfig = instance.getAvailableDeviceTypes().get("opendaq_opcua_config").createDefaultConfig();
    deviceConfig.setPropertyValue("AllowedStreamingProtocols", List<IString>("opendaq_native_streaming", "opendaq_lt_streaming"));
    deviceConfig.setPropertyValue("PrimaryStreamingProtocol", "opendaq_lt_streaming");
    deviceConfig.setPropertyValue("StreamingConnectionHeuristic", 0);

    DevicePtr device = instance.addDevice("daq.opcua://127.0.0.1", deviceConfig);
    ASSERT_TRUE(device.assigned());

    MirroredSignalConfigPtr signal = device.getSignalsRecursive()[0];

    ASSERT_TRUE(signal.getActiveStreamingSource().assigned());
    ASSERT_TRUE(signal.getActiveStreamingSource().toView().find("daq.lt://") != std::string::npos);
    ASSERT_EQ(signal.getActiveStreamingSource(), "daq.lt://127.0.0.1:7414");

    ListPtr<IString> streamingSources = signal.getStreamingSources();
    ASSERT_EQ(streamingSources.getCount(), 2u);

    ASSERT_NE(std::find(streamingSources.begin(), streamingSources.end(), "daq.lt://127.0.0.1:7414"), streamingSources.end());
    ASSERT_NE(std::find(streamingSources.begin(), streamingSources.end(), "daq.ns://127.0.0.1:7420"), streamingSources.end());

    ASSERT_TRUE(signal.getStreamed());
}

// Corresponding document: Antora/modules/howto_guides/pages/howto_configure_streaming.adoc
TEST_F(StreamingConfigTest, WebsocketStreamingRead)
{
    SKIP_TEST_MAC_CI;

    InstancePtr server = docs_test_helpers::setupSimulatorServers();
    InstancePtr instance = Instance();

    PropertyObjectPtr deviceConfig = instance.getAvailableDeviceTypes().get("opendaq_opcua_config").createDefaultConfig();
    deviceConfig.setPropertyValue("AllowedStreamingProtocols", List<IString>("opendaq_native_streaming", "opendaq_lt_streaming"));
    deviceConfig.setPropertyValue("PrimaryStreamingProtocol", "opendaq_lt_streaming");
    deviceConfig.setPropertyValue("StreamingConnectionHeuristic", 0);

    DevicePtr device = instance.addDevice("daq.opcua://127.0.0.1", deviceConfig);
    ASSERT_TRUE(device.assigned());

    MirroredSignalConfigPtr signal = device.getSignalsRecursive()[0];
    ASSERT_NO_THROW(signal.setActiveStreamingSource("daq.lt://127.0.0.1:7414"));

    using namespace std::chrono_literals;
    StreamReaderPtr reader = StreamReader<double, uint64_t>(signal);

    double samples[100];
    for (int i = 0; i < 5; ++i)
    {
        docs_test_helpers::waitForSamplesReady();
        SizeT count = 100;
        reader.read(samples, &count);
        ASSERT_GT(count, 0u);
    }
}

// Corresponding document: Antora/modules/howto_guides/pages/howto_configure_streaming.adoc
TEST_F(StreamingConfigTest, NativeStreamingRead)
{
    SKIP_TEST_MAC_CI;

    InstancePtr server = docs_test_helpers::setupSimulatorServers();
    InstancePtr instance = Instance();

    PropertyObjectPtr deviceConfig = instance.getAvailableDeviceTypes().get("opendaq_opcua_config").createDefaultConfig();
    deviceConfig.setPropertyValue("AllowedStreamingProtocols", List<IString>("opendaq_native_streaming", "opendaq_lt_streaming"));
    deviceConfig.setPropertyValue("PrimaryStreamingProtocol", "opendaq_lt_streaming");
    deviceConfig.setPropertyValue("StreamingConnectionHeuristic", 0);

    DevicePtr device = instance.addDevice("daq.opcua://127.0.0.1", deviceConfig);
    ASSERT_TRUE(device.assigned());

    MirroredSignalConfigPtr signal = device.getSignals(search::Recursive(search::Any()))[0];
    ASSERT_NO_THROW(signal.setActiveStreamingSource("daq.ns://127.0.0.1:7420"));

    using namespace std::chrono_literals;
    StreamReaderPtr reader = StreamReader<double, uint64_t>(signal);

    double samples[100];
    for (int i = 0; i < 5; ++i)
    {
        docs_test_helpers::waitForSamplesReady();
        SizeT count = 100;
        reader.read(samples, &count);
        ASSERT_GT(count, 0u);
    }
}

// Corresponding document: Antora/modules/howto_guides/pages/howto_configure_streaming.adoc
TEST_F(StreamingConfigTest, ServerSideConfiguration)
{
    // using namespace std::chrono_literals;

    const InstancePtr instance = Instance();

    instance.setRootDevice("daqref://device1");

    // Creates and registers a Server capability with the ID `opendaq_lt_streaming` and the default port number 7414
    instance.addServer("openDAQ LT Streaming", nullptr);

    // Creates and registers a Server capability with the ID `opendaq_native_streaming` and the default port number 7420
    instance.addServer("openDAQ Native Streaming", nullptr);

    // As the Streaming servers were added first, the registered Server capabilities are published over OPC UA
    instance.addServer("openDAQ OpcUa", nullptr);

    // while (true)
    //     std::this_thread::sleep_for(100ms);
}

END_NAMESPACE_OPENDAQ
