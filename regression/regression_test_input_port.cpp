#include <gtest/gtest.h>
#include <opendaq/opendaq.h>

using namespace daq;

class RegressionTestInputPort : public testing::TestWithParam<StringPtr>
{
private:
    ModuleManagerPtr moduleManager;
    ContextPtr context;
    FunctionBlockPtr fb;

protected:
    InstancePtr instance;
    InputPortPtr port;

    void SetUp() override
    {
        moduleManager = ModuleManager("");
        context = Context(nullptr, Logger(), TypeManager(), moduleManager);

        instance = InstanceCustom(context, "mock_instance");

        // TODO: should not rely on "ref_fb_module_trigger" being present
        fb = instance.addFunctionBlock("ref_fb_module_trigger");

        port = fb.getInputPorts()[0];
    }
};

TEST_P(RegressionTestInputPort, acceptsSignalConnectGetSignalGetConnectionDisconnect)
{
    auto device = instance.addDevice(GetParam());
    auto signal = device.getSignals()[0];
    Bool accepts;
    ASSERT_NO_THROW(accepts = port.acceptsSignal(signal));
    ASSERT_EQ(accepts, True);
    ASSERT_NO_THROW(port.connect(signal));
    ASSERT_NO_THROW(port.getSignal());
    ASSERT_NO_THROW(port.getConnection());
    ASSERT_NO_THROW(port.disconnect());
}

TEST_P(RegressionTestInputPort, getRequiresSignal)
{
    ASSERT_NO_THROW(port.getRequiresSignal());
}

// TODO: "daq.opcua://127.0.0.1"?
INSTANTIATE_TEST_SUITE_P(InputPort,
                         RegressionTestInputPort,
                         testing::Values("daq.opcua://127.0.0.1", "daq.ns://127.0.0.1", "daq.lt://127.0.0.1"));
