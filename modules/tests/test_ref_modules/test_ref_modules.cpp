#include <testutils/testutils.h>
#include <opendaq/context_factory.h>
#include <opendaq/module_ptr.h>
#include <opendaq/instance_factory.h>
#include <opendaq/device_ptr.h>
#include <thread>
#include <opendaq/range_factory.h>
#include "testutils/memcheck_listener.h"

using RefModulesTest = testing::Test;
using namespace daq;

class InvertDestructOrderTest : public RefModulesTest
{
protected:
    void TearDown() override
    {
        auto instance = Instance("[[none]]");
        instance.release();
    }
};

TEST_F(RefModulesTest, DISABLED_RunDeviceAndRendererSimple)
{
    const auto instance = Instance();
    const auto device = instance.addDevice("daqref://device1");

    const auto rendererFb = instance.addFunctionBlock("ref_fb_module_renderer");

    const auto deviceChannel0 = device.getChannels()[0];
    const auto deviceSignal0 = deviceChannel0.getSignalsRecursive()[0];

    const auto rendererInputPort0 = rendererFb.getInputPorts()[0];

    rendererInputPort0.connect(deviceSignal0);

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
}

TEST_F(RefModulesTest, DISABLED_RunDeviceAndRendererNameChange)
{
    const auto instance = Instance();
    const auto device = instance.addDevice("daqref://device1");

    const auto rendererFb = instance.addFunctionBlock("ref_fb_module_renderer");

    const auto deviceChannel0 = device.getChannels()[0];
    const auto deviceSignal0 = deviceChannel0.getSignalsRecursive()[0];

    const auto rendererInputPort0 = rendererFb.getInputPorts()[0];

    rendererInputPort0.connect(deviceSignal0);

    std::this_thread::sleep_for(std::chrono::milliseconds(2500));
    deviceSignal0.setPropertyValue("Name", "My signal");
    std::this_thread::sleep_for(std::chrono::milliseconds(2500));
}

TEST_F(RefModulesTest, DISABLED_RunDeviceAndRendererRemoveDevice)
{
    const auto instance = Instance();
    const auto device = instance.addDevice("daqref://device1");

    const auto rendererFb = instance.addFunctionBlock("ref_fb_module_renderer");

    const auto deviceChannel0 = device.getChannels()[0];
    const auto deviceSignal0 = deviceChannel0.getSignals()[0];

    const auto rendererInputPort0 = rendererFb.getInputPorts()[0];

    rendererInputPort0.connect(deviceSignal0);

    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    instance.removeDevice(device);
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
}

TEST_F(RefModulesTest, DISABLED_RunDeviceAndRendererSimpleCounter)
{
    const auto instance = Instance();

    const auto device = instance.addDevice("daqref://device1");
    const auto rendererFb = instance.addFunctionBlock("ref_fb_module_renderer");

    const auto deviceChannel0 = device.getChannels()[0];
    const auto deviceSignal0 = deviceChannel0.getSignals()[0];
    const auto rendererInputPort0 = rendererFb.getInputPorts()[0];

    deviceChannel0.setPropertyValue("Waveform", 3);

    rendererInputPort0.connect(deviceSignal0);

    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    deviceChannel0.getPropertyValue("ResetCounter").dispatch();
    std::this_thread::sleep_for(std::chrono::milliseconds(8000));
}

TEST_F(RefModulesTest, DISABLED_RunDeviceAndRenderer)
{
    const auto instance = Instance();

    const auto device = instance.addDevice("daqref://device1");

    const auto deviceChannel0 = device.getChannels()[0];
    const auto deviceChannel1 = device.getChannels()[1];

    deviceChannel0.setPropertyValue("UseGlobalSampleRate", False);
    deviceChannel0.setPropertyValue("SampleRate", 1000.0);
    deviceChannel0.setPropertyValue("Frequency", 1.0);
    deviceChannel0.setPropertyValue("NoiseAmplitude", 0.2);

    deviceChannel1.setPropertyValue("ClientSideScaling", True);

    const auto rendererFb = instance.addFunctionBlock("ref_fb_module_renderer");

    const auto deviceSignal0_0 = deviceChannel0.getSignals()[0];
    const auto deviceSignal1_0 = deviceChannel1.getSignals()[0];
    const auto rendererInputPort0 = rendererFb.getInputPorts()[0];
    rendererInputPort0.connect(deviceSignal0_0);
    const auto rendererInputPort1 = rendererFb.getInputPorts()[1];

    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    rendererInputPort1.connect(deviceSignal1_0);

    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    deviceChannel0.setPropertyValue("SampleRate", 50.0);

    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    rendererInputPort1.disconnect();

    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
}

TEST_F(RefModulesTest, DISABLED_RunDeviceStatisticsRenderer)
{
    const auto instance = Instance();

    const auto device = instance.addDevice("daqref://device1");
    device.setPropertyValue("GlobalSampleRate", 10000);

    const auto rendererFb = instance.addFunctionBlock("ref_fb_module_renderer");
    rendererFb.setPropertyValue("Duration", 10.0);
    const auto statisticsFb = instance.addFunctionBlock("ref_fb_module_statistics");
    statisticsFb.setPropertyValue("BlockSize", 20);

    const auto deviceChannel = device.getChannels()[0];
    deviceChannel.setPropertyValue("Frequency", 10.0);
    const auto deviceSignal = deviceChannel.getSignals()[0];

    const auto statisticsInputPort = statisticsFb.getInputPorts()[0];
    const auto rmsSignal = statisticsFb.getSignals()[1];
    statisticsInputPort.connect(deviceSignal);

    const auto rendererInputPort0 = rendererFb.getInputPorts()[0];
    rendererInputPort0.connect(deviceSignal);

    const auto rendererInputPort1 = rendererFb.getInputPorts()[1];
    rendererInputPort1.connect(rmsSignal);

    std::this_thread::sleep_for(std::chrono::milliseconds(10000));

/*    statisticsFb.setPropertyValue("DomainSignalType", 1);
    statisticsFb.setPropertyValue("BlockSize", 50);
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));

    statisticsFb.setPropertyValue("DomainSignalType", 2);
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));*/
}

TEST_F(RefModulesTest, DISABLED_RunDeviceStatisticsRendererDeviceRemove)
{
    const auto instance = Instance();

    const auto device = instance.addDevice("daqref://device1");

    const auto rendererFb = instance.addFunctionBlock("ref_fb_module_renderer");
    const auto statisticsFb = instance.addFunctionBlock("ref_fb_module_statistics");

    const auto deviceChannel = device.getChannels()[0];
    const auto deviceSignal = deviceChannel.getSignals()[0];

    const auto statisticsInputPort = statisticsFb.getInputPorts()[0];
    const auto statisticsSignal = statisticsFb.getSignals()[0];
    statisticsInputPort.connect(deviceSignal);

    const auto rendererInputPort0 = rendererFb.getInputPorts()[0];
    rendererInputPort0.connect(deviceSignal);

    const auto rendererInputPort1 = rendererFb.getInputPorts()[1];
    rendererInputPort1.connect(statisticsSignal);

    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    instance.removeFunctionBlock(statisticsFb);
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    instance.removeDevice(device);
    ASSERT_EQ(rendererFb.getInputPorts().getCount(), 1u);
    ASSERT_FALSE(rendererFb.getInputPorts()[0].getSignal().assigned());
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    instance.removeFunctionBlock(rendererFb);
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
}

TEST_F(RefModulesTest, DISABLED_RendererSync)
{
    const auto instance = Instance();
    const auto device = instance.addDevice("daqref://device0");
    const auto deviceChannel0 = device.getChannels()[0];
    deviceChannel0.setPropertyValue("UseGlobalSampleRate", False);
    deviceChannel0.setPropertyValue("SampleRate", 100.0);
    deviceChannel0.setPropertyValue("Frequency", 1.0);

    const auto rendererFb = instance.addFunctionBlock("ref_fb_module_renderer");
    rendererFb.setPropertyValue("Duration", 10.0);
    rendererFb.setPropertyValue("SingleXAxis", True);

    const auto statisticsFb = instance.addFunctionBlock("ref_fb_module_statistics");
    statisticsFb.setPropertyValue("BlockSize", 5);

    const auto deviceSignal0 = deviceChannel0.getSignals()[0];
    const auto avgSignal = statisticsFb.getSignals()[0];

    const auto statisticsInputPort0 = statisticsFb.getInputPorts()[0];
    statisticsInputPort0.connect(deviceSignal0);

    const auto rendererInputPort0 = rendererFb.getInputPorts()[0];
    rendererInputPort0.connect(deviceSignal0);
    const auto rendererInputPort1 = rendererFb.getInputPorts()[1];
    rendererInputPort1.connect(avgSignal);

    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    deviceSignal0.setActive(false);
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    deviceSignal0.setActive(true);
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    deviceChannel0.setPropertyValue("SampleRate", 5.0);
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));

    const auto device1 = instance.addDevice("daqref://device1");
    const auto device1Channel0 = device1.getChannels()[1];
    device1Channel0.setPropertyValue("UseGlobalSampleRate", False);
    device1Channel0.setPropertyValue("SampleRate", 100.0);
    device1Channel0.setPropertyValue("Frequency", 0.5);
    const auto device1Signal0 = device1Channel0.getSignals()[0];

    const auto rendererInputPort2 = rendererFb.getInputPorts()[2];
    rendererInputPort2.connect(device1Signal0);

    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    deviceSignal0.setActive(false);
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    deviceSignal0.setActive(true);
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
}

TEST_F(InvertDestructOrderTest, InvertedDestructOrder)
{
    auto instance = Instance();
    auto device = instance.addDevice("daqref://device1");

    instance.release();
    device.release();
}

TEST_F(RefModulesTest, FindComponentSignal)
{
    auto instance = Instance("", "localInstance");
    auto device = instance.addDevice("daqref://device1");

    auto comp = instance.findComponent(nullptr, "Dev/ref_dev1/IO/ai/refch0/Sig/ai0");
    ASSERT_TRUE(comp.assigned());
    ASSERT_TRUE(comp.supportsInterface<ISignal>());
}

TEST_F(RefModulesTest, FindComponentSignalRelative)
{
    auto instance = Instance("", "localInstance");
    auto device = instance.addDevice("daqref://device1");
    auto ch = device.getChannels()[0];

    auto comp = instance.findComponent(ch, "Sig/ai0");
    ASSERT_TRUE(comp.assigned());
    ASSERT_TRUE(comp.supportsInterface<ISignal>());
}

TEST_F(RefModulesTest, FindComponentChannel)
{
    auto instance = Instance("", "localInstance");
    auto device = instance.addDevice("daqref://device1");

    auto comp = instance.findComponent(nullptr, "Dev/ref_dev1/IO/ai/refch0");
    ASSERT_TRUE(comp.assigned());
    ASSERT_TRUE(comp.supportsInterface<IChannel>());
}

TEST_F(RefModulesTest, FindComponentDevice)
{
    auto instance = Instance("", "localInstance");
    auto device = instance.addDevice("daqref://device1");

    auto comp = instance.findComponent(nullptr, "Dev/ref_dev1");
    ASSERT_TRUE(comp.assigned());
    ASSERT_TRUE(comp.supportsInterface<IDevice>());
}


TEST_F(RefModulesTest, DISABLED_RunDevicePowerRenderer)
{
    const auto instance = Instance();

    const auto device0 = instance.addDevice("daqref://device0");
    device0.setPropertyValue("GlobalSampleRate", 10000);
    const auto device1 = instance.addDevice("daqref://device1");
    device1.setPropertyValue("GlobalSampleRate", 10000);

    const auto rendererFb = instance.addFunctionBlock("ref_fb_module_renderer");
    rendererFb.setPropertyValue("Duration", 2.0);

    const auto powerFb = instance.addFunctionBlock("ref_fb_module_power");

    powerFb.setPropertyValue("VoltageScale", 2.0);
    powerFb.setPropertyValue("VoltageOffset", 1.0);
    powerFb.setPropertyValue("CurrentScale", 1.5);
    powerFb.setPropertyValue("CurrentOffset", -1.0);

    powerFb.setPropertyValue("UseCustomOutputRange", True);
    powerFb.setPropertyValue("CustomHighValue", 75.0);
    powerFb.setPropertyValue("CustomLowValue", -75.0);

    const auto device0Channel = device0.getChannels()[0];
    device0Channel.setPropertyValue("Frequency", 10.0);
    const auto device0Signal = device0Channel.getSignals()[0];

    const auto device1Channel = device1.getChannels()[0];
    device1Channel.setPropertyValue("Frequency", 0.8);
    const auto device1Signal = device1Channel.getSignals()[0];

    powerFb.getInputPorts()[0].connect(device0Signal);
    powerFb.getInputPorts()[1].connect(device1Signal);

    auto powerSignal = powerFb.getSignals()[0];

    rendererFb.getInputPorts()[0].connect(device0Signal);
    rendererFb.getInputPorts()[1].connect(device1Signal);
    rendererFb.getInputPorts()[2].connect(powerSignal);

    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    rendererFb.setPropertyValue("Freeze", True);
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
}

TEST_F(RefModulesTest, DISABLED_RunDeviceScalingRenderer)
{
    const auto instance = Instance();

    const auto device = instance.addDevice("daqref://device0");
    device.setPropertyValue("GlobalSampleRate", 1000);

    const auto deviceChannel = device.getChannels()[0];
    deviceChannel.setPropertyValue("Frequency", 0.37);
    const auto deviceSignal = deviceChannel.getSignals()[0];

    const auto rendererFb = instance.addFunctionBlock("ref_fb_module_renderer");
    rendererFb.setPropertyValue("Duration", 2.0);
    rendererFb.setPropertyValue("SingleXAxis", 2.0);
    rendererFb.setPropertyValue("ShowLastValue", True);

    const auto scalingFb = instance.addFunctionBlock("ref_fb_module_scaling");
    scalingFb.setPropertyValue("Scale", 2.0);
    scalingFb.setPropertyValue("Offset", 1.0);

    scalingFb.getInputPorts()[0].connect(deviceSignal);
    const auto scaledSignal = scalingFb.getSignals()[0];

    rendererFb.getInputPorts()[0].connect(deviceSignal);
    rendererFb.getInputPorts()[1].connect(scaledSignal);

    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    rendererFb.setPropertyValue("Freeze", True);
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    rendererFb.setPropertyValue("Freeze", False);

    scalingFb.setPropertyValue("UseCustomOutputRange", True);
    scalingFb.setPropertyValue("OutputHighValue", 40.0);
    scalingFb.setPropertyValue("OutputLowValue", -40.0);
    scalingFb.setPropertyValue("OutputName", "Scaled signal");
    scalingFb.setPropertyValue("OutputUnit", "kV");

    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
}

TEST_F(RefModulesTest, SerializeDevicePower)
{
    const auto instance = Instance();

    const auto device0 = instance.addDevice("daqref://device0");
    device0.setPropertyValue("GlobalSampleRate", 10000);
    auto syncComponent = device0.getItem("sync");
    syncComponent.setPropertyValue("UseSync", True);

    const auto device1 = instance.addDevice("daqref://device1");
    device1.setPropertyValue("GlobalSampleRate", 10000);

    const auto powerFb = instance.addFunctionBlock("ref_fb_module_power");

    powerFb.setPropertyValue("VoltageScale", 2.0);
    powerFb.setPropertyValue("VoltageOffset", 1.0);
    powerFb.setPropertyValue("CurrentScale", 1.5);
    powerFb.setPropertyValue("CurrentOffset", -1.0);

    powerFb.setPropertyValue("UseCustomOutputRange", True);
    powerFb.setPropertyValue("CustomHighValue", 75.0);
    powerFb.setPropertyValue("CustomLowValue", -75.0);

    const auto device0Channel = device0.getChannels()[0];
    device0Channel.setPropertyValue("Frequency", 10.0);
    device0Channel.setPropertyValue("CustomRange", Range(-20.0, 20.0));
    const auto device0Signal = device0Channel.getSignals()[0];

    const auto device1Channel = device1.getChannels()[0];
    device1Channel.setPropertyValue("Frequency", 0.8);
    const auto device1Signal = device1Channel.getSignals()[0];

    powerFb.getInputPorts()[0].connect(device0Signal);
    powerFb.getInputPorts()[1].connect(device1Signal);

    const auto configuration = instance.saveConfiguration();
    std::cout << configuration << std::endl;
}

TEST_F(RefModulesTest, UpdateDevicePower)
{
    auto instance = Instance();

    auto device0 = instance.addDevice("daqref://device0");
    device0.setPropertyValue("GlobalSampleRate", 10000);
    const auto syncComponent = device0.getItem("sync");
    syncComponent.setPropertyValue("UseSync", True);
    auto device1 = instance.addDevice("daqref://device1");
    device1.setPropertyValue("GlobalSampleRate", 10000);

    auto powerFb = instance.addFunctionBlock("ref_fb_module_power");

    powerFb.setPropertyValue("VoltageScale", 2.0);
    powerFb.setPropertyValue("VoltageOffset", 1.0);
    powerFb.setPropertyValue("CurrentScale", 1.5);
    powerFb.setPropertyValue("CurrentOffset", -1.0);

    powerFb.setPropertyValue("UseCustomOutputRange", True);
    powerFb.setPropertyValue("CustomHighValue", 75.0);
    powerFb.setPropertyValue("CustomLowValue", -75.0);

    auto device0Channel = device0.getChannels()[0];
    device0Channel.setPropertyValue("Frequency", 10.2);
    device0Channel.setPropertyValue("CustomRange", Range(-20.0, 20.0));
    auto device0Signal = device0Channel.getSignals()[0];

    auto device1Channel = device1.getChannels()[0];
    device1Channel.setPropertyValue("Frequency", 0.8);
    auto device1Signal = device1Channel.getSignals()[0];

    powerFb.getInputPorts()[0].connect(device0Signal);
    powerFb.getInputPorts()[1].connect(device1Signal);

/*    auto rendererFb = instance.addFunctionBlock("ref_fb_module_renderer");
    rendererFb.getInputPorts()[0].connect(powerFb.getSignals()[0]);
    rendererFb.getInputPorts()[1].connect(device0Signal);
    rendererFb.getInputPorts()[2].connect(device1Signal);

    std::this_thread::sleep_for(std::chrono::milliseconds(5000));*/

    const auto configuration = instance.saveConfiguration();

    powerFb.release();
    device0.release();
    device1.release();
    device0Channel.release();
    device1Channel.release();
    device0Signal.release();
    device1Signal.release();
//    rendererFb.release();

    std::cout << configuration << std::endl;

    instance = Instance();

    device0 = instance.addDevice("daqref://device0");
    device1 = instance.addDevice("daqref://device1");

    instance.loadConfiguration(configuration);

    ASSERT_EQ(device0.getItem("sync").getPropertyValue("UseSync"), True);
    ASSERT_EQ(device0.getPropertyValue("GlobalSampleRate"), 10000);
    device0Channel = device0.getChannels()[0];
    ASSERT_EQ(device0Channel.getPropertyValue("Frequency"), 10.2);
    ASSERT_EQ(device0Channel.getPropertyValue("CustomRange"), Range(-20.0, 20.0));

    powerFb = instance.getFunctionBlocks()[0];
//    powerFb = instance.getFunctionBlocks()[1];

    ASSERT_EQ(powerFb.getPropertyValue("VoltageScale"), 2.0);
    ASSERT_EQ(powerFb.getPropertyValue("VoltageOffset"), 1.0);
    ASSERT_EQ(powerFb.getPropertyValue("CurrentScale"), 1.5);
    ASSERT_EQ(powerFb.getPropertyValue("CurrentOffset"), -1.0);

    ASSERT_EQ(powerFb.getPropertyValue("UseCustomOutputRange"), True);
    ASSERT_EQ(powerFb.getPropertyValue("CustomHighValue"), 75.0);
    ASSERT_EQ(powerFb.getPropertyValue("CustomLowValue"), -75.0);

//    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
}
