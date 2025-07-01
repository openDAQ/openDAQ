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
    builder->setSizeInBytes(800);
    PacketBufferPtr buffer = PacketBuffer(builder);
    SizeT mem;

    buffer->getAvailableMemory(&mem);

    ASSERT_TRUE(mem == 800);
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

    SizeT mem = 80;

    DataPacketPtr destination[2];

    buffer->createPacket(mem, desc, domain, &destination[0]);

    ASSERT_EQ(buffer->createPacket(100000, desc, domain, &destination[1]), 0);
}

TEST_F(PacketBufferTest, bufferFillUp)
{
    auto builder = PacketBufferBuilder();
    builder->setSizeInBytes(800);
    auto buffer = PacketBuffer(builder);

    auto [desc, domain] = generate_building_blocks();

    SizeT check;
    buffer->getAvailableMemory(&check);


    SizeT mem = 10;

    std::vector<DataPacketPtr> destination;
    DataPacketPtr middle;
    for (int i = 0; i < 8; i++) {
        buffer->createPacket(mem, desc, domain, &middle);
        buffer->getAvailableMemory(&check);
        destination.push_back(middle.detach());
    }
    buffer->getAvailableMemory(&mem);
}

TEST_F(PacketBufferTest, emptyPacket)
{
    auto builder = PacketBufferBuilder();
    builder.setSizeInBytes(800);
    auto buffer = PacketBuffer(builder);

    auto [desc, domain] = generate_building_blocks();

    SizeT mem = 0;

    DataPacketPtr destination;

    ASSERT_EQ(buffer->createPacket(mem, desc, domain, &destination), 0);
}

TEST_F(PacketBufferTest, writeAhead)
{
    auto builder = PacketBufferBuilder();
    builder.setSizeInBytes(800);
    auto buffer = PacketBuffer(builder);

    auto [desc, domain] = generate_building_blocks();

    SizeT mem = 20;

    {
        DataPacketPtr destination;
        buffer->createPacket(mem, desc, domain, &destination);
    }
    DataPacketPtr destination, destination2;

    mem = 70;
    buffer->createPacket(mem, desc, domain, &destination);

    mem = 20;
    ASSERT_EQ(buffer->createPacket(mem, desc, domain, &destination2), 0);
}

TEST_F(PacketBufferTest, readAhead)
{
    auto builder = PacketBufferBuilder();
    builder.setSizeInBytes(800);
    auto buffer = PacketBuffer(builder);

    auto [desc, domain] = generate_building_blocks();

    SizeT mem = 20;

    {
        DataPacketPtr destination;
        buffer->createPacket(mem, desc, domain, &destination);
    }

    {
        DataPacketPtr destination, destination2;

        mem = 50;
        buffer->createPacket(mem, desc, domain, &destination);

        mem = 20;
        buffer->createPacket(mem, desc, domain, &destination2);
    }
    auto left = buffer.getAvailableContinousSampleLeft(desc);
    auto right = buffer.getAvailableContinousSampleRight(desc);
    ASSERT_TRUE(right > left);
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
    builder.setSizeInBytes(800);
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

    // Somewhat of a magic number here
    ASSERT_EQ(buffer->createPacket(mem, descriptor, domain, &destination), 2147483649);
    // The important thing in this return is that it need to be different to 0
}

TEST_F(PacketBufferTest, dynamicPacketDestruction)
{
    auto builder = PacketBufferBuilder();
    builder.setSizeInBytes(800);
    auto buffer = PacketBuffer(builder);


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
