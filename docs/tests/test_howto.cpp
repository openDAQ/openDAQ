#include <gtest/gtest.h>
#include <opendaq/opendaq.h>
#include <fstream>

#include "docs_test_helpers.h"
#include <thread>

using HowToTest = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

// Corresponding document: Antora/modules/howto_guides/pages/howto_connect_to_device.adoc
TEST_F(HowToTest, ConnectToDevice1)
{
    SKIP_TEST_MAC_CI;

    InstancePtr server = docs_test_helpers::setupSimulatorServers();
    daq::InstancePtr instance = daq::Instance();

    daq::DevicePtr device = instance.addDevice("daq.opcua://127.0.0.1/");
    ASSERT_TRUE(device.assigned());
}

// Corresponding document: Antora/modules/howto_guides/pages/howto_connect_to_device.adoc
TEST_F(HowToTest, ConnectToDevice2)
{
    daq::InstancePtr instance = daq::Instance();

    ListPtr<IDevice> devices = List<IDevice>();
    for (const auto& deviceInfo : instance.getAvailableDevices())
        if (deviceInfo.getConnectionString().toStdString().find("daqref://") != std::string::npos)
            devices.pushBack(instance.addDevice(deviceInfo.getConnectionString()));

    ASSERT_EQ(devices.getCount(), (SizeT) 2);
}

void printProperty(const PropertyPtr& /*prop*/)
{
//    std::cout << prop.getName() << std::endl;
}

// Corresponding document: Antora/modules/howto_guides/pages/howto_add_function_block.adoc
TEST_F(HowToTest, AddFunctionBlock)
{
    // Create an openDAQ(TM) instance, loading modules from the current directory
    InstancePtr instance = Instance();

    // add simulated device
    DevicePtr device = instance.addDevice("daqref://device0");

    // get available function block types
    DictPtr<IString, IFunctionBlockType> functionBlockTypes = instance.getAvailableFunctionBlockTypes();
    for (const auto& functionBlockType : functionBlockTypes.getKeys())
        std::cout << functionBlockType << std::endl;

    // if there is not statistics function block available, exit with error
    if (!functionBlockTypes.hasKey("ref_fb_module_statistics"))
        ASSERT_TRUE(false) << "Function block does not exist";

    // add function block on the host computer
    FunctionBlockPtr functionBlock = instance.addFunctionBlock("ref_fb_module_statistics");

    // print function block type info
    FunctionBlockTypePtr functionBlockType = functionBlock.getFunctionBlockType();
    std::cout << functionBlockType.getId() << std::endl;
    std::cout << functionBlockType.getName() << std::endl;
    std::cout << functionBlockType.getDescription() << std::endl;
}

// Corresponding document: Antora/modules/howto_guides/pages/howto_configure_function_block.adoc
TEST_F(HowToTest, ConfigureFunctionBlock)
{
    // Create an openDAQ(TM) instance, loading modules from the current directory
    InstancePtr instance = Instance();

    // add simulated device
    DevicePtr device = instance.addDevice("daqref://device0");

    // add function block on the host computer
    FunctionBlockPtr functionBlock = instance.addFunctionBlock("ref_fb_module_statistics");

    // list properties of the function block
    ListPtr<IProperty> functionBlockProperties = functionBlock.getVisibleProperties();
    for (const auto& prop : functionBlockProperties)
        std::cout << prop.getName() << std::endl;

    // print current block size
    Int currentBlockSize = functionBlock.getPropertyValue("BlockSize");
    std::cout << "Current block size is " << currentBlockSize << std::endl;

    // configure the properties of the function block
    functionBlock.setPropertyValue("BlockSize", 100);

    // connect the first signal of the first channel from the device to the first input port on the function block
    functionBlock.getInputPorts()[0].connect(device.getChannels()[0].getSignals()[0]);

    // get the output signal of the function block
    SignalPtr outputSignal = functionBlock.getSignals()[0];

    std::cout << outputSignal.getDescriptor().getName() << std::endl;
}

// Corresponding document: Antora/modules/howto_guides/pages/howto_save_load_configuration.adoc
TEST_F(HowToTest, SaveLoadConfiguration)
{
    {
        InstancePtr instance = Instance();

        // save configuration to string
        std::string jsonStr = instance.saveConfiguration();

        // write configuration string to file
        std::ofstream configFile("config.json");
        configFile << jsonStr;
        configFile.close();
    }

    {
        InstancePtr instance = Instance();

        // read configuration from file
        std::ifstream configFile("config.json");
        std::stringstream jsonStr;
        jsonStr << configFile.rdbuf();

        // load configuration from string
        instance.loadConfiguration(jsonStr.str());
    }
}

TEST_F(HowToTest, InstanceConfiguration)
{
    InstanceBuilderPtr builder = InstanceBuilder()
        .setGlobalLogLevel(LogLevel::Info)
        .setModulePath("")
        .setSchedulerWorkerNum(1)
        .setRootDevice("daqref://device0");

    InstancePtr instance = builder.build();
    daq::DevicePtr device = instance.getRootDevice();
    ASSERT_TRUE(device.assigned());
}


END_NAMESPACE_OPENDAQ
