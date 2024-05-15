#include <gtest/gtest.h>
#include "get_protocol.h"

using namespace daq;

class RegressionTestInputPort : public testing::Test
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
        instance.setRootDevice(connectionString);
        device = instance.getRootDevice();

        // TODO: should not rely on "ref_fb_module_trigger" being present
        fb = device.addFunctionBlock("ref_fb_module_trigger");
        // TODO: remove function block???
        // TODO: add function block once in simulator?

        port = fb.getInputPorts()[0];
    }
};

TEST_F(RegressionTestInputPort, acceptsSignalConnectGetSignalGetConnectionDisconnect)
{
    if (protocol == "opcua")
    {
        return;
    }

    auto signal = device.getSignalsRecursive()[0];
    Bool accepts;
    ASSERT_NO_THROW(accepts = port.acceptsSignal(signal));
    ASSERT_EQ(accepts, True);
    ASSERT_NO_THROW(port.connect(signal));
    ASSERT_NO_THROW(port.getSignal());
    ASSERT_NO_THROW(port.getConnection());
    ASSERT_NO_THROW(port.disconnect());
}

TEST_F(RegressionTestInputPort, getRequiresSignal)
{
    ASSERT_NO_THROW(port.getRequiresSignal());
}
