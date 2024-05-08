#include <gtest/gtest.h>
#include <opendaq/opendaq.h>

using namespace daq;

class RegressionTestInputPort : public testing::TestWithParam<StringPtr>
{
private:
    ModuleManagerPtr moduleManager;
    InstancePtr instance;
    ContextPtr context;
    FunctionBlockPtr fb;

protected:
    DevicePtr device;
    InputPortPtr port;

    void SetUp() override
    {
        moduleManager = ModuleManager("");
        context = Context(nullptr, Logger(), TypeManager(), moduleManager);

        instance = InstanceCustom(context, "mock_instance");

        // TODO: to be able to get input port from functin block
        instance.setRootDevice(GetParam());
        device = instance.getRootDevice();

        // TODO: should not rely on "ref_fb_module_trigger" being present
        fb = device.addFunctionBlock("ref_fb_module_trigger");
        // TODO: remove function block???
        // TODO: add function block once in simulator?

        port = fb.getInputPorts()[0];
    }
};

TEST_P(RegressionTestInputPort, acceptsSignalConnectGetSignalGetConnectionDisconnect)
{
    // TODO: fails here for OPC UA
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

// TODO: ???
INSTANTIATE_TEST_SUITE_P(InputPort,
                         RegressionTestInputPort,
                         testing::Values("daq.opcua://127.0.0.1", "daq.ns://127.0.0.1", "daq.lt://127.0.0.1"));
