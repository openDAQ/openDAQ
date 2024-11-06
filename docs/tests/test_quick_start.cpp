#include <gtest/gtest.h>
#include <testutils/testutils.h>
#include <thread>
#include <opendaq/opendaq.h>
#include "docs_test_helpers.h"

using QuickStartTest = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

// Corresponding document: Antora/modules/quick_start/pages/quick_start_application.adoc
TEST_F(QuickStartTest, QuickStartAppEnumerate)
{
    daq::InstancePtr instance = daq::Instance();

    daq::ListPtr<daq::IDeviceInfo> availableDevicesInfo = instance.getAvailableDevices();
    int refDeviceCnt = 0;
    for (const auto& deviceInfo : availableDevicesInfo)
        if (deviceInfo.getConnectionString().toStdString().find("daqref://") != std::string::npos)
            refDeviceCnt++;

    ASSERT_EQ(refDeviceCnt, 2);
}

// Corresponding document: Antora/modules/quick_start/pages/quick_start_application.adoc
TEST_F(QuickStartTest, QuickStartAppConnect)
{
    SKIP_TEST_MAC_CI;

    InstancePtr server = docs_test_helpers::setupSimulatorServers();
    daq::InstancePtr instance = daq::Instance();

    daq::DevicePtr device = instance.addDevice("daq.opcua://127.0.0.1");
    ASSERT_TRUE(device.assigned());

    ASSERT_EQ(device.getInfo().getName(), "Device 1");
}

// Corresponding document: Antora/modules/quick_start/pages/quick_start_application.adoc
TEST_F(QuickStartTest, QuickStartAppReader)
{
    SKIP_TEST_MAC_CI;

    InstancePtr server = docs_test_helpers::setupSimulatorServers();
    daq::InstancePtr instance = daq::Instance();

    daq::DevicePtr device = instance.addDevice("daq.opcua://127.0.0.1");
    ASSERT_TRUE(device.assigned());

    using namespace std::chrono_literals;
    StreamReaderPtr reader = daq::StreamReader<double, uint64_t>(device.getSignals(search::Recursive(search::Any()))[0], ReadTimeoutType::Any);

    {
        daq::SizeT count = 0;
        reader.read(nullptr, &count, 1000);
        // TODO: needed becuase there are two descriptor changes,
        // due to reference domain "stuff" only being fully supported over Native
        // can be deleted once full support is added
        reader.read(nullptr, &count, 1000);
    }

    double samples[100];
    for (int i = 0; i < 5; ++i)
    {
        daq::SizeT count = 100;
        reader.read(samples, &count, 1000);
        ASSERT_GT(count, 0u);
    }
}

// Corresponding document: Antora/modules/quick_start/pages/quick_start_application.adoc
TEST_F(QuickStartTest, QuickStartAppStatistics)
{
    SKIP_TEST_MAC_CI;

    InstancePtr server = docs_test_helpers::setupSimulatorServers();
    daq::InstancePtr instance = daq::Instance();

    daq::DevicePtr device = instance.addDevice("daq.opcua://127.0.0.1");
    ASSERT_TRUE(device.assigned());

    daq::FunctionBlockPtr statistics = instance.addFunctionBlock("RefFBModuleStatistics");
    statistics.getInputPorts()[0].connect(device.getSignalsRecursive()[0]);
    const daq::ChannelPtr sineChannel = device.getChannels()[0];

    for (daq::PropertyPtr prop : sineChannel.getVisibleProperties())
        std::cout << prop.getName() << std::endl;

    sineChannel.setPropertyValue("Frequency", 5);
    ASSERT_EQ(sineChannel.getPropertyValue("Frequency"), 5);
    sineChannel.setPropertyValue("NoiseAmplitude", 0.75);
    ASSERT_EQ(sineChannel.getPropertyValue("NoiseAmplitude"), 0.75);

    StreamReaderPtr reader = daq::StreamReader<double, uint64_t>(device.getSignals(search::Recursive(search::Any()))[0], ReadTimeoutType::Any);

    {
        daq::SizeT count = 0;
        reader.read(nullptr, &count, 1000);
        // TODO: needed becuase there are two descriptor changes,
        // due to reference domain "stuff" only being fully supported over Native
        // can be deleted once full support is added
        reader.read(nullptr, &count, 1000);
    }

    double amplStep = 0.1;
    double samples[100];

    for (int cnt = 0; cnt < 10; ++cnt)
    {
        const double ampl = sineChannel.getPropertyValue("Amplitude");
        if (9.95 < ampl || ampl < 1.05)
            amplStep *= -1;
        sineChannel.setPropertyValue("Amplitude", ampl + amplStep);
        ASSERT_EQ(sineChannel.getPropertyValue("Amplitude"), ampl + amplStep);

        daq::SizeT count = 250;
        reader.read(samples, &count, 100);
        ASSERT_GT(count, 0u);
    }
}

#if defined(OPENDAQ_ENABLE_WEBSOCKET_STREAMING)
// Corresponding document: Antora/modules/quick_start/pages/quick_start_application.adoc
TEST_F(QuickStartTest, QuickStartAppConnectWebsocket)
{
    SKIP_TEST_MAC_CI;

    InstancePtr server = docs_test_helpers::setupSimulatorServers();
    daq::InstancePtr instance = daq::Instance();

    daq::DevicePtr device = instance.addDevice("daq.lt://127.0.0.1");
    ASSERT_TRUE(device.assigned());

    ASSERT_EQ(device.getInfo().getName(), "WebsocketClientPseudoDevice");
}

TEST_F(QuickStartTest, QuickStartAppConnectOldWebsocket)
{
    SKIP_TEST_MAC_CI;

    InstancePtr server = docs_test_helpers::setupSimulatorServers();
    daq::InstancePtr instance = daq::Instance();

    daq::DevicePtr device = instance.addDevice("daq.ws://127.0.0.1");
    ASSERT_TRUE(device.assigned());

    ASSERT_EQ(device.getInfo().getName(), "WebsocketClientPseudoDevice");
}

// Corresponding document: Antora/modules/quick_start/pages/quick_start_application.adoc
TEST_F_FLAKY_SKIPPED(QuickStartTest, QuickStartAppReaderWebsocket)
{
    SKIP_TEST_MAC_CI;

    InstancePtr server = docs_test_helpers::setupSimulatorServers();
    daq::InstancePtr instance = daq::Instance();

    daq::DevicePtr device = instance.addDevice("daq.lt://127.0.0.1");
    ASSERT_TRUE(device.assigned());

    using namespace std::chrono_literals;
    StreamReaderPtr reader = daq::StreamReader<double, uint64_t>(device.getSignals()[0], ReadTimeoutType::Any);

    {
        daq::SizeT count = 0;
        reader.read(nullptr, &count, 1000);
    }

    double samples[100];
    for (int i = 0; i < 5; ++i)
    {
        daq::SizeT count = 100;
        reader.read(samples, &count, 1000);
        ASSERT_GT(count, 0u);
    }
}
#endif

#if defined(OPENDAQ_ENABLE_NATIVE_STREAMING)
// Corresponding document: Antora/modules/quick_start/pages/quick_start_application.adoc
TEST_F(QuickStartTest, QuickStartAppConnectNativePseudoDevice)
{
    SKIP_TEST_MAC_CI;

    InstancePtr server = docs_test_helpers::setupSimulatorServers();
    daq::InstancePtr instance = daq::Instance();

    daq::DevicePtr device = instance.addDevice("daq.ns://127.0.0.1");
    ASSERT_TRUE(device.assigned());

    ASSERT_EQ(device.getInfo().getName(), "NativeStreamingClientPseudoDevice");
}

// Corresponding document: Antora/modules/quick_start/pages/quick_start_application.adoc
TEST_F(QuickStartTest, QuickStartAppReaderNativePseudoDevice)
{
    SKIP_TEST_MAC_CI;

    InstancePtr server = docs_test_helpers::setupSimulatorServers();
    daq::InstancePtr instance = daq::Instance();

    daq::DevicePtr device = instance.addDevice("daq.ns://127.0.0.1");
    ASSERT_TRUE(device.assigned());

    using namespace std::chrono_literals;
    StreamReaderPtr reader = daq::StreamReader<double, uint64_t>(device.getSignals()[0], ReadTimeoutType::Any);

    {
        daq::SizeT count = 0;
        reader.read(nullptr, &count, 1000);
    }

    double samples[100];
    for (int i = 0; i < 5; ++i)
    {
        daq::SizeT count = 100;
        reader.read(samples, &count, 1000);
        ASSERT_GT(count, 0u);
    }
}
#endif

END_NAMESPACE_OPENDAQ
