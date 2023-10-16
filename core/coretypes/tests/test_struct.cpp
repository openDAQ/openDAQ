#include <gtest/gtest.h>
#include <coretypes/coretypes.h>

using namespace daq;

using StructObjectTest = testing::Test;

TEST_F(StructObjectTest, RatioStruct)
{
    const StructPtr ratio = Ratio(10, 11);

    ASSERT_EQ(ratio.getStructType().getName(), "ratio");

    ASSERT_EQ(ratio.getFieldNames()[0], "numerator");
    ASSERT_EQ(ratio.getFieldNames()[1], "denominator");

    ASSERT_EQ(ratio.getFieldValues()[0], 10);
    ASSERT_EQ(ratio.getFieldValues()[1], 11);
}

TEST_F(StructObjectTest, ComplexNumberStruct)
{
    const StructPtr complexNumber = ComplexNumber(10.1, 11.2);

    ASSERT_EQ(complexNumber.getStructType().getName(), "complexNumber");

    ASSERT_EQ(complexNumber.getFieldNames()[0], "real");
    ASSERT_EQ(complexNumber.getFieldNames()[1], "imaginary");

    ASSERT_EQ(complexNumber.getFieldValues()[0], 10.1);
    ASSERT_EQ(complexNumber.getFieldValues()[1], 11.2);
}

TEST_F(StructObjectTest, SimpleStruct)
{
    const auto manager = TypeManager();

    const auto structMembers = Dict<IString, IBaseObject>({{"string", "bar"}, {"integer", 10}, {"float", 5.123}});
    const auto simpleStruct = Struct("foo", structMembers, manager);

    ASSERT_EQ(manager.getTypes().getCount(), 1);
    ASSERT_EQ(simpleStruct.getStructType(), manager.getType("foo"));
    ASSERT_EQ(structMembers, simpleStruct.getAsDictionary());

    ASSERT_TRUE(simpleStruct.hasField("string"));
    ASSERT_FALSE(simpleStruct.hasField("foo"));
}

TEST_F(StructObjectTest, NestedStructCustom)
{
    const auto manager = TypeManager();
    const auto innerStruct = Struct("bar", Dict<IString, IBaseObject>({{"float", 5.123}}), manager);
    const auto structMembers = Dict<IString, IBaseObject>({{"string", "bar"}, {"integer", 10}, {"struct", innerStruct}});
    const auto nestedStruct = Struct("foo", structMembers, manager);

    ASSERT_EQ(manager.getTypes().getCount(), 2);
    ASSERT_EQ(nestedStruct.getStructType(), manager.getType("foo"));
    ASSERT_EQ(structMembers, nestedStruct .getAsDictionary());
}

TEST_F(StructObjectTest, NestedStructPredefined)
{
    const auto manager = TypeManager();
    const auto ratio = Ratio(10, 15);
    const auto complexNumber = ComplexNumber(1.123, 2.234);
    const auto structMembers = Dict<IString, IBaseObject>({{"string", "bar"}, {"integer", 10}, {"ratio", ratio},
                                                           {"complexNumber", complexNumber}});
    const auto nestedStruct = Struct("foo", structMembers, manager);

    ASSERT_EQ(manager.getTypes().getCount(), 1);
    ASSERT_EQ(nestedStruct.getStructType(), manager.getType("foo"));
    ASSERT_EQ(structMembers, nestedStruct .getAsDictionary());
}

TEST_F(StructObjectTest, RegisteredStruct)
{
    const auto manager = TypeManager();
    const auto type = StructType("foo",
                                 List<IString>("string", "int", "float"),
                                 List<IType>(SimpleType(ctString), SimpleType(ctInt), SimpleType(ctFloat)));
    manager.addType(type);

    const auto registeredStruct1 = Struct("foo", Dict<IString, IBaseObject>({{"string", "foo"}, {"int", 10}, {"float", 1.123}}), manager);
    const auto registeredStruct2 = Struct("foo", Dict<IString, IBaseObject>({{"string", "bar"},{"float", 1.123}, {"int", 10}}), manager);

    ASSERT_EQ(type, registeredStruct1.getStructType());
    ASSERT_EQ(type, registeredStruct2.getStructType());
}

TEST_F(StructObjectTest, RegisteredStructDefaults)
{
    const auto manager = TypeManager();
    const auto type = StructType("foo",
                                 List<IString>("string", "int", "float"),
                                 List<IBaseObject>("foo", 10, 1.123),
                                 List<IType>(SimpleType(ctString), SimpleType(ctInt), SimpleType(ctFloat)));
    manager.addType(type);

    const auto registeredStruct1 = Struct("foo", nullptr, manager);
    const auto registeredStruct2 = Struct("foo", Dict<IString, IBaseObject>({{"float", 5.432}}), manager);

    ASSERT_EQ(registeredStruct1.getFieldValues(), type.getFieldDefaultValues());

    ASSERT_EQ(registeredStruct2.get("float"), 5.432);
    ASSERT_EQ(registeredStruct2.get("string"), "foo");
    ASSERT_EQ(registeredStruct2.get("int"), 10);
}

TEST_F(StructObjectTest, RegisteredStructNested)
{
    const auto manager = TypeManager();
    const auto nestedType = StructType("foo",
                                 List<IString>("string", "int", "float"),
                                 List<IBaseObject>("foo", 10, 1.123),
                                 List<IType>(SimpleType(ctString), SimpleType(ctInt), SimpleType(ctFloat)));
    const auto type = StructType("bar", List<IString>("ratio", "custom"), List<IType>(RatioStructType(), nestedType));

    manager.addType(nestedType);
    manager.addType(type);

    const auto nestedStruct = Struct("bar",
                                     Dict<IString, IBaseObject>({{"ratio", Ratio(1, 2)}, {"custom", Struct("foo", nullptr, manager)}}),
                                     manager);

    const auto innerStruct = Struct("foo", nullptr, manager);
    ASSERT_EQ(nestedStruct.get("ratio"), Ratio(1,2));
    ASSERT_EQ(nestedStruct.get("custom"), innerStruct);
}

TEST_F(StructObjectTest, RegisteredStructInvalidNames)
{
    const auto manager = TypeManager();
    const auto type = StructType("foo",
                                 List<IString>("string", "int", "float"),
                                 List<IBaseObject>("foo", 10, 1.123),
                                 List<IType>(SimpleType(ctString), SimpleType(ctInt), SimpleType(ctFloat)));

    manager.addType(type);
    ASSERT_THROW(Struct("foo", Dict<IString, IBaseObject>({{"string", "foo"}, {"integer", 10}, {"float", 1.123}}), manager),
                 InvalidParameterException);
}

TEST_F(StructObjectTest, RegisteredStructInvalidTypes)
{
    const auto manager = TypeManager();
    const auto type = StructType("foo",
                                 List<IString>("string", "int", "float"),
                                 List<IBaseObject>("foo", 10, 1.123),
                                 List<IType>(SimpleType(ctString), SimpleType(ctInt), SimpleType(ctFloat)));
    
    manager.addType(type);
    ASSERT_THROW(Struct("foo", Dict<IString, IBaseObject>({{"string", "foo"}, {"int", 10}, {"float", "invalid"}}), manager),
                 InvalidParameterException);
}

TEST_F(StructObjectTest, RegisteredStructNoDefaultValues)
{
    const auto manager = TypeManager();
    const auto type = StructType("foo",
                                 List<IString>("string", "int", "float"),
                                 List<IType>(SimpleType(ctString), SimpleType(ctInt), SimpleType(ctFloat)));
    
    manager.addType(type);
    const auto nullStruct = Struct("foo", nullptr, manager);

    ASSERT_EQ(nullStruct.getFieldValues(), List<IBaseObject>(nullptr, nullptr, nullptr));
}

TEST_F(StructObjectTest, UndefinedMemberType)
{
    const auto manager = TypeManager();
    const auto type = StructType("foo",
                                 List<IString>("string", "int", "float"),
                                 List<IType>(SimpleType(ctUndefined), SimpleType(ctUndefined), SimpleType(ctUndefined)));

    manager.addType(type);
    ASSERT_NO_THROW(Struct("foo", Dict<IString, IBaseObject>({{"string", 10}, {"int", "foo"}, {"float", "bar"}}), manager));
}

TEST_F(StructObjectTest, SimpleStructSerialization)
{
    const auto manager = TypeManager();

    const auto structMembers = Dict<IString, IBaseObject>({{"string", "bar"}, {"integer", 10}, {"float", 5.123}});
    const auto simpleStruct = Struct("foo", structMembers, manager);

    const auto serializer = JsonSerializer();
    
    simpleStruct.serialize(serializer);
    const auto serializedJson = serializer.getOutput();
    
    const auto deserializer = JsonDeserializer();
    const StructPtr simpleStructDeserialized = deserializer.deserialize(serializedJson, manager);

    ASSERT_EQ(simpleStruct, simpleStructDeserialized);
}


TEST_F(StructObjectTest, ComplexStructSerialization)
{
    const auto manager = TypeManager();
    const auto innerStruct = Struct("bar", Dict<IString, IBaseObject>({{"float", 5.123}}), manager);
    const auto structMembers = Dict<IString, IBaseObject>({{"string", "bar"}, {"integer", 10}, {"struct", innerStruct}, {"ratio", Ratio(10, 5)}});
    const auto nestedStruct = Struct("foo", structMembers, manager);

    const auto serializer = JsonSerializer();
    
    nestedStruct.serialize(serializer);
    const auto serializedJson = serializer.getOutput();
    
    const auto deserializer = JsonDeserializer();
    const StructPtr nestedStructDeserialized = deserializer.deserialize(serializedJson, manager);

    ASSERT_EQ(nestedStruct, nestedStructDeserialized);
}

TEST_F(StructObjectTest, ComplexStructSerializationEmptyManager)
{
    const auto manager = TypeManager();
    const auto innerStruct = Struct("bar", Dict<IString, IBaseObject>({{"float", 5.123}}), manager);
    const auto structMembers = Dict<IString, IBaseObject>({{"string", "bar"}, {"integer", 10}, {"struct", innerStruct}, {"ratio", Ratio(10, 5)}});
    const auto nestedStruct = Struct("foo", structMembers, manager);

    const auto serializer = JsonSerializer();
    
    nestedStruct.serialize(serializer);
    const auto serializedJson = serializer.getOutput();

    const auto emptyManager = TypeManager();
    const auto deserializer = JsonDeserializer();
    const StructPtr nestedStructDeserialized = deserializer.deserialize(serializedJson, emptyManager);

    ASSERT_EQ(nestedStruct, nestedStructDeserialized);
}
