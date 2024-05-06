#include <gtest/gtest.h>
#include <opendaq/mock/mock_device_module.h>
#include <opendaq/opendaq.h>

using namespace daq;

class RegressionTestDevice : public testing::TestWithParam<StringPtr>
{
private:
    ModuleManagerPtr moduleManager;
    ContextPtr context;
    ModulePtr deviceModule;
    InstancePtr instance;

protected:
    DevicePtr device;

    void SetUp() override
    {
        moduleManager = ModuleManager("");
        context = Context(nullptr, Logger(), TypeManager(), moduleManager);

        deviceModule = MockDeviceModule_Create(context);
        moduleManager.addModule(deviceModule);

        instance = InstanceCustom(context, "mock_instance");

        // device = instance.addDevice(GetParam());

        // TODO: to be able to add and remove function blocks from Device?
        instance.setRootDevice(GetParam());
        device = instance.getRootDevice();
    }
};

TEST_P(RegressionTestDevice, getInfo)
{
    ASSERT_NO_THROW(device.getInfo());
}

TEST_P(RegressionTestDevice, getDomain)
{
    ASSERT_NO_THROW(device.getDomain());
}

TEST_P(RegressionTestDevice, getInputsOutputsFolder)
{
    ASSERT_NO_THROW(device.getInputsOutputsFolder());
}

TEST_P(RegressionTestDevice, getCustomComponents)
{
    ASSERT_NO_THROW(device.getCustomComponents());
}

TEST_P(RegressionTestDevice, getSignals)
{
    ASSERT_NO_THROW(device.getSignals());
}

TEST_P(RegressionTestDevice, getSignalsRecursive)
{
    ASSERT_NO_THROW(device.getSignalsRecursive());
}

TEST_P(RegressionTestDevice, getChannels)
{
    ASSERT_NO_THROW(device.getChannels());
}

TEST_P(RegressionTestDevice, getChannelsRecursive)
{
    ASSERT_NO_THROW(device.getChannelsRecursive());
}

TEST_P(RegressionTestDevice, getDevices)
{
    ASSERT_NO_THROW(device.getDevices());
}

TEST_P(RegressionTestDevice, getAvailableDevices)
{
    ASSERT_NO_THROW(device.getAvailableDevices());
}

TEST_P(RegressionTestDevice, getAvailableDeviceTypes)
{
    ASSERT_NO_THROW(device.getAvailableDeviceTypes());
}

// TODO: ???
TEST_P(RegressionTestDevice, addDeviceRemoveDevice)
{
    DevicePtr dev;
    ASSERT_NO_THROW(dev = device.addDevice("mock_phys_device"));
    ASSERT_NO_THROW(device.removeDevice(dev));
}

TEST_P(RegressionTestDevice, getFunctionBlocks)
{
    ASSERT_NO_THROW(device.getFunctionBlocks());
}

TEST_P(RegressionTestDevice, getAvailableFunctionBlockTypes)
{
    DictPtr<IString, IFunctionBlockType> fbTypes;
    ASSERT_NO_THROW(fbTypes = device.getAvailableFunctionBlockTypes());
}

TEST_P(RegressionTestDevice, addFunctionBlockRemoveFunctionBlock)
{
    // TODO: should not rely on "ref_fb_module_trigger" being present
    FunctionBlockPtr fb;
    ASSERT_NO_THROW(fb = device.addFunctionBlock("ref_fb_module_trigger"));
    ASSERT_NO_THROW(device.removeFunctionBlock(fb));
}

TEST_P(RegressionTestDevice, saveConfigurationLoadConfiguration)
{
    StringPtr config;
    ASSERT_NO_THROW(config = device.saveConfiguration());
    ASSERT_NO_THROW(device.loadConfiguration(config));
}

TEST_P(RegressionTestDevice, getTicksSinceOrigin)
{
    ASSERT_NO_THROW(device.getTicksSinceOrigin());
}

INSTANTIATE_TEST_SUITE_P(Device,
                         RegressionTestDevice,
                         testing::Values("daq.opcua://127.0.0.1", "daq.ns://127.0.0.1", "daq.lt://127.0.0.1"));
