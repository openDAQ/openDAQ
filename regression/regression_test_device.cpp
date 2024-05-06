#include <gtest/gtest.h>
#include <opendaq/mock/mock_device_module.h>
#include <opendaq/opendaq.h>

using namespace daq;

class RegressionTestDevice
{
protected:
    ModuleManagerPtr moduleManager;
    ContextPtr context;
    InstancePtr instance;
    DevicePtr device;

    void setUp()
    {
        moduleManager = ModuleManager("");
        context = Context(nullptr, Logger(), TypeManager(), moduleManager);
        instance = InstanceCustom(context, "mock_instance");
    }
};

class RegressionTestDeviceAll : public RegressionTestDevice, public testing::TestWithParam<StringPtr>
{
    void SetUp() override
    {
        setUp();

        // device = instance.addDevice(GetParam());

        // TODO: to be able to add and remove function blocks from Device?
        instance.setRootDevice(GetParam());
        device = instance.getRootDevice();
    }
};

class RegressionTestDeviceOpcUa : public RegressionTestDevice, public testing::Test
{
    void SetUp() override
    {
        setUp();

        // device = instance.addDevice(GetParam());

        // TODO: to be able to add and remove function blocks from Device?
        instance.setRootDevice("daq.opcua://127.0.0.1");
        device = instance.getRootDevice();
    }
};

class RegressionTestDeviceNs : public RegressionTestDevice, public testing::Test
{
    void SetUp() override
    {
        setUp();

        // device = instance.addDevice(GetParam());

        // TODO: to be able to add and remove function blocks from Device?
        instance.setRootDevice("daq.ns://127.0.0.1");
        device = instance.getRootDevice();
    }
};

class RegressionTestDeviceLt : public RegressionTestDevice, public testing::Test
{
    void SetUp() override
    {
        setUp();

        // device = instance.addDevice(GetParam());

        // TODO: to be able to add and remove function blocks from Device?
        instance.setRootDevice("daq.lt://127.0.0.1");
        device = instance.getRootDevice();
    }
};

TEST_F(RegressionTestDeviceOpcUa, getInfo)
{
    DeviceInfoPtr info;
    ASSERT_NO_THROW(info = device.getInfo());
    ASSERT_EQ(info.getName(), "Device 1");
    ASSERT_EQ(info.getModel(), "Reference Device");
    ASSERT_EQ(info.getSerialNumber(), "dev_ser_1");
}

TEST_F(RegressionTestDeviceNs, getInfo)
{
    DeviceInfoPtr info;
    ASSERT_NO_THROW(info = device.getInfo());
    ASSERT_EQ(info.getName(), "NativeStreamingClientPseudoDevice");
}

TEST_F(RegressionTestDeviceLt, getInfo)
{
    DeviceInfoPtr info;
    ASSERT_NO_THROW(info = device.getInfo());
    ASSERT_EQ(info.getName(), "WebsocketClientPseudoDevice");
}

TEST_F(RegressionTestDeviceOpcUa, getDomain)
{
    DeviceDomainPtr domain;
    ASSERT_NO_THROW(domain = device.getDomain());
    ASSERT_EQ(domain.getTickResolution().getNumerator(), 1);
    ASSERT_EQ(domain.getTickResolution().getDenominator(), 1000000);
    ASSERT_EQ(domain.getOrigin(), "1970-01-01T00:00:00Z");
    ASSERT_EQ(domain.getUnit(), Unit(""));
}

TEST_F(RegressionTestDeviceOpcUa, getInputsOutputsFolder)
{
    FolderPtr folder;
    ASSERT_NO_THROW(folder = device.getInputsOutputsFolder());
    ASSERT_EQ(folder.getItems().getCount(), 2);
}

TEST_F(RegressionTestDeviceOpcUa, getCustomComponents)
{
    ListPtr<IComponent> components;
    ASSERT_NO_THROW(components = device.getCustomComponents());
    ASSERT_EQ(components[0].getAllProperties()[0].getName(), "UseSync");
}

TEST_F(RegressionTestDeviceOpcUa, getSignals)
{
    ListPtr<ISignal> signals;
    ASSERT_NO_THROW(signals = device.getSignals());
    ASSERT_EQ(signals.getCount(), 0);
}

TEST_F(RegressionTestDeviceNs, getSignals)
{
    ListPtr<ISignal> signals;
    ASSERT_NO_THROW(signals = device.getSignals());
    ASSERT_EQ(signals.getCount(), 2);
}

TEST_F(RegressionTestDeviceLt, getSignals)
{
    ListPtr<ISignal> signals;
    ASSERT_NO_THROW(signals = device.getSignals());
    ASSERT_EQ(signals.getCount(), 4);
}

TEST_F(RegressionTestDeviceOpcUa, getSignalsRecursive)
{
    ListPtr<ISignal> signals;
    ASSERT_NO_THROW(signals = device.getSignalsRecursive());
    ASSERT_EQ(signals.getCount(), 2);
}

TEST_F(RegressionTestDeviceNs, getSignalsRecursive)
{
    ListPtr<ISignal> signals;
    ASSERT_NO_THROW(signals = device.getSignalsRecursive());
    ASSERT_EQ(signals.getCount(), 2);
}

TEST_F(RegressionTestDeviceLt, getSignalsRecursive)
{
    ListPtr<ISignal> signals;
    ASSERT_NO_THROW(signals = device.getSignalsRecursive());
    ASSERT_EQ(signals.getCount(), 4);
}

TEST_F(RegressionTestDeviceOpcUa, getChannels)
{
    ListPtr<IChannel> channels;
    ASSERT_NO_THROW(channels = device.getChannels());
    ASSERT_EQ(channels.getCount(), 2);
}

TEST_F(RegressionTestDeviceNs, getChannels)
{
    ListPtr<IChannel> channels;
    ASSERT_NO_THROW(channels = device.getChannels());
    ASSERT_EQ(channels.getCount(), 0);
}

TEST_F(RegressionTestDeviceLt, getChannels)
{
    ListPtr<IChannel> channels;
    ASSERT_NO_THROW(channels = device.getChannels());
    ASSERT_EQ(channels.getCount(), 0);
}

TEST_F(RegressionTestDeviceOpcUa, getChannelsRecursive)
{
    ListPtr<IChannel> channels;
    ASSERT_NO_THROW(channels = device.getChannelsRecursive());
    ASSERT_EQ(channels.getCount(), 2);
}

TEST_F(RegressionTestDeviceNs, getChannelsRecursive)
{
    ListPtr<IChannel> channels;
    ASSERT_NO_THROW(channels = device.getChannelsRecursive());
    ASSERT_EQ(channels.getCount(), 0);
}

TEST_F(RegressionTestDeviceLt, getChannelsRecursive)
{
    ListPtr<IChannel> channels;
    ASSERT_NO_THROW(channels = device.getChannelsRecursive());
    ASSERT_EQ(channels.getCount(), 0);
}

TEST_P(RegressionTestDeviceAll, getDevices)
{
    ListPtr<IDevice> devices;
    ASSERT_NO_THROW(devices = device.getDevices());
    ASSERT_EQ(devices.getCount(), 0);
}

TEST_P(RegressionTestDeviceAll, getAvailableDevices)
{
    ListPtr<IDeviceInfo> infos;
    ASSERT_NO_THROW(infos = device.getAvailableDevices());
    ASSERT_EQ(infos.getCount(), 0);
}

TEST_P(RegressionTestDeviceAll, getAvailableDeviceTypes)
{
    DictPtr<IString, IDeviceType> types;
    ASSERT_NO_THROW(types = device.getAvailableDeviceTypes());
    ASSERT_EQ(types.getCount(), 0);
}

TEST_P(RegressionTestDeviceAll, getFunctionBlocks)
{
    ListPtr<IFunctionBlock> functionBlocks;
    ASSERT_NO_THROW(functionBlocks = device.getFunctionBlocks());
    ASSERT_EQ(functionBlocks.getCount(), 0);
}

TEST_P(RegressionTestDeviceAll, getAvailableFunctionBlockTypes)
{
    DictPtr<IString, IFunctionBlockType> types;
    ASSERT_NO_THROW(types = device.getAvailableFunctionBlockTypes());
    ASSERT_EQ(types.getCount(), 8);
}

TEST_P(RegressionTestDeviceAll, addFunctionBlockRemoveFunctionBlock)
{
    // TODO: should not rely on "ref_fb_module_trigger" being present
    FunctionBlockPtr fb;
    ASSERT_NO_THROW(fb = device.addFunctionBlock("ref_fb_module_trigger"));
    ASSERT_NO_THROW(device.removeFunctionBlock(fb));
}

TEST_P(RegressionTestDeviceAll, saveConfigurationLoadConfiguration)
{
    StringPtr config;
    ASSERT_NO_THROW(config = device.saveConfiguration());
    ASSERT_NO_THROW(device.loadConfiguration(config));
}

TEST_P(RegressionTestDeviceAll, getTicksSinceOrigin)
{
    ASSERT_NO_THROW(device.getTicksSinceOrigin());
}

INSTANTIATE_TEST_SUITE_P(Device,
                         RegressionTestDeviceAll,
                         testing::Values("daq.opcua://127.0.0.1", "daq.ns://127.0.0.1", "daq.lt://127.0.0.1"));
