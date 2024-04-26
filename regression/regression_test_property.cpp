#include <gtest/gtest.h>
#include <opendaq/opendaq.h>

using namespace daq;

class RegressionTestProperty : public testing::TestWithParam<StringPtr>
{
private:
    ModuleManagerPtr moduleManager;
    ContextPtr context;
    InstancePtr instance;
    DevicePtr device;

protected:
    PropertyPtr property;

    void SetUp() override
    {
        moduleManager = ModuleManager("");
        context = Context(nullptr, Logger(), TypeManager(), moduleManager);

        instance = InstanceCustom(context, "mock_instance");

        device = instance.addDevice(GetParam());

        property = device.getAllProperties()[0];
    }
};

TEST_P(RegressionTestProperty, getValueType)
{
    ASSERT_NO_THROW(property.getValueType());
}

TEST_P(RegressionTestProperty, getKeyType)
{
    ASSERT_NO_THROW(property.getKeyType());
}

TEST_P(RegressionTestProperty, getItemType)
{
    ASSERT_NO_THROW(property.getItemType());
}

TEST_P(RegressionTestProperty, getName)
{
    ASSERT_NO_THROW(property.getName());
}

TEST_P(RegressionTestProperty, getDescription)
{
    ASSERT_NO_THROW(property.getDescription());
}

TEST_P(RegressionTestProperty, getUnit)
{
    ASSERT_NO_THROW(property.getUnit());
}

TEST_P(RegressionTestProperty, getMinValue)
{
    ASSERT_NO_THROW(property.getMinValue());
}

TEST_P(RegressionTestProperty, getMaxValue)
{
    ASSERT_NO_THROW(property.getMaxValue());
}
/*
TEST_P(RegressionTestProperty, )
{
    ASSERT_NO_THROW(property.());
}
*/
INSTANTIATE_TEST_SUITE_P(Property,
                         RegressionTestProperty,
                         testing::Values("daq.opcua://127.0.0.1", "daq.ns://127.0.0.1", "daq.lt://127.0.0.1"));
