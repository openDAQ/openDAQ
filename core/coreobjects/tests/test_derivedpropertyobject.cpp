#include <gtest/gtest.h>
#include <coreobjects/property_object_class_ptr.h>
#include <coreobjects/property_object_class_ptr.h>
#include "test_value_object_factory.h"
#include "test_object_property_class.h"
#include "test_object_factory.h"

using namespace daq;

class DerivedPropertyObjectTest : public testing::Test
{
protected:
    PropertyObjectClassPtr testPropClass;

    void SetUp() override
    {
        testObjectPropertyClassRegistrator = new TestObjectPropertyClassRegistrator();
    }

    void TearDown() override
    {
        delete testObjectPropertyClassRegistrator;
    }

protected:
    TestObjectPropertyClassRegistrator* testObjectPropertyClassRegistrator{};
};

TEST_F(DerivedPropertyObjectTest, Create)
{
    auto testObj = TestObject(testObjectPropertyClassRegistrator->manager);
}

TEST_F(DerivedPropertyObjectTest, TestProperties)
{
    auto testObj = TestObject(testObjectPropertyClassRegistrator->manager);
    const auto valueObj = testObj.getValueObject();
    ASSERT_EQ(valueObj.getValue(), 0);

    testObj.setValueObject(TestValueObject(1));
    const auto valueObj1 = testObj.getValueObject();
    ASSERT_EQ(valueObj1.getValue(), 1);

    testObj.clearPropertyValue("ValueObject");
    const auto valueObj2 = testObj.getValueObject();
    ASSERT_EQ(valueObj2.getValue(), 0);
}
