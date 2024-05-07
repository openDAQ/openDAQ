#include <gtest/gtest.h>
#include <opendaq/opendaq.h>

using namespace daq;

class RegressionTestChannel : public testing::TestWithParam<StringPtr>
{
private:
    ModuleManagerPtr moduleManager;
    ContextPtr context;
    InstancePtr instance;
    DevicePtr device;

protected:
    ChannelPtr channel;

    void SetUp() override
    {
        moduleManager = ModuleManager("");
        context = Context(nullptr, Logger(), TypeManager(), moduleManager);

        instance = InstanceCustom(context, "mock_instance");

        device = instance.addDevice(GetParam());

        channel = instance.getChannelsRecursive()[0];
    }
};

TEST_P(RegressionTestChannel, getVisibleProperties)
{
    ListPtr<IProperty> properties;
    ASSERT_NO_THROW(properties = channel.getVisibleProperties());
    ASSERT_EQ(properties.getCount(), 11);
}

TEST_P(RegressionTestChannel, setPropertyValueGetPropertyValue)
{
    ASSERT_NO_THROW(channel.setPropertyValue("NoiseAmplitude", 0.2));
    FloatPtr property;
    ASSERT_NO_THROW(property = channel.getPropertyValue("NoiseAmplitude"));
    ASSERT_FLOAT_EQ(property, 0.2);
}

// TODO: ??? + no need for TEST_P if only OPC UA
INSTANTIATE_TEST_SUITE_P(Channel,
                         RegressionTestChannel,
                         testing::Values("daq.opcua://127.0.0.1" /*, "daq.ns://127.0.0.1", "daq.lt://127.0.0.1"*/));
