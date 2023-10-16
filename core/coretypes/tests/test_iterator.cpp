#include <gtest/gtest.h>
#include <coretypes/coretypes.h>

using namespace daq;

using IteratorTest = testing::Test;

TEST_F(IteratorTest, Guid)
{
    static constexpr IntfID IteratorGuid = { 0x1B66CB09, 0x960D, 0x52DC, { { 0x82, 0x82, 0xA1, 0xE2, 0x43, 0x19, 0xD6, 0x8E } } };
    ASSERT_EQ(IIterator::Id, IteratorGuid);
}
