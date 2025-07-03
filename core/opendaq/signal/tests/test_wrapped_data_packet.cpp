#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>
#include <opendaq/data_rule_factory.h>
#include <opendaq/wrapped_data_packet_factory.h>
#include <opendaq/data_descriptor_factory.h>
#include <opendaq/packet_factory.h>
#include <opendaq/scaling_factory.h>

using namespace daq;

using WrappedDataPacketTest = ::testing::Test;

TEST_F(WrappedDataPacketTest, Packet)
{
    const auto domainDescriptor = DataDescriptorBuilder()
        .setSampleType(SampleType::Int32)
        .setRule(LinearDataRule(1, 0))
        .setTickResolution(Ratio(1, 1000))
        .build();

    const auto sourceValueDescriptor = DataDescriptorBuilder()
        .setSampleType(SampleType::Float32)
        .setPostScaling(LinearScaling(1.0, 0.0, SampleType::Int32, ScaledSampleType::Float32))
        .build();

    constexpr size_t sampleCount = 4;

    auto sourcePacket = DataPacket(sourceValueDescriptor, sampleCount);
    auto sourceRawData = static_cast<int32_t*>(sourcePacket.getRawData());
    for (size_t i = 0; i < sampleCount; i++)
        *sourceRawData++ = static_cast<int32_t>(i);

    auto sourceData = static_cast<float*>(sourcePacket.getData());
    for (size_t i = 0; i < sampleCount; i++)
        ASSERT_FLOAT_EQ(*sourceData++, static_cast<float>(i));

    const auto wrappedValueDescriptor = DataDescriptorBuilder()
        .setSampleType(SampleType::Float32)
        .setPostScaling(LinearScaling(2.0, 1.0, SampleType::Int32, ScaledSampleType::Float32))
        .build();

    auto wrappedPacket = WrappedDataPacket(sourcePacket, wrappedValueDescriptor);

    auto wrappedData = static_cast<float*>(wrappedPacket.getData());
    for (size_t i = 0; i < sampleCount; i++)
        ASSERT_FLOAT_EQ(*wrappedData++, 2.0f * static_cast<float>(i) + 1.0f);
}
