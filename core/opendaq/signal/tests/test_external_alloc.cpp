#include <opendaq/external_allocator_factory.h>
#include <opendaq/deleter_factory.h>
#include <opendaq/signal_exceptions.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace testing;
using ExternalAllocatorTest = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

TEST_F(ExternalAllocatorTest, TestFactory)
{
    auto deleter = daq::Deleter([](void* address){});
    void* ptr = std::malloc(32);

    ASSERT_NO_THROW(ExternalAllocator(ptr, deleter));

    std::free(ptr);
}

TEST_F(ExternalAllocatorTest, TestFactoryErrors)
{
    auto deleter = daq::Deleter([](void* address){});
    ASSERT_THROW(ExternalAllocator(nullptr, deleter), InvalidParameterException);

    void* ptr = std::malloc(32);
    ASSERT_THROW(ExternalAllocator(ptr, nullptr), ArgumentNullException);

    std::free(ptr);
}

TEST_F(ExternalAllocatorTest, TestAllocation)
{
    void* ptr = std::malloc(32);
    auto deleter = daq::Deleter([](void* address){});
    AllocatorPtr allocator = ExternalAllocator(ptr, deleter);
    void* allocatedPtr;

    ASSERT_NO_THROW(allocatedPtr = allocator.allocate(nullptr, 0, 0));
    ASSERT_EQ(ptr, allocatedPtr);

    std::free(ptr);
}

TEST_F(ExternalAllocatorTest, TestAllocationErrors)
{
    void* ptr = std::malloc(32);
    auto deleter = daq::Deleter([](void* address){});
    AllocatorPtr allocator = ExternalAllocator(ptr, deleter);

    ASSERT_NO_THROW(allocator.allocate(nullptr, 0, 0));
    ASSERT_THROW(allocator.allocate(nullptr, 0, 0), MemoryAllocationFailedException);

    std::free(ptr);
}

TEST_F(ExternalAllocatorTest, TestFree)
{
    void* ptr = std::malloc(32);
    MockFunction<void(void*)> mockCallback;
    auto deleter = daq::Deleter(mockCallback.AsStdFunction());

    AllocatorPtr allocator = ExternalAllocator(ptr, deleter);
    ASSERT_NO_THROW(allocator.free(nullptr));

    void* allocatedPtr = allocator.allocate(nullptr, 0, 0);
    EXPECT_CALL(mockCallback, Call(_)).Times(AtLeast(1));
    ASSERT_NO_THROW(allocator.free(allocatedPtr));

    std::free(ptr);
}

TEST_F(ExternalAllocatorTest, TestFreeErrors)
{
    void* ptr = std::malloc(32);
    auto deleter = daq::Deleter([](void* address){});

    AllocatorPtr allocator = ExternalAllocator(ptr, deleter);
    void* allocatedPtr = allocator.allocate(nullptr, 0, 0);

    void* ptrTest = std::malloc(32);
    ASSERT_THROW(allocator.free(ptrTest), MemoryDeallocationFailedException);
    std::free(ptrTest);

    ASSERT_NO_THROW(allocator.free(allocatedPtr));
    ASSERT_THROW(allocator.free(allocatedPtr), MemoryDeallocationFailedException);

    std::free(ptr);
}

END_NAMESPACE_OPENDAQ
