#include <coreobjects/property_factory.h>
#include <gtest/gtest.h>
#include <coreobjects/property_object_class_factory.h>
#include <coretypes/type_manager_factory.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_internal_ptr.h>

#include "coreobjects/exceptions.h"

using namespace daq;

static void testPropCallback(PropertyObjectPtr& prop, PropertyValueEventArgsPtr& args)
{
    switch (args.getPropertyEventType())
    {
        case PropertyEventType::Clear:
        {
            IntPtr value = prop.getPropertyValue("PropClearCount");
            prop.setPropertyValue("PropClearCount", value + 1);
            break;
        }
        case PropertyEventType::Update:
        {
            IntPtr value = prop.getPropertyValue("PropWriteCount");
            prop.setPropertyValue("PropWriteCount", value + 1);
            break;
        }

        case PropertyEventType::Read:
        {
            IntPtr value = prop.getPropertyValue("PropReadCount");
            prop.setPropertyValue("PropReadCount", value + 1);
            break;
        }
    }
}

static void testObjCallback(PropertyObjectPtr& prop, PropertyValueEventArgsPtr& args)
{
    switch (args.getPropertyEventType())
    {
        case PropertyEventType::Clear:
        {
            IntPtr value = prop.getPropertyValue("ClearCount");
            prop.setPropertyValue("ClearCount", value + 1);
            break;
        }
        case PropertyEventType::Update:
        {
            IntPtr value = prop.getPropertyValue("WriteCount");
            prop.setPropertyValue("WriteCount", value + 1);
            break;
        }

        case PropertyEventType::Read:
        {
            IntPtr value = prop.getPropertyValue("ReadCount");
            prop.setPropertyValue("ReadCount", value + 1);
            break;
        }
    }
}

class PropertyValueChangedEventsTest : public testing::Test
{
protected:
    TypeManagerPtr manager;

    void SetUp() override
    {
        manager = TypeManager();

        const auto intProp = IntProperty("IntProperty", 10);
        intProp.getOnPropertyValueWrite() += testPropCallback;
        intProp.getOnPropertyValueRead() += testPropCallback;
        
        const auto floatProp = FloatProperty("FloatProperty", 1.23);
        floatProp.getOnPropertyValueWrite() += testPropCallback;
        floatProp.getOnPropertyValueRead() += testPropCallback;

        const auto stringProp = StringProperty("StringProperty", "test");
        stringProp.getOnPropertyValueWrite() += testPropCallback;
        stringProp.getOnPropertyValueRead() += testPropCallback;

        const auto testPropClass = PropertyObjectClassBuilder("TestClass")
                                   .addProperty(intProp)
                                   .addProperty(floatProp)
                                   .addProperty(stringProp)
                                   .addProperty(IntProperty("ClearCount", 0))
                                   .addProperty(IntProperty("WriteCount", 0))
                                   .addProperty(IntProperty("ReadCount", 0))
                                   .addProperty(IntProperty("PropClearCount", 0))
                                   .addProperty(IntProperty("PropWriteCount", 0))
                                   .addProperty(IntProperty("PropReadCount", 0))
                                   .build();

        manager.addType(testPropClass);
    }
};

TEST_F(PropertyValueChangedEventsTest, ObjectCallOnWrite)
{
    auto obj = PropertyObject(manager, "TestClass");
    obj.getOnPropertyValueWrite("IntProperty") += testObjCallback;

    ASSERT_EQ(obj.getPropertyValue("WriteCount"), 0);
    obj.setPropertyValue("IntProperty", 5);
    ASSERT_EQ(obj.getPropertyValue("WriteCount"), 1);
}

TEST_F(PropertyValueChangedEventsTest, ObjectCallOnRead)
{
    auto obj = PropertyObject(manager, "TestClass");
    obj.getOnPropertyValueRead("IntProperty") += testObjCallback;

    obj.getPropertyValue("IntProperty");
    ASSERT_EQ(obj.getPropertyValue("ReadCount"), 1);

    obj.getPropertyValue("IntProperty");
    ASSERT_EQ(obj.getPropertyValue("ReadCount"), 2);
}

TEST_F(PropertyValueChangedEventsTest, MultipleObjectsCallOnWrite)
{
    auto obj1 = PropertyObject(manager, "TestClass");
    auto obj2 = PropertyObject(manager, "TestClass");

    obj1.getOnPropertyValueWrite("FloatProperty") += testObjCallback;
    obj2.getOnPropertyValueWrite("FloatProperty") += testObjCallback;

    obj1.setPropertyValue("FloatProperty", 2.34);
    obj2.setPropertyValue("FloatProperty", 3.45);

    ASSERT_EQ(obj1.getPropertyValue("WriteCount"), 1);
    ASSERT_EQ(obj2.getPropertyValue("WriteCount"), 1);

    ASSERT_EQ(obj1.getPropertyValue("PropWriteCount"), 1);
    ASSERT_EQ(obj2.getPropertyValue("PropWriteCount"), 1);
}

TEST_F(PropertyValueChangedEventsTest, MultipleObjectsCallOnRead)
{
    auto obj1 = PropertyObject(manager, "TestClass");
    auto obj2 = PropertyObject(manager, "TestClass");

    obj1.getOnPropertyValueRead("FloatProperty") += testObjCallback;
    obj2.getOnPropertyValueRead("FloatProperty") += testObjCallback;

    obj1.getPropertyValue("FloatProperty");
    ASSERT_EQ(obj1.getPropertyValue("ReadCount"), 1);
    obj1.getPropertyValue("FloatProperty");
    ASSERT_EQ(obj1.getPropertyValue("ReadCount"), 2);

    obj2.getPropertyValue("FloatProperty");
    ASSERT_EQ(obj2.getPropertyValue("ReadCount"), 1);
    obj2.getPropertyValue("FloatProperty");
    ASSERT_EQ(obj2.getPropertyValue("ReadCount"), 2);

    ASSERT_EQ(obj1.getPropertyValue("PropReadCount"), 2);
    ASSERT_EQ(obj2.getPropertyValue("PropReadCount"), 2);
}

TEST_F(PropertyValueChangedEventsTest, ObjectClear)
{
    auto obj1 = PropertyObject(manager, "TestClass");

    obj1.getOnPropertyValueWrite("FloatProperty") += testObjCallback;

    obj1.setPropertyValue("FloatProperty", 0.1);
    obj1.clearPropertyValue("FloatProperty");
    ASSERT_EQ(obj1.getPropertyValue("ClearCount"), 1);

    obj1.setPropertyValue("FloatProperty", 0.1);
    obj1.clearPropertyValue("FloatProperty");
    ASSERT_EQ(obj1.getPropertyValue("ClearCount"), 2);

    ASSERT_EQ(obj1.getPropertyValue("PropClearCount"), 2);
}

TEST_F(PropertyValueChangedEventsTest, MultipleObjectsClear)
{
    auto obj1 = PropertyObject(manager, "TestClass");
    auto obj2 = PropertyObject(manager, "TestClass");

    obj1.getOnPropertyValueWrite("FloatProperty") += testObjCallback;
    obj2.getOnPropertyValueWrite("FloatProperty") += testObjCallback;

    obj1.setPropertyValue("FloatProperty", 0.1);
    obj1.clearPropertyValue("FloatProperty");
    ASSERT_EQ(obj1.getPropertyValue("ClearCount"), 1);
    obj1.setPropertyValue("FloatProperty", 0.1);
    obj1.clearPropertyValue("FloatProperty");
    ASSERT_EQ(obj1.getPropertyValue("ClearCount"), 2);

    obj2.setPropertyValue("FloatProperty", 0.1);
    obj2.clearPropertyValue("FloatProperty");
    ASSERT_EQ(obj2.getPropertyValue("ClearCount"), 1);
    obj2.setPropertyValue("FloatProperty", 0.1);
    obj2.clearPropertyValue("FloatProperty");
    ASSERT_EQ(obj2.getPropertyValue("ClearCount"), 2);

    ASSERT_EQ(obj1.getPropertyValue("PropClearCount"), 2);
    ASSERT_EQ(obj2.getPropertyValue("PropClearCount"), 2);
}

TEST_F(PropertyValueChangedEventsTest, PropertyEventsOnClass)
{
    int callCount = 0;
    PropertyPtr prop = StringProperty("str", "test");
    prop.getOnPropertyValueWrite() += [&](PropertyObjectPtr&, PropertyValueEventArgsPtr&)
    {
        callCount++;
    };

    const auto propClass = PropertyObjectClassBuilder("test").addProperty(prop).build();
    const auto manager = TypeManager();
    manager.addType(propClass);

    const auto obj1 = PropertyObject(manager, "test");
    const auto obj2 = PropertyObject(manager, "test");
    const auto obj3 = PropertyObject(manager, "test");

    obj1.setPropertyValue("str", "test1");
    obj2.setPropertyValue("str", "test1");
    obj3.setPropertyValue("str", "test1");

    ASSERT_EQ(callCount, 3);
}

TEST_F(PropertyValueChangedEventsTest, PropertyEventsOnObject)
{
    int callCount = 0;
    PropertyPtr prop = StringProperty("str", "test");
    prop.getOnPropertyValueWrite() += [&](PropertyObjectPtr&, PropertyValueEventArgsPtr&)
    {
        callCount++;
    };

    const auto propClass = PropertyObjectClassBuilder("test").addProperty(prop).build();
    const auto manager = TypeManager();
    manager.addType(propClass);

    const auto obj1 = PropertyObject(manager, "test");
    const auto obj2 = PropertyObject(manager, "test");
    const auto obj3 = PropertyObject(manager, "test");

    obj1.getProperty("str").getOnPropertyValueWrite() += [&](PropertyObjectPtr&, PropertyValueEventArgsPtr&)
    {
        callCount++;
    };

    obj1.setPropertyValue("str", "test1");
    obj2.setPropertyValue("str", "test1");
    obj3.setPropertyValue("str", "test1");

    ASSERT_EQ(callCount, 4);
}

TEST_F(PropertyValueChangedEventsTest, PropertyEventClassGetter)
{
    PropertyPtr prop = StringProperty("str", "test");
    prop.getOnPropertyValueWrite() += [&](PropertyObjectPtr&, PropertyValueEventArgsPtr&) {};
    prop.getOnPropertyValueRead() += [&](PropertyObjectPtr&, PropertyValueEventArgsPtr&) {};
    ASSERT_EQ(prop.getOnPropertyValueWrite().getListenerCount(), 1);
    ASSERT_EQ(prop.getOnPropertyValueRead().getListenerCount(), 1);

    const auto propClass = PropertyObjectClassBuilder("test").addProperty(prop).build();
    const auto manager = TypeManager();
    manager.addType(propClass);
    
    const auto obj1 = PropertyObject(manager, "test");
    const auto obj2 = PropertyObject(manager, "test");
    const auto obj3 = PropertyObject();
    obj3.addProperty(prop);

    obj1.getProperty("str").getOnPropertyValueWrite() += [&](PropertyObjectPtr&, PropertyValueEventArgsPtr&) {};
    obj1.getProperty("str").getOnPropertyValueRead() += [&](PropertyObjectPtr&, PropertyValueEventArgsPtr&) {};
    
    ASSERT_EQ(propClass.getProperty("str").asPtr<IPropertyInternal>().getClassOnPropertyValueRead().getListenerCount(), 1);
    ASSERT_EQ(propClass.getProperty("str").asPtr<IPropertyInternal>().getClassOnPropertyValueWrite().getListenerCount(), 1);

    ASSERT_EQ(propClass.getProperty("str").getOnPropertyValueRead().getListenerCount(), 1);
    ASSERT_EQ(propClass.getProperty("str").getOnPropertyValueWrite().getListenerCount(), 1);

    ASSERT_EQ(obj1.getProperty("str").asPtr<IPropertyInternal>().getClassOnPropertyValueRead().getListenerCount(), 1);
    ASSERT_EQ(obj1.getProperty("str").asPtr<IPropertyInternal>().getClassOnPropertyValueWrite().getListenerCount(), 1);
    
    ASSERT_EQ(obj1.getProperty("str").getOnPropertyValueRead().getListenerCount(), 1);
    ASSERT_EQ(obj1.getProperty("str").getOnPropertyValueWrite().getListenerCount(), 1);
    
    ASSERT_EQ(obj2.getProperty("str").asPtr<IPropertyInternal>().getClassOnPropertyValueRead().getListenerCount(), 1);
    ASSERT_EQ(obj2.getProperty("str").asPtr<IPropertyInternal>().getClassOnPropertyValueWrite().getListenerCount(), 1);
    
    ASSERT_EQ(obj2.getProperty("str").getOnPropertyValueRead().getListenerCount(), 0);
    ASSERT_EQ(obj2.getProperty("str").getOnPropertyValueWrite().getListenerCount(), 0);

    ASSERT_EQ(obj3.getProperty("str").asPtr<IPropertyInternal>().getClassOnPropertyValueRead().getListenerCount(), 1);
    ASSERT_EQ(obj3.getProperty("str").asPtr<IPropertyInternal>().getClassOnPropertyValueWrite().getListenerCount(), 1);
    
    ASSERT_EQ(obj3.getProperty("str").getOnPropertyValueRead().getListenerCount(), 1);
    ASSERT_EQ(obj3.getProperty("str").getOnPropertyValueWrite().getListenerCount(), 1);
}


TEST_F(PropertyValueChangedEventsTest, PropertyEventClassCall)
{
    PropertyPtr prop = StringProperty("str", "test");
    const auto propClass = PropertyObjectClassBuilder("test").addProperty(prop).build();
    const auto manager = TypeManager();
    manager.addType(propClass);
    
    const auto obj1 = PropertyObject(manager, "test");
    const auto obj2 = PropertyObject(manager, "test");
    const auto obj3 = PropertyObject();

    int callCount = 0;

    prop.getOnPropertyValueWrite() +=
        [&callCount](const PropertyObjectPtr&, const PropertyValueEventArgsPtr&)
        {
            callCount++;
        };

    obj3.addProperty(prop);
    obj1.getProperty("str").getOnPropertyValueWrite() +=
        [&callCount](const PropertyObjectPtr&, const PropertyValueEventArgsPtr&)
        {
            callCount += 10;
        };

    obj1.setPropertyValue("str", "1");
    obj2.setPropertyValue("str", "2");
    obj3.setPropertyValue("str", "3");

    ASSERT_EQ(callCount, 13);
}
