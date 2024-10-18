#include <gtest/gtest.h>
#include <opcuatms/converters/property_object_conversion_utils.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_factory.h>
#include <testutils/test_comparators.h>

using PropertyObjectConversionUtilsTest = testing::Test;

using namespace daq;
using namespace daq::opcua;
using namespace daq::opcua::tms;


static PropertyObjectPtr CreateTestPropertyObject()
{
    auto obj = PropertyObject();
    obj.addProperty(StringProperty("Name", ""));
    obj.addProperty(IntProperty("age", 0));
    obj.addProperty(FloatProperty("weight", 0.0));
    obj.addProperty(BoolProperty("isTheBest", false));
    return obj;
}


TEST_F(PropertyObjectConversionUtilsTest, SimpleObject)
{
    auto obj = CreateTestPropertyObject();
    obj.setPropertyValue("Name", "Jovanka");
    obj.setPropertyValue("age", 99);
    obj.setPropertyValue("weight", 60.5);
    obj.setPropertyValue("isTheBest", true);

    auto variant = PropertyObjectConversionUtils::ToDictVariant(obj);

    ASSERT_TRUE(variant->type == &UA_TYPES_DAQBT[UA_TYPES_DAQBT_DAQKEYVALUEPAIR]);
    ASSERT_EQ(variant->arrayLength, obj.getAllProperties().getCount());

    auto objOut = CreateTestPropertyObject();
    PropertyObjectConversionUtils::ToPropertyObject(variant, objOut);
    ASSERT_TRUE(TestComparators::PropertyObjectEquals(obj, objOut));
}

TEST_F(PropertyObjectConversionUtilsTest, EmptyObject)
{
    auto obj = CreateTestPropertyObject();

    auto variant = PropertyObjectConversionUtils::ToDictVariant(obj);

    ASSERT_TRUE(variant->type == &UA_TYPES_DAQBT[UA_TYPES_DAQBT_DAQKEYVALUEPAIR]);
    ASSERT_EQ(variant->arrayLength, obj.getAllProperties().getCount());

    auto objOut = CreateTestPropertyObject();
    PropertyObjectConversionUtils::ToPropertyObject(variant, objOut);
    ASSERT_TRUE(TestComparators::PropertyObjectEquals(obj, objOut));
}

TEST_F(PropertyObjectConversionUtilsTest, CloneDefault)
{
    auto obj = CreateTestPropertyObject();
    auto clone = PropertyObjectConversionUtils::ClonePropertyObject(obj);

    ASSERT_TRUE(TestComparators::PropertyObjectEquals(obj, clone));
}

TEST_F(PropertyObjectConversionUtilsTest, Clone)
{
    auto obj = CreateTestPropertyObject();
    obj.setPropertyValue("Name", "Jovanka");
    obj.setPropertyValue("age", 99);
    obj.setPropertyValue("weight", 60.5);
    obj.setPropertyValue("isTheBest", true);

    auto clone = PropertyObjectConversionUtils::ClonePropertyObject(obj);

    ASSERT_TRUE(TestComparators::PropertyObjectEquals(obj, clone));
}
