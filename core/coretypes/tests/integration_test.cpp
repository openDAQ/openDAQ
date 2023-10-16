#include <gtest/gtest.h>
#include <coretypes/coretypes.h>

using namespace daq;

using IntegrationTest = testing::Test;

TEST_F(IntegrationTest, List)
{
    auto list = List<IBaseObject>();
    list.pushBack((Int) 2);
    list.pushBack("Hello");
    list.pushBack(BaseObject());
    list.pushFront(2.0);
    list.pushFront(L"4.0");
    list.pushBack(True);

    Int num = list.popFront();
    ASSERT_EQ(num, 4);
    ASSERT_EQ(list.getCount(), 5u);

    auto obj = list.popFront();
    ASSERT_EQ(2.0, obj);
    ASSERT_EQ(obj, 2.0);

    auto obj1 = list.popBack();
    ASSERT_EQ(obj1, True);

    auto obj2 = list.getItemAt(0);
    ASSERT_EQ(obj2, (Int) 2);
}

TEST_F(IntegrationTest, Dict)
{
    auto dict = Dict<IString, IBaseObject>();
    dict.set("Name", "RangeProp");
    dict.set("Min", 3.0f);
    dict.set("Max", 6.0f);
    dict.set("Count", (Int) 3);

    Float min = dict.get("Min");
    ASSERT_DOUBLE_EQ(min, 3.0f);
    Float max = dict.get("Max");
    ASSERT_DOUBLE_EQ(max, 6.0f);
    Int count = dict.get("Count");
    ASSERT_EQ(count, 3);
    StringPtr str = dict.get("Name");
    ASSERT_EQ(str, "RangeProp");
}

TEST_F(IntegrationTest, ListIterators)
{
    auto list = List<IInteger>();

    for (const auto& it : list)
    {
        ASSERT_TRUE(it.assigned());
    }

    list.pushBack(1);
    list.pushBack(2);
    list.pushBack(3);

    Int sum = 0;

    for (const auto& num : list)
    {
        sum += Int(num);
    }

    ASSERT_EQ(sum, 6);
}

TEST_F(IntegrationTest, DictKeysIterator)
{
    auto dict = Dict<IInteger, IInteger>();

    for (const auto& it : dict.getKeys())
    {
        ASSERT_TRUE(it.assigned());
    }

    dict.set(1, 11);
    dict.set(2, 22);
    dict.set(3, 33);

    Int sum = 0;

    for (const auto& num : dict.getKeys())
    {
        sum += Int(num);
    }

    ASSERT_EQ(sum, 6);
}

TEST_F(IntegrationTest, DictValuesIterator)
{
    auto dict = Dict<IInteger, IInteger>();

    for (const auto& it : dict.getValues())
    {
        ASSERT_TRUE(it.assigned());
    }

    dict.set(1, 11);
    dict.set(2, 22);
    dict.set(3, 33);

    Int sum = 0;

    for (const auto& num : dict.getValues())
    {
        sum += Int(num);
    }

    ASSERT_EQ(sum, 66);
}

TEST_F(IntegrationTest, HashStruct)
{
    auto obj = Integer(2);
    ASSERT_EQ(std::hash<IntPtr>{}(obj), 2u);
}

TEST_F(IntegrationTest, EqualToStruct)
{
    auto obj0 = Integer(2);
    auto obj1 = Integer(2);
    auto obj2 = Integer(3);
    ASSERT_TRUE(std::equal_to<IntPtr>{}(obj0, obj1));
    ASSERT_FALSE(std::equal_to<IntPtr>{}(obj0, obj2));
}
