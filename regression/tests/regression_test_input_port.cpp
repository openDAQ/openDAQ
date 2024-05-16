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

        device = instance.addDevice(connectionString);

        fb = device.getFunctionBlocks()[0];

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
