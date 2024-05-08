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

        property = device.getProperty("UserName");
    }
};

TEST_P(RegressionTestProperty, getValueType)
{
    CoreType type;
    ASSERT_NO_THROW(type = property.getValueType());
    ASSERT_EQ(type, CoreType::ctString);
}

TEST_P(RegressionTestProperty, getKeyType)
{
    CoreType type;
    ASSERT_NO_THROW(type = property.getKeyType());
    ASSERT_EQ(type, CoreType::ctUndefined);
}

TEST_P(RegressionTestProperty, getItemType)
{
    CoreType type;
    ASSERT_NO_THROW(type = property.getItemType());
    ASSERT_EQ(type, CoreType::ctUndefined);
}

TEST_P(RegressionTestProperty, getName)
{
    StringPtr name;
    ASSERT_NO_THROW(name = property.getName());
    ASSERT_EQ(name, "UserName");
}

TEST_P(RegressionTestProperty, getDescription)
{
    StringPtr description;
    ASSERT_NO_THROW(description = property.getDescription());
    ASSERT_EQ(description, nullptr);
}

TEST_P(RegressionTestProperty, getUnit)
{
    UnitPtr unit;
    ASSERT_NO_THROW(unit = property.getUnit());
    ASSERT_EQ(unit, nullptr);
}

TEST_P(RegressionTestProperty, getMinValue)
{
    NumberPtr value;
    ASSERT_NO_THROW(value = property.getMinValue());
    ASSERT_EQ(value, nullptr);
}

TEST_P(RegressionTestProperty, getMaxValue)
{
    NumberPtr value;
    ASSERT_NO_THROW(value = property.getMaxValue());
    ASSERT_EQ(value, nullptr);
}

TEST_P(RegressionTestProperty, getDefaultValue)
{
    BaseObjectPtr value;
    ASSERT_NO_THROW(value = property.getDefaultValue());
    ASSERT_EQ(value, "");
}

TEST_P(RegressionTestProperty, getSuggestedValues)
{
    ListPtr<IBaseObject> values;
    ASSERT_NO_THROW(values = property.getSuggestedValues());
    ASSERT_EQ(values, nullptr);
}

TEST_P(RegressionTestProperty, getVisible)
{
    Bool visible;
    ASSERT_NO_THROW(visible = property.getVisible());
    ASSERT_EQ(visible, True);
}

TEST_P(RegressionTestProperty, getReadOnly)
{
    Bool readOnly;
    ASSERT_NO_THROW(readOnly = property.getReadOnly());
    ASSERT_EQ(readOnly, False);
}

TEST_P(RegressionTestProperty, getSelectionValues)
{
    BaseObjectPtr values;
    ASSERT_NO_THROW(values = property.getSelectionValues());
    ASSERT_EQ(values, nullptr);
}

TEST_P(RegressionTestProperty, getReferencedProperty)
{
    PropertyPtr prop;
    ASSERT_NO_THROW(prop = property.getReferencedProperty());
    ASSERT_EQ(prop, nullptr);
}

TEST_P(RegressionTestProperty, getIsReferenced)
{
    Bool isReferenced;
    ASSERT_NO_THROW(isReferenced = property.getIsReferenced());
    ASSERT_EQ(isReferenced, False);
}

TEST_P(RegressionTestProperty, getValidator)
{
    ValidatorPtr validator;
    ASSERT_NO_THROW(validator = property.getValidator());
    ASSERT_EQ(validator, nullptr);
}

TEST_P(RegressionTestProperty, getCoercer)
{
    CoercerPtr coercer;
    ASSERT_NO_THROW(coercer = property.getCoercer());
    ASSERT_EQ(coercer, nullptr);
}

TEST_P(RegressionTestProperty, getCallableInfo)
{
    CallableInfoPtr info;
    ASSERT_NO_THROW(info = property.getCallableInfo());
    ASSERT_EQ(info, nullptr);
}

// TODO: ???
TEST_P(RegressionTestProperty, DISABLED_getStructType)
{
    ASSERT_NO_THROW(property.getStructType());
}

TEST_P(RegressionTestProperty, getOnPropertyValueWrite)
{
    auto event = property.getOnPropertyValueWrite();
    ASSERT_NE(event, nullptr);
}

TEST_P(RegressionTestProperty, getOnPropertyValueRead)
{
    auto event = property.getOnPropertyValueRead();
    ASSERT_NE(event, nullptr);
}

INSTANTIATE_TEST_SUITE_P(Property,
                         RegressionTestProperty,
                         testing::Values("daq.opcua://127.0.0.1", "daq.ns://127.0.0.1", "daq.lt://127.0.0.1"));
