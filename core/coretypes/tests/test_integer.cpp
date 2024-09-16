#include <gtest/gtest.h>
#include <coretypes/coretypes.h>

using namespace daq;

using IntegerTest = testing::Test;

TEST_F(IntegerTest, Basic)
{
    auto intObj = Integer(5);
    Int intVal;
    ASSERT_TRUE(OPENDAQ_SUCCEEDED(intObj->getValue(&intVal)));
    ASSERT_EQ(intVal, 5);
}

TEST_F(IntegerTest, Equality)
{
    auto intObj1 = Integer(3);
    auto intObj2 = Integer(3);
    auto intObj3 = Integer(4);

    Bool eq{false};
    intObj1->equals(intObj1, &eq);
    ASSERT_TRUE(eq);

    intObj1->equals(intObj2, &eq);
    ASSERT_TRUE(eq);

    intObj1->equals(intObj3, &eq);
    ASSERT_FALSE(eq);

    intObj1->equalsValue(3, &eq);
    ASSERT_TRUE(eq);
}

TEST_F(IntegerTest, Hashing)
{
    auto intObj1 = Integer(3);
    auto intObj2 = Integer(4);

    size_t hashCode1;
    size_t hashCode2;
    intObj1->getHashCode(&hashCode1);
    ASSERT_NE(hashCode1, 0u);
    intObj2->getHashCode(&hashCode2);
    ASSERT_NE(hashCode2, 0u);
    ASSERT_NE(hashCode1, hashCode2);
}

TEST_F(IntegerTest, CastToPtr)
{
    auto intObj1 = Integer(3);
    auto intObj2 = PTR_CAST(intObj1, IBaseObject);
}

TEST_F(IntegerTest, CastString)
{
    auto intObj = Integer(1);

    std::wstring str = intObj;
    ASSERT_EQ(str, L"1");
}

TEST_F(IntegerTest, CastInt)
{
    auto intObj = Integer(1);

    Int valInt = intObj;
    ASSERT_EQ(valInt, 1);
}

TEST_F(IntegerTest, CastFloat)
{
    auto intObj = Integer(1);

    Float valFloat = intObj;
    ASSERT_EQ(valFloat, 1.0);
}

TEST_F(IntegerTest, CastBool)
{
    auto intObj = Integer(1);

    Bool valBool = intObj;
    ASSERT_EQ(valBool, True);
}

TEST_F(IntegerTest, CastUint8_t)
{
    auto intObj = Integer(2);

    uint8_t valUint8 = intObj;
    ASSERT_EQ(valUint8, 2);
}

TEST_F(IntegerTest, CoreType)
{
    auto intObj = Integer(True);
    enum CoreType coreType = intObj.getCoreType();

    ASSERT_EQ(coreType, ctInt);
}

TEST_F(IntegerTest, HelperTraitsInt)
{
    IntegerPtr ptr = CoreTypeHelper<int>::Create(1);
    ASSERT_EQ(ptr, 1);
}

TEST_F(IntegerTest, HelperTraitsUInt)
{
    IntegerPtr ptr = CoreTypeHelper<unsigned int>::Create(1u);
    ASSERT_EQ(ptr, 1);
}

TEST_F(IntegerTest, HelperTraitsLong)
{
    IntegerPtr ptr = CoreTypeHelper<long>::Create(1);
    ASSERT_EQ(ptr, 1);
}

TEST_F(IntegerTest, HelperTraitsULong)
{
    IntegerPtr ptr = CoreTypeHelper<unsigned long>::Create(1ul);
    ASSERT_EQ(ptr, 1);
}

TEST_F(IntegerTest, HelperTraitsShort)
{
    IntegerPtr ptr = CoreTypeHelper<short>::Create(1);
    ASSERT_EQ(ptr, 1);
}

TEST_F(IntegerTest, HelperTraitsUShort)
{
    IntegerPtr ptr = CoreTypeHelper<unsigned short>::Create(1);
    ASSERT_EQ(ptr, 1);
}

TEST_F(IntegerTest, HelperTraitsEnum)
{
    IntegerPtr ptr = CoreTypeHelper<CoreType>::Create(CoreType::ctObject);
    ASSERT_EQ(ptr, CoreType::ctObject);
}

TEST_F(IntegerTest, SupportsNumber)
{
    auto i = Integer(1);
    ASSERT_TRUE(i.supportsInterface<INumber>());
}

TEST_F(IntegerTest, AssignToNumber)
{
    auto i = Integer(1);

    NumberPtr number = i;

    ASSERT_EQ(number.getFloatValue(), 1.0);
    ASSERT_EQ(number.getIntValue(), 1);

    Float floatNum = number;
    Int intNum = number;

    ASSERT_EQ(floatNum, 1.0);
    ASSERT_EQ(intNum, 1);

}

TEST_F(IntegerTest, NumberConstructsFromInt)
{
    NumberPtr number(1);
    ASSERT_EQ(number, 1);
}

TEST_F(IntegerTest, NumberAssignedFromInt)
{
    NumberPtr number;
    number = 1;
    ASSERT_EQ(number, 1);
}

static void numberIsOne(const NumberPtr& number)
{
    ASSERT_EQ(number, 1);
}

TEST_F(IntegerTest, NumberAcceptsInt)
{
    numberIsOne(1);
}

TEST_F(IntegerTest, Inspectable)
{
    auto obj = Integer(1);

    auto ids = obj.asPtr<IInspectable>(true).getInterfaceIds();
    ASSERT_EQ(ids[0], IInteger::Id);
}

TEST_F(IntegerTest, ImplementationName)
{
    auto obj = Integer(1);
    std::string className = obj.asPtr<IInspectable>(true).getRuntimeClassName();

    auto prefix = className.find("daq::NumberImpl<");
    ASSERT_EQ(prefix, 0u);
}

static constexpr auto INTERFACE_ID = FromTemplatedTypeName("IInteger", "daq");

TEST_F(IntegerTest, InterfaceId)
{
    ASSERT_EQ(INTERFACE_ID, IInteger::Id);
}

TEST_F(IntegerTest, InterfaceIdString)
{
    ASSERT_EQ(daqInterfaceIdString<IInteger>(), "{B5C52F78-45F9-5C54-9BC1-CA65A46472CB}");
}
