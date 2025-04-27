#include <array>
#include <opendaq/mem_pool_allocator.h>
#include <gtest/gtest.h>

#include <numeric>

using MemPoolAllocatorTest = ::testing::Test;

using namespace daq::details;

TEST_F(MemPoolAllocatorTest, Stack)
{
    StaticMemPool<int, 16> memPool;

    std::vector<int, MemPoolAllocator<int>> vec((MemPoolAllocator<int>(memPool)));
    vec.reserve(4);

    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);
    vec.push_back(4);

    ASSERT_EQ(std::accumulate(vec.begin(), vec.end(), 0), 10);

    ASSERT_TRUE(vec.data() >= reinterpret_cast<int*>(memPool.startPtr) && vec.data() < reinterpret_cast<int*>(memPool.endPtr));
}

TEST_F(MemPoolAllocatorTest, Fallback)
{
    StaticMemPool<int, 2> memPool;

    std::vector<int, MemPoolAllocator<int>> vec((MemPoolAllocator<int>(memPool)));
    vec.reserve(4);

    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);
    vec.push_back(4);

    ASSERT_EQ(std::accumulate(vec.begin(), vec.end(), 0), 10);
    ASSERT_FALSE(vec.data() >= reinterpret_cast<int*>(memPool.startPtr) && vec.data() < reinterpret_cast<int*>(memPool.endPtr));
}
