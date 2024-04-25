#include <gtest/gtest.h>
#include <opendaq/opendaq.h>

using namespace daq;

class RegressionTestSignal : public testing::TestWithParam<StringPtr>
{
private:
    ModuleManagerPtr moduleManager;
    ContextPtr context;
    InstancePtr instance;

protected:
    SignalPtr signal;

    void SetUp() override
    {
        moduleManager = ModuleManager("");
        context = Context(nullptr, Logger(), TypeManager(), moduleManager);

        instance = InstanceCustom(context, "mock_instance");

        auto device = instance.addDevice(GetParam());

        signal = device.getSignals()[0];
    }
};

TEST_P(RegressionTestSignal, getInfo)
{
    ASSERT_NO_THROW(signal.getPublic());
}

INSTANTIATE_TEST_SUITE_P(Signal,
                         RegressionTestSignal,
                         testing::Values("daq.opcua://127.0.0.1", "daq.ns://127.0.0.1", "daq.lt://127.0.0.1"));
