#include <gtest/gtest.h>
#include <coreobjects/property_object_class_ptr.h>
#include <coreobjects/property_object_class_factory.h>
#include <coretypes/type_manager_factory.h>
#include <coreobjects/property_factory.h>
#include <coreobjects/eval_value_factory.h>

using namespace daq;

class PropertyObjectClassTest : public testing::Test
{
protected:
    PropertyObjectClassPtr propObjClass;
    PropertyObjectClassBuilderPtr propObjClassBuilder;

    void SetUp() override
    {
        auto nameProp = PropertyBuilder("Name")
                        .setValueType(ctString)
                        .setDefaultValue("")
                        .build();

        auto indexProp = PropertyBuilder("Index")
                         .setValueType(ctInt)
                         .setDefaultValue(0)
                         .build();

        auto unitProp = PropertyBuilder("Unit")
                        .setValueType(ctInt)
                        .setDefaultValue(0)
                        .setSelectionValues(List<IString>("a", "b"))
                        .build();

        propObjClassBuilder = PropertyObjectClassBuilder("PropertyObject")
                              .addProperty(nameProp)
                              .addProperty(indexProp)
                              .addProperty(unitProp);

        propObjClass = propObjClassBuilder.build();
    }
};

TEST_F(PropertyObjectClassTest, Name)
{
    ASSERT_EQ(propObjClass.getName(), "PropertyObject");
}

TEST_F(PropertyObjectClassTest, Get)
{
    ASSERT_NO_THROW(propObjClass.getProperty("Name"));
    ASSERT_NO_THROW(propObjClass.getProperty("Index"));
    ASSERT_NO_THROW(propObjClass.getProperty("Unit"));
    ASSERT_THROW(propObjClass.getProperty("Name1"), NotFoundException);
}

TEST_F(PropertyObjectClassTest, EnumProps)
{
    auto props = propObjClass.getProperties(false);
    ASSERT_EQ(props.getCount(), 3u);

    props = propObjClass.getProperties(false);
    ASSERT_EQ(props.getCount(), 3u);
    PropertyPtr prop = props.getItemAt(0);

    props = propObjClass.getProperties(false);
    ASSERT_EQ(props.getCount(), 3u);
    prop = props.getItemAt(0);
}

TEST_F(PropertyObjectClassTest, EnumInsertionOrder)
{
    auto props = propObjClass.getProperties(false);

    ASSERT_EQ(props[0].getName(), "Name");
    ASSERT_EQ(props[1].getName(), "Index");
    ASSERT_EQ(props[2].getName(), "Unit");
}

TEST_F(PropertyObjectClassTest, EnumCustomOrder)
{
    propObjClassBuilder->setPropertyOrder(List<IString>("Unit", "Name", "Index"));
    auto props = propObjClassBuilder.build().getProperties(false);

    ASSERT_EQ(props[0].getName(), "Unit");
    ASSERT_EQ(props[1].getName(), "Name");
    ASSERT_EQ(props[2].getName(), "Index");

    propObjClassBuilder->setPropertyOrder(nullptr);
}

TEST_F(PropertyObjectClassTest, SerializeJson)
{
    std::string expectedJson =
        R"({"__type":"PropertyObjectClass","name":"PropertyObject","properties":{"Name":{"__type":"Property","name":"Name","valueType":3,"defaultValue":"","readOnly":false,"visible":true},"Index":{"__type":"Property","name":"Index","valueType":1,"defaultValue":0,"readOnly":false,"visible":true},"Unit":{"__type":"Property","name":"Unit","valueType":1,"defaultValue":0,"readOnly":false,"visible":true,"selectionValues":["a","b"]}}})";
    auto serializer = JsonSerializer();

    propObjClass.serialize(serializer);

    StringPtr str;
    ErrCode errCode = serializer->getOutput(&str);
    ASSERT_TRUE(OPENDAQ_SUCCEEDED(errCode));

    ASSERT_EQ(str.toStdString(), expectedJson);
}

TEST_F(PropertyObjectClassTest, HasProperty)
{
    ASSERT_TRUE(propObjClass.hasProperty("Name"));
}

TEST_F(PropertyObjectClassTest, HasPropertyFalse)
{
    ASSERT_FALSE(propObjClass.hasProperty("DoesNotExist"));
}

TEST_F(PropertyObjectClassTest, Inspectable)
{
    auto obj = PropertyObjectClassBuilder("Test").build();

    auto ids = obj.asPtr<IInspectable>(true).getInterfaceIds();
    ASSERT_EQ(ids[0], IPropertyObjectClass::Id);
}

TEST_F(PropertyObjectClassTest, ImplementationName)
{
    auto obj = PropertyObjectClassBuilder("Test").build();

    StringPtr className = obj.asPtr<IInspectable>(true).getRuntimeClassName();
    ASSERT_EQ(className, "daq::PropertyObjectClassImpl");
}

TEST_F(PropertyObjectClassTest, RemoveProperty)
{
    auto obj = PropertyObjectClassBuilder("Test").addProperty(FloatProperty("Test", 0.0));
    ASSERT_NO_THROW(obj.removeProperty("Test"));
    ASSERT_THROW(obj.removeProperty("Test"), NotFoundException);
}

TEST_F(PropertyObjectClassTest, DuplicateReferenceCheck)
{
    auto obj = PropertyObjectClassBuilder("Test").addProperty(ReferenceProperty("ref1", EvalValue("%ReferencedVariable")));
    ASSERT_THROW(obj.addProperty(ReferenceProperty("ref2", EvalValue("%ReferencedVariable"))), InvalidValueException);
}

TEST_F(PropertyObjectClassTest, PropertyObjectClassBuilderSetGet)
{
    auto properties = Dict<IString, IProperty>();
    properties["Test"] = FloatProperty("Test", 0.0);
    const auto propertyOrder = List<IString>("Test");

    const auto propertyObjectClassBuilder = PropertyObjectClassBuilder("Test")
                                            .setName("PropertyObjectClass")
                                            .setParentName("PropertyObjectClassParent")
                                            .addProperty(properties["Test"])
                                            .setPropertyOrder(propertyOrder);
    
    ASSERT_EQ(propertyObjectClassBuilder.getName(), "PropertyObjectClass");
    ASSERT_EQ(propertyObjectClassBuilder.getParentName(), "PropertyObjectClassParent");
    ASSERT_EQ(propertyObjectClassBuilder.getProperties(), properties);
    ASSERT_EQ(propertyObjectClassBuilder.getPropertyOrder(), propertyOrder);
}

TEST_F(PropertyObjectClassTest, PropertyObjectClassCreateFactory)
{
    auto properties = Dict<IString, IProperty>();
    properties["Test"] = FloatProperty("Test", 0.0);

    const auto propertyObjectClassBuilder = PropertyObjectClassBuilder("Test")
                                            .setName("PropertyObjectClass")
                                            .setParentName("PropertyObjectClassParent")
                                            .addProperty(properties["Test"]);
    const auto propertyObjectClass = PropertyObjectClassFromBuilder(propertyObjectClassBuilder);

    ASSERT_EQ(propertyObjectClass.getName(), "PropertyObjectClass");
    ASSERT_EQ(propertyObjectClass.getParentName(), "PropertyObjectClassParent");
    ASSERT_EQ(propertyObjectClass.getProperties(false), List<IProperty>(properties["Test"]));
}
