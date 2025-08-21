#include "setup_regression.h"

class RegressionTestFunctionBlock : public testing::Test
{
private:
    ModuleManagerPtr moduleManager;
    ContextPtr context;
    InstancePtr instance;
    DevicePtr device;

protected:
    FunctionBlockPtr fb;

    void SetUp() override
    {
        PROTOCOLS("opcua", "nd")

        moduleManager = ModuleManager("");
        context = Context(nullptr, Logger(), TypeManager(), moduleManager, nullptr, nullptr, nullptr);

        instance = InstanceCustom(context, "mock_instance");

        device = instance.addDevice(connectionString);

        fb = device.getFunctionBlocks()[0];
    }
};

TEST_F(RegressionTestFunctionBlock, getFunctionBlockType)
{
    PROTOCOLS("opcua", "nd")

    FunctionBlockTypePtr type;
    ASSERT_NO_THROW(type = fb.getFunctionBlockType());
    ASSERT_EQ(type.getId(), "ref_fb_module_trigger");
    if (protocol == "opcua")
    {
        ASSERT_EQ(type.getName(), "Trigger");
        ASSERT_EQ(type.getDescription(), "Trigger");
    }
    else if (protocol == "nd")
    {
        ASSERT_EQ(type.getName(), "ref_fb_module_trigger");
        ASSERT_EQ(type.getDescription(), "");
    }
}

TEST_F(RegressionTestFunctionBlock, getInputPorts)
{
    PROTOCOLS("opcua", "nd")

    ListPtr<IInputPort> ports;
    ASSERT_NO_THROW(ports = fb.getInputPorts());
    ASSERT_EQ(ports.getCount(), 1);
}

TEST_F(RegressionTestFunctionBlock, getSignals)
{
    PROTOCOLS("opcua", "nd")

    ListPtr<ISignal> signals;
    ASSERT_NO_THROW(signals = fb.getSignals());
    ASSERT_EQ(signals.getCount(), 2);
}

TEST_F(RegressionTestFunctionBlock, getSignalsRecursive)
{
    PROTOCOLS("opcua", "nd")

    ListPtr<ISignal> signals;
    ASSERT_NO_THROW(signals = fb.getSignalsRecursive());
    ASSERT_EQ(signals.getCount(), 2);
}

TEST_F(RegressionTestFunctionBlock, getStatusSignal)
{
    PROTOCOLS("opcua", "nd")

    SignalPtr status;
    ASSERT_NO_THROW(status = fb.getStatusSignal());
    ASSERT_EQ(status, nullptr);
}

TEST_F(RegressionTestFunctionBlock, getFunctionBlocks)
{
    PROTOCOLS("opcua", "nd")

    ListPtr<IFunctionBlock> fbs;
    ASSERT_NO_THROW(fbs = fb.getFunctionBlocks());
    ASSERT_EQ(fbs.getCount(), 0);
}
