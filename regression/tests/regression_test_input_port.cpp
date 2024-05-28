#include "setup_regression.h"

class RegressionTestInputPort : public RegressionTest
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
        if (protocol == "ns" || protocol == "lt")
        {
            return;
        }

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
    else if (protocol == "nd")
    {
        auto signal = device.getSignalsRecursive()[0];
        Bool accepts;
        ASSERT_NO_THROW(accepts = port.acceptsSignal(signal));
        ASSERT_EQ(accepts, True);
        ASSERT_NO_THROW(port.connect(signal));
        ASSERT_NO_THROW(port.getSignal());
        ASSERT_NO_THROW(port.getConnection());
        ASSERT_NO_THROW(port.disconnect());
    }
    else if (protocol == "ns" || protocol == "lt")
    {
        return;
    }
}

TEST_F(RegressionTestInputPort, getRequiresSignal)
{
    if (protocol == "ns" || protocol == "lt")
    {
        return;
    }

    ASSERT_NO_THROW(port.getRequiresSignal());
}
