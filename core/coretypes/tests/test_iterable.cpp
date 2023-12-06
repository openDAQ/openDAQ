#include <coretypes/coretypes.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace daq;

using IterableTest = testing::Test;

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

TEST_F(IterableTest, Guid)
{
    static constexpr IntfID IterableGuid = {0xec09f2e5, 0x614d, 0x5780, {{0x81, 0xcb, 0xec, 0xe8, 0xec, 0xb2, 0x65, 0x5b}}};
    ASSERT_EQ(IIterable::Id, IterableGuid);
}

static constexpr auto INTERFACE_ID = FromTemplatedTypeName("IIterable", "daq");

TEST_F(IterableTest, InterfaceId)
{
    ASSERT_EQ(INTERFACE_ID, IIterable::Id);
}

TEST_F(IterableTest, InterfaceIdString)
{
    ASSERT_EQ(daqInterfaceIdString<IIterable>(), "{EC09F2E5-614D-5780-81CB-ECE8ECB2655B}");
}
