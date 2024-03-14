#include <gtest/gtest.h>
#include <coreobjects/property_object_factory.h>
#include <coretypes/json_serializer_factory.h>
#include <coretypes/json_deserializer_factory.h>
#include <coreobjects/property_factory.h>
#include <coreobjects/property_object_class_factory.h>
#include <coretypes/type_manager_factory.h>
#include <coreobjects/property_object_protected_ptr.h>


using namespace daq;
class SerializationTest : public testing::Test
{
protected:
    void SetUp() override
    {
        auto name = PropertyBuilder("Name")
                    .setValueType(ctString)
                    .setDefaultValue("")
                    .build();

        auto readonlyName = PropertyBuilder("ReadonlyName")
                            .setValueType(ctString)
                            .setDefaultValue("")
                            .setReadOnly(true)
                            .build();

        auto child = PropertyBuilder("Child")
                     .setValueType(ctObject)
                     .setDefaultValue(PropertyObject())
                     .build();

        auto parentClass = PropertyObjectClassBuilder("ParentClass")
                           .addProperty(name)
                           .addProperty(readonlyName)
                           .addProperty(child)
                           .build();

        auto childName = PropertyBuilder("Name")
                         .setValueType(ctString)
                         .setDefaultValue("")
                         .build();

        auto childClass = PropertyObjectClassBuilder("ChildClass")
                          .addProperty(childName)
                          .build();

        manager = TypeManager();
        manager.addType(parentClass);
        manager.addType(childClass);
    }

    void TearDown() override
    {
        manager.removeType("ParentClass");
        manager.removeType("ChildClass");
    }

    TypeManagerPtr manager;
};

TEST_F(SerializationTest, ChildObject)
{
    auto parentObject = PropertyObject(manager, "ParentClass");
    parentObject.setPropertyValue("Name", "IamParent");
    parentObject.asPtr<IPropertyObjectProtected>(true).setProtectedPropertyValue("ReadonlyName", "ReadonlyValue");

    auto childObject = PropertyObject(manager,"ChildClass");
    childObject.setPropertyValue("Name", "IamChild");

    parentObject.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue("Child", childObject);

    auto serializer = JsonSerializer();
    parentObject.serialize(serializer);

    const auto objStr = serializer.getOutput();

    auto deserializer = JsonDeserializer();
    PropertyObjectPtr parentObject1 = deserializer.deserialize(objStr, manager);
    ASSERT_EQ(parentObject1.getPropertyValue("Name"), "IamParent");
    ASSERT_EQ(parentObject1.getPropertyValue("ReadonlyName"), "ReadonlyValue");

    PropertyObjectPtr childObject1 = parentObject1.getPropertyValue("Child");
    ASSERT_EQ(childObject1.getPropertyValue("Name"), "IamChild");
}

TEST_F(SerializationTest, JsonSerObjectToJsonString)
{
    PropertyObjectPtr parent = PropertyObject();
    PropertyObjectPtr child1 = PropertyObject();
    PropertyObjectPtr child2 = PropertyObject();
    PropertyObjectPtr child1_1 = PropertyObject();
    PropertyObjectPtr child1_2 = PropertyObject();
    PropertyObjectPtr child1_2_1 = PropertyObject();
    PropertyObjectPtr child2_1 = PropertyObject();

    child1_2_1.addProperty(StringProperty("String", "string"));
    child1_2_1.addProperty(StringPropertyBuilder("ReadOnlyString", "string").setReadOnly(true).build());

    child1_2.addProperty(ObjectProperty("child1_2_1", child1_2_1));
    child1_2.addProperty(IntProperty("Int", 1));

    child1_1.addProperty(FloatProperty("Float", 1.1));

    child1.addProperty(ObjectProperty("child1_1", child1_1));
    child1.addProperty(ObjectProperty("child1_2", child1_2));

    child2_1.addProperty(RatioProperty("Ratio", Ratio(1, 2)));

    child2.addProperty(ObjectProperty("child2_1", child2_1));

    parent.addProperty(ObjectProperty("child1", child1));
    parent.addProperty(ObjectProperty("child2", child2));

    const auto ser = JsonSerializer();
    parent.serialize(ser);
    const auto str = ser.getOutput();

    ProcedurePtr proc = [&str](const SerializedObjectPtr& obj) {
        std::string json = obj.toJson();
        ASSERT_EQ(json, str);
    };

    const auto deserializer = JsonDeserializer();
    deserializer.callCustomProc(proc, str);
}
