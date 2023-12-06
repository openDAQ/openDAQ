#include <gtest/gtest.h>
#include <coretypes/float_factory.h>
#include <coretypes/number_ptr.h>
#include <coretypes/inspectable_ptr.h>

using namespace daq;

using FloatTest = testing::Test;

TEST_F(FloatTest, Basic)
{
    auto floatObj = Floating(5.0);
    Float floatVal;
    ASSERT_TRUE(OPENDAQ_SUCCEEDED(floatObj->getValue(&floatVal)));
    ASSERT_EQ(floatVal, 5.0);
}

TEST_F(FloatTest, Equality)
{
    auto floatObj1 = Floating(3.0);
    auto floatObj2 = Floating(3.0);
    auto floatObj3 = Floating(4.0);

    Bool eq{false};
    floatObj1->equals(floatObj1, &eq);
    ASSERT_TRUE(eq);

    floatObj1->equals(floatObj2, &eq);
    ASSERT_TRUE(eq);

    floatObj1->equals(floatObj3, &eq);
    ASSERT_FALSE(eq);

    floatObj1->equalsValue(3.0, &eq);
    ASSERT_TRUE(eq);
}

TEST_F(FloatTest, Hashing)
{
    auto floatObj1 = Floating(3.0);
    auto floatObj2 = Floating(4.0);

    size_t hashCode1;
    size_t hashCode2;
    floatObj1->getHashCode(&hashCode1);
    ASSERT_NE(hashCode1, 0u);
    floatObj2->getHashCode(&hashCode2);
    ASSERT_NE(hashCode2, 0u);
    ASSERT_NE(hashCode1, hashCode2);
}

TEST_F(FloatTest, CastToPtr)
{
    auto floatObj1 = Floating(3.0);
    auto floatObj2 = PTR_CAST(floatObj1, IBaseObject);
}

TEST_F(FloatTest, Cast)
{
    auto floatObj = Floating(1);
    auto floatObjZero = Floating(0.0);

    Int valInt = floatObj;
    ASSERT_EQ(valInt, 1);

    Float valFloat = floatObj;
    ASSERT_EQ(valFloat, 1.0);

    Bool valBool = floatObj;
    ASSERT_EQ(valBool, True);

    Bool valBoolFalse = floatObjZero;
    ASSERT_EQ(valBoolFalse, False);

    std::string str = floatObj;
    ASSERT_EQ(str, "1");
}

TEST_F(FloatTest, CoreType)
{
    auto floatObj = Floating(1.0);
    enum CoreType coreType = floatObj.getCoreType();

    ASSERT_EQ(coreType, ctFloat);
}

TEST_F(FloatTest, SpecificComparison)
{
    auto floatObj = Floating(1.0);
    ASSERT_EQ(floatObj, 1.0);
}

TEST_F(FloatTest, CreateFromInt)
{
    ObjectPtr<IFloat> obj(5);
    ASSERT_EQ(obj , 5);
}

TEST_F(FloatTest, AssignFromInt)
{
    ObjectPtr<IFloat> obj = 5;
    ASSERT_EQ(obj , 5);
}

TEST_F(FloatTest, SupportsNumber)
{
    auto f = Floating(1.0);
    ASSERT_TRUE(f.supportsInterface<INumber>());
}

TEST_F(FloatTest, AssignToNumber)
{
    auto f = Floating(1.0);

    NumberPtr number = f;

    ASSERT_EQ(number.getFloatValue(), 1.0);
    ASSERT_EQ(number.getIntValue(), 1);

    Float floatNum = number;
    Int intNum = number;

    ASSERT_EQ(floatNum, 1.0);
    ASSERT_EQ(intNum, 1);

}

TEST_F(FloatTest, NumberConstructsFromFloat)
{
    NumberPtr number(1.0);
    ASSERT_EQ(number, 1.0);
}

TEST_F(FloatTest, NumberAssignedFromFloat)
{
    NumberPtr number;
    number = 1.0;
    ASSERT_EQ(number, 1.0);
}

static void numberIsOne(const NumberPtr& number)
{
    ASSERT_EQ(number, 1.0);
}

TEST_F(FloatTest, NumberAcceptsFloat)
{
    numberIsOne(1.0);
}

TEST_F(FloatTest, Inspectable)
{
    auto obj = Floating(3.14);

    auto ids = obj.asPtr<IInspectable>(true).getInterfaceIds();
    ASSERT_EQ(ids[0], IFloat::Id);
}

TEST_F(FloatTest, ImplementationName)
{
    auto obj = Floating(3.14);

    std::string className = obj.asPtr<IInspectable>(true).getRuntimeClassName();

    auto prefix = className.find("daq::NumberImpl<double,");
    ASSERT_EQ(prefix, 0u);
}

static constexpr auto INTERFACE_ID = FromTemplatedTypeName("IFloat", "daq");

TEST_F(FloatTest, InterfaceId)
{
    ASSERT_EQ(INTERFACE_ID, IFloat::Id);
}

TEST_F(FloatTest, InterfaceIdString)
{
    ASSERT_EQ(daqInterfaceIdString<IFloat>(), "{D1E14646-B7B0-5D3D-9553-BE6F1CD4F0D8}");
}
