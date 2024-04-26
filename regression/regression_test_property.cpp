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

TEST_P(RegressionTestProperty, getDefaultValue)
{
    ASSERT_NO_THROW(property.getDefaultValue());
}

TEST_P(RegressionTestProperty, getSuggestedValues)
{
    ASSERT_NO_THROW(property.getSuggestedValues());
}

TEST_P(RegressionTestProperty, getVisible)
{
    ASSERT_NO_THROW(property.getVisible());
}

TEST_P(RegressionTestProperty, getReadOnly)
{
    ASSERT_NO_THROW(property.getReadOnly());
}

TEST_P(RegressionTestProperty, getSelectionValues)
{
    ASSERT_NO_THROW(property.getSelectionValues());
}

TEST_P(RegressionTestProperty, getReferencedProperty)
{
    ASSERT_NO_THROW(property.getReferencedProperty());
}

TEST_P(RegressionTestProperty, getIsReferenced)
{
    ASSERT_NO_THROW(property.getIsReferenced());
}

TEST_P(RegressionTestProperty, getValidator)
{
    ASSERT_NO_THROW(property.getValidator());
}

TEST_P(RegressionTestProperty, getCoercer)
{
    ASSERT_NO_THROW(property.getCoercer());
}

TEST_P(RegressionTestProperty, getCallableInfo)
{
    ASSERT_NO_THROW(property.getCallableInfo());
}

// TODO enable
TEST_P(RegressionTestProperty, DISABLED_getStructType)
{
    ASSERT_NO_THROW(property.getStructType());
}

TEST_P(RegressionTestProperty, getOnPropertyValueWrite)
{
    ASSERT_NO_THROW(property.getOnPropertyValueWrite());
}

TEST_P(RegressionTestProperty, getOnPropertyValueRead)
{
    ASSERT_NO_THROW(property.getOnPropertyValueRead());
}

INSTANTIATE_TEST_SUITE_P(Property,
                         RegressionTestProperty,
                         testing::Values("daq.opcua://127.0.0.1", "daq.ns://127.0.0.1", "daq.lt://127.0.0.1"));
