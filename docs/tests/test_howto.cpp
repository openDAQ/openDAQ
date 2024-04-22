#include <gtest/gtest.h>
#include <opendaq/opendaq.h>
#include <fstream>

#include <thread>
#include "docs_test_helpers.h"

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
    // Create an openDAQ(TM) Instance, loading modules from the current directory
    InstancePtr instance = Instance();

    // Add simulated device
    DevicePtr device = instance.addDevice("daqref://device0");

    // Get available Function Block types
    DictPtr<IString, IFunctionBlockType> functionBlockTypes = instance.getAvailableFunctionBlockTypes();
    for (const auto& functionBlockType : functionBlockTypes.getKeys())
        std::cout << functionBlockType << std::endl;

    // If there is no Statistics Function Block available, exit with an error
    if (!functionBlockTypes.hasKey("ref_fb_module_statistics"))
        ASSERT_TRUE(false) << "Function Block does not exist";

    // Add Function Block on the host computer
    FunctionBlockPtr functionBlock = instance.addFunctionBlock("ref_fb_module_statistics");

    // Print Function Block type info
    FunctionBlockTypePtr functionBlockType = functionBlock.getFunctionBlockType();
    std::cout << functionBlockType.getId() << std::endl;
    std::cout << functionBlockType.getName() << std::endl;
    std::cout << functionBlockType.getDescription() << std::endl;
}

// Corresponding document: Antora/modules/howto_guides/pages/howto_configure_function_block.adoc
TEST_F(HowToTest, ConfigureFunctionBlock)
{
    // Create an openDAQ(TM) Instance, loading modules from the current directory
    InstancePtr instance = Instance();

    // Add simulated device
    DevicePtr device = instance.addDevice("daqref://device0");

    // Add Function Block on the host computer
    FunctionBlockPtr functionBlock = instance.addFunctionBlock("ref_fb_module_statistics");

    // List properties of the Function Block
    ListPtr<IProperty> functionBlockProperties = functionBlock.getVisibleProperties();
    for (const auto& prop : functionBlockProperties)
        std::cout << prop.getName() << std::endl;

    // Print current block size
    Int currentBlockSize = functionBlock.getPropertyValue("BlockSize");
    std::cout << "Current block size is " << currentBlockSize << std::endl;
    // Configure the properties of the Function Block
    functionBlock.setPropertyValue("BlockSize", 100);

    // Connect the first Signal of the first Channel from the Device to the first Input Port on the Function Block
    functionBlock.getInputPorts()[0].connect(device.getChannels()[0].getSignals()[0]);
    // Read data from the Signal
    // ...

    // Get the output Signal of the Function Block
    SignalPtr outputSignal = functionBlock.getSignals()[0];

    std::cout << outputSignal.getDescriptor().getName() << std::endl;
}

// Corresponding document: Antora/modules/howto_guides/pages/howto_save_load_configuration.adoc
TEST_F(HowToTest, SaveLoadConfiguration)
{
    {
        InstancePtr instance = Instance();

        // Save Configuration to string
        std::string jsonStr = instance.saveConfiguration();
        // Write Configuration string to file
        std::ofstream configFile("config.json");
        configFile << jsonStr;
        configFile.close();
    }

    {
        InstancePtr instance = Instance();

        // Read Configuration from file
        std::ifstream configFile("config.json");
        std::stringstream jsonStr;
        jsonStr << configFile.rdbuf();
        // Load Configuration from string
        instance.loadConfiguration(jsonStr.str());
    }
}

TEST_F(HowToTest, InstanceConfiguration)
{
    InstanceBuilderPtr builder =
        InstanceBuilder().setGlobalLogLevel(LogLevel::Info).setModulePath("").setSchedulerWorkerNum(1).setRootDevice("daqref://device0");

    InstancePtr instance = builder.build();
    daq::DevicePtr device = instance.getRootDevice();
    ASSERT_TRUE(device.assigned());
}

END_NAMESPACE_OPENDAQ
