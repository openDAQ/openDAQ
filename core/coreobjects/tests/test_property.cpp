#include <gtest/gtest.h>
#include <coreobjects/property_factory.h>
#include <coretypes/coretype.h>
#include <coretypes/string_ptr.h>
#include <coretypes/baseobject_factory.h>
#include <coretypes/json_serializer_factory.h>
#include <coretypes/json_deserializer_factory.h>
#include <coreobjects/coercer_factory.h>
#include <coreobjects/validator_factory.h>
#include <coretypes/type_manager_factory.h>
#include <coreobjects/property_object_class_factory.h>

#include "coreobjects/property_object_factory.h"

using PropertyTest = testing::Test;

using namespace daq;

TEST_F(PropertyTest, Guid)
{
    static constexpr IntfID PropertyGuid = {0X4DA1E04E, 0X06CE, 0x5411, {{0x84, 0x7a, 0x57, 0x58, 0xd6, 0xdf, 0xc3, 0xf1}}};
    ASSERT_EQ(IProperty::Id, PropertyGuid);

#if !defined(NDEBUG) && !defined(DS_CI_RUNNER)
    ASSERT_EQ(IProperty::GuidSource, "IProperty.daq");
#endif // !_DEBUG
}

TEST_F(PropertyTest, Create)
{
    auto prop = PropertyBuilder("Empty");
}

TEST_F(PropertyTest, CreateAndDefine)
{
    auto prop = PropertyBuilder("Progress")
                .setValueType(ctInt)
                .setDefaultValue(0LL)
                .setMinValue(0LL)
                .setMaxValue(100LL)
                .build();

    ASSERT_EQ(prop.getName(), "Progress");
    ASSERT_EQ(prop.getValueType(), ctInt);
    ASSERT_EQ(prop.getDefaultValue(), 0LL);
    ASSERT_EQ(prop.getMinValue(), 0LL);
    ASSERT_EQ(prop.getMaxValue(), 100LL);
}

TEST_F(PropertyTest, BoolPropSerialize)
{
    std::string expected =
        R"~({"__type":"Property","name":"DualCore","valueType":0,"defaultValue":false,"readOnly":false,"coercer":{"__type":"Coercer","EvalStr":"if(value == True, True, False)"},"validator":{"__type":"Validator","EvalStr":"value == True"}})~";

    auto property = PropertyBuilder("DualCore")
                    .setValueType(ctBool)
                    .setDefaultValue(False)
                    .setVisible(nullptr)
                    .setSelectionValues(nullptr)
                    .setCoercer(Coercer("if(value == True, True, False)"))
                    .setValidator(Validator("value == True"))
                    .build();

    auto serializer = JsonSerializer();
    property.serialize(serializer);

    std::string json = serializer.getOutput().toStdString();

    ASSERT_EQ(expected, json);
}

TEST_F(PropertyTest, BoolPropDeserializeNoValidatorsCoercers)
{
    std::string json =
        R"({"__type":"Property","name":"DualCore","valueType":0,"defaultValue":false,"readOnly":false,"visible":true})";

    auto deserializer = JsonDeserializer();
    BaseObjectPtr ptr = deserializer.deserialize(json);
    ASSERT_TRUE(ptr.assigned());

    auto serializer = JsonSerializer();
    ptr.serialize(serializer);

    std::string serialized = serializer.getOutput();
    ASSERT_EQ(json, serialized);
}

TEST_F(PropertyTest, BoolPropDeserializeWithValidatorsCoercers)
{
    std::string json = R"~({"__type":"Property","name":"DualCore","valueType":0,"defaultValue":false,"readOnly":false,"visible":true,"coercer":{"__type":"Coercer","EvalStr":"if(value == True, True, False)"},"validator":{"__type":"Validator","EvalStr":"value == True"}})~";

    auto deserializer = JsonDeserializer();
    BaseObjectPtr ptr = deserializer.deserialize(json);
    ASSERT_TRUE(ptr.assigned());

    auto serializer = JsonSerializer();
    ptr.serialize(serializer);

    std::string serialized = serializer.getOutput();
    ASSERT_EQ(json, serialized);
}

TEST_F(PropertyTest, SelectionValuesWithCustomClass)
{
    auto manager = TypeManager();

    auto testClass = PropertyObjectClassBuilder("TestClass")
                     .addProperty(IntProperty("TestIntProp", 0))
                     .build();
    manager.addType(testClass);

    std::string json =
        R"~({"__type":"Property","name":"TestProperty","valueType":1,"defaultValue":0,"readOnly":false,"visible":true,"selectionValues":[{"__type":"PropertyObject","className":"TestClass","propValues":{"TestIntProp":5}}]})~";

    auto deserializer = JsonDeserializer();
    PropertyPtr ptr = deserializer.deserialize(json, manager);
    ASSERT_TRUE(ptr.assigned());

    auto selectionValues = ptr.getSelectionValues();
    ASSERT_TRUE(selectionValues.assigned());
    auto selectionValuesList = selectionValues.asPtrOrNull<IList>(true);
    ASSERT_TRUE(selectionValuesList.assigned());
    ASSERT_EQ(selectionValuesList.getCount(), 1u);
    PropertyObjectPtr firstElement = selectionValuesList[0];
    ASSERT_EQ(firstElement.getClassName(), "TestClass");

    auto serializer = JsonSerializer();
    ptr.serialize(serializer);

    std::string serialized = serializer.getOutput();
    ASSERT_EQ(json, serialized);
}

TEST_F(PropertyTest, Id)
{
    constexpr IntfID ID{0x4DA1E04E, 0x06CE, 0x5411, {{0x84, 0x7A, 0x57, 0x58, 0xD6, 0xDF, 0xC3, 0xF1}}};
    ASSERT_EQ(ID, IProperty::Id);
}

TEST_F(PropertyTest, Inspectable)
{
    auto obj = IntPropertyBuilder("test", 0);

    auto ids = obj.asPtr<IInspectable>(true).getInterfaceIds();
    ASSERT_EQ(ids[0], IPropertyBuilder::Id);

    
    ids = obj.build().asPtr<IInspectable>(true).getInterfaceIds();
    ASSERT_EQ(ids[0], IProperty::Id);
}

TEST_F(PropertyTest, ImplementationName)
{
    auto obj = IntPropertyBuilder("test", 0);

    StringPtr className = obj.asPtr<IInspectable>(true).getRuntimeClassName();
    ASSERT_EQ(className, "daq::PropertyBuilderImpl");

     className = obj.build().asPtr<IInspectable>(true).getRuntimeClassName();
    ASSERT_EQ(className, "daq::PropertyImpl");
}

TEST_F(PropertyTest, ObjectPropertyMetadata)
{
    auto prop = ObjectPropertyBuilder("test", PropertyObject())
                .setVisible(false)
                .setReadOnly(true);
    ASSERT_NO_THROW(prop.build());
}
