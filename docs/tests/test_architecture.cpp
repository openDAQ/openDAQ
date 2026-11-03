#include <gtest/gtest.h>
#include <opendaq/instance_ptr.h>

#include <opendaq/opendaq.h>

using ArchitectureTest = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

// Corresponding document : Antora/modules/background_info/pages/function_blocks.adoc
TEST_F(ArchitectureTest, GetChannels)
{
    InstancePtr instance = daq::Instance();

    DevicePtr device = instance.addDevice("daqref://device0");
    ListPtr<IChannel> channels  = device.getChannels();
    ASSERT_GT(channels.getCount(), 0u);
}

// Corresponding document : Antora/modules/background_info/pages/function_blocks.adoc
TEST_F(ArchitectureTest, ConnectSignal)
{
    InstancePtr instance = daq::Instance();
    DevicePtr device = instance.addDevice("daqref://device0");
    FunctionBlockPtr fb = instance.addFunctionBlock("RefFBModuleStatistics");
    SignalPtr signal = device.getSignalsRecursive()[0];
    InputPortPtr inputPort = fb.getInputPorts()[0];
    // doc code
    inputPort.connect(signal);
    // signal is now connected to the inputPort
    daq::SignalPtr signal1 = inputPort.getSignal();
    ASSERT_EQ(signal1, signal);
    //
}

// Corresponding document : Antora/modules/background_info/pages/function_blocks.adoc
TEST_F(ArchitectureTest, CreateFunctionBlock)
{
    InstancePtr instance = daq::Instance();
    DevicePtr device = instance.addDevice("daqref://device0");
    // doc code
    daq::FunctionBlockPtr fb = instance.addFunctionBlock("RefFBModuleStatistics");
    // function block appears under FunctionBlocks of the instance
    daq::ListPtr<IFunctionBlock> fbs = instance.getFunctionBlocks();
    daq::FunctionBlockPtr fb1 = fbs[fbs.getCount() - 1];
    ASSERT_EQ(fb, fb1);  //
}

// Corresponding document : Antora/modules/background_info/pages/data_path.adoc
TEST_F(ArchitectureTest, InputPortConnection)
{
    InstancePtr instance = daq::Instance();
    DevicePtr device = instance.addDevice("daqref://device0");
    SignalPtr signal = device.getSignalsRecursive()[0];
    FunctionBlockPtr fb = instance.addFunctionBlock("RefFBModuleStatistics");
    InputPortPtr inputPort = fb.getInputPorts()[0];
    // doc code
    inputPort.connect(signal);
    // connection object is now accessible on inputPort
    daq::ConnectionPtr connection = inputPort.getConnection();
    ASSERT_TRUE(connection.assigned());
    //
}

// Corresponding document : Antora/modules/background_info/pages/data_path.adoc
TEST_F(ArchitectureTest, ConnectionDequeue)
{
    InstancePtr instance = daq::Instance();
    DevicePtr device = instance.addDevice("daqref://device0");
    SignalPtr signal = device.getSignalsRecursive()[0];
    FunctionBlockPtr fb = instance.addFunctionBlock("RefFBModuleStatistics");
    InputPortPtr inputPort = fb.getInputPorts()[0];
    inputPort.connect(signal);
    // doc code
    [[maybe_unused]] PacketPtr packet = inputPort.getConnection().dequeue();
    //
}

END_NAMESPACE_OPENDAQ
