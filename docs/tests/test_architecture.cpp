#include <gtest/gtest.h>
#include <chrono>
#include <thread>
#include <opendaq/opendaq.h>
#include "docs_test_helpers.h"

using ArchitectureTest = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

// Corresponding document: Antora/modules/explanation/pages/blueberry_architecture.adoc
TEST_F(ArchitectureTest, ClientModule)
{
    InstancePtr instance = daq::Instance();
    ModuleManagerPtr manager = instance.getModuleManager();
    ASSERT_TRUE(manager.assigned());
    ASSERT_GT(manager.getModules().getCount(), 0u);

    ModulePtr clientModule;
    for (auto mod : manager.getModules())
    {
        if (mod.getName() == "openDAQ OpcUa client module")
        {
            clientModule = mod;
            break;
        }
    }

    ASSERT_TRUE(clientModule.assigned());
    ASSERT_NO_THROW(clientModule.getAvailableDevices());
    ASSERT_NO_THROW(clientModule.getAvailableServerTypes());
    ASSERT_NO_THROW(clientModule.getAvailableFunctionBlockTypes());

    auto root = instance.getRootDevice();
    ASSERT_EQ(instance.getAvailableFunctionBlockTypes().getCount(), root.getAvailableFunctionBlockTypes().getCount());
}

TEST_F(ArchitectureTest, AddDevice)
{
    SKIP_TEST_MAC_CI;

    InstancePtr server = docs_test_helpers::setupSimulatorServers();
    InstancePtr instance = daq::Instance();

    DevicePtr device = instance.addDevice("daq.opcua://127.0.0.1");
    ASSERT_NE(device.getChannels().getCount(), 0u);

    ASSERT_TRUE(device.asPtr<IPropertyObject>().assigned());
    ASSERT_TRUE(device.getChannels()[0].asPtr<IPropertyObject>().assigned());
    ASSERT_TRUE(device.getChannels()[0].asPtr<IFunctionBlock>().assigned());

    using namespace std::chrono_literals;
    std::this_thread::sleep_for(1000ms);
}

TEST_F(ArchitectureTest, Statistics)
{
    SKIP_TEST_MAC_CI;

    InstancePtr server = docs_test_helpers::setupSimulatorServers();
    InstancePtr instance = daq::Instance();

    DevicePtr device = instance.addDevice("daq.opcua://127.0.0.1");

    FunctionBlockPtr statistics = instance.addFunctionBlock("ref_fb_module_statistics");
    ASSERT_TRUE(statistics.asPtr<IPropertyObject>().assigned());
    ASSERT_GT(statistics.getInputPorts().getCount(), 0u);
    ASSERT_GT(statistics.getSignals().getCount(), 0u);

    SignalPtr deviceOut = device.getSignalsRecursive()[0];

    statistics.getInputPorts()[0].connect(deviceOut);

    using namespace std::chrono_literals;
    std::this_thread::sleep_for(500ms);

    SignalPtr statisticsOut = statistics.getSignalsRecursive()[0];
    ASSERT_TRUE(statisticsOut.getDomainSignal().assigned());

    ASSERT_TRUE(statistics.getInputPorts()[0].getConnection().assigned());
    ASSERT_GT(deviceOut.getConnections().getCount(), 0u);
}

TEST_F(ArchitectureTest, DataRule)
{
    SKIP_TEST_MAC_CI;

    InstancePtr server = docs_test_helpers::setupSimulatorServers();
    InstancePtr instance = daq::Instance();

    DevicePtr device = instance.addDevice("daq.opcua://127.0.0.1");

    DataRulePtr rule = device.getSignals(search::Recursive(search::Any()))[1].getDescriptor().getRule();
    ASSERT_NO_THROW(rule.getParameters().get("start"));
    ASSERT_NO_THROW(rule.getParameters().get("delta"));
    ASSERT_EQ(rule.getType(), DataRuleType::Linear);
}

TEST_F(ArchitectureTest, Readers)
{
    SKIP_TEST_MAC_CI;

    InstancePtr server = docs_test_helpers::setupSimulatorServers();
    InstancePtr instance = daq::Instance();

    DevicePtr device = instance.addDevice("daq.opcua://127.0.0.1");

    FunctionBlockPtr statistics = instance.addFunctionBlock("ref_fb_module_statistics");
    statistics.getInputPorts()[0].connect(device.getChannels()[0].getSignals()[0]);

    PacketReaderPtr packetReader = PacketReader(statistics.getSignals(search::Recursive(search::Any()))[0]);
    StreamReaderPtr streamReader = StreamReader<double, uint64_t>(statistics.getSignals(search::Recursive(search::Any()))[0]);

    using namespace std::chrono_literals;
    std::this_thread::sleep_for(1000ms);

    PacketPtr packet = packetReader.read();
    ASSERT_TRUE(packet.assigned());

    double data[1000];
    SizeT count = 1000;
    streamReader.read(data, &count);
    ASSERT_GT(count, 0u);
}

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
    FunctionBlockPtr fb = instance.addFunctionBlock("ref_fb_module_statistics");
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
    daq::FunctionBlockPtr fb = instance.addFunctionBlock("ref_fb_module_statistics");
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
    FunctionBlockPtr fb = instance.addFunctionBlock("ref_fb_module_statistics");
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
    FunctionBlockPtr fb = instance.addFunctionBlock("ref_fb_module_statistics");
    InputPortPtr inputPort = fb.getInputPorts()[0];
    inputPort.connect(signal);
    // doc code
    [[maybe_unused]] PacketPtr packet = inputPort.getConnection().dequeue();
    //
}

END_NAMESPACE_OPENDAQ
