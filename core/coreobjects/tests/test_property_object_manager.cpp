#include <gtest/gtest.h>
#include <coreobjects/property_object_class_ptr.h>
#include <coretypes/type_manager_factory.h>
#include <coreobjects/property_factory.h>
#include <coreobjects/property_object_class_factory.h>

using namespace daq;

class PropertyObjectClassManagerTest : public testing::Test
{
protected:

    void SetUp() override
    {
        manager = TypeManager();
        auto testPropClass = PropertyObjectClassBuilder("Test");

        manager.addType(testPropClass.build());

        auto baseClass = PropertyObjectClassBuilder("BaseClass");
        manager.addType(baseClass.build());

        auto specificClass = PropertyObjectClassBuilder("SpecificClass").setParentName("BaseClass");
        manager.addType(specificClass.build());

        const auto nameProp = PropertyBuilder("Name")
                              .setValueType(ctString)
                              .setDefaultValue("")
                              .build();

        const auto indexProp = PropertyBuilder("Index")
                               .setDefaultValue("")
                               .setValueType(ctInt)
                               .build();

        const auto unitProp = PropertyBuilder("Unit")
                              .setDefaultValue("")
                              .setValueType(ctInt)
                              .setSelectionValues(List<IString>("a", "b"))
                              .build();

        propObjClass = PropertyObjectClassBuilder(manager, "PropertyObject")
                       .addProperty(nameProp)
                       .addProperty(indexProp)
                       .addProperty(unitProp)
                       .build();

        manager.addType(propObjClass);
    }

    void TearDown() override
    {
        manager.removeType("Test");
        manager.removeType("SpecificClass");
        manager.removeType("BaseClass");
        manager.removeType("PropertyObject");
        propObjClass.release();
    }

    TypeManagerPtr manager;
    PropertyObjectClassPtr propObjClass;
};

TEST_F(PropertyObjectClassManagerTest, GetClass)
{
    auto propertyClass = manager.getType("Test");
    ASSERT_EQ(propertyClass.getName().toStdString(), "Test");
}

TEST_F(PropertyObjectClassManagerTest, FindNonExistentClass)
{
    IType* propertyObjectClass;
    ASSERT_EQ(manager->getType(StringPtr("Test1"), &propertyObjectClass), OPENDAQ_ERR_NOTFOUND);
}

TEST_F(PropertyObjectClassManagerTest, ClassList)
{
    auto list = manager.getTypes();
    ASSERT_EQ(list.getCount(), 4u);
}

TEST_F(PropertyObjectClassManagerTest, EnumPropsDerivedClass)
{
    auto prop = PropertyBuilder("Description")
                .setValueType(ctString)
                .setDefaultValue("")
                .build();

    auto propObjClass2 = PropertyObjectClassBuilder(manager, "PropertyObject2")
                         .setParentName("PropertyObject")
                         .addProperty(prop)
                         .build();
    manager.addType(propObjClass2);

    auto props = propObjClass2.getProperties(false);
    ASSERT_EQ(props.getCount(), 1u);

    props = propObjClass2.getProperties(true);
    ASSERT_EQ(props.getCount(), 4u);

    manager.removeType("PropertyObject2");
}

TEST_F(PropertyObjectClassManagerTest, HasPropertyParent)
{
    auto propObjClass2 = PropertyObjectClassBuilder(manager, "PropertyObject2")
                         .setParentName("PropertyObject")
                         .build();
    manager.addType(propObjClass2);

    ASSERT_TRUE(propObjClass2.hasProperty("Name"));
    manager.removeType("PropertyObject2");
}
