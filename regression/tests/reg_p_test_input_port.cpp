#include "setup_regression.h"
#include <testutils/testutils.h>

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
        PROTOCOLS("opcua", "nd")

        moduleManager = ModuleManager("");
        context = Context(nullptr, Logger(), TypeManager(), moduleManager, nullptr, nullptr, nullptr);

        instance = InstanceCustom(context, "mock_instance");

        device = instance.addDevice(connectionString);

        fb = device.getFunctionBlocks()[0];

        port = fb.getInputPorts()[0];
    }
};

TEST_F(RegressionTestInputPort, acceptsSignalConnectGetSignalGetConnectionDisconnect)
{
    PROTOCOLS("nd")

    auto signal1 = device.getSignalsRecursive()[0];
    ASSERT_THROW_MSG(port.acceptsSignal(signal1), DaqException, "Operation not supported by the protocol version currently in use");
    ASSERT_NO_THROW(port.connect(signal1));
    SignalPtr signal2;
    ASSERT_NO_THROW(signal2 = port.getSignal());
    ASSERT_EQ(signal1, signal2);
    ConnectionPtr connection;
    ASSERT_NO_THROW(connection = port.getConnection());
    ASSERT_NE(connection, nullptr);
    ASSERT_NO_THROW(port.disconnect());
    ASSERT_EQ(port.getSignal(), nullptr);
}

TEST_F(RegressionTestInputPort, getRequiresSignal)
{
    PROTOCOLS("opcua", "nd")

    Bool reqs;
    ASSERT_NO_THROW(reqs = port.getRequiresSignal());
    ASSERT_EQ(reqs, True);
}
