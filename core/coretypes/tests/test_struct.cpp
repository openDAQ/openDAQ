#include <gtest/gtest.h>
#include <coretypes/coretypes.h>

using namespace daq;

using StructObjectTest = testing::Test;

TEST_F(StructObjectTest, RatioStruct)
{
    const StructPtr ratio = Ratio(10, 11);

    ASSERT_EQ(ratio.getStructType().getName(), "Ratio");

    ASSERT_EQ(ratio.getFieldNames()[0], "Numerator");
    ASSERT_EQ(ratio.getFieldNames()[1], "Denominator");

    ASSERT_EQ(ratio.getFieldValues()[0], 10);
    ASSERT_EQ(ratio.getFieldValues()[1], 11);
}

TEST_F(StructObjectTest, ComplexNumberStruct)
{
    const StructPtr complexNumber = ComplexNumber(10.1, 11.2);

    ASSERT_EQ(complexNumber.getStructType().getName(), "ComplexNumber");

    ASSERT_EQ(complexNumber.getFieldNames()[0], "Real");
    ASSERT_EQ(complexNumber.getFieldNames()[1], "Imaginary");

    ASSERT_EQ(complexNumber.getFieldValues()[0], 10.1);
    ASSERT_EQ(complexNumber.getFieldValues()[1], 11.2);
}

TEST_F(StructObjectTest, SimpleStruct)
{
    const auto manager = TypeManager();

    const auto structMembers = Dict<IString, IBaseObject>({{"String", "bar"}, {"Integer", 10}, {"Float", 5.123}});
    const auto simpleStruct = Struct("foo", structMembers, manager);

    ASSERT_EQ(manager.getTypes().getCount(), 1u);
    ASSERT_EQ(simpleStruct.getStructType(), manager.getType("foo"));
    ASSERT_EQ(structMembers, simpleStruct.getAsDictionary());

    ASSERT_TRUE(simpleStruct.hasField("String"));
    ASSERT_FALSE(simpleStruct.hasField("foo"));
}

TEST_F(StructObjectTest, NestedStructCustom)
{
    const auto manager = TypeManager();
    const auto innerStruct = Struct("bar", Dict<IString, IBaseObject>({{"Float", 5.123}}), manager);
    const auto structMembers = Dict<IString, IBaseObject>({{"String", "bar"}, {"Integer", 10}, {"Struct", innerStruct}});
    const auto nestedStruct = Struct("foo", structMembers, manager);

    ASSERT_EQ(manager.getTypes().getCount(), 2u);
    ASSERT_EQ(nestedStruct.getStructType(), manager.getType("foo"));
    ASSERT_EQ(structMembers, nestedStruct .getAsDictionary());
}

TEST_F(StructObjectTest, NestedStructPredefined)
{
    const auto manager = TypeManager();
    const auto ratio = Ratio(10, 15);
    const auto complexNumber = ComplexNumber(1.123, 2.234);
    const auto structMembers = Dict<IString, IBaseObject>({{"String", "bar"}, {"Integer", 10}, {"Ratio", ratio},
                                                           {"ComplexNumber", complexNumber}});
    const auto nestedStruct = Struct("foo", structMembers, manager);

    ASSERT_EQ(manager.getTypes().getCount(), 1u);
    ASSERT_EQ(nestedStruct.getStructType(), manager.getType("foo"));
    ASSERT_EQ(structMembers, nestedStruct .getAsDictionary());
}

TEST_F(StructObjectTest, RegisteredStruct)
{
    const auto manager = TypeManager();
    const auto type = StructType("foo",
                                 List<IString>("String", "Int", "Float"),
                                 List<IType>(SimpleType(ctString), SimpleType(ctInt), SimpleType(ctFloat)));
    manager.addType(type);

    const auto registeredStruct1 = Struct("foo", Dict<IString, IBaseObject>({{"String", "foo"}, {"Int", 10}, {"Float", 1.123}}), manager);
    const auto registeredStruct2 = Struct("foo", Dict<IString, IBaseObject>({{"String", "bar"},{"Float", 1.123}, {"Int", 10}}), manager);

    ASSERT_EQ(type, registeredStruct1.getStructType());
    ASSERT_EQ(type, registeredStruct2.getStructType());
}

TEST_F(StructObjectTest, RegisteredStructDefaults)
{
    const auto manager = TypeManager();
    const auto type = StructType("foo",
                                 List<IString>("String", "Int", "Float"),
                                 List<IBaseObject>("foo", 10, 1.123),
                                 List<IType>(SimpleType(ctString), SimpleType(ctInt), SimpleType(ctFloat)));
    manager.addType(type);

    const auto registeredStruct1 = Struct("foo", nullptr, manager);
    const auto registeredStruct2 = Struct("foo", Dict<IString, IBaseObject>({{"Float", 5.432}}), manager);

    ASSERT_EQ(registeredStruct1.getFieldValues(), type.getFieldDefaultValues());

    ASSERT_EQ(registeredStruct2.get("Float"), 5.432);
    ASSERT_EQ(registeredStruct2.get("String"), "foo");
    ASSERT_EQ(registeredStruct2.get("Int"), 10);
}

TEST_F(StructObjectTest, RegisteredStructNested)
{
    const auto manager = TypeManager();
    const auto nestedType = StructType("foo",
                                 List<IString>("String", "Int", "Float"),
                                 List<IBaseObject>("foo", 10, 1.123),
                                 List<IType>(SimpleType(ctString), SimpleType(ctInt), SimpleType(ctFloat)));
    const auto type = StructType("bar", List<IString>("Ratio", "custom"), List<IType>(RatioStructType(), nestedType));

    manager.addType(nestedType);
    manager.addType(type);

    const auto nestedStruct = Struct("bar",
                                     Dict<IString, IBaseObject>({{"Ratio", Ratio(1, 2)}, {"custom", Struct("foo", nullptr, manager)}}),
                                     manager);

    const auto innerStruct = Struct("foo", nullptr, manager);
    ASSERT_EQ(nestedStruct.get("Ratio"), Ratio(1,2));
    ASSERT_EQ(nestedStruct.get("custom"), innerStruct);
}

TEST_F(StructObjectTest, RegisteredStructInvalidNames)
{
    const auto manager = TypeManager();
    const auto type = StructType("foo",
                                 List<IString>("String", "Int", "Float"),
                                 List<IBaseObject>("foo", 10, 1.123),
                                 List<IType>(SimpleType(ctString), SimpleType(ctInt), SimpleType(ctFloat)));

    manager.addType(type);
    ASSERT_THROW(Struct("foo", Dict<IString, IBaseObject>({{"String", "foo"}, {"Integer", 10}, {"Float", 1.123}}), manager),
                 InvalidParameterException);
}

TEST_F(StructObjectTest, RegisteredStructInvalidTypes)
{
    const auto manager = TypeManager();
    const auto type = StructType("foo",
                                 List<IString>("String", "Int", "Float"),
                                 List<IBaseObject>("foo", 10, 1.123),
                                 List<IType>(SimpleType(ctString), SimpleType(ctInt), SimpleType(ctFloat)));
    
    manager.addType(type);
    ASSERT_THROW(Struct("foo", Dict<IString, IBaseObject>({{"String", "foo"}, {"Int", 10}, {"Float", "invalid"}}), manager),
                 InvalidParameterException);
}

TEST_F(StructObjectTest, RegisteredStructNoDefaultValues)
{
    const auto manager = TypeManager();
    const auto type = StructType("foo",
                                 List<IString>("String", "Int", "Float"),
                                 List<IType>(SimpleType(ctString), SimpleType(ctInt), SimpleType(ctFloat)));
    
    manager.addType(type);
    const auto nullStruct = Struct("foo", nullptr, manager);

    ASSERT_EQ(nullStruct.getFieldValues(), List<IBaseObject>(nullptr, nullptr, nullptr));
}

TEST_F(StructObjectTest, UndefinedMemberType)
{
    const auto manager = TypeManager();
    const auto type = StructType("foo",
                                 List<IString>("String", "Int", "Float"),
                                 List<IType>(SimpleType(ctUndefined), SimpleType(ctUndefined), SimpleType(ctUndefined)));

    manager.addType(type);
    ASSERT_NO_THROW(Struct("foo", Dict<IString, IBaseObject>({{"String", 10}, {"Int", "foo"}, {"Float", "bar"}}), manager));
}

TEST_F(StructObjectTest, SimpleStructSerialization)
{
    const auto manager = TypeManager();

    const auto structMembers = Dict<IString, IBaseObject>({{"String", "bar"}, {"Integer", 10}, {"Float", 5.123}});
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
    const auto innerStruct = Struct("bar", Dict<IString, IBaseObject>({{"Float", 5.123}}), manager);
    const auto structMembers = Dict<IString, IBaseObject>({{"String", "bar"}, {"Integer", 10}, {"Struct", innerStruct}, {"Ratio", Ratio(10, 5)}});
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
    const auto innerStruct = Struct("bar", Dict<IString, IBaseObject>({{"Float", 5.123}}), manager);
    const auto structMembers = Dict<IString, IBaseObject>({{"String", "bar"}, {"Integer", 10}, {"Struct", innerStruct}, {"Ratio", Ratio(10, 5)}});
    const auto nestedStruct = Struct("foo", structMembers, manager);

    const auto serializer = JsonSerializer();
    
    nestedStruct.serialize(serializer);
    const auto serializedJson = serializer.getOutput();

    const auto emptyManager = TypeManager();
    const auto deserializer = JsonDeserializer();
    const StructPtr nestedStructDeserialized = deserializer.deserialize(serializedJson, emptyManager);

    ASSERT_EQ(nestedStruct, nestedStructDeserialized);
}

static constexpr auto INTERFACE_ID = FromTemplatedTypeName("IStruct", "daq");

TEST_F(StructObjectTest, InterfaceId)
{
    ASSERT_EQ(INTERFACE_ID, IStruct::Id);
}

TEST_F(StructObjectTest, InterfaceIdString)
{
    ASSERT_EQ(daqInterfaceIdString<IStruct>(), "{2B9F7790-512A-591E-86AC-886E9DE68A52}");
}

static constexpr auto STRUCT_TYPE_ID = FromTemplatedTypeName("IStructType", "daq");

TEST_F(StructObjectTest, StructTypeId)
{
    ASSERT_EQ(STRUCT_TYPE_ID, IStructType::Id);
}

TEST_F(StructObjectTest, StructTypeString)
{
    ASSERT_EQ(daqInterfaceIdString<IStructType>(), "{2AC3D9FA-7059-5BEF-8439-351258DDBE72}");
}

TEST_F(StructObjectTest, BuilderTest)
{
    const auto manager = TypeManager();
    const auto type = StructType("foo",
                                 List<IString>("String", "Int", "Float"),
                                 List<IType>(SimpleType(ctString), SimpleType(ctInt), SimpleType(ctFloat)));
    manager.addType(type);

    const auto builder = StructBuilder("foo", manager).set("String", "foo").set("Int", 10).set("Float", 1.123);
    const auto registeredStruct1 = Struct("foo", Dict<IString, IBaseObject>({{"String", "foo"}, {"Int", 10}, {"Float", 1.123}}), manager);
    const auto built = builder.build();
    ASSERT_EQ(built, registeredStruct1);
}

TEST_F(StructObjectTest, BuilderFromStructTest1)
{
    const auto manager = TypeManager();
    const auto type = StructType("foo",
                                 List<IString>("String", "Int", "Float"),
                                 List<IType>(SimpleType(ctString), SimpleType(ctInt), SimpleType(ctFloat)));
    manager.addType(type);
    
    const auto registeredStruct1 = Struct("foo", Dict<IString, IBaseObject>({{"String", "foo"}, {"Int", 10}, {"Float", 1.123}}), manager);
    const auto builder = StructBuilder(registeredStruct1);

    ASSERT_EQ(builder.build(), registeredStruct1);
}

TEST_F(StructObjectTest, BuilderFromStructTest2)
{
    const auto manager = TypeManager();
    const auto type = StructType("foo",
                                 List<IString>("String", "Int", "Float"),
                                 List<IType>(SimpleType(ctString), SimpleType(ctInt), SimpleType(ctFloat)));
    manager.addType(type);
    
    const auto registeredStruct1 = Struct("foo", Dict<IString, IBaseObject>({{"String", "foo"}, {"Int", 10}, {"Float", 1.123}}), manager);
    const auto builder = StructBuilder(registeredStruct1).set("String", "foo1").set("Int", 15);
    const auto built = builder.build();

    ASSERT_NE(built, registeredStruct1);
    ASSERT_EQ(built.get("String"), "foo1");
    ASSERT_EQ(built.get("Int"), 15);
}

TEST_F(StructObjectTest, BuilderTestNull)
{
    const auto manager = TypeManager();
    const auto type = StructType("foo",
                                 List<IString>("String", "Int", "Float"),
                                 List<IType>(SimpleType(ctString), SimpleType(ctInt), SimpleType(ctFloat)));
    manager.addType(type);
    
    const auto builder1 = StructBuilder("foo", manager).set("String", "foo").set("Int", 10).set("Float", nullptr);
    ASSERT_EQ(builder1.build().get("Float"), nullptr);
    const auto builder2 = StructBuilder("foo", manager).set("String", "foo").set("Int", 10);
    ASSERT_EQ(builder2.build().get("Float"), nullptr);
}

TEST_F(StructObjectTest, BuilderTestInvalidTypes)
{
    const auto manager = TypeManager();
    const auto type = StructType("foo",
                                 List<IString>("String", "Int", "Float"),
                                 List<IType>(SimpleType(ctString), SimpleType(ctInt), SimpleType(ctFloat)));
    manager.addType(type);
    
    ASSERT_THROW(StructBuilder("foo", manager).set("String", 10), InvalidParameterException);
}

TEST_F(StructObjectTest, BuilderTestDefaults)
{
    const auto manager = TypeManager();
    const auto type = StructType("foo",
                                 List<IString>("String", "Int", "Float"),
                                 List<IBaseObject>("foo", nullptr, 5.123),
                                 List<IType>(SimpleType(ctString), SimpleType(ctInt), SimpleType(ctFloat)));
    manager.addType(type);

    const auto builder = StructBuilder("foo", manager);
    ASSERT_EQ(builder.get("String"), "foo");
    ASSERT_EQ(builder.get("Int"), nullptr);

    const auto struct_ = builder.build();

    ASSERT_EQ(struct_.get("String"), "foo");
    ASSERT_EQ(struct_.get("Int"), nullptr);
}

TEST_F(StructObjectTest, BuilderGetters)
{
    const auto manager = TypeManager();
    const auto type = StructType("foo",
                                 List<IString>("String", "Int", "Float"),
                                 List<IBaseObject>("foo", nullptr, 5.123),
                                 List<IType>(SimpleType(ctString), SimpleType(ctInt), SimpleType(ctFloat)));
    manager.addType(type);

    const auto builder = StructBuilder("foo", manager);
    ASSERT_EQ(builder.get("String"), "foo");
    ASSERT_EQ(builder.getFieldNames(), List<IString>("String", "Int", "Float"));
    const auto dict = Dict<IString, IBaseObject>({{"String", "foo"}, {"Int", nullptr}, {"Float", 5.123}} );
    ASSERT_EQ(builder.getAsDictionary().getKeyList(), dict.getKeyList());
    ASSERT_EQ(builder.getAsDictionary().getValueList(), dict.getValueList());
    ASSERT_EQ(builder.getFieldValues(), List<IBaseObject>("foo", nullptr, 5.123));
    ASSERT_EQ(builder.getStructType(), type);
}

TEST_F(StructObjectTest, NestedStructBuilder)
{
    const auto manager = TypeManager();
    const auto nestedType = StructType("foo",
                                 List<IString>("String", "Int", "Float"),
                                 List<IBaseObject>("foo", 10, 1.123),
                                 List<IType>(SimpleType(ctString), SimpleType(ctInt), SimpleType(ctFloat)));
    const auto type = StructType("bar", List<IString>("Ratio", "custom"), List<IType>(RatioStructType(), nestedType));

    manager.addType(nestedType);
    manager.addType(type);

    const auto nestedStruct = Struct("bar",
                                     Dict<IString, IBaseObject>({{"Ratio", Ratio(1, 2)}, {"custom", Struct("foo", nullptr, manager)}}),
                                     manager);
    const auto builtStruct = StructBuilder(nestedStruct).build();

    ASSERT_EQ(nestedStruct, builtStruct);
}

TEST_F(StructObjectTest, PrintTrackedObjectWithoutDeadlock)
{
    if (!daqIsTrackingObjects())
        GTEST_SKIP() << "The test has no meaning if object tracking is disabled";

    const auto manager = TypeManager();

    const auto structMembers = Dict<IString, IBaseObject>({{"String", "bar"}, {"Integer", 10}});
    const auto simpleStruct = Struct("foo", structMembers, manager);

    daqPrintTrackedObjects();  // Struct::ToString should not create any new objects (deadlock)
}
