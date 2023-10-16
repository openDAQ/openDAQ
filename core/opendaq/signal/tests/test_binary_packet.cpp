#include <opendaq/binary_data_packet_factory.h>
#include <opendaq/data_descriptor_factory.h>
#include <opendaq/deleter_factory.h>
#include <gtest/gtest.h>

using BinaryPacketTest = testing::Test;

static daq::DataDescriptorPtr setupDescriptor(daq::SampleType sampleType = daq::SampleType::Binary)
{
    return daq::DataDescriptorBuilder().setSampleType(sampleType).build();
}

// Tests

TEST_F(BinaryPacketTest, TestConstructorErrors)
{
    ASSERT_THROW(daq::BinaryDataPacket(nullptr, nullptr, 0), daq::ArgumentNullException);
    ASSERT_THROW(daq::BinaryDataPacket(nullptr, setupDescriptor(daq::SampleType::Float32), 0), daq::InvalidParameterException);
}

TEST_F(BinaryPacketTest, TestConstructorErrorsExternalMemory)
{
    ASSERT_THROW(
        daq::BinaryDataPacketWithExternalMemory(nullptr, setupDescriptor(daq::SampleType::Float32), 0, nullptr, daq::Deleter([](void*) {})),
        daq::InvalidParameterException);
}

TEST_F(BinaryPacketTest, DataPacketTestGetters)
{
    auto desc = setupDescriptor();
    auto packet = daq::BinaryDataPacket(nullptr, desc, 16);

    ASSERT_EQ(packet.getType(), daq::PacketType::Data);
    ASSERT_EQ(packet.getDataDescriptor(), desc);
    ASSERT_EQ(packet.getSampleCount(), 1u);
    ASSERT_EQ(packet.getOffset(), 0u);
    ASSERT_NE(packet.getData(), nullptr);
    ASSERT_NE(packet.getRawData(), nullptr);
    ASSERT_EQ(packet.getSampleMemSize(), 16u);
}

TEST_F(BinaryPacketTest, DataPacketTestGettersExternalMemory)
{
    auto desc = setupDescriptor();
    void* data = std::malloc(16);
    auto packet = daq::BinaryDataPacketWithExternalMemory(
        nullptr, desc, 16, data, daq::Deleter([](void* address) { std::free(address); }));

    ASSERT_EQ(packet.getType(), daq::PacketType::Data);
    ASSERT_EQ(packet.getDataDescriptor(), desc);
    ASSERT_EQ(packet.getSampleCount(), 1u);
    ASSERT_EQ(packet.getOffset(), 0u);
    ASSERT_EQ(packet.getData(), data);
    ASSERT_EQ(packet.getRawData(), data);
    ASSERT_EQ(packet.getSampleMemSize(), 16u);
}
