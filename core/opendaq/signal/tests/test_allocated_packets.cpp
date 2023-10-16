#include <gtest/gtest.h>
#include <opendaq/packet_factory.h>
#include <opendaq/data_descriptor_factory.h>
#include <opendaq/gmock/allocator.h>
#include <opendaq/deleter_factory.h>
#include <opendaq/sample_type_traits.h>

using namespace testing;

using AllocatedPacketsTest = Test;

BEGIN_NAMESPACE_OPENDAQ

// Helper methods

static DataDescriptorPtr setupDescriptor(SampleType sampleType)
{
    return DataDescriptorBuilder().setSampleType(sampleType).build();;
}

// Tests

TEST_F(AllocatedPacketsTest, TestDataPacket)
{
    const SampleType sampleType = SampleType::Float64;
    const std::size_t sampleSize = getSampleSize(sampleType);
    const SizeT sampleCount = 100;
    auto allocator = MockAllocator::Strict();
    DataPacketPtr packet;

    EXPECT_CALL(allocator.mock(), allocate(_, sampleCount*sampleSize, sampleSize, _));
    ASSERT_NO_THROW(packet = DataPacket(setupDescriptor(sampleType), sampleCount, 10, allocator));
    ASSERT_NE(packet.getRawData(), nullptr);

    EXPECT_CALL(allocator.mock(), free(_)).Times(AtLeast(1));
}

TEST_F(AllocatedPacketsTest, DataPacketWithExternalMemory)
{
    const SampleType sampleType = SampleType::Float64;
    const std::size_t sampleSize = getSampleSize(sampleType);
    const SizeT sampleCount = 10;
    void* data = std::malloc(sampleCount*sampleSize);

    MockFunction<void(void*)> mockCallback;
    auto packet = DataPacketWithExternalMemory(
        nullptr,
        setupDescriptor(sampleType),
        sampleCount,
        data,
        Deleter([&mockCallback](void* address) {
            mockCallback.Call(address);
            std::free(address);
        })
    );

    ASSERT_EQ(packet.getRawData(), data);

    EXPECT_CALL(mockCallback, Call(data)).Times(AtLeast(1));
}

END_NAMESPACE_OPENDAQ
