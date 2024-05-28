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
        if (protocol == "ns" || protocol == "lt")
        {
            return;
        }

        moduleManager = ModuleManager("");
        context = Context(nullptr, Logger(), TypeManager(), moduleManager);

        instance = InstanceCustom(context, "mock_instance");

        device = instance.addDevice(connectionString);

        channel = instance.getChannelsRecursive()[0];
    }
};

TEST_F(RegressionTestChannel, getVisibleProperties)
{
    if (protocol == "ns" || protocol == "lt")
    {
        return;
    }

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
    if (protocol == "opcua")
    {
        return;
    }
    else if (protocol == "nd")
    {
        ASSERT_NO_THROW(channel.setPropertyValue("NoiseAmplitude", 0.2));
        FloatPtr property;
        ASSERT_NO_THROW(property = channel.getPropertyValue("NoiseAmplitude"));
        ASSERT_FLOAT_EQ(property, 0.2);
    }
    else if (protocol == "ns" || protocol == "lt")
    {
        return;
    }
}
