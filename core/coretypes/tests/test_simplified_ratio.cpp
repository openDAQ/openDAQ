#include <gtest/gtest.h>
#include <coretypes/ratio_factory.h>
#include <coretypes/json_serializer_factory.h>
#include <coretypes/json_deserializer_factory.h>
#include <coretypes/integer_factory.h>

using SimplifiedRatioTest = testing::Test;

using namespace daq;

TEST_F(SimplifiedRatioTest, Create)
{
    ASSERT_NO_THROW(SimplifiedRatio(0, 1));
}

TEST_F(SimplifiedRatioTest, CreateAndSimplify)
{
    ASSERT_EQ(SimplifiedRatio(7, 21), Ratio(1, 3));
}

TEST_F(SimplifiedRatioTest, CreateFromInt)
{
    SimplifiedRatioPtr ratio = 1000;
    ASSERT_EQ(ratio, Ratio(1000, 1));
}

TEST_F(SimplifiedRatioTest, CreateInvalid)
{
    ASSERT_THROW(SimplifiedRatio(1, 0), InvalidParameterException);
}

TEST_F(SimplifiedRatioTest, GetNumerator)
{
    auto ratio = SimplifiedRatio(3, 6);

    ASSERT_EQ(ratio.getNumerator(), 1);
}

TEST_F(SimplifiedRatioTest, GetDenominator)
{
    auto ratio = SimplifiedRatio(3, 6);

    ASSERT_EQ(ratio.getDenominator(), 2);
}

TEST_F(SimplifiedRatioTest, ConvertFloat)
{
    auto ratio = SimplifiedRatio(3,6);
    auto floatValue = ratio.convertTo(ctFloat);

    ASSERT_DOUBLE_EQ(floatValue, 1 / 2.0);
}

TEST_F(SimplifiedRatioTest, ConvertInt)
{
    auto ratio = SimplifiedRatio(3,6);
    auto intValue = ratio.convertTo(ctInt);

    ASSERT_EQ(intValue, 1);
}

TEST_F(SimplifiedRatioTest, ConvertBoolTrue)
{
    auto ratio = SimplifiedRatio(3, 6);
    auto boolValue = ratio.convertTo(ctBool);

    ASSERT_TRUE(boolValue);
}

TEST_F(SimplifiedRatioTest, ConvertBoolFalse)
{
    auto ratio = SimplifiedRatio(0, 2);
    auto boolValue = ratio.convertTo(ctBool);

    ASSERT_FALSE(boolValue);
}

TEST_F(SimplifiedRatioTest, Serialize)
{
    auto serializer = JsonSerializer();

    auto ratio = SimplifiedRatio(21, 7);
    std::string serializeId = ratio.getSerializeId();
    std::string expectedJson = R"({"__type":")" + serializeId + R"(","num":3,"den":1})";

    ratio.serialize(serializer);
    auto serializedJson = serializer.getOutput();

    ASSERT_EQ(serializedJson, expectedJson);
}

TEST_F(SimplifiedRatioTest, Deserialize)
{
    auto serializer = JsonSerializer();
    auto deserializer = JsonDeserializer();
    auto ratio = SimplifiedRatio(40, 10);

    ratio.serialize(serializer);
    auto serializedJson = serializer.getOutput();

    RatioPtr ptr = deserializer.deserialize(serializedJson);

    ASSERT_EQ(ptr.getNumerator(), ratio.getNumerator());
    ASSERT_EQ(ptr.getDenominator(), ratio.getDenominator());
}

TEST_F(SimplifiedRatioTest, Compare)
{
    auto ratio1 = SimplifiedRatio(100, 100);
    auto ratio2 = SimplifiedRatio(12, 9);

    ASSERT_EQ(ratio1, ratio1);
    ASSERT_EQ(ratio2, ratio2);
    ASSERT_GT(ratio2, ratio1);
    ASSERT_LT(ratio1, ratio2);
}

TEST_F(SimplifiedRatioTest, ToString)
{
    auto ratio = SimplifiedRatio(242, 64);
    ASSERT_EQ(static_cast<std::string>(ratio), "121/32");
}

TEST_F(SimplifiedRatioTest, RatioPtrCreate)
{
    auto ratio = SimplifiedRatioPtr(BaseObjectPtr(10));
    ASSERT_EQ(ratio, Ratio(10, 1));

    BaseObjectPtr intObj = 20;
    ratio = SimplifiedRatioPtr(intObj);
    ASSERT_EQ(ratio, 20);

    BaseObjectPtr tempRatio = SimplifiedRatio(30, 1);
    ratio = SimplifiedRatioPtr(tempRatio);
    ASSERT_EQ(ratio, 30);
}

TEST_F(SimplifiedRatioTest, Simplify)
{
    Int num = 20, den = 2;
    simplify(num, den);
    ASSERT_EQ(num, 10);
    ASSERT_EQ(den, 1);
    num = -3, den = -15;
    simplify(num, den);
    ASSERT_EQ(num, -1);
    ASSERT_EQ(den, -5);
    num = -45, den = 5;
    simplify(num, den);
    ASSERT_EQ(num, -9);
    ASSERT_EQ(den, 1);
    num = 100, den = -10;
    simplify(num, den);
    ASSERT_EQ(num, 10);
    ASSERT_EQ(den, -1);
    num = 53, den = 12;
    simplify(num, den);
    ASSERT_EQ(num, 53);
    ASSERT_EQ(den, 12);
}

TEST_F(SimplifiedRatioTest, SimplifiedRatioMultiplyByInt)
{
    SimplifiedRatioPtr positiveRatio = SimplifiedRatio(3, 5);
    ASSERT_EQ(positiveRatio * Int(5), Ratio(3, 1));
    ASSERT_EQ(positiveRatio * Int(0), Ratio(0, 5));
    ASSERT_EQ(positiveRatio * Int(-15), Ratio(-9, 1));

    SimplifiedRatioPtr negativeRatio = SimplifiedRatio(-3, 5);
    ASSERT_EQ(negativeRatio * Int(5), Ratio(-3, 1));
    ASSERT_EQ(negativeRatio * Int(0), Ratio(0, 5));
    ASSERT_EQ(negativeRatio * Int(-15), Ratio(9, 1));

    auto ratio = SimplifiedRatio(4, 7) * Int(14);
    ASSERT_EQ(ratio.getNumerator(), 8);
    ASSERT_EQ(ratio.getDenominator(), 1);
}

TEST_F(SimplifiedRatioTest, IntMultiplyBySimplifiedRatio)
{
    SimplifiedRatioPtr positiveRatio = SimplifiedRatio(3, 5);
    ASSERT_EQ(Int(5) * positiveRatio, Ratio(3, 1));
    ASSERT_EQ(Int(0) * positiveRatio, Ratio(0, 5));
    ASSERT_EQ(Int(-15) * positiveRatio, Ratio(-9, 1));

    SimplifiedRatioPtr negativeRatio = SimplifiedRatio(-3, 5);
    ASSERT_EQ(Int(5) * negativeRatio, Ratio(-3, 1));
    ASSERT_EQ(Int(0) * negativeRatio, Ratio(0, 5));
    ASSERT_EQ(Int(-15) * negativeRatio, Ratio(9, 1));

    auto ratio = Int(14) * SimplifiedRatio(4, 7);
    ASSERT_EQ(ratio.getNumerator(), 8);
    ASSERT_EQ(ratio.getDenominator(), 1);
}

TEST_F(SimplifiedRatioTest, SimplifiedRatioMultiplyBySimplifiedRatio)
{
    ASSERT_EQ(SimplifiedRatio(3, 5) * SimplifiedRatio(7, 2), Ratio(21, 10));
    ASSERT_EQ(SimplifiedRatio(0, 5) * SimplifiedRatio(0, 3), Ratio(0, 15));
    ASSERT_EQ(SimplifiedRatio(-3, 5) * SimplifiedRatio(30, 3), Ratio(-6, 1));
    ASSERT_EQ(SimplifiedRatio(3, -5) * SimplifiedRatio(30, 3), Ratio(6, -1));
    ASSERT_EQ(SimplifiedRatio(-3, 5) * SimplifiedRatio(-30, -3), Ratio(6, -1));
    ASSERT_EQ(SimplifiedRatio(1000, 10) * SimplifiedRatio(10, 1000), Ratio(1, 1));
    ASSERT_EQ(SimplifiedRatio(2, 3) * SimplifiedRatio(0, 2), Ratio(0, 6));

    auto ratio = SimplifiedRatio(14, 3) * SimplifiedRatio(4, 7);
    ASSERT_EQ(ratio.getNumerator(), 8);
    ASSERT_EQ(ratio.getDenominator(), 3);
}

TEST_F(SimplifiedRatioTest, SimplifiedRatioMultiplyByRatio)
{
    ASSERT_EQ(SimplifiedRatio(3, 5) * Ratio(7, 2), Ratio(21, 10));
    ASSERT_EQ(SimplifiedRatio(0, 5) * Ratio(0, 3), Ratio(0, 15));
    ASSERT_EQ(SimplifiedRatio(-3, 5) * Ratio(30, 3), Ratio(-6, 1));
    ASSERT_EQ(SimplifiedRatio(3, -5) * Ratio(30, 3), Ratio(6, -1));
    ASSERT_EQ(SimplifiedRatio(-3, 5) * Ratio(-30, -3), Ratio(6, -1));
    ASSERT_EQ(SimplifiedRatio(1000, 10) * Ratio(10, 1000), Ratio(1, 1));
    ASSERT_EQ(SimplifiedRatio(2, 3) * Ratio(0, 2), Ratio(0, 6));

    auto ratio = Ratio(14, 3) * SimplifiedRatio(4, 7);
    ASSERT_EQ(ratio.getNumerator(), 8);
    ASSERT_EQ(ratio.getDenominator(), 3);
}

TEST_F(SimplifiedRatioTest, RatioMultiplyBySimplifiedRatio)
{
    ASSERT_EQ(Ratio(3, 5) * SimplifiedRatio(7, 2), Ratio(21, 10));
    ASSERT_EQ(Ratio(0, 5) * SimplifiedRatio(0, 3), Ratio(0, 15));
    ASSERT_EQ(Ratio(-3, 5) * SimplifiedRatio(30, 3), Ratio(-6, 1));
    ASSERT_EQ(Ratio(3, -5) * SimplifiedRatio(30, 3), Ratio(6, -1));
    ASSERT_EQ(Ratio(-3, 5) * SimplifiedRatio(-30, -3), Ratio(6, -1));
    ASSERT_EQ(Ratio(1000, 10) * SimplifiedRatio(10, 1000), Ratio(1, 1));
    ASSERT_EQ(Ratio(2, 3) * SimplifiedRatio(0, 2), Ratio(0, 6));

    auto ratio = SimplifiedRatio(14, 3) * Ratio(4, 7);
    ASSERT_EQ(ratio.getNumerator(), 8);
    ASSERT_EQ(ratio.getDenominator(), 3);
}

#if defined(_MSC_VER)
    #pragma warning(push)
    #pragma warning(disable : 4834)
#endif

TEST_F(SimplifiedRatioTest, SimplifiedRatioDivideByInt)
{
    SimplifiedRatioPtr tmp;
    SimplifiedRatioPtr positiveRatio = SimplifiedRatio(3, 5);
    ASSERT_EQ(positiveRatio / Int(6), Ratio(1, 10));
    ASSERT_THROW(tmp = positiveRatio / Int(0), InvalidParameterException);
    ASSERT_EQ(positiveRatio / Int(-15), Ratio(1, -25));

    SimplifiedRatioPtr negativeRatio = SimplifiedRatio(3, -5);
    ASSERT_EQ(negativeRatio / Int(6), Ratio(1, -10));
    ASSERT_THROW(tmp = negativeRatio / Int(0), InvalidParameterException);
    ASSERT_EQ(negativeRatio / Int(-15), Ratio(1, 25));

    auto ratio = SimplifiedRatio(14, 3) / Int(7);
    ASSERT_EQ(ratio.getNumerator(), 2);
    ASSERT_EQ(ratio.getDenominator(), 3);
}

TEST_F(SimplifiedRatioTest, IntDivideBySimplifiedRatio)
{
    SimplifiedRatioPtr tmp;
    SimplifiedRatioPtr positiveRatio = SimplifiedRatio(3, 5);
    ASSERT_EQ(Int(6) / positiveRatio, Ratio(10, 1));
    ASSERT_EQ(Int(0) / positiveRatio, Ratio(0, 3));
    ASSERT_EQ(Int(-15) / positiveRatio, Ratio(-25, 1));

    SimplifiedRatioPtr negativeRatio = SimplifiedRatio(3, -5);
    ASSERT_EQ(Int(6) / negativeRatio, Ratio(-10, 1));
    ASSERT_EQ(Int(0) / negativeRatio, Ratio(0, 3));
    ASSERT_EQ(Int(-15) / negativeRatio, Ratio(25, 1));

    ASSERT_THROW(tmp = Int(10) / Ratio(0, 1), InvalidParameterException);

    auto ratio = Int(7) / SimplifiedRatio(14, 3);
    ASSERT_EQ(ratio.getNumerator(), 3);
    ASSERT_EQ(ratio.getDenominator(), 2);
}

TEST_F(SimplifiedRatioTest, SimplifiedRatioDivideBySimplifiedRatio)
{
    SimplifiedRatioPtr tmp;
    ASSERT_EQ(SimplifiedRatio(0, 1) / SimplifiedRatio(10, 5), Ratio(0, 10));
    ASSERT_EQ(SimplifiedRatio(5, 2) / SimplifiedRatio(-30, 2), Ratio(1, -6));
    ASSERT_EQ(SimplifiedRatio(5, -7) / SimplifiedRatio(13, -21), Ratio(-15, -13));
    ASSERT_EQ(SimplifiedRatio(17, 21) / SimplifiedRatio(-5, 30), Ratio(170, -35));
    ASSERT_THROW(tmp = SimplifiedRatio(2, 3) / SimplifiedRatio(0, 1), InvalidParameterException);

    auto ratio = SimplifiedRatio(7, 6) / SimplifiedRatio(14, 3);
    ASSERT_EQ(ratio.getNumerator(), 1);
    ASSERT_EQ(ratio.getDenominator(), 4);
}

TEST_F(SimplifiedRatioTest, SimplifiedRatioDivideByRatio)
{
    SimplifiedRatioPtr tmp;
    ASSERT_EQ(SimplifiedRatio(0, 1) / Ratio(10, 5), Ratio(0, 10));
    ASSERT_EQ(SimplifiedRatio(5, 2) / Ratio(-30, 2), Ratio(1, -6));
    ASSERT_EQ(SimplifiedRatio(5, -7) / Ratio(13, -21), Ratio(-15, -13));
    ASSERT_EQ(SimplifiedRatio(17, 21) / Ratio(-5, 30), Ratio(170, -35));
    ASSERT_THROW(tmp = SimplifiedRatio(2, 3) / Ratio(0, 1), InvalidParameterException);

    auto ratio = SimplifiedRatio(7, 6) / Ratio(14, 3);
    ASSERT_EQ(ratio.getNumerator(), 1);
    ASSERT_EQ(ratio.getDenominator(), 4);
}

TEST_F(SimplifiedRatioTest, RatioDivideBySimplifiedRatio)
{
    SimplifiedRatioPtr tmp;
    ASSERT_EQ(Ratio(0, 1) / SimplifiedRatio(10, 5), Ratio(0, 10));
    ASSERT_EQ(Ratio(5, 2) / SimplifiedRatio(-30, 2), Ratio(1, -6));
    ASSERT_EQ(Ratio(5, -7) / SimplifiedRatio(13, -21), Ratio(-15, -13));
    ASSERT_EQ(Ratio(17, 21) / SimplifiedRatio(-5, 30), Ratio(170, -35));
    ASSERT_THROW(tmp = Ratio(2, 3) / SimplifiedRatio(0, 1), InvalidParameterException);

    auto ratio = Ratio(7, 6) / SimplifiedRatio(14, 3);
    ASSERT_EQ(ratio.getNumerator(), 1);
    ASSERT_EQ(ratio.getDenominator(), 4);
}

#if defined(_MSC_VER)
    #pragma warning(pop)
#endif
