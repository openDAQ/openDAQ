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

    parentObject.setPropertyValue("Child", childObject);

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
