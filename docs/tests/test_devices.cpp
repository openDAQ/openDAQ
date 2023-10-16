#include <gtest/gtest.h>
#include <chrono>
#include <opendaq/opendaq.h>
#include "docs_test_helpers.h"

using DevicesTest = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

// Corresponding document: Antora/modules/explanation/pages/device.adoc
TEST_F(DevicesTest, DeviceInfo)
{
    auto instance = docs_test_helpers::setupInstance();
    auto info = instance.getInfo();
    ASSERT_TRUE(info.getSerialNumber().assigned());
    ASSERT_TRUE(info.getName().assigned());
}

// Corresponding document: Antora/modules/explanation/pages/device.adoc
TEST_F(DevicesTest, DeviceComponents)
{
    auto instance = docs_test_helpers::setupInstance();
    auto device = instance.getRootDevice();
    
    ListPtr<IFunctionBlock> functionBlocks = device.getFunctionBlocks();
    ListPtr<ISignal> signals = device.getSignals();
    ListPtr<IDevice> devices = device.getDevices();
    FolderPtr inputsOutputsFolder = device.getInputsOutputsFolder();
    ListPtr<IChannel> channels =  device.getChannels();
    ListPtr<IComponent> customComponents = device.getCustomComponents();

    ASSERT_TRUE(functionBlocks.assigned());
    ASSERT_TRUE(signals.assigned());
    ASSERT_TRUE(devices.assigned());
    ASSERT_TRUE(inputsOutputsFolder.assigned());
    ASSERT_TRUE(channels.assigned());
    ASSERT_TRUE(customComponents.assigned());
}

// Corresponding document: Antora/modules/explanation/pages/device.adoc
TEST_F(DevicesTest, InstanceAndRoot)
{
    auto instance = docs_test_helpers::setupInstance();
    auto availableDevices1 = instance.getAvailableDevices();
    auto availableDevices2 = instance.getRootDevice().getAvailableDevices();
}

END_NAMESPACE_OPENDAQ
