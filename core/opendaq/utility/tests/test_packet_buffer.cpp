#include <gtest/gtest.h>
#include <thread>
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
    auto left = buffer.getAvailableSampleLeft(desc);
    auto right = buffer.getAvailableSampleRight(desc);
    ASSERT_TRUE(right > left);
}

TEST_F(PacketBufferTest, fullBufferRead)
{
    auto builder = PacketBufferBuilder();
    builder.setSizeInBytes(800);
    auto buffer = PacketBuffer(builder);

    auto [desc, domain] = generate_building_blocks();

    SizeT mem = 20;

    {
        DataPacketPtr destination, destination2, destination3, destination4;
        destination = buffer.createPacket(mem, desc, domain);

        destination2 = buffer.createPacket(mem, desc, domain);
        destination3 = buffer.createPacket(mem, desc, domain);
        destination4 = buffer.createPacket(mem, desc, domain);
    }

    ASSERT_EQ(buffer.getAvailableSampleRight(desc), 80);
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

    auto [desc, domain] = generate_building_blocks();

    SizeT mem = 10;

    {
        // Create one
        DataPacketPtr c1;
        c1 = buffer.createPacket(mem, desc, domain);
        {
            // Create one
            DataPacketPtr c2;
            c2 = buffer.createPacket(mem, desc, domain);
        }
        {
            // Create one
            DataPacketPtr c3;
            c3 = buffer.createPacket(mem, desc, domain);
            {
                // Create one
                DataPacketPtr c4;
                c4 = buffer.createPacket(mem, desc, domain);
                {
                    // Create one
                    DataPacketPtr c5;
                    c5 = buffer.createPacket(mem, desc, domain);
                }
            }
        }
        {
            // Create one
            DataPacketPtr c6;
            c6 = buffer.createPacket(mem, desc, domain);
            {
                // Create one
                DataPacketPtr c7;
                c7 = buffer.createPacket(mem, desc, domain);
            }
        }
        {
            // Create one
            DataPacketPtr c8;
            c8 = buffer.createPacket(mem, desc, domain);
        }
    }

    // The buffer should be empty (left + right should be full buffer)
    ASSERT_EQ((buffer.getAvailableSampleLeft(desc) + buffer.getAvailableSampleRight(desc)), 80);
}

TEST_F(PacketBufferTest, multithreadBasicFunctionallity)
{
    auto builder = PacketBufferBuilder();
    builder.setSizeInBytes(800);
    auto buffer = PacketBuffer(builder);

    auto [desc, domain] = generate_building_blocks();

    auto check = [buffer = buffer, desc = desc, domain = domain](SizeT t, DataPacketPtr& destination)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            destination = buffer.createPacket(t, desc, domain);
        };

    std::thread t1;
    std::thread t2;

    DataPacketPtr r1;
    DataPacketPtr r2;

    t1 = std::thread(check, 20, std::ref(r1));
    t2 = std::thread(check, 50, std::ref(r2));

    t1.join();
    t2.join();

    SizeT out = buffer.getMaxAvailableContinousSampleCount(desc);

    ASSERT_EQ(out, 10);
}

TEST_F(PacketBufferTest, resetTest)
{
    auto builder = PacketBufferBuilder();
    builder.setSizeInBytes(800);
    auto buffer = PacketBuffer(builder);

    auto [desc, domain] = generate_building_blocks();

    auto check = [buffer = buffer, desc = desc, domain = domain](SizeT t, DataPacketPtr& destination)
    {
        destination = buffer.createPacket(t, desc, domain);
    };

    auto reset = [buffer = buffer](SizeT t)
    {
        std::this_thread::sleep_for(std::chrono::microseconds(20));
        buffer.resize(t);
    };

    std::thread t1;
    std::thread t2;
    std::thread t3;

    {
        DataPacketPtr r1;
        t1 = std::thread(check, 20, std::ref(r1));
        t1.join();
    }
    t2 = std::thread(reset, 1000);
    {
        DataPacketPtr r2;
        t3 = std::thread(check, 20, std::ref(r2));
        t3.join();
    }
    t2.join();

    ASSERT_EQ(buffer.getAvailableSampleRight(desc), 100);
}

TEST_F(PacketBufferTest, fullDynamicFunctionallityWorkflow)
{
    auto builder = PacketBufferBuilder();
    builder.setSizeInBytes(800);
    auto buffer = builder.build();

    auto [desc, domain] = generate_building_blocks();

    std::thread t1;
    std::thread t2;
    std::thread t3;

}
