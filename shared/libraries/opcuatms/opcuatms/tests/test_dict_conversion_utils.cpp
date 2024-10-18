#include <gtest/gtest.h>
#include <opcuatms/converters/property_object_conversion_utils.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_factory.h>
#include <testutils/test_comparators.h>

using DictConversionUtilsTest = testing::Test;

using namespace daq;
using namespace daq::opcua;
using namespace daq::opcua::tms;


static PropertyObjectPtr CreateTestPropertyObject()
{
    auto obj = PropertyObject();
    obj.addProperty(StringProperty("name", ""));
    obj.addProperty(IntProperty("age", 0));
    obj.addProperty(FloatProperty("weight", 0.0));
    obj.addProperty(BoolProperty("isTheBest", false));
    return obj;
}


TEST_F(DictConversionUtilsTest, SimpleObject)
{
    auto obj = CreateTestPropertyObject();
    obj.setPropertyValue("name", "Jovanka");
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

TEST_F(DictConversionUtilsTest, EmptyObject)
{
    auto obj = CreateTestPropertyObject();

    auto variant = PropertyObjectConversionUtils::ToDictVariant(obj);

    ASSERT_TRUE(variant->type == &UA_TYPES_DAQBT[UA_TYPES_DAQBT_DAQKEYVALUEPAIR]);
    ASSERT_EQ(variant->arrayLength, obj.getAllProperties().getCount());

    auto objOut = CreateTestPropertyObject();
    PropertyObjectConversionUtils::ToPropertyObject(variant, objOut);
    ASSERT_TRUE(TestComparators::PropertyObjectEquals(obj, objOut));
}
