#include <gtest/gtest.h>
#include <coretypes/coretypes.h>

using namespace daq;

using TypeTest = testing::Test;

TEST_F(TypeTest, SimpleTypes)
{
    for (SizeT i = 0; i <= static_cast<SizeT>(ctStruct); ++i)
    {
        const auto simpleType = SimpleType(static_cast<CoreType>(i));
        ASSERT_EQ(static_cast<CoreType>(i), simpleType.getCoreType());
    }
}

TEST_F(TypeTest, SimpleTypeEquals)
{
    const auto simpleTypeInt1 = SimpleType(ctInt); 
    const auto simpleTypeInt2 = SimpleType(ctInt); 
    const auto simpleTypeString = SimpleType(ctString);

    ASSERT_EQ(simpleTypeInt1, simpleTypeInt2);
    ASSERT_NE(simpleTypeInt1, simpleTypeString);
}

TEST_F(TypeTest, SimpleTypeSerialization)
{
    const auto simpleType = SimpleType(ctString);
    const auto serializer = JsonSerializer();
    
    simpleType.serialize(serializer);
    const auto serializedJson = serializer.getOutput();
    
    const auto deserializer = JsonDeserializer();
    const ObjectPtr<ISimpleType> simpleTypeDeserialized = deserializer.deserialize(serializedJson);

    ASSERT_EQ(simpleType, simpleTypeDeserialized);
}

TEST_F(TypeTest, StructTypeRatio)
{
    const StructPtr ratioStruct = Ratio(10, 1);
    ASSERT_EQ(ratioStruct.getStructType(), RatioStructType());
}

TEST_F(TypeTest, StructTypeComplexNumber)
{
    const StructPtr complexNumberStruct = ComplexNumber(10, 1);
    ASSERT_EQ(complexNumberStruct .getStructType(), ComplexNumberStructType());
}

TEST_F(TypeTest, StructType)
{
    const auto fieldNames = List<IString>("field1", "field2");
    const auto defaultValues = List<IBaseObject>(Ratio(1, 10), "val2");
    const auto fieldTypes = List<IType>(RatioStructType(), SimpleType(ctString));
    const auto structType = StructType("foo", fieldNames, defaultValues, fieldTypes);

    ASSERT_EQ(structType.getFieldTypes(), fieldTypes);
    ASSERT_EQ(structType.getFieldNames(), fieldNames);
    ASSERT_EQ(structType.getFieldDefaultValues(), defaultValues);
    ASSERT_EQ(structType.getName(), "foo");
}

TEST_F(TypeTest, StructTypeNoDefaults)
{
    const auto fieldNames = List<IString>("field1", "field2");
    const auto fieldTypes = List<IType>(RatioStructType(), SimpleType(ctString));
    const auto structType = StructType("foo", fieldNames, fieldTypes);

    ASSERT_EQ(structType.getFieldTypes(), fieldTypes);
    ASSERT_EQ(structType.getFieldNames(), fieldNames);
    ASSERT_EQ(structType.getFieldDefaultValues(), nullptr);
    ASSERT_EQ(structType.getName(), "foo");
}

TEST_F(TypeTest, StructTypeEquals)
{
    const auto fieldNames = List<IString>("field1", "field2");
    const auto fieldNames1 = List<IString>("field3", "field4");
    const auto defaultValues = List<IBaseObject>(Ratio(1, 10), "val2");
    const auto defaultValues1 = List<IBaseObject>(Ratio(1, 10), "val1");
    const auto fieldTypes = List<IType>(RatioStructType(), SimpleType(ctString));
    const auto fieldTypes1 = List<IType>(SimpleType(ctInt), SimpleType(ctString));

    const auto structType1 = StructType("foo", fieldNames, defaultValues, fieldTypes);
    const auto structType2 = StructType("foo", fieldNames, defaultValues, fieldTypes);
    ASSERT_EQ(structType1, structType2);

    const auto structType3 = StructType("foo", fieldNames1, defaultValues, fieldTypes);
    const auto structType4 = StructType("foo", fieldNames, defaultValues, fieldTypes1);
    const auto structType5 = StructType("bar", fieldNames, defaultValues, fieldTypes);
    const auto structType6 = StructType("foo", fieldNames, defaultValues1, fieldTypes);

    ASSERT_NE(structType1, structType3);
    ASSERT_NE(structType1, structType4);
    ASSERT_NE(structType1, structType5);
    ASSERT_NE(structType1, structType6);
}

TEST_F(TypeTest, StructTypeListCountMismatch)
{
    ASSERT_THROW(StructType("foo", List<IString>("field1"), List<IType>()), InvalidParameterException);
    ASSERT_THROW(StructType("foo", List<IString>("field1"), List<IBaseObject>("foo", "bar"), List<IType>(SimpleType(ctString))), InvalidParameterException);
}

TEST_F(TypeTest, StructTypeInvalidCoreType)
{
    ASSERT_THROW(StructType("foo", List<IString>("field1"), List<IType>(SimpleType(ctFunc))), InvalidParameterException);
    ASSERT_THROW(StructType("foo", List<IString>("field1"), List<IType>(SimpleType(ctObject))), InvalidParameterException);
}

TEST_F(TypeTest, StructTypeSerialization)
{
    const auto structType = StructType("foo",
                                       List<IString>("field1", "field2"),
                                       List<IBaseObject>(1, "val"),
                                       List<IType>(SimpleType(ctInt), SimpleType(ctString)));
    
    const auto serializer = JsonSerializer();
    
    structType.serialize(serializer);
    const auto serializedJson = serializer.getOutput();
    
    const auto deserializer = JsonDeserializer();
    const StructTypePtr structTypeDeserialized = deserializer.deserialize(serializedJson);

    ASSERT_EQ(structType, structTypeDeserialized);
}

TEST_F(TypeTest, StructTypeSerializationNoDefaults)
{
    const auto structType = StructType("foo",
                                       List<IString>("field1", "field2"),
                                       List<IType>(SimpleType(ctList), SimpleType(ctString)));
    
    const auto serializer = JsonSerializer();
    
    structType.serialize(serializer);
    const auto serializedJson = serializer.getOutput();
    
    const auto deserializer = JsonDeserializer();
    const StructTypePtr structTypeDeserialized = deserializer.deserialize(serializedJson);

    ASSERT_EQ(structType, structTypeDeserialized);
}

TEST_F(TypeTest, StructTypeSerializationNestedCustomStructType)
{
    const auto structTypeInner = StructType("foo",
                                       List<IString>("field1", "field2"),
                                       List<IBaseObject>(1, "val"),
                                       List<IType>(SimpleType(ctInt), SimpleType(ctString)));

    const auto structType = StructType("bar", List<IString>("string", "struct"), List<IType>(SimpleType(ctString), structTypeInner));
    
    const auto serializer = JsonSerializer();
    
    structType.serialize(serializer);
    const auto serializedJson = serializer.getOutput();
    
    const auto deserializer = JsonDeserializer();
    const StructTypePtr structTypeDeserialized = deserializer.deserialize(serializedJson);

    ASSERT_EQ(structType, structTypeDeserialized);
}

TEST_F(TypeTest, StructTypeSerializationNestedDefaultStructType)
{
    const auto structType = StructType("bar", List<IString>("string", "ratio", "complexNumber"), List<IType>(SimpleType(ctString), RatioStructType(), ComplexNumberStructType()));
    
    const auto serializer = JsonSerializer();
    
    structType.serialize(serializer);
    const auto serializedJson = serializer.getOutput();
    
    const auto deserializer = JsonDeserializer();
    const StructTypePtr structTypeDeserialized = deserializer.deserialize(serializedJson);

    ASSERT_EQ(structType, structTypeDeserialized);
}

