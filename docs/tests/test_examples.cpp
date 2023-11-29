#include <gtest/gtest.h>
#include <thread>
#include <opendaq/opendaq.h>
#include "docs_test_helpers.h"

// The following examples correspond to the code examples published by openDAQ.

using ExamplesTest = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

// examples: stream_reader
TEST_F(ExamplesTest, StreamReader)
{
    using namespace std::chrono_literals;

    const InstancePtr instance = Instance();
    
    DevicePtr device = instance.addDevice("daqref://device0");
    SignalPtr signal = device.getSignalsRecursive()[0];
    
    StreamReaderPtr reader = StreamReader<double, uint64_t>(signal);
    
    double samples[5000];
    for (int i = 0; i < 10; ++i)
    {
        docs_test_helpers::waitForSamplesReady();
        SizeT count = 5000;
        reader.read(samples, &count);
        ASSERT_GT(count, 0);
    }
}

// examples: function_block
TEST_F(ExamplesTest, FunctionBlock)
{
	SKIP_TEST_MAC_CI;
	
    const InstancePtr instance = Instance();
    DevicePtr device = instance.addDevice("daqref://device0");
    FunctionBlockPtr statistics = instance.addFunctionBlock("ref_fb_module_statistics");

    const ChannelPtr sineChannel = device.getChannels()[0];
    const SignalPtr sineSignal = sineChannel.getSignals()[0];
    
    sineChannel.setPropertyValue("NoiseAmplitude", 1);
    ASSERT_EQ(sineChannel.getPropertyValue("NoiseAmplitude"), 1);
    
    statistics.getInputPorts()[0].connect(sineSignal);
    const SignalPtr averagedSine = statistics.getSignalsRecursive()[0];
    StreamReaderPtr reader = StreamReader<double, uint64_t>(averagedSine);
    std::this_thread::sleep_for(std::chrono::milliseconds(900));

    double samples[5000];
    double ampl_step = 0.1;
    for (int i = 0; i < 10; ++i)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        const double ampl = sineChannel.getPropertyValue("Amplitude");
        if (9.95 < ampl || ampl < 3.05)
            ampl_step *= -1;
        sineChannel.setPropertyValue("Amplitude", ampl + ampl_step);
        ASSERT_EQ(sineChannel.getPropertyValue("Amplitude"), ampl + ampl_step);

        SizeT count = 5000;
        reader.read(samples, &count);
        ASSERT_GT(count, 0);
    }
}

// examples: device
TEST_F(ExamplesTest, Device)
{
    SKIP_TEST_MAC_CI;

    InstancePtr instance = docs_test_helpers::setupSimulatorServers();
    ASSERT_TRUE(instance.assigned());
}

// examples: client_local, stream_reader
TEST_F(ExamplesTest, Client)
{
    SKIP_TEST_MAC_CI;

    using namespace std::chrono_literals;

    InstancePtr server = docs_test_helpers::setupSimulatorServers();

    const InstancePtr instance = Instance();
    DevicePtr device = instance.addDevice("daq.opcua://127.0.0.1");

    DeviceInfoPtr info = device.getInfo();
    ASSERT_EQ(info.getName(), "Device 1");

    StreamReaderPtr reader = StreamReader<double, uint64_t>(device.getSignals(search::Recursive(search::Any()))[0]);

    double samples[5000];
    for (int i = 0; i < 10; ++i)
    {
        docs_test_helpers::waitForSamplesReady();
        SizeT count = 5000;
        reader.read(samples, &count);
        ASSERT_GT(count, 0);
    }
}

// examples: client_discovery
TEST_F(ExamplesTest, Discovery)
{
    const InstancePtr instance = Instance();
    ListPtr<IDeviceInfo> availableDevicesInfo = instance.getAvailableDevices();
    int refDeviceCnt = 0;
    for (const auto& deviceInfo : availableDevicesInfo)
        if (deviceInfo.getConnectionString().toStdString().find("daqref://") != std::string::npos)
            refDeviceCnt++;

    ASSERT_EQ(refDeviceCnt, 2);
}

END_NAMESPACE_OPENDAQ
