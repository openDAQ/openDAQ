#include "setup_regression.h"

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
        PROTOCOLS("opcua", "nd")

        moduleManager = ModuleManager("");
        context = Context(nullptr, Logger(), TypeManager(), moduleManager, nullptr, nullptr, nullptr);

        instance = InstanceCustom(context, "mock_instance");

        device = instance.addDevice(connectionString);

        channel = instance.getChannelsRecursive()[0];
    }
};

TEST_F(RegressionTestChannel, getVisibleProperties)
{
    PROTOCOLS("opcua", "nd")

    ListPtr<IProperty> properties;
    ASSERT_NO_THROW(properties = channel.getVisibleProperties());
    if (protocol == "opcua")
    {
        ASSERT_EQ(properties.getCount(), 11);
    }
    else if (protocol == "nd")
    {
        ASSERT_EQ(properties.getCount(), 10);
    }
}

TEST_F(RegressionTestChannel, setPropertyValueGetPropertyValue)
{
    PROTOCOLS("nd")

    ASSERT_NO_THROW(channel.setPropertyValue("NoiseAmplitude", 0.2));
    FloatPtr property;
    ASSERT_NO_THROW(property = channel.getPropertyValue("NoiseAmplitude"));
    ASSERT_FLOAT_EQ(property, 0.2f);
}
