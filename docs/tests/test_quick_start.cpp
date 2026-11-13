#include <gtest/gtest.h>
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

    daq::InstancePtr server = docs_test_helpers::setupSimulatorServers();

    daq::InputPortPtr port;
    daq::InstancePtr instance = daq::Instance();
    port = daq::InputPort(instance.getContext(), nullptr, "readsig");

    daq::DevicePtr device = instance.addDevice("daq.ns://127.0.0.1");
    ASSERT_TRUE(device.assigned());

    using namespace std::chrono_literals;
    StreamReaderPtr reader = daq::StreamReaderFromPort(port, SampleType::Float64, SampleType::UInt64);
    port.connect(device.getSignalsRecursive()[0]);

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
