#include <gtest/gtest.h>
#include <testutils/testutils.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_factory.h>
#include <coreobjects/search_filter_factory.h>
#include <coreobjects/argument_info_factory.h>
#include <coreobjects/callable_info_factory.h>
#include <coreobjects/property_object_class_factory.h>
#include <coreobjects/property_object_protected_ptr.h>

using namespace daq;
using namespace search;
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

TEST_F(PropertySearchTest, FindAnyInEmptyObject)
{
    auto propertyObject = PropertyObject();
    auto foundProperties = propertyObject.findProperties(Any());

    ASSERT_EQ(foundProperties.getCount(), 0u);
}

TEST_F(PropertySearchTest, FindAny)
{
    auto foundProperties = testPropertyObject.findProperties(Any());
    ASSERT_EQ(foundProperties.getCount(), 13u);

    auto recursivelyFoundProperties = testPropertyObject.findProperties(Recursive(Any()));
    ASSERT_EQ(recursivelyFoundProperties.getCount(), 26u);
}

TEST_F(PropertySearchTest, FindVisible)
{
    auto foundProperties = testPropertyObject.findProperties(Visible());
    ASSERT_EQ(foundProperties.getCount(), 10u);

    auto recursivelyFoundProperties = testPropertyObject.findProperties(Recursive(Visible()));
    ASSERT_EQ(recursivelyFoundProperties.getCount(), 20u);
}

TEST_F(PropertySearchTest, FindReadOnly)
{
    auto foundProperties = testPropertyObject.findProperties(ReadOnly());
    ASSERT_EQ(foundProperties.getCount(), 3u);

    auto recursivelyFoundProperties = testPropertyObject.findProperties(Recursive(ReadOnly()));
    ASSERT_EQ(recursivelyFoundProperties.getCount(), 6u);
}

TEST_F(PropertySearchTest, FindByName)
{
    auto foundProperties = testPropertyObject.findProperties(Name("FloatProp"));
    ASSERT_EQ(foundProperties.getCount(), 1u);

    auto recursivelyFoundProperties = testPropertyObject.findProperties(Recursive(Name("FloatProp")));
    ASSERT_EQ(recursivelyFoundProperties.getCount(), 2u);
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

    auto recursivelyFoundProperties = testPropertyObject.findProperties(Recursive(Custom(customSearch)));
    ASSERT_EQ(recursivelyFoundProperties.getCount(), 4u);
}

TEST_F(PropertySearchTest, FindNot)
{
    auto foundProperties = testPropertyObject.findProperties(Not(Visible()));
    ASSERT_EQ(foundProperties.getCount(), 3u);

    auto recursivelyFoundProperties = testPropertyObject.findProperties(Recursive(Not(Visible())));
    ASSERT_EQ(recursivelyFoundProperties.getCount(), 6u);
}

TEST_F(PropertySearchTest, FindAnd)
{
    auto foundProperties = testPropertyObject.findProperties(And(Visible(), ReadOnly()));
    ASSERT_EQ(foundProperties.getCount(), 2u);

    auto recursivelyFoundProperties = testPropertyObject.findProperties(Recursive(And(Visible(), ReadOnly())));
    ASSERT_EQ(recursivelyFoundProperties.getCount(), 4u);
}

TEST_F(PropertySearchTest, FindOr)
{
    auto foundProperties = testPropertyObject.findProperties(Or(Visible(), ReadOnly()));
    ASSERT_EQ(foundProperties.getCount(), 11u);

    auto recursivelyFoundProperties = testPropertyObject.findProperties(Recursive(Or(Visible(), ReadOnly())));
    ASSERT_EQ(recursivelyFoundProperties.getCount(), 22u);
}

TEST_F(PropertySearchTest, ChangeFoundProperties)
{
    auto foundProperties = testPropertyObject.findProperties(Recursive(Name("StringProp")));

    ASSERT_EQ(foundProperties.getCount(), 2u);
    ASSERT_NO_THROW(foundProperties[0].setValue("NewString"));
    ASSERT_NO_THROW(foundProperties[1].setValue("NewString"));

    ASSERT_EQ(testPropertyObject.getPropertyValue("StringProp"), "NewString");
    ASSERT_EQ(nestedPropertyObject.getPropertyValue("StringProp"), "NewString");
}

TEST_F(PropertySearchTest, ChangeFoundReadOnlyProperties)
{
    auto foundProperties = testPropertyObject.findProperties(Recursive(Name("FloatProp")));

    ASSERT_EQ(foundProperties.getCount(), 2u);
    ASSERT_THROW(foundProperties[0].setValue(3.14), AccessDeniedException);
    ASSERT_THROW(foundProperties[1].setValue(3.14), AccessDeniedException);
}

TEST_F(PropertySearchTest, ChangeFoundProtectedProperties)
{
    auto foundProperties = testPropertyObject.findProperties(Recursive(Name("ObjectProp")));

    ASSERT_EQ(foundProperties.getCount(), 2u);
    ASSERT_THROW(foundProperties[0].setValue(PropertyObject()), AccessDeniedException);
    ASSERT_THROW(foundProperties[1].setValue(PropertyObject()), AccessDeniedException);
}
