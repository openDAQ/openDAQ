#include <gtest/gtest.h>
#include "get_protocol.h"

using namespace daq;

class RegressionTestChannel : public testing::Test
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

        device = instance.addDevice(connectionString);

        channel = instance.getChannelsRecursive()[0];
    }
};

TEST_F(RegressionTestChannel, getVisibleProperties)
{
    ListPtr<IProperty> properties;
    ASSERT_NO_THROW(properties = channel.getVisibleProperties());
    ASSERT_GT(properties.getCount(), 0);
}

TEST_F(RegressionTestChannel, setPropertyValueGetPropertyValue)
{
    ASSERT_NO_THROW(channel.setPropertyValue("NoiseAmplitude", 0.2));
    FloatPtr property;
    ASSERT_NO_THROW(property = channel.getPropertyValue("NoiseAmplitude"));
    ASSERT_FLOAT_EQ(property, 0.2);
}
