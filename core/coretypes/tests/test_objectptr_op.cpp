#include <gtest/gtest.h>
#include <coretypes/coretypes.h>

using namespace daq;

using ObjectPtrOpTest = testing::Test;

TEST_F(ObjectPtrOpTest, IntSum)
{
    Int a = Integer(1) + 2;
    ASSERT_EQ(3, a);

    a = Integer(1) + Integer(2);
    ASSERT_EQ(3, a);

    a = Integer(1) + Floating(2.0);
    ASSERT_EQ(3, a);

    a = Integer(1) + BaseObjectPtr(2);
    ASSERT_EQ(3, a);

    a = BaseObjectPtr(1) + BaseObjectPtr(2);
    ASSERT_EQ(3, a);

    a = BaseObjectPtr(1) + 2;
    ASSERT_EQ(3, a);
}

TEST_F(ObjectPtrOpTest, IntSub)
{
    Int a = Integer(2) - 1;
    ASSERT_EQ(1, a);

    a = Integer(2) - Integer(1);
    ASSERT_EQ(1, a);

    a = Integer(2) - Floating(1.0);
    ASSERT_EQ(1, a);

    a = Integer(2) - BaseObjectPtr(1);
    ASSERT_EQ(1, a);

    a = BaseObjectPtr(2) - BaseObjectPtr(1);
    ASSERT_EQ(1, a);
}

TEST_F(ObjectPtrOpTest, IntUnaryMinus)
{
    Int a = -Integer(2);
    ASSERT_EQ(-2, a);

    a = -BaseObjectPtr(2);
    ASSERT_EQ(-2, a);
}

TEST_F(ObjectPtrOpTest, IntMul)
{
    Int a = Integer(2) * 3;
    ASSERT_EQ(6, a);

    a = Integer(2) * Integer(3);
    ASSERT_EQ(6, a);

    a = Integer(2) * Floating(3.0);
    ASSERT_EQ(6, a);

    a = Integer(2) * BaseObjectPtr(3.0);
    ASSERT_EQ(6, a);

    a = BaseObjectPtr(2) * BaseObjectPtr(3.0);
    ASSERT_EQ(6, a);
}

TEST_F(ObjectPtrOpTest, IntDiv)
{
    Int a = Integer(6) / 3;
    ASSERT_EQ(2, a);

    a = Integer(6) / Integer(3);
    ASSERT_EQ(2, a);

    a = Integer(6) / Floating(3.0);
    ASSERT_EQ(2, a);

    a = Integer(6) / BaseObjectPtr(3.0);
    ASSERT_EQ(2, a);

    a = BaseObjectPtr(6) / BaseObjectPtr(3.0);
    ASSERT_EQ(2, a);
}

TEST_F(ObjectPtrOpTest, FloatMul)
{
    Int a = Floating(2.0) * 3.0;
    ASSERT_EQ(6.0, a);

    a = Floating(2.0) * Floating(3);
    ASSERT_EQ(6.0, a);

    a = Floating(2.0) * Integer(3);
    ASSERT_EQ(6.0, a);

    a = Floating(2.0) * BaseObjectPtr(3.0);
    ASSERT_EQ(6.0, a);

    a = BaseObjectPtr(2.0) * BaseObjectPtr(3.0);
    ASSERT_EQ(6.0, a);
}

TEST_F(ObjectPtrOpTest, LogicalOr)
{
    Bool a = Boolean(True) || False;
    ASSERT_EQ(True, a);

    a = Boolean(False) || False;
    ASSERT_EQ(False, a);

    a = Integer(False) || 3;
    ASSERT_EQ(True, a);

    a = Boolean(True) || Boolean(False);
    ASSERT_EQ(True, a);

    a = Boolean(True) || BaseObjectPtr(False);
    ASSERT_EQ(True, a);

    a = BaseObjectPtr(True) || BaseObjectPtr(False);
    ASSERT_EQ(True, a);
}

TEST_F(ObjectPtrOpTest, LogicalAnd)
{
    Bool a = Boolean(True) && False;
    ASSERT_EQ(False, a);

    a = Boolean(True) && True;
    ASSERT_EQ(True, a);

    a = Integer(True) && 3;
    ASSERT_EQ(True, a);

    a = Boolean(True) && BaseObjectPtr(False);
    ASSERT_EQ(False, a);

    a = BaseObjectPtr(True) && BaseObjectPtr(False);
    ASSERT_EQ(False, a);
}

TEST_F(ObjectPtrOpTest, sumList)
{
    auto list1 = List<IInteger>(1, 2, 3);
    auto list2 = List<IInteger>(3, 2, 1);

    ListPtr<IInteger> resultList = BaseObjectPtr(list1) + BaseObjectPtr(list2);
    Int sum = 0;
    for (const auto& e : resultList)
        sum = sum + static_cast<Int>(e);
    ASSERT_EQ(12, sum);
}

TEST_F(ObjectPtrOpTest, AddScalarToList)
{
    auto list = List<IInteger>(1, 2, 3);
    ListPtr<IInteger> resultList = BaseObjectPtr(list) + BaseObjectPtr(1);

    Int sum = 0;
    for (const auto& e : resultList)
    {
        sum = sum + static_cast<Int>(e);
    }
    ASSERT_EQ(9, sum);

    resultList = BaseObjectPtr(list) + 1;
    sum = 0;
    for (const auto& e : resultList)
    {
        sum = sum + static_cast<Int>(e);
    }
    ASSERT_EQ(9, sum);
}
