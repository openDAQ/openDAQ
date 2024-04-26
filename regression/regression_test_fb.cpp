#include <gtest/gtest.h>
#include <opendaq/opendaq.h>

using namespace daq;

class RegressionTestFunctionBlock : public testing::TestWithParam<StringPtr>
{
private:
    ModuleManagerPtr moduleManager;
    ContextPtr context;
    InstancePtr instance;

protected:
    FunctionBlockPtr fb;

    void SetUp() override
    {
        moduleManager = ModuleManager("");
        context = Context(nullptr, Logger(), TypeManager(), moduleManager);

        instance = InstanceCustom(context, "mock_instance");

        // TODO: should not rely on "ref_fb_module_trigger" being present
        fb = instance.addFunctionBlock("ref_fb_module_trigger");
    }
};

TEST_P(RegressionTestFunctionBlock, getFunctionBlockType)
{
    ASSERT_NO_THROW(fb.getFunctionBlockType());
}

TEST_P(RegressionTestFunctionBlock, getInputPorts)
{
    ASSERT_NO_THROW(fb.getInputPorts());
}

TEST_P(RegressionTestFunctionBlock, getSignals)
{
    ASSERT_NO_THROW(fb.getSignals());
}

TEST_P(RegressionTestFunctionBlock, getSignalsRecursive)
{
    ASSERT_NO_THROW(fb.getSignalsRecursive());
}

TEST_P(RegressionTestFunctionBlock, getStatusSignal)
{
    ASSERT_NO_THROW(fb.getStatusSignal());
}

TEST_P(RegressionTestFunctionBlock, getFunctionBlocks)
{
    ASSERT_NO_THROW(fb.getFunctionBlocks());
}

INSTANTIATE_TEST_SUITE_P(FunctionBlock,
                         RegressionTestFunctionBlock,
                         testing::Values("daq.opcua://127.0.0.1", "daq.ns://127.0.0.1", "daq.lt://127.0.0.1"));
