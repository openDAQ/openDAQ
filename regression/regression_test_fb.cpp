#include <gtest/gtest.h>
#include <opendaq/opendaq.h>

using namespace daq;

class RegressionTestFunctionBlock : public testing::TestWithParam<StringPtr>
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
        moduleManager = ModuleManager("");
        context = Context(nullptr, Logger(), TypeManager(), moduleManager);

        instance = InstanceCustom(context, "mock_instance");

        device = instance.addDevice(GetParam());

        // TODO: should not rely on "ref_fb_module_trigger" being present?
        fb = device.addFunctionBlock("ref_fb_module_trigger");
    }
};

TEST_P(RegressionTestFunctionBlock, getFunctionBlockType)
{
    FunctionBlockTypePtr type;
    ASSERT_NO_THROW(type = fb.getFunctionBlockType());
    ASSERT_EQ(type.getId(), "ref_fb_module_trigger");
    ASSERT_EQ(type.getName(), "Trigger");
    ASSERT_EQ(type.getDescription(), "Trigger");
}

TEST_P(RegressionTestFunctionBlock, getInputPorts)
{
    ListPtr<IInputPort> ports;
    ASSERT_NO_THROW(ports = fb.getInputPorts());
    ASSERT_EQ(ports.getCount(), 1);
}

TEST_P(RegressionTestFunctionBlock, getSignals)
{
    ListPtr<ISignal> signals;
    ASSERT_NO_THROW(signals = fb.getSignals());
    ASSERT_EQ(signals.getCount(), 2);
}

TEST_P(RegressionTestFunctionBlock, getSignalsRecursive)
{
    ListPtr<ISignal> signals;
    ASSERT_NO_THROW(signals = fb.getSignalsRecursive());
    ASSERT_EQ(signals.getCount(), 2);
}

TEST_P(RegressionTestFunctionBlock, getStatusSignal)
{
    SignalPtr status;
    ASSERT_NO_THROW(status = fb.getStatusSignal());
    ASSERT_EQ(status, nullptr);
}

TEST_P(RegressionTestFunctionBlock, getFunctionBlocks)
{
    ListPtr<IFunctionBlock> fbs;
    ASSERT_NO_THROW(fbs = fb.getFunctionBlocks());
    ASSERT_EQ(fbs.getCount(), 0);
}

// TODO: ??? + no need for TEST_P if only OPC UA
INSTANTIATE_TEST_SUITE_P(FunctionBlock,
                         RegressionTestFunctionBlock,
                         testing::Values("daq.opcua://127.0.0.1" /*, "daq.ns://127.0.0.1", "daq.lt://127.0.0.1"*/));
