#include <coretypes/coretypes.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace daq;

using IterableTest = testing::Test;

TEST_F(IterableTest, Guid)
{
    static constexpr IntfID IterableGuid = { 0x2B517416, 0x7E97, 0x560F, { { 0xB5, 0x45, 0x4F, 0x2D, 0x61, 0xF5, 0xAB, 0xAA } } };
    ASSERT_EQ(IIterable::Id, IterableGuid);
}

TEST_F(IterableTest, IterateList)
{
    IterablePtr<IInteger> iter = List<Int>(1, 2, 3, 4);
    for (auto element : iter)
    {
        std::cout << element << std::endl;
    }
}

TEST_F(IterableTest, IterateListDirect)
{
    auto list = List<Int>(1, 2, 3, 4);
    for (auto element : list)
    {
        std::cout << element << std::endl;
    }

    ASSERT_THAT(list, testing::ElementsAre(1, 2, 3, 4));
}

TEST_F(IterableTest, IterateDict)
{
    auto dict = Dict<IString, IInteger>();
    dict.set("testing1", 1);
    dict.set("testing2", 2);
    dict.set("testing3", 3);
    dict.set("testing4", 4);

    IterablePtr<IList> iter = dict;
    for (auto element : iter)
    {
        std::cout << element << std::endl;
    }
}

TEST_F(IterableTest, IterateDictDirect)
{
    auto dict = Dict<IString, IInteger>();
    dict.set("testing1", 1);
    dict.set("testing2", 2);
    dict.set("testing3", 3);
    dict.set("testing4", 4);

    for (auto pair : dict)
    {
        std::cout << pair.first << ", " << pair.second << std::endl;
    }

    ASSERT_THAT(dict, (testing::ElementsAre<std::pair<StringPtr, IntegerPtr>>(
        std::make_pair("testing1", 1),
        std::make_pair("testing2", 2),
        std::make_pair("testing3", 3),
        std::make_pair("testing4", 4)
    )));
}
