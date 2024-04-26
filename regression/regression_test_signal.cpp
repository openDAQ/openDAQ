#include <gtest/gtest.h>
#include <opendaq/opendaq.h>

using namespace daq;

class RegressionTestSignal : public testing::TestWithParam<StringPtr>
{
private:
    ModuleManagerPtr moduleManager;
    ContextPtr context;
    InstancePtr instance;
    DevicePtr device;

protected:
    SignalPtr signal;

    void SetUp() override
    {
        moduleManager = ModuleManager("");
        context = Context(nullptr, Logger(), TypeManager(), moduleManager);

        instance = InstanceCustom(context, "mock_instance");

        device = instance.addDevice(GetParam());

        signal = device.getSignals()[0];
    }
};

TEST_P(RegressionTestSignal, getPublic)
{
    ASSERT_NO_THROW(signal.getPublic());
}

TEST_P(RegressionTestSignal, setPublic)
{
    ASSERT_NO_THROW(signal.setPublic(True));
}

TEST_P(RegressionTestSignal, getDescriptor)
{
    ASSERT_NO_THROW(signal.getDescriptor());
}

TEST_P(RegressionTestSignal, getDomainSignal)
{
    ASSERT_NO_THROW(signal.getDomainSignal());
}

TEST_P(RegressionTestSignal, getRelatedSignals)
{
    ASSERT_NO_THROW(signal.getRelatedSignals());
}

TEST_P(RegressionTestSignal, getConnections)
{
    ASSERT_NO_THROW(signal.getConnections());
}

TEST_P(RegressionTestSignal, getStreamed)
{
    ASSERT_NO_THROW(signal.getStreamed());
}

TEST_P(RegressionTestSignal, setStreamed)
{
    ASSERT_NO_THROW(signal.setStreamed(True));
}

TEST_P(RegressionTestSignal, getLastValue)
{
    ASSERT_NO_THROW(signal.getLastValue());
}

// TODO ???
INSTANTIATE_TEST_SUITE_P(Signal,
                         RegressionTestSignal,
                         testing::Values(/*"daq.opcua://127.0.0.1",*/ "daq.ns://127.0.0.1", "daq.lt://127.0.0.1"));
