#include "setup_regression.h"

class RegressionTestProperty : public testing::Test
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

        device = instance.addDevice(connectionString);

        property = device.getProperty("UserName");
    }
};

TEST_F(RegressionTestProperty, getValueType)
{
    CoreType type;
    ASSERT_NO_THROW(type = property.getValueType());
    ASSERT_EQ(type, CoreType::ctString);
}

TEST_F(RegressionTestProperty, getKeyType)
{
    CoreType type;
    ASSERT_NO_THROW(type = property.getKeyType());
    ASSERT_EQ(type, CoreType::ctUndefined);
}

TEST_F(RegressionTestProperty, getItemType)
{
    CoreType type;
    ASSERT_NO_THROW(type = property.getItemType());
    ASSERT_EQ(type, CoreType::ctUndefined);
}

TEST_F(RegressionTestProperty, getName)
{
    StringPtr name;
    ASSERT_NO_THROW(name = property.getName());
    ASSERT_EQ(name, "UserName");
}

TEST_F(RegressionTestProperty, getDescription)
{
    StringPtr description;
    ASSERT_NO_THROW(description = property.getDescription());
    ASSERT_EQ(description, nullptr);
}

TEST_F(RegressionTestProperty, getUnit)
{
    UnitPtr unit;
    ASSERT_NO_THROW(unit = property.getUnit());
    ASSERT_EQ(unit, nullptr);
}

TEST_F(RegressionTestProperty, getMinValue)
{
    NumberPtr value;
    ASSERT_NO_THROW(value = property.getMinValue());
    ASSERT_EQ(value, nullptr);
}

TEST_F(RegressionTestProperty, getMaxValue)
{
    NumberPtr value;
    ASSERT_NO_THROW(value = property.getMaxValue());
    ASSERT_EQ(value, nullptr);
}

TEST_F(RegressionTestProperty, getDefaultValue)
{
    BaseObjectPtr value;
    ASSERT_NO_THROW(value = property.getDefaultValue());
    ASSERT_EQ(value, "");
}

TEST_F(RegressionTestProperty, getSuggestedValues)
{
    ListPtr<IBaseObject> values;
    ASSERT_NO_THROW(values = property.getSuggestedValues());
    ASSERT_EQ(values, nullptr);
}

TEST_F(RegressionTestProperty, getVisible)
{
    Bool visible;
    ASSERT_NO_THROW(visible = property.getVisible());
    ASSERT_EQ(visible, True);
}

TEST_F(RegressionTestProperty, getReadOnly)
{
    Bool readOnly;
    ASSERT_NO_THROW(readOnly = property.getReadOnly());
    ASSERT_EQ(readOnly, False);
}

TEST_F(RegressionTestProperty, getSelectionValues)
{
    BaseObjectPtr values;
    ASSERT_NO_THROW(values = property.getSelectionValues());
    ASSERT_EQ(values, nullptr);
}

TEST_F(RegressionTestProperty, getReferencedProperty)
{
    PropertyPtr prop;
    ASSERT_NO_THROW(prop = property.getReferencedProperty());
    ASSERT_EQ(prop, nullptr);
}

TEST_F(RegressionTestProperty, getIsReferenced)
{
    Bool isReferenced;
    ASSERT_NO_THROW(isReferenced = property.getIsReferenced());
    ASSERT_EQ(isReferenced, False);
}

TEST_F(RegressionTestProperty, getValidator)
{
    ValidatorPtr validator;
    ASSERT_NO_THROW(validator = property.getValidator());
    ASSERT_EQ(validator, nullptr);
}

TEST_F(RegressionTestProperty, getCoercer)
{
    CoercerPtr coercer;
    ASSERT_NO_THROW(coercer = property.getCoercer());
    ASSERT_EQ(coercer, nullptr);
}

TEST_F(RegressionTestProperty, getCallableInfo)
{
    CallableInfoPtr info;
    ASSERT_NO_THROW(info = property.getCallableInfo());
    ASSERT_EQ(info, nullptr);
}

// TODO: ???
TEST_F(RegressionTestProperty, DISABLED_getStructType)
{
    ASSERT_NO_THROW(property.getStructType());
}

TEST_F(RegressionTestProperty, getOnPropertyValueWrite)
{
    auto event = property.getOnPropertyValueWrite();
    ASSERT_NE(event, nullptr);
}

TEST_F(RegressionTestProperty, getOnPropertyValueRead)
{
    auto event = property.getOnPropertyValueRead();
    ASSERT_NE(event, nullptr);
}
