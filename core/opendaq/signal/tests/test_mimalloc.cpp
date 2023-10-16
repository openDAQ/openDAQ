#include <opendaq/mimalloc_allocator_factory.h>
#include <gtest/gtest.h>

using MiMallocAllocatorTest = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

TEST_F(MiMallocAllocatorTest, TestFactory)
{
    AllocatorPtr allocator;
    void* ptr = nullptr;

    ASSERT_NO_THROW(allocator = MiMallocAllocator());

    // About all we can do here is make sure the implementation doesn't
    // crash in corner cases like align=0, descriptor=nullptr, etc.

    ASSERT_NO_THROW(ptr = allocator.allocate(nullptr, 32, 8));
    ASSERT_NO_THROW(allocator.free(ptr));
    ASSERT_NO_THROW(ptr = allocator.allocate(nullptr, 32, 0));
    ASSERT_NO_THROW(allocator.free(ptr));
}

END_NAMESPACE_OPENDAQ
