#include "setup_regression.h"

class RegressionTestProperty : public testing::Test
{
private:
    ModuleManagerPtr moduleManager;
    ContextPtr context;
    InstancePtr instance;

protected:
    DevicePtr device;

    void SetUp() override
    {
        PROTOCOLS("nd")

        moduleManager = ModuleManager("");
        context = Context(nullptr, Logger(), TypeManager(), moduleManager, nullptr, nullptr, nullptr);

        instance = InstanceCustom(context, "mock_instance");

        device = instance.addDevice(connectionString);
    }
};

TEST_F(RegressionTestProperty, getValueType)
{
    PROTOCOLS("nd")

    auto prop = device.getProperty("TestString");
    CoreType type;
    ASSERT_NO_THROW(type = prop.getValueType());
    ASSERT_EQ(type, CoreType::ctString);
}

TEST_F(RegressionTestProperty, getKeyType)
{
    PROTOCOLS("nd")

    auto prop = device.getProperty("TestDict");
    CoreType type;
    ASSERT_NO_THROW(type = prop.getKeyType());
    ASSERT_EQ(type, CoreType::ctString);
}

TEST_F(RegressionTestProperty, getItemType)
{
    PROTOCOLS("nd")

    auto prop = device.getProperty("TestDict");
    CoreType type;
    ASSERT_NO_THROW(type = prop.getItemType());
    ASSERT_EQ(type, CoreType::ctString);
}

TEST_F(RegressionTestProperty, getName)
{
    PROTOCOLS("nd")

    auto prop = device.getProperty("TestString");
    StringPtr name;
    ASSERT_NO_THROW(name = prop.getName());
    ASSERT_EQ(name, "TestString");
}

TEST_F(RegressionTestProperty, getDescription)
{
    PROTOCOLS("nd")

    auto prop = device.getProperty("TestString");
    StringPtr description;
    ASSERT_NO_THROW(description = prop.getDescription());
    ASSERT_EQ(description, "TestDescription");
}

TEST_F(RegressionTestProperty, getUnit)
{
    PROTOCOLS("nd")

    auto prop = device.getProperty("TestInt");
    UnitPtr unit;
    ASSERT_NO_THROW(unit = prop.getUnit());
    ASSERT_EQ(unit, Unit("TestUnit", -1, "TestName", "TestQunatity"));
}

TEST_F(RegressionTestProperty, getMinValue)
{
    PROTOCOLS("nd")

    auto prop = device.getProperty("TestInt");
    NumberPtr value;
    ASSERT_NO_THROW(value = prop.getMinValue());
    ASSERT_EQ(value, -666);
}

TEST_F(RegressionTestProperty, getMaxValue)
{
    PROTOCOLS("nd")

    auto prop = device.getProperty("TestInt");
    NumberPtr value;
    ASSERT_NO_THROW(value = prop.getMaxValue());
    ASSERT_EQ(value, 777);
}

TEST_F(RegressionTestProperty, getDefaultValue)
{
    PROTOCOLS("nd")

    auto prop = device.getProperty("TestInt");
    NumberPtr value;
    ASSERT_NO_THROW(value = prop.getDefaultValue());
    ASSERT_EQ(value, 42);
}

TEST_F(RegressionTestProperty, getSuggestedValues)
{
    PROTOCOLS("nd")

    auto prop = device.getProperty("TestInt");
    ListPtr<INumber> values;
    ASSERT_NO_THROW(values = prop.getSuggestedValues());
    auto expected = List<Int>();
    expected.pushBack(1);
    expected.pushBack(2);
    expected.pushBack(3);
    ASSERT_EQ(values, expected);
}

TEST_F(RegressionTestProperty, getVisible)
{
    PROTOCOLS("nd")

    auto prop = device.getProperty("TestString");
    Bool visible;
    ASSERT_NO_THROW(visible = prop.getVisible());
    ASSERT_EQ(visible, True);
}

TEST_F(RegressionTestProperty, getReadOnly)
{
    PROTOCOLS("nd")

    auto prop = device.getProperty("TestString");
    Bool readOnly;
    ASSERT_NO_THROW(readOnly = prop.getReadOnly());
    ASSERT_EQ(readOnly, False);
}

TEST_F(RegressionTestProperty, getSelectionValues)
{
    PROTOCOLS("nd")

    auto prop = device.getProperty("TestSelection");
    BaseObjectPtr values;
    ASSERT_NO_THROW(values = prop.getSelectionValues());
    auto expected = List<IInteger>();
    expected.pushBack(1);
    expected.pushBack(2);
    expected.pushBack(3);
    ASSERT_EQ(values, expected);
}

TEST_F(RegressionTestProperty, getReferencedProperty)
{
    PROTOCOLS("nd")

    auto prop = device.getProperty("TestReference");
    ASSERT_NO_THROW(prop = prop.getReferencedProperty());
    ASSERT_EQ(prop.getDefaultValue(), "TestDefaultString");
}

TEST_F(RegressionTestProperty, getIsReferenced)
{
    PROTOCOLS("nd")

    auto prop1 = device.getProperty("TestString");
    Bool isReferenced1;
    ASSERT_NO_THROW(isReferenced1 = prop1.getIsReferenced());
    ASSERT_EQ(isReferenced1, True);

    auto prop2 = device.getProperty("TestInt");
    Bool isReferenced2;
    ASSERT_NO_THROW(isReferenced2 = prop2.getIsReferenced());
    ASSERT_EQ(isReferenced2, False);
}

TEST_F(RegressionTestProperty, getValidator)
{
    PROTOCOLS("nd")

    auto prop = device.getProperty("TestInt");
    ValidatorPtr validator;
    ASSERT_NO_THROW(validator = prop.getValidator());
    ASSERT_EQ(validator.getEval(), "Value < 800");
}

TEST_F(RegressionTestProperty, getCoercer)
{
    PROTOCOLS("nd")

    auto prop = device.getProperty("TestInt");
    CoercerPtr coercer;
    ASSERT_NO_THROW(coercer = prop.getCoercer());
    ASSERT_EQ(coercer.getEval(), "if(Value > 900, Value, 900)");
}

TEST_F(RegressionTestProperty, getCallableInfo)
{
    PROTOCOLS("nd")

    auto prop = device.getProperty("TestString");
    CallableInfoPtr info;
    ASSERT_NO_THROW(info = prop.getCallableInfo());
    ASSERT_EQ(info, nullptr);
}

TEST_F(RegressionTestProperty, getStructType)
{
    PROTOCOLS("nd")

    auto prop = device.getProperty("TestStruct");
    StructTypePtr stru;
    ASSERT_NO_THROW(stru = prop.getStructType());
    ASSERT_EQ(stru.getName(), "TestName");
    ASSERT_EQ(stru.getFieldNames()[0], "TestKey");
}
