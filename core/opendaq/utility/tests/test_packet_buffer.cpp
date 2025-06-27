#include <gtest/gtest.h>
#include <opendaq/packet_buffer_factory.h>
#include <opendaq/packet_buffer_builder_factory.h>
#include <opendaq/data_descriptor_factory.h>
#include <opendaq/data_rule_factory.h>
#include <opendaq/dimension_factory.h>
#include <opendaq/data_rule_calc_private.h>
#include <opendaq/packet_factory.h>
#include <opendaq/reusable_data_packet_ptr.h>
#include <opendaq/sample_type_traits.h>

using namespace daq;

using PacketBufferTest = testing::Test;

std::tuple<daq::DataDescriptorPtr, daq::DataPacketPtr> generate_building_blocks()
{
    auto dimensions = daq::List<daq::IDimension>();
    dimensions.pushBack(daq::Dimension(daq::LinearDimensionRule(10, 10, 10)));

    auto rule = daq::DataRule(daq::DataRuleType::Explicit, {});

    auto descriptor = daq::DataDescriptorBuilder()
                          .setRule(rule)
                          .setSampleType(daq::SampleType::Int8)
                          .setDimensions(dimensions)
                          .setUnit(daq::Unit("s", 10))
                          .build();
    daq::DataPacketPtr domain = daq::DataPacket(descriptor, 100, 0);
    return {descriptor, domain};
}

TEST_F(PacketBufferTest, SanityCheck)
{
    PacketBufferBuilderPtr builder = PacketBufferBuilder();
    PacketBufferPtr buffer = PacketBuffer(builder);
    SizeT mem;

    buffer->getAvailableMemory(&mem);
    
    ASSERT_TRUE(mem == 100);
}

TEST_F(PacketBufferTest, makeAPacket)
{
    auto builder = PacketBufferBuilder();
    auto buffer = PacketBuffer(builder);

    auto [desc, domain] = generate_building_blocks();
    
    SizeT mem = 10;

    DataPacketPtr destination;

    auto errCode = buffer->createPacket(mem, desc, domain, &destination);

    ASSERT_TRUE(OPENDAQ_SUCCEEDED(errCode));
}

TEST_F(PacketBufferTest, fullRange)
{
    auto builder = PacketBufferBuilder();
    builder->setSizeInBytes(1000);
    auto buffer = PacketBuffer(builder);

    auto [desc, domain] = generate_building_blocks();

    SizeT mem = 1000;

    DataPacketPtr destination[2];

    buffer->createPacket(mem, desc, domain, &destination[0]);

    //auto errCode2 = buffer->createPacket(10000, desc, domain, &destination[1]);

    ASSERT_THROW(buffer->createPacket(100000, desc, domain, &destination[1]), InvalidParameterException);
}

TEST_F(PacketBufferTest, writeAhead)
{

}

TEST_F(PacketBufferTest, emptyPacket)
{
    auto builder = PacketBufferBuilder();
    auto buffer = PacketBuffer(builder);

    auto [desc, domain] = generate_building_blocks();

    SizeT mem = 0;

    DataPacketPtr destination;

    ASSERT_NO_THROW(buffer->createPacket(mem, desc, domain, &destination));
}

TEST_F(PacketBufferTest, emptyPacketRead)
{
    // Might be redundant
}

TEST_F(PacketBufferTest, fullBufferRead)
{

}

TEST_F(PacketBufferTest, dynamicWorkflowSimulation)
{
}

TEST_F(PacketBufferTest, linearRuleFail)
{
    auto builder = PacketBufferBuilder();
    auto buffer = PacketBuffer(builder);

    auto dimensions = daq::List<daq::IDimension>();
    dimensions.pushBack(daq::Dimension(daq::LinearDimensionRule(10, 10, 10)));

    auto rule = LinearDataRule(10, 10);

    auto descriptor = daq::DataDescriptorBuilder()
                          .setRule(rule)
                          .setSampleType(daq::SampleType::Int8)
                          .setDimensions(dimensions)
                          .setUnit(daq::Unit("s", 10))
                          .build();
    daq::DataPacketPtr domain = daq::DataPacket(descriptor, 100, 0);

    SizeT mem = 10;

    DataPacketPtr destination;

    ASSERT_THROW(buffer->createPacket(mem, descriptor, domain, &destination), InvalidParameterException);
}

TEST_F(PacketBufferTest, dynamicPacketDestruction)
{

}

TEST_F(PacketBufferTest, multithreadBasicFunctionallity)
{

}

TEST_F(PacketBufferTest, resetTest)
{

}

TEST_F(PacketBufferTest, fullDynamicFunctionallityWorkflow)
{

}
