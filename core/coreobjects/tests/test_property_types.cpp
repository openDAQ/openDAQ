#include <testutils/testutils.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_factory.h>
#include <coreobjects/eval_value_factory.h>
#include <coreobjects/argument_info_factory.h>
#include <coreobjects/callable_info_factory.h>

using namespace daq;

class PropertyTypeTest : public testing::Test
{
public:
    void SetUp() override
    {
        manager = TypeManager();
        obj = PropertyObject(manager, "");

        obj.addProperty(BoolProperty("Bool", true));
		obj.addProperty(IntPropertyBuilder("Int", 10).build());
		obj.addProperty(FloatPropertyBuilder("Float", 5.12).build());
        obj.addProperty(StringPropertyBuilder("String", "foo").build());

        obj.addProperty(ListProperty("IntList", List<IInteger>(1, 2, 3)));
        obj.addProperty(ListProperty("StringList", List<IString>("foo", "bar")));

        obj.addProperty(DictProperty("IntIntDict", Dict<IInteger, IInteger>({{0, 10}, {5, 20}})));
        obj.addProperty(DictProperty("IntStringDict", Dict<IInteger, IString>({{0, "foo"}, {10, "bar"}})));
        
        obj.addProperty(RatioProperty("Ratio", Ratio(1, 1000)));

        obj.addProperty(FunctionProperty("Function", FunctionInfo(ctString, List<IArgumentInfo>(ArgumentInfo("Int", ctInt)))));
        obj.addProperty(FunctionProperty("Procedure", ProcedureInfo(List<IArgumentInfo>(ArgumentInfo("Int", ctInt)))));

        obj.addProperty(StructProperty(
            "Struct", Struct("TestStruct", Dict<IString, IBaseObject>({{"String", "bar"}, {"Integer", 10}, {"Float", 5.123}}), manager)));
        obj.addProperty(EnumerationProperty("Enumeration", EnumerationType("TestEnum", List<IString>("Test1", "Test2"))));

        obj.addProperty(ReferenceProperty("Reference", EvalValue("%IntList")));

        obj.addProperty(SparseSelectionProperty("SparseSelectionInt", Dict<IInteger, IInteger>({{0, 10}, {5, 20}}), 5));
        obj.addProperty(SparseSelectionProperty("SparseSelectionString", Dict<IInteger, IString>({{0, "foo"}, {10, "bar"}}), 0));

        obj.addProperty(SelectionProperty("IndexSelectionInt", List<IInteger>(10, 20, 30), 1));
        obj.addProperty(SelectionProperty("IndexSelectionString", List<IString>("foo", "bar"), 0));

        obj.addProperty(StringPropertyBuilder("StringSelection", "foo").setSelectionValues(List<IString>("foo", "bar")).build());
        obj.addProperty(IntPropertyBuilder("IntSelection", 10).setSelectionValues(List<IInteger>(0, 6, 15, 10)).setIsIntegerValueSelection().build());
		obj.addProperty(FloatPropertyBuilder("FloatSelection", 5.12).setSelectionValues(List<IFloat>(0.12, -5.2, 5.12, 10.2)).build());
    }

    PropertyObjectPtr obj;
    TypeManagerPtr manager;
};

TEST_F(PropertyTypeTest, BoolType)
{
    auto boolProp = obj.getProperty("Bool");
    ASSERT_EQ(boolProp.getValueType(), ctBool);
    ASSERT_EQ(boolProp.getItemType(), ctUndefined);
    ASSERT_EQ(boolProp.getKeyType(), ctUndefined);
    ASSERT_EQ(boolProp.getPropertyType(), PropertyType::Bool);
}

TEST_F(PropertyTypeTest, IntType)
{
    auto intProp = obj.getProperty("Int");
    ASSERT_EQ(intProp.getValueType(), ctInt);
    ASSERT_EQ(intProp.getItemType(), ctUndefined);
    ASSERT_EQ(intProp.getKeyType(), ctUndefined);
    ASSERT_FALSE(intProp.getSelectionValues().assigned());
    ASSERT_EQ(intProp.getPropertyType(), PropertyType::Int);
}

TEST_F(PropertyTypeTest, FloatType)
{
    auto floatProp = obj.getProperty("Float");
    ASSERT_EQ(floatProp.getValueType(), ctFloat);
    ASSERT_EQ(floatProp.getItemType(), ctUndefined);
    ASSERT_EQ(floatProp.getKeyType(), ctUndefined);
    ASSERT_FALSE(floatProp.getSelectionValues().assigned());
    ASSERT_EQ(floatProp.getPropertyType(), PropertyType::Float);
}

TEST_F(PropertyTypeTest, StringType)
{
    auto stringProp = obj.getProperty("String");
    ASSERT_EQ(stringProp.getValueType(), ctString);
    ASSERT_EQ(stringProp.getItemType(), ctUndefined);
    ASSERT_EQ(stringProp.getKeyType(), ctUndefined);
    ASSERT_FALSE(stringProp.getSelectionValues().assigned());
    ASSERT_EQ(stringProp.getPropertyType(), PropertyType::String);
}

TEST_F(PropertyTypeTest, ListType)
{
    auto intListProp = obj.getProperty("IntList");
    ASSERT_EQ(intListProp.getValueType(), ctList);
    ASSERT_EQ(intListProp.getItemType(), ctInt);
    ASSERT_EQ(intListProp.getKeyType(), ctUndefined);
    ASSERT_EQ(intListProp.getPropertyType(), PropertyType::List);

    auto stringListProp = obj.getProperty("StringList");
    ASSERT_EQ(stringListProp.getValueType(), ctList);
    ASSERT_EQ(stringListProp.getItemType(), ctString);
    ASSERT_EQ(stringListProp.getKeyType(), ctUndefined);
    ASSERT_EQ(stringListProp.getPropertyType(), PropertyType::List);
}

TEST_F(PropertyTypeTest, DictType)
{
    auto intIntDictProp = obj.getProperty("IntIntDict");
    ASSERT_EQ(intIntDictProp.getValueType(), ctDict);
    ASSERT_EQ(intIntDictProp.getItemType(), ctInt);
    ASSERT_EQ(intIntDictProp.getKeyType(), ctInt);
    ASSERT_FALSE(intIntDictProp.getSelectionValues().assigned());
    ASSERT_EQ(intIntDictProp.getPropertyType(), PropertyType::Dict);

    auto intStringDictProp = obj.getProperty("IntStringDict");
    ASSERT_EQ(intStringDictProp.getValueType(), ctDict);
    ASSERT_EQ(intStringDictProp.getItemType(), ctString);
    ASSERT_EQ(intStringDictProp.getKeyType(), ctInt);
    ASSERT_FALSE(intStringDictProp.getSelectionValues().assigned());
    ASSERT_EQ(intStringDictProp.getPropertyType(), PropertyType::Dict);
}

TEST_F(PropertyTypeTest, RatioType)
{
    auto ratioProp = obj.getProperty("Ratio");
    ASSERT_EQ(ratioProp.getValueType(), ctRatio);
    ASSERT_EQ(ratioProp.getItemType(), ctUndefined);
    ASSERT_EQ(ratioProp.getKeyType(), ctUndefined);
    ASSERT_EQ(ratioProp.getPropertyType(), PropertyType::Ratio);
}

TEST_F(PropertyTypeTest, FunctionAndProcedureTypes)
{
    auto functionProp = obj.getProperty("Function");
    ASSERT_EQ(functionProp.getValueType(), ctFunc);
    ASSERT_EQ(functionProp.getItemType(), ctUndefined);
    ASSERT_EQ(functionProp.getKeyType(), ctUndefined);
    auto funcInfo = functionProp.getCallableInfo();
    ASSERT_TRUE(funcInfo.assigned());
    ASSERT_EQ(funcInfo.getReturnType(), ctString);
    ASSERT_EQ(functionProp.getPropertyType(), PropertyType::Function);

    auto procedureProp = obj.getProperty("Procedure");
    ASSERT_EQ(procedureProp.getValueType(), ctProc);
    ASSERT_EQ(procedureProp.getItemType(), ctUndefined);
    ASSERT_EQ(procedureProp.getKeyType(), ctUndefined);
    auto procInfo = procedureProp.getCallableInfo();
    ASSERT_TRUE(procInfo.assigned());
    ASSERT_EQ(procInfo.getReturnType(), ctUndefined);
    ASSERT_EQ(procedureProp.getPropertyType(), PropertyType::Procedure);
}

TEST_F(PropertyTypeTest, StructType)
{
    auto structProp = obj.getProperty("Struct");
    ASSERT_EQ(structProp.getValueType(), ctStruct);
    ASSERT_EQ(structProp.getItemType(), ctUndefined);
    ASSERT_EQ(structProp.getKeyType(), ctUndefined);
    auto structType = structProp.getStructType();
    ASSERT_TRUE(structType.assigned());
    ASSERT_EQ(structType.getName(), "TestStruct");
    ASSERT_EQ(structProp.getPropertyType(), PropertyType::Struct);
}

TEST_F(PropertyTypeTest, EnumerationType)
{
    auto enumProp = obj.getProperty("Enumeration");
    ASSERT_EQ(enumProp.getValueType(), ctEnumeration);
    ASSERT_EQ(enumProp.getItemType(), ctUndefined);
    ASSERT_EQ(enumProp.getKeyType(), ctUndefined);
    auto enumType = enumProp.getValue().asPtr<IEnumeration>().getEnumerationType();
    ASSERT_TRUE(enumType.assigned());
    ASSERT_EQ(enumType.getName(), "TestEnum");
    ASSERT_EQ(enumProp.getPropertyType(), PropertyType::Enumeration);
}

TEST_F(PropertyTypeTest, ReferenceType)
{
    // Reference properties always return the property type of the referenced property, so in this case Selection since it references
    // IntList which is a selection property

    auto refProp = obj.getProperty("Reference");
    ASSERT_EQ(refProp.getValueType(), ctList);
    ASSERT_EQ(refProp.getItemType(), ctInt);
    ASSERT_EQ(refProp.getKeyType(), ctUndefined);
    ASSERT_EQ(refProp.getPropertyType(), PropertyType::List);
}

TEST_F(PropertyTypeTest, SparseSelectionType)
{
    auto sparseSelectionIntProp = obj.getProperty("SparseSelectionInt");
    ASSERT_EQ(sparseSelectionIntProp.getValueType(), ctInt);
    ASSERT_EQ(sparseSelectionIntProp.getItemType(), ctInt);
    ASSERT_EQ(sparseSelectionIntProp.getKeyType(), ctInt);
    ASSERT_TRUE(sparseSelectionIntProp.getSelectionValues().assigned());
    ASSERT_TRUE(sparseSelectionIntProp.getSelectionValues().supportsInterface<IDict>());
}

TEST_F(PropertyTypeTest, IndexSelectionType)
{
    auto indexSelectionIntProp = obj.getProperty("IndexSelectionInt");
    ASSERT_EQ(indexSelectionIntProp.getValueType(), ctInt);
    ASSERT_EQ(indexSelectionIntProp.getItemType(), ctInt);
    ASSERT_EQ(indexSelectionIntProp.getKeyType(), ctUndefined);
    ASSERT_TRUE(indexSelectionIntProp.getSelectionValues().assigned());
    ASSERT_TRUE(indexSelectionIntProp.getSelectionValues().supportsInterface<IList>());
}


TEST_F(PropertyTypeTest, ValueBasedSelectionTypes)
{
    
}
