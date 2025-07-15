#include <gtest/gtest.h>
#include <thread>
#include <opendaq/packet_buffer_factory.h>
#include <opendaq/data_descriptor_factory.h>
#include <opendaq/data_rule_factory.h>
#include <opendaq/dimension_factory.h>
#include <opendaq/data_rule_calc_private.h>
#include <opendaq/packet_factory.h>
#include <opendaq/reusable_data_packet_ptr.h>
#include <opendaq/sample_type_traits.h>

using namespace daq;

using PacketBufferTest = testing::Test;

std::tuple<daq::DataDescriptorPtr, daq::DataPacketPtr> generateBuildingBlocks()
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
    builder.setSizeInBytes(800);
    PacketBufferPtr buffer = PacketBuffer(builder);
    SizeT mem;

    auto [desc, domain] = generateBuildingBlocks();

    mem = buffer.getMaxAvailableContinousSampleCount(desc);

    ASSERT_TRUE(mem == 80);
}

TEST_F(PacketBufferTest, MakeAPacket)
{
    auto builder = PacketBufferBuilder();
    auto buffer = PacketBuffer(builder);

    auto [desc, domain] = generateBuildingBlocks();
    
    SizeT mem = 10;

    DataPacketPtr destination;

    auto errCode = buffer->createPacket(mem, desc, domain, &destination);

    ASSERT_TRUE(OPENDAQ_SUCCEEDED(errCode));
}

TEST_F(PacketBufferTest, FullRange)
{
    auto builder = PacketBufferBuilder();
    builder.setSizeInBytes(1000);
    auto buffer = PacketBuffer(builder);

    auto [desc, domain] = generateBuildingBlocks();

    SizeT mem = 80;

    DataPacketPtr destination[2];

    destination[0] = buffer.createPacket(mem, desc, domain);

    ASSERT_NO_THROW(destination[1] = buffer.createPacket(100000, desc, domain));
}

TEST_F(PacketBufferTest, BufferFillUp)
{
    auto builder = PacketBufferBuilder();
    builder.setSizeInBytes(800);
    auto buffer = PacketBuffer(builder);

    auto [desc, domain] = generateBuildingBlocks();

    SizeT check = 0;

    check = buffer.getMaxAvailableContinousSampleCount(desc);


    SizeT mem = 10;

    std::vector<DataPacketPtr> destination;
    DataPacketPtr middle;
    for (int i = 0; i < 8; i++) {
        middle = buffer.createPacket(mem, desc, domain);
        check = buffer.getMaxAvailableContinousSampleCount(desc);
        destination.push_back(middle.detach());
    }
    mem = buffer.getAvailableSampleCount(desc);

    ASSERT_EQ(check, mem);
}

TEST_F(PacketBufferTest, EmptyPacket)
{
    auto builder = PacketBufferBuilder();
    builder.setSizeInBytes(800);
    auto buffer = PacketBuffer(builder);

    auto [desc, domain] = generateBuildingBlocks();

    SizeT mem = 0;

    DataPacketPtr destination;

    ASSERT_NO_THROW(destination = buffer.createPacket(mem, desc, domain));
}

TEST_F(PacketBufferTest, WriteAhead)
{
    auto builder = PacketBufferBuilder();
    builder.setSizeInBytes(800);
    auto buffer = PacketBuffer(builder);

    auto [desc, domain] = generateBuildingBlocks();

    SizeT mem = 20;

    {
        DataPacketPtr destination;
        destination = buffer.createPacket(mem, desc, domain);
    }
    DataPacketPtr destination, destination2;

    mem = 70;
    destination = buffer.createPacket(mem, desc, domain);

    mem = 20;
    ASSERT_NO_THROW(destination2 = buffer.createPacket(mem, desc, domain));
}

TEST_F(PacketBufferTest, ReadAhead)
{
    auto builder = PacketBufferBuilder();
    builder.setSizeInBytes(800);
    auto buffer = PacketBuffer(builder);

    auto [desc, domain] = generateBuildingBlocks();

    SizeT mem = 20;

    {
        DataPacketPtr destination;
        destination = buffer.createPacket(mem, desc, domain);
    }

    {
        DataPacketPtr destination, destination2;

        mem = 50;
        destination = buffer.createPacket(mem, desc, domain);

        mem = 20;
        destination2 = buffer.createPacket(mem, desc, domain);
    }
    auto right = buffer.getAvailableSampleCount(desc);
    ASSERT_TRUE(right > 20);
}

TEST_F(PacketBufferTest, FullBufferRead)
{
    auto builder = PacketBufferBuilder();
    builder.setSizeInBytes(800);
    auto buffer = PacketBuffer(builder);

    auto [desc, domain] = generateBuildingBlocks();

    SizeT mem = 20;

    {
        DataPacketPtr destination, destination2, destination3, destination4;
        destination = buffer.createPacket(mem, desc, domain);

        destination2 = buffer.createPacket(mem, desc, domain);
        destination3 = buffer.createPacket(mem, desc, domain);
        destination4 = buffer.createPacket(mem, desc, domain);
    }

    ASSERT_EQ(buffer.getAvailableSampleCount(desc), 80);
}

TEST_F(PacketBufferTest, LinearRuleFail)
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
    ASSERT_ANY_THROW(destination = buffer.createPacket(mem, descriptor, domain));
    // The important thing in this return is that it need to be different to 0
}

TEST_F(PacketBufferTest, DynamicPacketDestruction)
{
    auto builder = PacketBufferBuilder();
    builder.setSizeInBytes(800);
    auto buffer = PacketBuffer(builder);

    auto [desc, domain] = generateBuildingBlocks();

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
    ASSERT_EQ((buffer.getAvailableSampleCount(desc)), 80);
}

TEST_F(PacketBufferTest, MultithreadBasicFunctionallity)
{
    auto builder = PacketBufferBuilder();
    builder.setSizeInBytes(800);
    auto buffer = PacketBuffer(builder);

    auto [desc, domain] = generateBuildingBlocks();

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

TEST_F(PacketBufferTest, ResetTest)
{
    auto builder = PacketBufferBuilder();
    builder.setSizeInBytes(800);
    auto buffer = PacketBuffer(builder);

    auto [desc, domain] = generateBuildingBlocks();

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

    ASSERT_EQ(buffer.getAvailableSampleCount(desc), 100);
}

TEST_F(PacketBufferTest, FullDynamicFunctionallityWorkflow)
{
    auto builder = PacketBufferBuilder();
    builder.setSizeInBytes(800);
    auto buffer = builder.build();

    auto [desc, domain] = generateBuildingBlocks();

    auto check = [buffer = buffer, desc = desc, domain = domain](SizeT t, DataPacketPtr& destination)
        {
            destination = buffer.createPacket(t, desc, domain);
        };

    std::thread t1;
    std::thread t2;
    std::thread t3;

    {
        DataPacketPtr r1;
        t1 = std::thread(check, 20, std::ref(r1));
        t1.join();
    }
    {
        DataPacketPtr r2, r3;
        t2 = std::thread(check, 20, std::ref(r2));

        t3 = std::thread(check, 40, std::ref(r3));
        t2.join();
        t3.join();
    }

    ASSERT_EQ(buffer.getMaxAvailableContinousSampleCount(desc), 80);
}
