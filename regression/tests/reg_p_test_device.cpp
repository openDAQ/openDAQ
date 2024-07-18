#include "setup_regression.h"

class RegressionTestDevice : public testing::Test
{
private:
    ModuleManagerPtr moduleManager;
    ContextPtr context;
    InstancePtr instance;

protected:
    DevicePtr device;

    void SetUp() override
    {
        moduleManager = ModuleManager("");
        context = Context(nullptr, Logger(), TypeManager(), moduleManager, nullptr, nullptr, nullptr);

        instance = InstanceCustom(context, "mock_instance");

        device = instance.addDevice(connectionString);
    }
};

TEST_F(RegressionTestDevice, getInfo)
{
    DeviceInfoPtr info;
    ASSERT_NO_THROW(info = device.getInfo());
    ASSERT_TRUE(info.assigned());
    if (protocol == "opcua" || protocol == "nd")
    {
        ASSERT_EQ(info.getName(), "ref_dev1");
        ASSERT_EQ(info.getModel(), "Reference Device");
        ASSERT_EQ(info.getSerialNumber(), "dev_ser_1");
    }
    else if (protocol == "ns")
    {
        ASSERT_EQ(info.getName(), "NativeStreamingClientPseudoDevice");
    }
    else if (protocol == "lt")
    {
        ASSERT_EQ(info.getName(), "WebsocketClientPseudoDevice");
    }
}

TEST_F(RegressionTestDevice, getDomain)
{
    PROTOCOLS("opcua", "nd")

    DeviceDomainPtr domain;
    ASSERT_NO_THROW(domain = device.getDomain());
    ASSERT_EQ(domain.getTickResolution().getNumerator(), 1);
    ASSERT_EQ(domain.getTickResolution().getDenominator(), 1000000);
    ASSERT_EQ(domain.getOrigin(), "1970-01-01T00:00:00Z");

    if (protocol == "opcua")
    {
        ASSERT_EQ(domain.getUnit(), Unit("", -1, "", ""));
    }
    else if (protocol == "nd")
    {
        ASSERT_EQ(domain.getUnit(), Unit("s", -1, "second", "time"));
    }
}

TEST_F(RegressionTestDevice, getInputsOutputsFolder)
{
    FolderPtr folder;
    ASSERT_NO_THROW(folder = device.getInputsOutputsFolder());
    if (protocol == "opcua" || protocol == "nd")
    {
        ASSERT_EQ(folder.getItems().getCount(), 2);
    }
    else if (protocol == "ns" || protocol == "lt")
    {
        ASSERT_EQ(folder.getItems().getCount(), 0);
    }
}

TEST_F(RegressionTestDevice, getCustomComponents)
{
    ListPtr<IComponent> components;
    ASSERT_NO_THROW(components = device.getCustomComponents());
    if (protocol == "opcua" || protocol == "nd")
    {
        ASSERT_EQ(components.getCount(), 1);
    }
    else if (protocol == "ns" || protocol == "lt")
    {
        ASSERT_EQ(components.getCount(), 0);
    }
}

TEST_F(RegressionTestDevice, getSignals)
{
    ListPtr<ISignal> signals;
    ASSERT_NO_THROW(signals = device.getSignals());
    if (protocol == "opcua" || protocol == "nd")
    {
        ASSERT_EQ(signals.getCount(), 0);
    }
    else if (protocol == "ns")
    {
        ASSERT_EQ(signals.getCount(), 4);
    }
    else if (protocol == "lt")
    {
        ASSERT_EQ(signals.getCount(), 6);
    }
}

TEST_F(RegressionTestDevice, getSignalsRecursive)
{
    ListPtr<ISignal> signals;
    ASSERT_NO_THROW(signals = device.getSignalsRecursive());
    if (protocol == "opcua" || protocol == "nd" || protocol == "ns")
    {
        ASSERT_EQ(signals.getCount(), 4);
    }
    else if (protocol == "lt")
    {
        ASSERT_EQ(signals.getCount(), 6);
    }
}

TEST_F(RegressionTestDevice, getChannels)
{
    ListPtr<IChannel> channels;
    ASSERT_NO_THROW(channels = device.getChannels());
    if (protocol == "opcua" || protocol == "nd")
    {
        ASSERT_EQ(channels.getCount(), 2);
    }
    else if (protocol == "ns" || protocol == "lt")
    {
        ASSERT_EQ(channels.getCount(), 0);
    }
}

TEST_F(RegressionTestDevice, getChannelsRecursive)
{
    ListPtr<IChannel> channels;
    ASSERT_NO_THROW(channels = device.getChannelsRecursive());
    if (protocol == "opcua" || protocol == "nd")
    {
        ASSERT_EQ(channels.getCount(), 2);
    }
    else if (protocol == "ns" || protocol == "lt")
    {
        ASSERT_EQ(channels.getCount(), 0);
    }
}

TEST_F(RegressionTestDevice, getDevices)
{
    ListPtr<IDevice> devices;
    ASSERT_NO_THROW(devices = device.getDevices());
    ASSERT_EQ(devices.getCount(), 0);
}

TEST_F(RegressionTestDevice, getAvailableDevices)
{
    ListPtr<IDeviceInfo> infos;
    ASSERT_NO_THROW(infos = device.getAvailableDevices());
    ASSERT_EQ(infos.getCount(), 0);
}

TEST_F(RegressionTestDevice, getAvailableDeviceTypes)
{
    DictPtr<IString, IDeviceType> types;
    ASSERT_NO_THROW(types = device.getAvailableDeviceTypes());
    ASSERT_EQ(types.getCount(), 0);
}

TEST_F(RegressionTestDevice, getFunctionBlocks)
{
    ListPtr<IFunctionBlock> functionBlocks;
    ASSERT_NO_THROW(functionBlocks = device.getFunctionBlocks());

    if (protocol == "opcua" || protocol == "nd")
    {
        ASSERT_EQ(functionBlocks.getCount(), 1);
    }
    else if (protocol == "ns" || protocol == "lt")
    {
        ASSERT_EQ(functionBlocks.getCount(), 0);
    }
}

TEST_F(RegressionTestDevice, getAvailableFunctionBlockTypes)
{
    DictPtr<IString, IFunctionBlockType> types;
    ASSERT_NO_THROW(types = device.getAvailableFunctionBlockTypes());
    if (protocol == "opcua" || protocol == "nd")
    {
        ASSERT_EQ(types.getCount(), 8);
    }
    else if (protocol == "ns" || protocol == "lt")
    {
        ASSERT_EQ(types.getCount(), 0);
    }
}

TEST_F(RegressionTestDevice, addFunctionBlockRemoveFunctionBlock)
{
    PROTOCOLS("opcua", "nd")

    FunctionBlockPtr fb;
    ASSERT_NO_THROW(fb = device.addFunctionBlock("ref_fb_module_trigger"));
    ASSERT_NO_THROW(device.removeFunctionBlock(fb));
}

TEST_F(RegressionTestDevice, saveConfigurationLoadConfiguration)
{
    StringPtr config;
    ASSERT_NO_THROW(config = device.saveConfiguration());

    PROTOCOLS("nd", "ns", "lt")  // TODO: why opcua doesn't work?

    ASSERT_NO_THROW(device.loadConfiguration(config));
}

TEST_F(RegressionTestDevice, getTicksSinceOrigin)
{
    ASSERT_NO_THROW(device.getTicksSinceOrigin());
}

TEST_F(RegressionTestDevice, getServerCapabilities)
{
    // We changed the names of Server Capabilities to PascalCase, hence this test

    PROTOCOLS("opcua", "nd")

    auto caps = device.getInfo().getServerCapabilities();

    if (protocol == "opcua")
        ASSERT_EQ(caps.getCount(), 1);
    else if (protocol == "nd")
        ASSERT_EQ(caps.getCount(), 4);

    std::set<std::string> allowed = {
        "OpenDAQNativeConfiguration", "OpenDAQOPCUAConfiguration", "OpenDAQNativeStreaming", "OpenDAQLTStreaming"};

    for (auto& cap : caps)
    {
        ASSERT_EQ(allowed.find(cap.getProtocolId()) != allowed.end(), true);
    }
}
