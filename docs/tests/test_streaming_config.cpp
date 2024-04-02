#include <gtest/gtest.h>
#include <thread>
#include <opendaq/opendaq.h>
#include "docs_test_helpers.h"

using StreamingConfigTest = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

// Corresponding document: Antora/modules/howto_guides/pages/howto_configure_streaming.adoc
TEST_F(StreamingConfigTest, ModifyConfiguration)
{
    daq::InstancePtr instance = daq::Instance();

    daq::DictPtr<daq::IString, daq::IDeviceType> deviceTypes;
    deviceTypes = instance.getAvailableDeviceTypes();
    ASSERT_TRUE(deviceTypes.hasKey("daq.opcua"));

    daq::PropertyObjectPtr deviceConfig = deviceTypes.get("daq.opcua").createDefaultConfig();
    ASSERT_TRUE(deviceConfig.assigned());

    ASSERT_TRUE(deviceConfig.hasProperty("AllowedStreamingProtocols"));
    ASSERT_NO_THROW(deviceConfig.setPropertyValue("AllowedStreamingProtocols",
                                                  daq::List<daq::IString>("daq.ns", "daq.ws")));

    ASSERT_TRUE(deviceConfig.hasProperty("PrimaryStreamingProtocol"));
    ASSERT_NO_THROW(deviceConfig.setPropertyValue("PrimaryStreamingProtocol", "daq.ws"));

    ASSERT_TRUE(deviceConfig.hasProperty("StreamingConnectionHeuristic"));
    ASSERT_NO_THROW(deviceConfig.setPropertyValue("StreamingConnectionHeuristic", 0));
}

// Corresponding document: Antora/modules/howto_guides/pages/howto_configure_streaming.adoc
TEST_F(StreamingConfigTest, AddPseudoDevice)
{
    SKIP_TEST_MAC_CI;

    InstancePtr server = docs_test_helpers::setupSimulatorServers();
    daq::InstancePtr instance = daq::Instance();

    daq::DevicePtr device = instance.addDevice("daq.ns://127.0.0.1");
    ASSERT_TRUE(device.assigned());

    ASSERT_EQ(device.getInfo().getName(), "NativeStreamingClientPseudoDevice");
}

// Corresponding document: Antora/modules/howto_guides/pages/howto_configure_streaming.adoc
TEST_F(StreamingConfigTest, AddDeviceWithConfig)
{
    SKIP_TEST_MAC_CI;

    InstancePtr server = docs_test_helpers::setupSimulatorServers();
    daq::InstancePtr instance = daq::Instance();

    daq::PropertyObjectPtr deviceConfig = instance.getAvailableDeviceTypes().get("daq.opcua").createDefaultConfig();
    deviceConfig.setPropertyValue("AllowedStreamingProtocols", daq::List<daq::IString>("daq.ns", "daq.ws"));
    deviceConfig.setPropertyValue("PrimaryStreamingProtocol", "daq.ws");
    deviceConfig.setPropertyValue("StreamingConnectionHeuristic", 0);

    daq::DevicePtr device = instance.addDevice("daq.opcua://127.0.0.1", deviceConfig);
    ASSERT_TRUE(device.assigned());

    ASSERT_EQ(device.getInfo().getName(), "Device 1");
}

// Corresponding document: Antora/modules/howto_guides/pages/howto_configure_streaming.adoc
TEST_F(StreamingConfigTest, StreamingSources)
{
    SKIP_TEST_MAC_CI;

    InstancePtr server = docs_test_helpers::setupSimulatorServers();
    daq::InstancePtr instance = daq::Instance();

    daq::PropertyObjectPtr deviceConfig = instance.getAvailableDeviceTypes().get("daq.opcua").createDefaultConfig();
    deviceConfig.setPropertyValue("AllowedStreamingProtocols", daq::List<daq::IString>("daq.ns", "daq.ws"));
    deviceConfig.setPropertyValue("PrimaryStreamingProtocol", "daq.ws");
    deviceConfig.setPropertyValue("StreamingConnectionHeuristic", 0);

    daq::DevicePtr device = instance.addDevice("daq.opcua://127.0.0.1", deviceConfig);
    ASSERT_TRUE(device.assigned());

    daq::MirroredSignalConfigPtr signal = device.getSignalsRecursive()[0];

    ASSERT_TRUE(signal.getActiveStreamingSource().assigned());
    ASSERT_TRUE(signal.getActiveStreamingSource().toView().find("daq.ws://") != std::string::npos);
    ASSERT_EQ(signal.getActiveStreamingSource(), "daq.ws://127.0.0.1:7414");

    daq::ListPtr<IString> streamingSources = signal.getStreamingSources();
    ASSERT_EQ(streamingSources.getCount(), 2u);

    ASSERT_NE(std::find(streamingSources.begin(), streamingSources.end(), "daq.ws://127.0.0.1:7414"),
              streamingSources.end());
    ASSERT_NE(std::find(streamingSources.begin(), streamingSources.end(), "daq.ns://127.0.0.1:7420"),
              streamingSources.end());

    ASSERT_TRUE(signal.getStreamed());
}

// Corresponding document: Antora/modules/howto_guides/pages/howto_configure_streaming.adoc
TEST_F(StreamingConfigTest, WebsocketStreamingRead)
{
    SKIP_TEST_MAC_CI;

    InstancePtr server = docs_test_helpers::setupSimulatorServers();
    daq::InstancePtr instance = daq::Instance();

    daq::PropertyObjectPtr deviceConfig = instance.getAvailableDeviceTypes().get("daq.opcua").createDefaultConfig();
    deviceConfig.setPropertyValue("AllowedStreamingProtocols", daq::List<daq::IString>("daq.ns", "daq.ws"));
    deviceConfig.setPropertyValue("PrimaryStreamingProtocol", "daq.ws");
    deviceConfig.setPropertyValue("StreamingConnectionHeuristic", 0);

    daq::DevicePtr device = instance.addDevice("daq.opcua://127.0.0.1", deviceConfig);
    ASSERT_TRUE(device.assigned());

    daq::MirroredSignalConfigPtr signal = device.getSignalsRecursive()[0];
    ASSERT_NO_THROW(signal.setActiveStreamingSource("daq.ws://127.0.0.1:7414"));

    using namespace std::chrono_literals;
    StreamReaderPtr reader = daq::StreamReader<double, uint64_t>(signal);

    double samples[100];
    for (int i = 0; i < 5; ++i)
    {
        docs_test_helpers::waitForSamplesReady();
        daq::SizeT count = 100;
        reader.read(samples, &count);
        ASSERT_GT(count, 0u);
    }
}

// Corresponding document: Antora/modules/howto_guides/pages/howto_configure_streaming.adoc
TEST_F(StreamingConfigTest, NativeStreamingRead)
{
    SKIP_TEST_MAC_CI;

    InstancePtr server = docs_test_helpers::setupSimulatorServers();
    daq::InstancePtr instance = daq::Instance();

    daq::PropertyObjectPtr deviceConfig = instance.getAvailableDeviceTypes().get("daq.opcua").createDefaultConfig();
    deviceConfig.setPropertyValue("AllowedStreamingProtocols", daq::List<daq::IString>("daq.ns", "daq.ws"));
    deviceConfig.setPropertyValue("PrimaryStreamingProtocol", "daq.ws");
    deviceConfig.setPropertyValue("StreamingConnectionHeuristic", 0);

    daq::DevicePtr device = instance.addDevice("daq.opcua://127.0.0.1", deviceConfig);
    ASSERT_TRUE(device.assigned());

    daq::MirroredSignalConfigPtr signal = device.getSignals(search::Recursive(search::Any()))[0];
    ASSERT_NO_THROW(signal.setActiveStreamingSource("daq.ns://127.0.0.1:7420"));

    using namespace std::chrono_literals;
    StreamReaderPtr reader = daq::StreamReader<double, uint64_t>(signal);

    double samples[100];
    for (int i = 0; i < 5; ++i)
    {
        docs_test_helpers::waitForSamplesReady();
        daq::SizeT count = 100;
        reader.read(samples, &count);
        ASSERT_GT(count, 0u);
    }
}

END_NAMESPACE_OPENDAQ
