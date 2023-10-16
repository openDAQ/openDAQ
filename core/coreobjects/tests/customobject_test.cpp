#include <gtest/gtest.h>
#include <coretypes/type_manager_factory.h>
#include <coreobjects/property_object_class_factory.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_factory.h>
#include <coreobjects/eval_value_factory.h>

using namespace daq;

class CustomObjectTest : public testing::Test
{
protected:
    TypeManagerPtr manager;
    void SetUp() override
    {
        manager = TypeManager();
    }
};


TEST_F(CustomObjectTest, Create)
{
    auto propObj = PropertyObject();
}

TEST_F(CustomObjectTest, TestProp)
{
    // define properties of "Person"
    auto prop = PropertyBuilder("Name")
                .setDefaultValue("John Doe")
                .setValueType(ctString)
                .build();

    // create class with name "Person"
    auto personClass = PropertyObjectClassBuilder("Person")
                       .addProperty(prop)
                       .build();

    // register "Person"
    manager.addType(personClass);

    // create instance of "Person"
    auto person = PropertyObject(manager , "Person");

    // no property "Name" set, should get default value "John Doe"
    auto name = person.getPropertyValue("Name");
    ASSERT_EQ(name, "John Doe");

    // set "Name" property to "Peter"
    person.setPropertyValue("Name", "Peter");

    // check if property "Name" matches current name set
    auto name1 = person.getPropertyValue("Name");
    ASSERT_EQ(name1, "Peter");

    // clear property value and check if default matches
    person.clearPropertyValue("Name");
    auto name2 = person.getPropertyValue("Name");
    ASSERT_EQ(name2, "John Doe");

    auto props = person.getVisibleProperties();
    ASSERT_EQ(props.getCount(), 1u);
    ASSERT_EQ(((PropertyPtr)props.getItemAt(0)).getName(), "Name");

    // cleanup
    manager.removeType("Person");
}

TEST_F(CustomObjectTest, TestDepProp)
{
    // define properties of "Person"
    const auto firstName = PropertyBuilder("FirstName")
                           .setDefaultValue("John")
                           .setValueType(ctString)
                           .build();

    const auto hasMiddleName = PropertyBuilder("HasMiddleName")
                               .setValueType(ctBool)
                               .setDefaultValue(True)
                               .build();

    const auto middleName = PropertyBuilder("MiddleName")
                            .setValueType(ctString)
                            .setDefaultValue("")
                            .build();

    const auto lastName = PropertyBuilder("LastName")
                          .setDefaultValue("Doe")
                          .setValueType(ctString)
                          .build();

    // create class with name "Person"
    auto personClass = PropertyObjectClassBuilder("Person")
                       .addProperty(firstName)
                       .addProperty(hasMiddleName)
                       .addProperty(middleName)
                       .addProperty(lastName)
                       .build();


    // register "Person"
    manager.addType(personClass);

    // create instance of "Person1"
    auto person1 = PropertyObject(manager, "Person");
    person1.setPropertyValue("FirstName", "Nick");
    person1.setPropertyValue("FirstName", "Nick1");
    person1.setPropertyValue("LastName", "Keith");
    person1.setPropertyValue("HasMiddleName", True);
    person1.setPropertyValue("MiddleName", "Michael");

    // create instance of "Person2"
    auto person2 = PropertyObject(manager, "Person");
    person2.setPropertyValue("FirstName", "David");
    person2.setPropertyValue("LastName", "Lopez");
    person2.setPropertyValue("HasMiddleName", False);
    person2.setPropertyValue("MiddleName", "Pau");

    ASSERT_TRUE(person1.getPropertyValue("MiddleName") == "Michael");
    ASSERT_NO_THROW(person2.getPropertyValue("MiddleName"));

    // cleanup
    manager.removeType("Person");
}

TEST_F(CustomObjectTest, TestAdvDepProp)
{
    // create class with name "Amplifier"
    // define properties of "Person"
    auto modeProp = PropertyBuilder("Mode")
                    .setDefaultValue(0LL)
                    .setValueType(ctInt)
                    .build();

    auto rangeProp = PropertyBuilder("Range")
                     .setValueType(ctInt)
                     .setDefaultValue(0LL)
                     .setSelectionValues(EvalValue("if(%Mode:value == 0, ['10V', '100V'], ['1/1', '1/10'])"))
                     .build();

    const auto amplClass = PropertyObjectClassBuilder("Amplifier")
                           .addProperty(modeProp)
                           .addProperty(rangeProp)
                           .build();

    manager.addType(amplClass);

    auto amplifier = PropertyObject(manager, "Amplifier");

    amplifier.setPropertyValue("Mode", 0LL);
    modeProp = amplifier.getProperty("Range");
    ListPtr<IBaseObject> modePropValues = modeProp.getSelectionValues();
    ASSERT_EQ(modePropValues.getCount(), 2u);
    ASSERT_EQ(modePropValues.getItemAt(0), "10V");

    amplifier.setPropertyValue("Mode", 1LL);
    modeProp = amplifier.getProperty("Range");
    modePropValues = modeProp.getSelectionValues();
    ASSERT_EQ(modePropValues.getCount(), 2u);
    ASSERT_EQ(modePropValues.getItemAt(0), "1/1");

    manager.removeType("Amplifier");
}
