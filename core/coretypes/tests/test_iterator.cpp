#include <gtest/gtest.h>
#include <coretypes/coretypes.h>

using namespace daq;

using IteratorTest = testing::Test;

TEST_F(IteratorTest, Guid)
{
    static constexpr IntfID IteratorGuid = { 0xf3b87158, 0xf4cd, 0x5890, { { 0x94, 0x76, 0x3c, 0xe, 0x31, 0x5c, 0x56, 0xd9 } } };
    ASSERT_EQ(IIterator::Id, IteratorGuid);
}

static constexpr auto INTERFACE_ID = FromTemplatedTypeName("IIterator", "daq");

TEST_F(IteratorTest, InterfaceId)
{
    ASSERT_EQ(INTERFACE_ID, IIterator::Id);
}

TEST_F(IteratorTest, InterfaceIdString)
{
    ASSERT_EQ(daqInterfaceIdString<IIterator>(), "{F3B87158-F4CD-5890-9476-3C0E315C56D9}");
}
