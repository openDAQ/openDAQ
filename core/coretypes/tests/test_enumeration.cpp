#include <gtest/gtest.h>
#include <coretypes/coretypes.h>

using namespace daq;

using EnumerationTypeTest = testing::Test;

TEST_F(EnumerationTypeTest, InvalidParameters)
{
    ASSERT_THROW(EnumerationTypeWithValues("enumType", Dict<IString, IInteger>()), InvalidParameterException);
    ASSERT_THROW(EnumerationType("enumType", List<IString>()), InvalidParameterException);

    ASSERT_THROW(EnumerationType("enumType", List<IString>("zero", "zero")), InvalidParameterException);
}

TEST_F(EnumerationTypeTest, Basic)
{
    auto enumerationType1 = EnumerationType("enumType", List<IString>("zero", "one", "two"));
    auto enumerationType2 = EnumerationTypeWithValues("enumType", Dict<IString, IInteger>({{"zero", 0}, {"one", 1}}));

    ASSERT_EQ(enumerationType1.getCoreType(), ctEnumeration);
    ASSERT_EQ(enumerationType2.getCoreType(), ctEnumeration);

    ASSERT_EQ(enumerationType1.getCount(), 3u);
    ASSERT_EQ(enumerationType2.getCount(), 2u);

    ASSERT_EQ(enumerationType1.getAsDictionary().getValueList(), List<IInteger>(0, 1, 2));
    ASSERT_EQ(enumerationType2.getAsDictionary().getValueList(), List<IInteger>(0, 1));

    ASSERT_EQ(enumerationType1.getEnumeratorNames(), List<IString>("zero", "one", "two"));
    ASSERT_EQ(enumerationType2.getEnumeratorNames(), List<IString>("zero", "one"));

    ASSERT_EQ(enumerationType1.getEnumeratorIntValue("zero"), 0);
    ASSERT_EQ(enumerationType2.getEnumeratorIntValue("zero"), 0);

    ASSERT_THROW(enumerationType1.getEnumeratorIntValue("foo"), NotFoundException);
    ASSERT_THROW(enumerationType2.getEnumeratorIntValue("foo"), NotFoundException);
}

TEST_F(EnumerationTypeTest, Equality)
{
    auto enumerationType1 = EnumerationType("enumType", List<IString>("zero", "one"));
    auto enumerationType2 = EnumerationTypeWithValues("enumType", Dict<IString, IInteger>({{"zero", 0}, {"one", 1}}));
    auto enumerationType3 = EnumerationType("enumTypeOther", List<IString>("zero", "one"));
    auto enumerationType4 = EnumerationType("enumType", List<IString>("zero", "one", "two"));

    Bool eq{false};
    enumerationType1->equals(enumerationType2, &eq);
    ASSERT_TRUE(eq);

    enumerationType1->equals(enumerationType3, &eq);
    ASSERT_FALSE(eq);

    enumerationType1->equals(enumerationType4, &eq);
    ASSERT_FALSE(eq);
}

TEST_F(EnumerationTypeTest, Serialization)
{
    auto enumerationType = EnumerationType("enumType", List<IString>("zero", "one"));

    const auto serializer = JsonSerializer();

    enumerationType.serialize(serializer);
    const auto serializedJson = serializer.getOutput();

    const auto deserializer = JsonDeserializer();
    const EnumerationTypePtr enumerationTypeDeserialized = deserializer.deserialize(serializedJson);

    ASSERT_EQ(enumerationType, enumerationTypeDeserialized);
}

static constexpr auto ENUMERATION_TYPE_ID = FromTemplatedTypeName("IEnumerationType", "daq");

TEST_F(EnumerationTypeTest, EnumerationTypeId)
{
    ASSERT_EQ(ENUMERATION_TYPE_ID, IEnumerationType::Id);
}

TEST_F(EnumerationTypeTest, EnumerationTypeString)
{
    ASSERT_EQ(daqInterfaceIdString<IEnumerationType>(), "{9C90D219-2A56-50D6-8767-DC866826ED56}");
}

using EnumerationObjectTest = testing::Test;

TEST_F(EnumerationObjectTest, Basic)
{
    auto manager = TypeManager();

    auto enumerationType1 = EnumerationType("enumType1", List<IString>("zero", "one"));
    auto enumerationType2 = EnumerationTypeWithValues("enumType2", Dict<IString, IInteger>({{"zero", 0}, {"one", 1}}));

    manager.addType(enumerationType1);
    manager.addType(enumerationType2);

    auto enumerationObj1 = Enumeration("enumType1", "one", manager);
    ASSERT_EQ(enumerationObj1.getValue(), "one");

    auto enumerationObj2 = Enumeration("enumType2", "one", manager);
    ASSERT_EQ(enumerationObj2.getValue(), "one");

    ASSERT_THROW(Enumeration("enumTypeOther", "one", manager), InvalidParameterException);
    ASSERT_THROW(Enumeration("enumType1", "foo", manager), InvalidParameterException);
}

TEST_F(EnumerationObjectTest, Equality)
{
    auto manager = TypeManager();

    auto enumerationType1 = EnumerationType("enumType1", List<IString>("zero", "one"));
    auto enumerationType2 = EnumerationTypeWithValues("enumType2", Dict<IString, IInteger>({{"zero", 0}, {"one", 1}}));

    manager.addType(enumerationType1);
    manager.addType(enumerationType2);

    auto enumerationObj1 = Enumeration("enumType1", "one", manager);
    auto enumerationObj2 = Enumeration("enumType1", "one", manager);
    auto enumerationObj3 = Enumeration("enumType2", "one", manager);
    auto enumerationObj4 = Enumeration("enumType1", "zero", manager);

    Bool eq{false};
    enumerationObj1->equals(enumerationObj2, &eq);
    ASSERT_TRUE(eq);

    enumerationObj1->equals(enumerationObj3, &eq);
    ASSERT_FALSE(eq);

    enumerationObj1->equals(enumerationObj4, &eq);
    ASSERT_FALSE(eq);
}

TEST_F(EnumerationObjectTest, Type)
{
    auto manager = TypeManager();
    auto enumerationType = EnumerationType("enumType", List<IString>("zero", "one"));
    manager.addType(enumerationType);

    auto enumerationObj = Enumeration("enumType", "one", manager);
    enum CoreType coreType = enumerationObj.getCoreType();

    ASSERT_EQ(coreType, ctEnumeration);
    ASSERT_EQ(enumerationObj.getEnumerationType(), enumerationType);
}

TEST_F(EnumerationObjectTest, Hashing)
{
    auto manager = TypeManager();

    auto enumerationType1 = EnumerationType("enumType1", List<IString>("zero", "one"));
    auto enumerationType2 = EnumerationTypeWithValues("enumType2", Dict<IString, IInteger>({{"zero", 0}, {"one", 1}}));

    manager.addType(enumerationType1);
    manager.addType(enumerationType2);

    auto enumerationObj1 = Enumeration("enumType1", "one", manager);
    auto enumerationObj2 = Enumeration("enumType2", "one", manager);

    size_t hashCode1;
    size_t hashCode2;
    enumerationObj1->getHashCode(&hashCode1);
    ASSERT_NE(hashCode1, 0u);
    enumerationObj2->getHashCode(&hashCode2);
    ASSERT_NE(hashCode2, 0u);
    ASSERT_NE(hashCode1, hashCode2);
}

TEST_F(EnumerationObjectTest, Conversion)
{
    auto manager = TypeManager();
    auto enumerationType = EnumerationType("enumType", List<IString>("zero", "one"));
    manager.addType(enumerationType);

    auto enumerationObj = Enumeration("enumType", "one", manager);

    auto baseObj = PTR_CAST(enumerationObj, IBaseObject);
    ASSERT_EQ(enumerationObj.toString(), "one");

    auto conv = PTR_CAST(enumerationObj, IConvertible);
    Int valInt;
    ASSERT_EQ(conv->toInt(&valInt), OPENDAQ_SUCCESS);
    ASSERT_EQ(valInt, 1);

    Float valFloat;
    ASSERT_EQ(conv->toFloat(&valFloat), OPENDAQ_SUCCESS);
    ASSERT_EQ(valFloat, 1);

    Bool valBool;
    ASSERT_EQ(conv->toBool(&valBool), OPENDAQ_SUCCESS);
    ASSERT_EQ(valBool, True);
}

TEST_F(EnumerationObjectTest, Serialization)
{
    auto manager = TypeManager();
    auto enumerationType = EnumerationType("enumType", List<IString>("zero", "one"));
    manager.addType(enumerationType);

    auto enumerationObj = Enumeration("enumType", "one", manager);

    const auto serializer = JsonSerializer();

    enumerationObj.serialize(serializer);
    const auto serializedJson = serializer.getOutput();

    const auto deserializer = JsonDeserializer();
    const EnumerationPtr enumerationObjDeserialized = deserializer.deserialize(serializedJson, manager);

    ASSERT_EQ(enumerationObj, enumerationObjDeserialized);
}

TEST_F(EnumerationObjectTest, Inspectable)
{
    auto manager = TypeManager();
    auto enumerationType = EnumerationType("enumType", List<IString>("zero", "one"));
    manager.addType(enumerationType);

    auto enumerationObj = Enumeration("enumType", "one", manager);

    auto ids = enumerationObj.asPtr<IInspectable>(true).getInterfaceIds();
    ASSERT_EQ(ids[0], IEnumeration::Id);
}

static constexpr auto INTERFACE_ID = FromTemplatedTypeName("IEnumeration", "daq");

TEST_F(EnumerationObjectTest, InterfaceId)
{
    ASSERT_EQ(INTERFACE_ID, IEnumeration::Id);
}

TEST_F(EnumerationObjectTest, InterfaceIdString)
{
    ASSERT_EQ(daqInterfaceIdString<IEnumeration>(), "{5E7D128C-87ED-5FE3-9480-CCB7E7CF8F49}");
}
