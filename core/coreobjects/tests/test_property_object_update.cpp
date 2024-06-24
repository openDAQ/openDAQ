#include <gtest/gtest.h>
#include <coretypes/coretypes.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_object_class_factory.h>
#include <coreobjects/property_factory.h>
#include <coreobjects/property_object_protected_ptr.h>

using namespace daq;


class UpdateObjectTest : public testing::Test
{
protected:
    void SetUp() override
    {
        const PropertyObjectPtr child1 = PropertyObject();
        const PropertyObjectPtr child2 = PropertyObject();
        const PropertyObjectPtr child1_1 = PropertyObject();
        const PropertyObjectPtr child1_2 = PropertyObject();
        const PropertyObjectPtr child1_2_1 = PropertyObject();
        const PropertyObjectPtr child2_1 = PropertyObject();

        child1_2_1.addProperty(StringProperty("String", "String"));
        child1_2_1.addProperty(StringPropertyBuilder("ReadOnlyString", "String").setReadOnly(true).build());

        child1_2.addProperty(ObjectProperty("child1_2_1", child1_2_1));
        child1_2.addProperty(IntProperty("Int", 1));
        child1_2.addProperty(ListProperty("List", List<IString>("foo", "bar")));

        child1_1.addProperty(FloatProperty("Float", 1.1));

        child1.addProperty(ObjectProperty("child1_1", child1_1));
        child1.addProperty(ObjectProperty("child1_2", child1_2));

        child2_1.addProperty(RatioProperty("Ratio", Ratio(1, 2)));
        child2_1.addProperty(DictProperty("Dict", Dict<IInteger, IString>({{1, "foo"}, {2, "bar"}})));

        child2.addProperty(ObjectProperty("child2_1", child2_1));
        
        parentObj1 = PropertyObject();
        parentObj1.addProperty(ObjectProperty("child1", child1));
        parentObj1.addProperty(ObjectProperty("child2", child2));

        parentObj2 = PropertyObject();
        parentObj2.addProperty(ObjectProperty("child1", child1));
        parentObj2.addProperty(ObjectProperty("child2", child2));

        PropertyObjectClassPtr objClass = PropertyObjectClassBuilder("TestClass")
                                              .addProperty(ObjectProperty("child1", child1))
                                              .addProperty(ObjectProperty("child2", child2))
                                              .build();

        typeManager = TypeManager();
        typeManager.addType(objClass);
        objWithClass1 = PropertyObject(typeManager, "TestClass");
        objWithClass2 = PropertyObject(typeManager, "TestClass");
    }

    void TearDown() override
    {
        parentObj1 = nullptr;
        parentObj2 = nullptr;
        objWithClass1 = nullptr;
        objWithClass2 = nullptr;
        typeManager = nullptr;
    }

    TypeManagerPtr typeManager = TypeManager();
    PropertyObjectPtr objWithClass1;
    PropertyObjectPtr objWithClass2;
    PropertyObjectPtr parentObj1;
    PropertyObjectPtr parentObj2;
};

TEST_F(UpdateObjectTest, UpdateObj)
{
    parentObj2.setPropertyValue("child1.child1_2.child1_2_1.String", "new_string");
    parentObj2.setPropertyValue("child1.child1_1.Float", 2.1);
    parentObj2.setPropertyValue("child1.child1_2.Int", 2);
    parentObj2.setPropertyValue("child1.child1_2.List", List<IString>("foo1", "bar1"));
    parentObj2.setPropertyValue("child2.child2_1.Ratio", Ratio(1, 5));
    parentObj2.setPropertyValue("child2.child2_1.Dict", Dict<IInteger, IString>({{10, "bar"}, {20, "foo"}}));
    parentObj2.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue("child1.child1_2.child1_2_1.ReadOnlyString", "new_string");

    const auto serializer = JsonSerializer();
    parentObj2.serialize(serializer);

    const auto deserializer = JsonDeserializer();
    deserializer.update(parentObj1, serializer.getOutput());

    const auto dict = Dict<IInteger, IString>({{10, "bar"}, {20, "foo"}});
    ASSERT_EQ(parentObj1.getPropertyValue("child1.child1_2.child1_2_1.String"), "new_string");
    ASSERT_EQ(parentObj1.getPropertyValue("child1.child1_1.Float"), 2.1);
    ASSERT_EQ(parentObj1.getPropertyValue("child1.child1_2.Int"), 2);
    ASSERT_EQ(parentObj1.getPropertyValue("child1.child1_2.List"), List<IString>("foo1", "bar1"));
    ASSERT_EQ(parentObj1.getPropertyValue("child2.child2_1.Ratio"), Ratio(1, 5));
    ASSERT_EQ(parentObj1.getPropertyValue("child2.child2_1.Dict"), dict);
    ASSERT_EQ(parentObj1.getPropertyValue("child1.child1_2.child1_2_1.ReadOnlyString"), "new_string");
}

TEST_F(UpdateObjectTest, UpdateClassObj)
{
    objWithClass1.setPropertyValue("child1.child1_2.child1_2_1.String", "new_string");
    objWithClass1.setPropertyValue("child1.child1_1.Float", 2.1);
    objWithClass1.setPropertyValue("child1.child1_2.Int", 2);
    objWithClass1.setPropertyValue("child1.child1_2.List", List<IString>("foo1", "bar1"));
    objWithClass1.setPropertyValue("child2.child2_1.Ratio", Ratio(1, 5));
    objWithClass1.setPropertyValue("child2.child2_1.Dict", Dict<IInteger, IString>({{10, "bar"}, {20, "foo"}}));
    objWithClass1.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue("child1.child1_2.child1_2_1.ReadOnlyString", "new_string");

    const auto serializer = JsonSerializer();
    objWithClass1.serialize(serializer);

    const auto deserializer = JsonDeserializer();
    deserializer.update(objWithClass2, serializer.getOutput());

    const auto dict = Dict<IInteger, IString>({{10, "bar"}, {20, "foo"}});
    ASSERT_EQ(objWithClass2.getPropertyValue("child1.child1_2.child1_2_1.String"), "new_string");
    ASSERT_EQ(objWithClass2.getPropertyValue("child1.child1_1.Float"), 2.1);
    ASSERT_EQ(objWithClass2.getPropertyValue("child1.child1_2.Int"), 2);
    ASSERT_EQ(objWithClass2.getPropertyValue("child1.child1_2.List"), List<IString>("foo1", "bar1"));
    ASSERT_EQ(objWithClass2.getPropertyValue("child2.child2_1.Ratio"), Ratio(1, 5));
    ASSERT_EQ(objWithClass2.getPropertyValue("child2.child2_1.Dict"), dict);
    ASSERT_EQ(objWithClass2.getPropertyValue("child1.child1_2.child1_2_1.ReadOnlyString"), "new_string");
}

TEST_F(UpdateObjectTest, UpdateObjReset)
{
    parentObj1.setPropertyValue("child1.child1_2.child1_2_1.String", "new_string");
    parentObj1.setPropertyValue("child1.child1_1.Float", 2.1);
    parentObj1.setPropertyValue("child1.child1_2.Int", 2);
    parentObj1.setPropertyValue("child1.child1_2.List", List<IString>("foo1", "bar1"));
    parentObj1.setPropertyValue("child2.child2_1.Ratio", Ratio(1, 5));
    parentObj1.setPropertyValue("child2.child2_1.Dict", Dict<IInteger, IString>({{10, "bar"}, {20, "foo"}}));
    parentObj1.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue("child1.child1_2.child1_2_1.ReadOnlyString", "new_string");

    const auto serializer = JsonSerializer();
    parentObj2.serialize(serializer);

    const auto deserializer = JsonDeserializer();
    deserializer.update(parentObj1, serializer.getOutput());

    const auto dict = Dict<IInteger, IString>({{1, "foo"}, {2, "bar"}});
    ASSERT_EQ(parentObj1.getPropertyValue("child1.child1_2.child1_2_1.String"), "String");
    ASSERT_EQ(parentObj1.getPropertyValue("child1.child1_1.Float"), 1.1);
    ASSERT_EQ(parentObj1.getPropertyValue("child1.child1_2.Int"), 1);
    ASSERT_EQ(parentObj1.getPropertyValue("child1.child1_2.List"), List<IString>("foo", "bar"));
    ASSERT_EQ(parentObj1.getPropertyValue("child2.child2_1.Ratio"), Ratio(1, 2));
    ASSERT_EQ(parentObj1.getPropertyValue("child2.child2_1.Dict"), dict);
    ASSERT_EQ(parentObj1.getPropertyValue("child1.child1_2.child1_2_1.ReadOnlyString"), "String");
}

TEST_F(UpdateObjectTest, UpdateClassObjReset)
{
    objWithClass1.setPropertyValue("child1.child1_2.child1_2_1.String", "new_string");
    objWithClass1.setPropertyValue("child1.child1_1.Float", 2.1);
    objWithClass1.setPropertyValue("child1.child1_2.Int", 2);
    objWithClass1.setPropertyValue("child1.child1_2.List", List<IString>("foo1", "bar1"));
    objWithClass1.setPropertyValue("child2.child2_1.Ratio", Ratio(1, 5));
    objWithClass1.setPropertyValue("child2.child2_1.Dict", Dict<IInteger, IString>({{10, "bar"}, {20, "foo"}}));
    objWithClass1.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue("child1.child1_2.child1_2_1.ReadOnlyString", "new_string");

    const auto serializer = JsonSerializer();
    objWithClass2.serialize(serializer);

    const auto deserializer = JsonDeserializer();
    deserializer.update(objWithClass1, serializer.getOutput());

    const auto dict = Dict<IInteger, IString>({{1, "foo"}, {2, "bar"}});
    ASSERT_EQ(objWithClass1.getPropertyValue("child1.child1_2.child1_2_1.String"), "String");
    ASSERT_EQ(objWithClass1.getPropertyValue("child1.child1_1.Float"), 1.1);
    ASSERT_EQ(objWithClass1.getPropertyValue("child1.child1_2.Int"), 1);
    ASSERT_EQ(objWithClass1.getPropertyValue("child1.child1_2.List"), List<IString>("foo", "bar"));
    ASSERT_EQ(objWithClass1.getPropertyValue("child2.child2_1.Ratio"), Ratio(1, 2));
    ASSERT_EQ(objWithClass1.getPropertyValue("child2.child2_1.Dict"), dict);
    ASSERT_EQ(objWithClass1.getPropertyValue("child1.child1_2.child1_2_1.ReadOnlyString"), "String");
}
