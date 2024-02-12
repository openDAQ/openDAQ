#include <opendaq/context_factory.h>
#include <opendaq/device_ptr.h>
#include <opendaq/instance_factory.h>
#include <opendaq/module_ptr.h>
#include <opendaq/range_factory.h>
#include <opendaq/reader_factory.h>
#include <testutils/testutils.h>
#include <opendaq/search_filter_factory.h>
#include <thread>
#include "classifier_test_helper.h"
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
    const auto deviceSignal0 = deviceChannel0.getSignals(search::Recursive(search::Visible()))[0];

    const auto rendererInputPort0 = rendererFb.getInputPorts()[0];

    rendererInputPort0.connect(deviceSignal0);

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
}

TEST_F(RefModulesTest, DISABLED_RunDeviceAndRendererCANChannel)
{
    const auto instance = Instance();
    const auto device = instance.addDevice("daqref://device1");
    device.setPropertyValue("EnableCANChannel", True);

    const auto rendererFb = instance.addFunctionBlock("ref_fb_module_renderer");

    const ChannelPtr canChannel = device.getInputsOutputsFolder().getItem("can").asPtr<IFolder>().getItems()[0];
    const auto canSignal = canChannel.getSignals(search::Recursive(search::Visible()))[0];

    const auto rendererInputPort0 = rendererFb.getInputPorts()[0];

    rendererInputPort0.connect(canSignal);

    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
}

TEST_F(RefModulesTest, DISABLED_RunDeviceAndRendererNameChange)
{
    const auto instance = Instance();
    const auto device = instance.addDevice("daqref://device1");

    const auto rendererFb = instance.addFunctionBlock("ref_fb_module_renderer");

    const auto deviceChannel0 = device.getChannels()[0];
    const auto deviceSignal0 = deviceChannel0.getSignals(search::Recursive(search::Visible()))[0];

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

TEST_F(RefModulesTest, DISABLED_RunDeviceAndRendererInUpdate)
{
    const auto instance = Instance();

    const auto device = instance.addDevice("daqref://device1");
    device.setPropertyValue("GlobalSampleRate", 25.0);

    const auto deviceChannel0 = device.getChannels()[0];

    ASSERT_FLOAT_EQ(deviceChannel0.getPropertyValue("SampleRate"), 25.0);

    const auto rendererFb = instance.addFunctionBlock("ref_fb_module_renderer");

    const auto deviceSignal0_0 = deviceChannel0.getSignals()[0];
    const auto rendererInputPort0 = rendererFb.getInputPorts()[0];
    rendererInputPort0.connect(deviceSignal0_0);

    std::this_thread::sleep_for(std::chrono::milliseconds(5000));

    deviceChannel0.beginUpdate();
    deviceChannel0.setPropertyValue("UseGlobalSampleRate", False);
    deviceChannel0.setPropertyValue("SampleRate", 50.0);
    deviceChannel0.setPropertyValue("ClientSideScaling", True);
    deviceChannel0.setPropertyValue("CustomRange", Range(-5.0, 5.0));
    ASSERT_FLOAT_EQ(deviceChannel0.getPropertyValue("SampleRate"), 25.0);
    deviceChannel0.endUpdate();
    ASSERT_FLOAT_EQ(deviceChannel0.getPropertyValue("SampleRate"), 50.0);

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

    auto comp = instance.findComponent("Dev/ref_dev1/IO/ai/refch0/Sig/ai0");
    ASSERT_TRUE(comp.assigned());
    ASSERT_TRUE(comp.supportsInterface<ISignal>());
}

TEST_F(RefModulesTest, FindComponentSignalRelative)
{
    auto instance = Instance("", "localInstance");
    auto device = instance.addDevice("daqref://device1");
    auto ch = device.getChannels()[0];

    auto comp = ch.findComponent("Sig/ai0");
    ASSERT_TRUE(comp.assigned());
    ASSERT_TRUE(comp.supportsInterface<ISignal>());
}

TEST_F(RefModulesTest, FindComponentChannel)
{
    auto instance = Instance("", "localInstance");
    auto device = instance.addDevice("daqref://device1");

    auto comp = instance.findComponent("Dev/ref_dev1/IO/ai/refch0");
    ASSERT_TRUE(comp.assigned());
    ASSERT_TRUE(comp.supportsInterface<IChannel>());
}

TEST_F(RefModulesTest, FindComponentDevice)
{
    auto instance = Instance("", "localInstance");
    auto device = instance.addDevice("daqref://device1");

    auto comp = instance.findComponent("Dev/ref_dev1");
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

TEST_F(RefModulesTest, ClassifierGeneralDescriptor)
{
    ClassifierTestHelper helper;
    helper.setUp();

    const auto inputSignal = helper.getInputSignal();
    const auto inputDomainDescriptor = helper.getInputDomainSignal().getDescriptor();

    const auto classifierSignal = helper.getOutputSignal();
    const auto classifierSignalDescription = classifierSignal.getDescriptor();
    const auto classifierDomainSignalDescription = classifierSignal.getDomainSignal().getDescriptor();

    // Classifier returning values with float float
    ASSERT_EQ(classifierSignalDescription.getSampleType(), SampleType::Float64);

    // Dimension of classifier's output signal have to be 1 (vector)
    ASSERT_EQ(classifierSignalDescription.getDimensions().getCount(), 1u);

    // Classifier returns values in range [0, 1]
    ASSERT_EQ(classifierSignalDescription.getValueRange(), Range(0, 1));

    // Classifier returns values are %
    ASSERT_EQ(classifierSignalDescription.getUnit().getSymbol(), "%");

    // Check tick resolution
    ASSERT_EQ(classifierDomainSignalDescription.getTickResolution(), inputDomainDescriptor.getTickResolution());
}

TEST_F(RefModulesTest, ClassifierRuleForSync)
{
    ClassifierTestHelper helper;
    helper.setUp();

    const auto inputSignal = helper.getInputSignal();
    const auto inputDomainDescriptor = helper.getInputDomainSignal().getDescriptor();

    const auto classifierSignal = helper.getOutputSignal();
    const auto classifierSignalDescription = classifierSignal.getDescriptor();
    const auto classifierDomainSignalDescription = classifierSignal.getDomainSignal().getDescriptor();

    const auto classifierDomainRule = classifierDomainSignalDescription.getRule();

    // Check Linear Rule
    ASSERT_EQ(classifierDomainRule.getType(), DataRuleType::Linear);

    // Check deltas
    auto classifierDelta = classifierDomainRule.getParameters().get("delta");
    auto inputResolution = inputDomainDescriptor.getTickResolution() * Int(1000);
    auto expectedClassifierDelata = 1.0 / static_cast<Float>(inputResolution);
    ASSERT_EQ(classifierDelta, expectedClassifierDelata);
}

TEST_F(RefModulesTest, ClassifierRuleForAsync)
{
    ClassifierTestHelper helper;
    helper.setUp(daq::SampleType::UInt64, Range(-10, 10), false);

    const auto classifierSignal = helper.getOutputSignal();
    const auto classifierSignalDescription = classifierSignal.getDescriptor();
    const auto classifierDomainSignalDescription = classifierSignal.getDomainSignal().getDescriptor();

    const auto classifierDomainRule = classifierDomainSignalDescription.getRule();

    // Check Explicit Rule
    ASSERT_EQ(classifierDomainRule.getType(), DataRuleType::Explicit);
}

TEST_F(RefModulesTest, ClassifierRangeSize)
{
    ClassifierTestHelper helper;
    helper.setUp(daq::SampleType::UInt64, Range(-3, 3));

    const auto classifierSignal = helper.getOutputSignal();
    const auto classifierSignalDescription = classifierSignal.getDescriptor();

    auto signalDimension = classifierSignalDescription.getDimensions()[0];

    ASSERT_EQ(signalDimension.getLabels(), List<NumberPtr>(-3, -2, -1, 0, 1, 2, 3));
}

TEST_F(RefModulesTest, ClassifierRangeSizeCustomClassCount)
{
    ClassifierTestHelper helper;
    helper.setUp(daq::SampleType::UInt64, Range(-5, 5));

    const auto classifierFb = helper.getClassifier();
    classifierFb.setPropertyValue("ClassCount", 4);

    const auto classifierSignal = helper.getOutputSignal();
    const auto classifierSignalDescription = classifierSignal.getDescriptor();

    const auto signalDimension = classifierSignalDescription.getDimensions()[0];

    ASSERT_EQ(signalDimension.getLabels(), List<NumberPtr>(-5, -1, 3, 7));
}

TEST_F(RefModulesTest, ClassifierRangeSizeCustomClasses)
{
    ClassifierTestHelper helper;
    helper.setUp(daq::SampleType::UInt64, Range(-10, 10));

    const auto classifierFb = helper.getClassifier();
    classifierFb.setPropertyValue("UseCustomClasses", true);
    classifierFb.setPropertyValue("CustomClassList", List<Float>(0, 1, 10, 100));

    const auto classifierSignal = helper.getOutputSignal();
    const auto classifierSignalDescription = classifierSignal.getDescriptor();

    const auto signalDimension = classifierSignalDescription.getDimensions()[0];

    ASSERT_EQ(signalDimension.getLabels(), List<Float>(0, 1, 10, 100));
}

TEST_F(RefModulesTest, ClassifierCheckSyncData)
{
    using inputSignalType = Int;
    using outputSignalType = Float;

    SampleType inputSignalSampleType = SampleType::Int64;
    auto inputSignalRange = Range(0, 20);
    bool inputSignalSync = true;

    ClassifierTestHelper helper;
    helper.setUp(inputSignalSampleType, inputSignalRange, inputSignalSync);

    const auto classifierFb = helper.getClassifier();
    classifierFb.setPropertyValue("BlockSize", 5);
    classifierFb.setPropertyValue("ClassCount", 1);

    auto reader = BlockReader<outputSignalType>(helper.getOutputSignal(), 1);

    auto dataPacket = helper.createDataPacket(5);
    auto dataPtr = static_cast<inputSignalType*>(dataPacket.getData());
    dataPtr[0] = 0;
    dataPtr[1] = 5;
    dataPtr[2] = 10;
    dataPtr[3] = 15;
    dataPtr[4] = 20;
    helper.sendPacket(dataPacket);

    size_t blockCnt = 1;
    auto outputData = std::make_unique<outputSignalType[]>(21);
    reader.read(outputData.get(), &blockCnt, 500);
    // check that was read output packet
    ASSERT_EQ(blockCnt, 1u);

    // check that sum of output values is eqauled to 1
    outputSignalType valuesSum = 0;
    for (size_t i = 0; i < 21; i++)
        valuesSum += outputData[i];
    ASSERT_EQ(valuesSum, 1.0);

    // check that values are in expected intervals
    ASSERT_EQ(outputData[0], 0.2);
    ASSERT_EQ(outputData[5], 0.2);
    ASSERT_EQ(outputData[10], 0.2);
    ASSERT_EQ(outputData[15], 0.2);
    ASSERT_EQ(outputData[20], 0.2);
}

TEST_F(RefModulesTest, ClassifierCheckSyncMultiData)
{
    using inputSignalType = Int;
    using outputSignalType = Float;

    SampleType inputSignalSampleType = SampleType::Int64;
    auto inputSignalRange = Range(0, 4);
    bool inputSignalSync = true;

    ClassifierTestHelper helper;
    helper.setUp(inputSignalSampleType, inputSignalRange, inputSignalSync);

    const auto classifierFb = helper.getClassifier();
    classifierFb.setPropertyValue("BlockSize", 5);
    classifierFb.setPropertyValue("ClassCount", 1);

    auto reader = BlockReader<outputSignalType>(helper.getOutputSignal(), 1);

    auto dataPacket = helper.createDataPacket(10);
    auto dataPtr = static_cast<inputSignalType*>(dataPacket.getData());
    // first input packet
    dataPtr[0] = 0;
    dataPtr[1] = 1;
    dataPtr[2] = 2;
    dataPtr[3] = 3;
    dataPtr[4] = 4;
    // second input packet
    dataPtr[5] = 0;
    dataPtr[6] = 1;
    dataPtr[7] = 2;
    dataPtr[8] = 3;
    dataPtr[9] = 4;
    helper.sendPacket(dataPacket);

    // reading first output block
    auto firstOutputData = std::make_unique<outputSignalType[]>(5);
    size_t firstBlockCnt = 1;
    reader.read(firstOutputData.get(), &firstBlockCnt, 500);
    ASSERT_EQ(firstBlockCnt, 1u);

    // reading second output block
    auto secondOutputData = std::make_unique<outputSignalType[]>(5);
    size_t secondBlockCnt = 1;
    reader.read(secondOutputData.get(), &secondBlockCnt, 500);
    ASSERT_EQ(secondBlockCnt, 1u);

    // check that values are in expected intervals for first result
    ASSERT_EQ(firstOutputData[0], 0.2);
    ASSERT_EQ(firstOutputData[1], 0.2);
    ASSERT_EQ(firstOutputData[2], 0.2);
    ASSERT_EQ(firstOutputData[3], 0.2);
    ASSERT_EQ(firstOutputData[4], 0.2);

    // check that values are in expected intervals for second result
    ASSERT_EQ(secondOutputData[0], 0.2);
    ASSERT_EQ(secondOutputData[1], 0.2);
    ASSERT_EQ(secondOutputData[2], 0.2);
    ASSERT_EQ(secondOutputData[3], 0.2);
    ASSERT_EQ(secondOutputData[4], 0.2);
}

TEST_F(RefModulesTest, ClassifierCheckDataWithCustomClass)
{
    using inputSignalType = Int;
    using outputSignalType = Float;

    SampleType inputSignalSampleType = SampleType::Int64;
    auto inputSignalRange = Range(0, 20);
    bool inputSignalSync = true;

    ClassifierTestHelper helper;
    helper.setUp(inputSignalSampleType, inputSignalRange, inputSignalSync);

    const auto classifierFb = helper.getClassifier();
    classifierFb.setPropertyValue("BlockSize", 5);
    classifierFb.setPropertyValue("ClassCount", 2);

    auto reader = BlockReader<outputSignalType>(helper.getOutputSignal(), 1);

    auto dataPacket = helper.createDataPacket(5);
    auto dataPtr = static_cast<inputSignalType*>(dataPacket.getData());
    dataPtr[0] = 0;
    dataPtr[1] = 5;
    dataPtr[2] = 10;
    dataPtr[3] = 15;
    dataPtr[4] = 20;
    helper.sendPacket(dataPacket);

    size_t blockCnt = 1;
    auto outputData = std::make_unique<outputSignalType[]>(11);
    reader.read(outputData.get(), &blockCnt, 500);

    // check that was read output packet
    ASSERT_EQ(blockCnt, 1u);

    // check that sum of output values is eqauled to 1
    outputSignalType valuesSum = 0;
    for (size_t i = 0; i < 11; i++)
        valuesSum += outputData[i];
    ASSERT_EQ(valuesSum, 1.0);

    // check that values are in expected intervals
    ASSERT_EQ(outputData[0], 0.2);
    ASSERT_EQ(outputData[2], 0.2);
    ASSERT_EQ(outputData[5], 0.2);
    ASSERT_EQ(outputData[7], 0.2);
    ASSERT_EQ(outputData[10], 0.2);
}

TEST_F(RefModulesTest, ClassifierCheckDataWithCustomClassList)
{
    using inputSignalType = Int;
    using outputSignalType = Float;

    SampleType inputSignalSampleType = SampleType::Int64;
    auto inputSignalRange = Range(0, 20);
    bool inputSignalSync = true;

    ClassifierTestHelper helper;
    helper.setUp(inputSignalSampleType, inputSignalRange, inputSignalSync);

    const auto classifierFb = helper.getClassifier();
    classifierFb.setPropertyValue("UseCustomClasses", true);
    classifierFb.setPropertyValue("BlockSize", 5);
    classifierFb.setPropertyValue("CustomClassList", List<Float>(0, 25, 50, 100));

    auto reader = BlockReader<outputSignalType>(helper.getOutputSignal(), 1);

    auto dataPacket = helper.createDataPacket(5);
    auto dataPtr = static_cast<inputSignalType*>(dataPacket.getData());
    dataPtr[0] = 0;
    dataPtr[1] = 5;
    dataPtr[2] = 10;
    dataPtr[3] = 15;
    dataPtr[4] = 20;
    helper.sendPacket(dataPacket);

    size_t blockCnt = 1;
    auto outputData = std::make_unique<outputSignalType[]>(4);
    reader.read(outputData.get(), &blockCnt, 500);
    // check that was read output packet
    ASSERT_EQ(blockCnt, 1u);

    // check that sum of output values is eqauled to 1
    outputSignalType valuesSum = 0;
    for (size_t i = 0; i < 4; i++)
        valuesSum += outputData[i];
    ASSERT_EQ(valuesSum, 1.0);

    // check that all values are in first interval from 0 to 25
    ASSERT_EQ(outputData[0], 1.0);
}

TEST_F(RefModulesTest, ClassifierAsyncData)
{
    using inputSignalType = Int;
    using outputSignalType = Float;

    SampleType inputSignalSampleType = SampleType::Int64;
    auto inputSignalRange = Range(0, 20);
    bool inputSignalSync = false;

    ClassifierTestHelper helper;
    helper.setUp(inputSignalSampleType, inputSignalRange, inputSignalSync);
    const auto classifierFb = helper.getClassifier();
    classifierFb.setPropertyValue("UseCustomClasses", true);
    classifierFb.setPropertyValue("BlockSize", 5);
    classifierFb.setPropertyValue("CustomClassList", List<Float>(0, 25, 50, 100));

    auto reader = BlockReader<outputSignalType>(helper.getOutputSignal(), 1);

    auto dataPacket = helper.createDataPacket(6);
    auto dataPtr = static_cast<inputSignalType*>(dataPacket.getData());
    dataPtr[0] = 0;
    dataPtr[1] = 5;
    dataPtr[2] = 10;
    dataPtr[3] = 15;
    dataPtr[4] = 20;
    dataPtr[5] = 25; // + 1 to add a markup of next block
    helper.sendPacket(dataPacket);

    size_t blockCnt = 1;
    auto outputData = std::make_unique<outputSignalType[]>(4);
    reader.read(outputData.get(), &blockCnt, 500);
    // check that was read output packet
    ASSERT_EQ(blockCnt, 1u);

    // check that sum of output values is eqauled to 1
    outputSignalType valuesSum = 0;
    for (size_t i = 0; i < 4; i++)
        valuesSum += outputData[i];
    ASSERT_EQ(valuesSum, 1.0);

    // check that all values are in first interval from 0 to 25
    ASSERT_EQ(outputData[0], 1.0);
}

TEST_F(RefModulesTest, ClassifierCheckAsyncMultiData)
{
    using inputSignalType = Int;
    using outputSignalType = Float;

    SampleType inputSignalSampleType = SampleType::Int64;
    auto inputSignalRange = Range(0, 4);
    bool inputSignalSync = false;

    ClassifierTestHelper helper;
    helper.setUp(inputSignalSampleType, inputSignalRange, inputSignalSync);

    const auto classifierFb = helper.getClassifier();
    classifierFb.setPropertyValue("BlockSize", 5);
    classifierFb.setPropertyValue("ClassCount", 1);

    auto reader = BlockReader<outputSignalType>(helper.getOutputSignal(), 1);

    auto dataPacket = helper.createDataPacket(11);
    auto dataPtr = static_cast<inputSignalType*>(dataPacket.getData());
    // first input packet
    dataPtr[0] = 0;
    dataPtr[1] = 1;
    dataPtr[2] = 2;
    dataPtr[3] = 3;
    dataPtr[4] = 4;
    // second input packet
    dataPtr[5] = 0;
    dataPtr[6] = 1;
    dataPtr[7] = 2;
    dataPtr[8] = 3;
    dataPtr[9] = 4;
    // second packet finish markup
    dataPtr[10] = 5;
    helper.sendPacket(dataPacket);

    // reading first output block
    auto firstOutputData = std::make_unique<outputSignalType[]>(5);
    size_t firstBlockCnt = 1;
    reader.read(firstOutputData.get(), &firstBlockCnt, 200);
    ASSERT_EQ(firstBlockCnt, 1u);

    // reading second output block
    auto secondOutputData = std::make_unique<outputSignalType[]>(5);
    size_t secondBlockCnt = 1;
    reader.read(secondOutputData.get(), &secondBlockCnt, 200);
    ASSERT_EQ(secondBlockCnt, 1u);

    // check that values are in expected intervals for first result
    ASSERT_EQ(firstOutputData[0], 0.2);
    ASSERT_EQ(firstOutputData[1], 0.2);
    ASSERT_EQ(firstOutputData[2], 0.2);
    ASSERT_EQ(firstOutputData[3], 0.2);
    ASSERT_EQ(firstOutputData[4], 0.2);

    // check that values are in expected intervals for second result
    ASSERT_EQ(secondOutputData[0], 0.2);
    ASSERT_EQ(secondOutputData[1], 0.2);
    ASSERT_EQ(secondOutputData[2], 0.2);
    ASSERT_EQ(secondOutputData[3], 0.2);
    ASSERT_EQ(secondOutputData[4], 0.2);
}
