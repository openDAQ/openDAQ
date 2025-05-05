#include <gtest/gtest.h>
#include <testutils/testutils.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_factory.h>
#include <coreobjects/property_filter_factory.h>
#include <coreobjects/argument_info_factory.h>
#include <coreobjects/callable_info_factory.h>
#include <coreobjects/property_object_class_factory.h>
#include <coreobjects/property_object_protected_ptr.h>

using namespace daq;
using namespace search::properties;

class PropertySearchTest : public testing::Test
{
protected:

    PropertyObjectPtr testPropertyObject;
    PropertyObjectPtr nestedPropertyObject;
    TypeManagerPtr typeManager;

    void SetUp() override
    {
        typeManager = TypeManager();

        const auto enumType = EnumerationType("EnumType", List<IString>("Option1", "Option2", "Option3"));
        typeManager.addType(enumType);

        const auto structType = StructType("StructType", List<IString>("Int", "Float"), List<IType>(SimpleType(ctInt), SimpleType(ctFloat)));
        typeManager.addType(structType);

        auto dict = Dict<IInteger, IString>();
        dict.set(0xA, "a");
        dict.set(0xB, "b");
        dict.set(0xC, "c");
        dict.set(0xD, "d");
        dict.set(0xE, "e");

        const auto structBuilder = StructBuilder("StructType", typeManager).set("Int", 5).set("Float", 5.0);

        PropertyObjectClassPtr propObjClass =
            PropertyObjectClassBuilder(typeManager, "PropertyObjectClass")
                .addProperty(FunctionProperty("FunctionProp", FunctionInfo(ctObject)))
                .addProperty(FunctionProperty("ProcedureProp", ProcedureInfo()))
                .addProperty(StringProperty("StringProp", "String"))
                .addProperty(FloatPropertyBuilder("FloatProp", 1.0).setReadOnly(true).build())
                .addProperty(ListProperty("ListProp", List<Int>(1, 2, 3, 4)))
                .addProperty(IntPropertyBuilder("IntProp", 1).setReadOnly(true).setVisible(false).build())
                .addProperty(DictProperty("DictProp", dict, false))
                .addProperty(SelectionProperty("SelectionProp", List<IString>("a", "b", "c"), 0))
                .addProperty(SparseSelectionProperty("SparseSelectionProp", dict, 0xA))
                .addProperty(ObjectProperty("ObjectProp", PropertyObject()))
                .addProperty(EnumerationProperty("EnumProp", Enumeration("EnumType", "Option1", typeManager)))
                .addProperty(StructProperty("StructProp", structBuilder.build(), false))
                .addProperty(BoolPropertyBuilder("BoolProp", True).setReadOnly(true).build())
                .build();
        typeManager.addType(propObjClass);

        nestedPropertyObject = PropertyObject(typeManager, "PropertyObjectClass");

        testPropertyObject = PropertyObject(typeManager, "PropertyObjectClass");
        testPropertyObject.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue("ObjectProp", nestedPropertyObject);
    }
};

TEST_F(PropertySearchTest, FindWithDefaultFilterInEmptyObject)
{
    auto propertyObject = PropertyObject();
    auto foundProperties = propertyObject.findProperties();

    ASSERT_EQ(foundProperties.getCount(), 0u);
}

// only top level visible properties
TEST_F(PropertySearchTest, FindWithDefaultFilter)
{
    auto foundProperties = testPropertyObject.findProperties();

    ASSERT_EQ(foundProperties.getCount(), 10u);
}

TEST_F(PropertySearchTest, FindAny)
{
    auto foundProperties = testPropertyObject.findProperties(Any());

    ASSERT_EQ(foundProperties.getCount(), 13u);
}

TEST_F(PropertySearchTest, FindVisible)
{
    auto foundProperties = testPropertyObject.findProperties(Visible());

    ASSERT_EQ(foundProperties.getCount(), 10u);
}

TEST_F(PropertySearchTest, FindReadOnly)
{
    auto foundProperties = testPropertyObject.findProperties(ReadOnly());

    ASSERT_EQ(foundProperties.getCount(), 3u);
}

TEST_F(PropertySearchTest, FindByName)
{
    auto foundProperties = testPropertyObject.findProperties(Name("FloatProp"));

    ASSERT_EQ(foundProperties.getCount(), 1u);
}

TEST_F(PropertySearchTest, FindByType)
{
    ASSERT_EQ(testPropertyObject.findProperties(Type(CoreType::ctStruct)).getCount(), 1u);
    ASSERT_EQ(testPropertyObject.findProperties(Type(CoreType::ctBool)).getCount(), 1u);
    ASSERT_EQ(testPropertyObject.findProperties(Type(CoreType::ctEnumeration)).getCount(), 1u);
    ASSERT_EQ(testPropertyObject.findProperties(Type(CoreType::ctList)).getCount(), 1u);
    ASSERT_EQ(testPropertyObject.findProperties(Type(CoreType::ctDict)).getCount(), 1u);
    ASSERT_EQ(testPropertyObject.findProperties(Type(CoreType::ctObject)).getCount(), 1u);
    ASSERT_EQ(testPropertyObject.findProperties(Type(CoreType::ctFloat)).getCount(), 1u);
    ASSERT_EQ(testPropertyObject.findProperties(Type(CoreType::ctProc)).getCount(), 1u);
    ASSERT_EQ(testPropertyObject.findProperties(Type(CoreType::ctFunc)).getCount(), 1u);
    ASSERT_EQ(testPropertyObject.findProperties(Type(CoreType::ctString)).getCount(), 1u);
    ASSERT_EQ(testPropertyObject.findProperties(Type(CoreType::ctInt)).getCount(), 3u); // selection props are also of type ctInt
}

TEST_F(PropertySearchTest, FindCustom)
{
    auto customSearch = Function(
        [](const PropertyPtr& prop)
        {
            return prop.getName().toStdString().find("Selection") != std::string::npos;
        }
    );
    auto foundProperties = testPropertyObject.findProperties(Custom(customSearch));

    ASSERT_EQ(foundProperties.getCount(), 2u);
}

TEST_F(PropertySearchTest, FindNot)
{
    auto foundProperties = testPropertyObject.findProperties(Not(Visible()));

    ASSERT_EQ(foundProperties.getCount(), 3u);
}

TEST_F(PropertySearchTest, FindAnd)
{
    auto foundProperties = testPropertyObject.findProperties(And(Visible(), ReadOnly()));

    ASSERT_EQ(foundProperties.getCount(), 2u);
}

TEST_F(PropertySearchTest, FindOr)
{
    auto foundProperties = testPropertyObject.findProperties(Or(Visible(), ReadOnly()));

    ASSERT_EQ(foundProperties.getCount(), 11u);
}

TEST_F(PropertySearchTest, ChangeFoundProperty)
{
    auto foundProperties = testPropertyObject.findProperties(Name("StringProp"));

    ASSERT_EQ(foundProperties.getCount(), 1u);
    ASSERT_NO_THROW(foundProperties[0].setValue("NewString"));
    ASSERT_EQ(testPropertyObject.getPropertyValue("StringProp"), "NewString");
}

TEST_F(PropertySearchTest, ChangeReadOnlyFoundProperty)
{
    auto foundProperties = testPropertyObject.findProperties(Name("FloatProp"));

    ASSERT_EQ(foundProperties.getCount(), 1u);
    ASSERT_THROW(foundProperties[0].setValue(3.14), AccessDeniedException);
}
