#include <gtest/gtest.h>
#include <coretypes/ratio_factory.h>
#include <coretypes/json_serializer_factory.h>
#include <coretypes/json_deserializer_factory.h>
#include <coretypes/integer_factory.h>
#include <coretypes/inspectable_ptr.h>

using RatioTest = testing::Test;

using namespace daq;

TEST_F(RatioTest, Create)
{
    ASSERT_NO_THROW(Ratio(0, 1));
}

TEST_F(RatioTest, CreateNotSimplified)
{
    auto ratio = Ratio(10, 2);
    ASSERT_EQ(ratio.getNumerator(), 10);
    ASSERT_EQ(ratio.getDenominator(), 2);
}

TEST_F(RatioTest, CreateAndSimplify)
{
    auto ratio = Ratio(10, 2).simplify();
    ASSERT_EQ(ratio.getNumerator(), 5);
    ASSERT_EQ(ratio.getDenominator(), 1);
}

TEST_F(RatioTest, CreateFromInt)
{
    RatioPtr ratio = 1000;
    ASSERT_EQ(ratio, Ratio(1000, 1));
}

TEST_F(RatioTest, CreateInvalid)
{
    ASSERT_THROW(Ratio(1, 0), InvalidParameterException);
}

TEST_F(RatioTest, GetNumerator)
{
    auto ratio = Ratio(1, 2);

    ASSERT_EQ(ratio.getNumerator(), 1);
}

TEST_F(RatioTest, GetDenominator)
{
    auto ratio = Ratio(1, 2);

    ASSERT_EQ(ratio.getDenominator(), 2);
}

TEST_F(RatioTest, ConvertFloat)
{
    auto ratio = Ratio(1, 2);
    auto floatValue = ratio.convertTo(ctFloat);

    ASSERT_DOUBLE_EQ(floatValue, 1 / 2.0);
}

TEST_F(RatioTest, ConvertInt)
{
    auto ratio = Ratio(1, 2);
    auto intValue = ratio.convertTo(ctInt);

    ASSERT_EQ(intValue, 1);
}

TEST_F(RatioTest, ConvertFromInt)
{
    auto val = Integer(1000);
    RatioPtr ratio = val.convertTo(ctRatio);

    ASSERT_EQ(ratio, Ratio(1000, 1));
}

TEST_F(RatioTest, ConvertBoolTrue)
{
    auto ratio = Ratio(1, 2);
    auto boolValue = ratio.convertTo(ctBool);

    ASSERT_TRUE(boolValue);
}

TEST_F(RatioTest, ConvertBoolFalse)
{
    auto ratio = Ratio(0, 2);
    auto boolValue = ratio.convertTo(ctBool);

    ASSERT_FALSE(boolValue);
}

TEST_F(RatioTest, Serialize)
{
    auto serializer = JsonSerializer();

    auto ratio = Ratio(1, 2);
    std::string serializeId = ratio.getSerializeId();
    std::string expectedJson = R"({"__type":")" + serializeId + R"(","num":1,"den":2})";

    ratio.serialize(serializer);
    auto serializedJson = serializer.getOutput();

    ASSERT_EQ(serializedJson, expectedJson);
}

TEST_F(RatioTest, Deserialize)
{
    auto serializer = JsonSerializer();
    auto deserializer = JsonDeserializer();
    auto ratio = Ratio(1, 2);

    ratio.serialize(serializer);
    auto serializedJson = serializer.getOutput();

    RatioPtr ptr = deserializer.deserialize(serializedJson);

    ASSERT_EQ(ptr.getNumerator(), ratio.getNumerator());
    ASSERT_EQ(ptr.getDenominator(), ratio.getDenominator());
}

TEST_F(RatioTest, CoreType)
{
    auto ratio = Ratio(1, 1);
    enum CoreType coreType = ratio.getCoreType();

    ASSERT_EQ(coreType, ctRatio);
}

TEST_F(RatioTest, Compare)
{
    auto ratio1 = Ratio(1, 1);
    auto ratio2 = Ratio(4, 3);

    ASSERT_EQ(ratio1, ratio1);
    ASSERT_EQ(ratio2, ratio2);
    ASSERT_GT(ratio2, ratio1);
    ASSERT_LT(ratio1, ratio2);
}

TEST_F(RatioTest, ToString)
{
    auto ratio = Ratio(121, 32);
    ASSERT_EQ(static_cast<std::string>(ratio), "121/32");
}

TEST_F(RatioTest, RatioPtrCreate)
{
    auto ratio = RatioPtr(BaseObjectPtr(10));
    ASSERT_EQ(ratio, Ratio(10, 1));

    BaseObjectPtr intObj = 20;
    ratio = RatioPtr(intObj);
    ASSERT_EQ(ratio, 20);

    BaseObjectPtr tempRatio = Ratio(30, 1);
    ratio = RatioPtr(tempRatio);
    ASSERT_EQ(ratio, 30);
}

TEST_F(RatioTest, RatioMultiplyByInt)
{
    RatioPtr positiveRatio = Ratio(3, 5);
    ASSERT_EQ(positiveRatio * Int(6), Ratio(18, 5));
    ASSERT_EQ(positiveRatio * Int(0), Ratio(0, 5));
    ASSERT_EQ(positiveRatio * Int(-15), Ratio(-45, 5));

    RatioPtr negativeRatio = Ratio(-3, 5);
    ASSERT_EQ(negativeRatio * Int(6), Ratio(-18, 5));
    ASSERT_EQ(negativeRatio * Int(0), Ratio(0, 5));
    ASSERT_EQ(negativeRatio * Int(-15), Ratio(45, 5));

    auto ratio = Ratio(4, 7) * Int(14);
    ASSERT_EQ(ratio.getNumerator(), 56);
    ASSERT_EQ(ratio.getDenominator(), 7);
}

TEST_F(RatioTest, IntMultiplyByRatio)
{
    RatioPtr positiveRatio = Ratio(3, 5);
    ASSERT_EQ(Int(6) * positiveRatio, Ratio(18, 5));
    ASSERT_EQ(Int(0) * positiveRatio, Ratio(0, 5));
    ASSERT_EQ(Int(-15) * positiveRatio, Ratio(-45, 5));

    RatioPtr negativeRatio = Ratio(-3, 5);
    ASSERT_EQ(Int(6) * negativeRatio, Ratio(-18, 5));
    ASSERT_EQ(Int(0) * negativeRatio, Ratio(0, 5));
    ASSERT_EQ(Int(-15) * negativeRatio, Ratio(45, 5));

    auto ratio = Int(14) * Ratio(4, 7);
    ASSERT_EQ(ratio.getNumerator(), 56);
    ASSERT_EQ(ratio.getDenominator(), 7);
}

TEST_F(RatioTest, RatioMultiplyByRatio)
{
    ASSERT_EQ(Ratio(3, 5) * Ratio(7, 2), Ratio(21, 10));
    ASSERT_EQ(Ratio(0, 5) * Ratio(0, 3), Ratio(0, 15));
    ASSERT_EQ(Ratio(-3, 5) * Ratio(30, 3), Ratio(-90, 15));
    ASSERT_EQ(Ratio(3, -5) * Ratio(30, 3), Ratio(90, -15));
    ASSERT_EQ(Ratio(-3, 5) * Ratio(-30, -3), Ratio(90, -15));
    ASSERT_EQ(Ratio(1000, 10) * Ratio(10, 1000), Ratio(10000, 10000));
    ASSERT_EQ(Ratio(2, 3) * Ratio(0, 2), Ratio(0, 6));

    auto ratio = Ratio(14, 3) * Ratio(4, 7);
    ASSERT_EQ(ratio.getNumerator(), 56);
    ASSERT_EQ(ratio.getDenominator(), 21);
}

#if defined(_MSC_VER)
    #pragma warning(push)
    #pragma warning(disable : 4834)
#endif

TEST_F(RatioTest, RatioDivideByInt)
{
    RatioPtr temp;

    RatioPtr positiveRatio = Ratio(3, 5);
    ASSERT_EQ(positiveRatio / Int(6), Ratio(3, 30));
    ASSERT_THROW(temp = positiveRatio / Int(0), InvalidParameterException);
    ASSERT_EQ(positiveRatio / Int(-15), Ratio(3, -75));

    RatioPtr negativeRatio = Ratio(3, -5);
    ASSERT_EQ(negativeRatio / Int(6), Ratio(3, -30));
    ASSERT_THROW(temp = negativeRatio / Int(0), InvalidParameterException);
    ASSERT_EQ(negativeRatio / Int(-15), Ratio(3, 75));

    auto ratio = Ratio(14, 3) / Int(7);
    ASSERT_EQ(ratio.getNumerator(), 14);
    ASSERT_EQ(ratio.getDenominator(), 21);
}

TEST_F(RatioTest, IntDivideByRatio)
{
    RatioPtr positiveRatio = Ratio(3, 5);
    ASSERT_EQ(Int(6) / positiveRatio, Ratio(30, 3));
    ASSERT_EQ(Int(0) / positiveRatio, Ratio(0, 3));
    ASSERT_EQ(Int(-15) / positiveRatio, Ratio(-75, 3));

    RatioPtr negativeRatio = Ratio(3, -5);
    ASSERT_EQ(Int(6) / negativeRatio, Ratio(-30, 3));
    ASSERT_EQ(Int(0) / negativeRatio, Ratio(0, 3));
    ASSERT_EQ(Int(-15) / negativeRatio, Ratio(75, 3));

    ASSERT_THROW(auto ratio = Int(10) / Ratio(0, 1), InvalidParameterException);

    auto ratio = Int(7) / Ratio(14, 3);
    ASSERT_EQ(ratio.getNumerator(), 21);
    ASSERT_EQ(ratio.getDenominator(), 14);
}

TEST_F(RatioTest, RatioDivideByRatio)
{
    RatioPtr tmp;
    ASSERT_EQ(Ratio(0, 1) / Ratio(10, 5), Ratio(0, 10));
    ASSERT_EQ(Ratio(5, 2) / Ratio(-30, 2), Ratio(10, -60));
    ASSERT_EQ(Ratio(5, -7) / Ratio(13, -21), Ratio(-105, -91));
    ASSERT_EQ(Ratio(17, 21) / SimplifiedRatio(-5, 30), Ratio(510, -105));
    ASSERT_THROW(tmp = Ratio(2, 3) / Ratio(0, 1), InvalidParameterException);

    auto ratio = Ratio(7, 6) / Ratio(14, 3);
    ASSERT_EQ(ratio.getNumerator(), 21);
    ASSERT_EQ(ratio.getDenominator(), 84);
}

TEST_F(RatioTest, ConvertToDouble)
{
    double val = Ratio(1, 2);
    ASSERT_DOUBLE_EQ(val, 0.5);
}

TEST_F(RatioTest, Equal)
{
    RatioPtr one = Ratio(1, 2);
    RatioPtr two = Ratio(1, 2);

    Bool eq{false};
    one->equals(two, &eq);
    ASSERT_TRUE(eq);

    two = Ratio(2, 4);
    one->equals(two, &eq);
    ASSERT_TRUE(eq);

    two = Ratio(3, 4);
    one->equals(two, &eq);
    ASSERT_FALSE(eq);

    one = Ratio(4, 2);
    two = Ratio(4, 2);

    one->equals(two, &eq);
    ASSERT_TRUE(eq);

    two = Ratio(8, 4);
    one->equals(two, &eq);
    ASSERT_TRUE(eq);

    two = Ratio(7, 4);
    one->equals(two, &eq);
    ASSERT_FALSE(eq);
}

#if defined(_MSC_VER)
    #pragma warning(pop)
#endif

TEST_F(RatioTest, Inspectable)
{
    auto obj = Ratio(1, 2);

    auto ids = obj.asPtr<IInspectable>(true).getInterfaceIds();
    ASSERT_EQ(ids[0], IRatio::Id);
}

TEST_F(RatioTest, ImplementationName)
{
    auto obj = Ratio(1, 2);

    StringPtr className = obj.asPtr<IInspectable>(true).getRuntimeClassName();
    ASSERT_EQ(className, "daq::RatioImpl");
}

TEST_F(RatioTest, StructType)
{
}

TEST_F(RatioTest, StructFields)
{
}

TEST_F(RatioTest, StructNames)
{
}

static constexpr auto INTERFACE_ID = FromTemplatedTypeName("IRatio", "daq");

TEST_F(RatioTest, InterfaceId)
{
    ASSERT_EQ(INTERFACE_ID, IRatio::Id);
}

TEST_F(RatioTest, InterfaceIdString)
{
    ASSERT_EQ(daqInterfaceIdString<IRatio>(), "{08D28C13-55A6-5FE5-A0F0-19A3F8707C15}");
}
