#include <gtest/gtest.h>
#include <opendaq/circularPacket.h>
#include <opendaq/data_descriptor_factory.h>
#include <opendaq/data_rule_factory.h>
#include <opendaq/dimension_factory.h>
#include <opendaq/data_rule_calc_private.h>
#include <opendaq/packet_factory.h>
#include <opendaq/reusable_data_packet_ptr.h>
#include <opendaq/sample_type_traits.h>
#include <opendaq/scaling_factory.h>
#include <opendaq/scaling_ptr.h>
#include <iostream>

using CircularPacketTest = testing::Test;

void display_write_read_pos(PacketBuffer* pb)
{
    //std::cout << "Write position: " << pb->getWritePos() << std::endl;
    //std::cout << "Read position: " << pb->getReadPos() << std::endl;
}

std::tuple<daq::DataDescriptorPtr, daq::DataPacketPtr> generate_building_blocks()
{
    auto dimensions = daq::List<daq::IDimension>();
    dimensions.pushBack(daq::Dimension(daq::LinearDimensionRule(10, 10, 10)));


    auto descriptor =
        daq::DataDescriptorBuilder()
        .setSampleType(daq::SampleType::Int8)
        .setDimensions(dimensions)
        .setUnit(daq::Unit("s", 10))
        .build();
    daq::DataPacketPtr domain = daq::DataPacket(descriptor, 100, 0);
    return {descriptor, domain};
}

TEST_F(CircularPacketTest, SanityWritePosCheck)
{
    PacketBuffer pb;
    void* check;
    size_t bb = 0;
    size_t* delique = &bb;
    //std::cout <<pb.getWritePos() << std::endl;
    *delique = 16;
    ASSERT_EQ(pb.WriteSample(delique, &check), 0);
    //int t = pb.WriteSample(16, &check);
    //std::cout << pb.getWritePos() << std::endl;
    *delique = 1024;
    ASSERT_EQ(pb.WriteSample(delique, &check), 2);
    //std::cout << "Adjusted size: " << pb.getAdjustedSize() << std::endl;
}

TEST_F(CircularPacketTest, WriteFullRangeFill)
{
    PacketBuffer pb;
    void* check;
    size_t bb = 0;
    size_t* fun = &bb;
    *fun = 512;
    ASSERT_EQ(pb.WriteSample(fun, &check), 0);
    ASSERT_EQ(pb.WriteSample(fun, &check), 0);
    *fun = 2;
    ASSERT_EQ(pb.WriteSample(fun, &check), 1);       // This one fails becouse the buffer is full
    //ASSERT_TRUE(pb.getIsFull());
}

TEST_F(CircularPacketTest, WriteAdjustedSize)
{
    PacketBuffer pb;
    void* check;
    size_t bb = 0;
    size_t* run = &bb;
    *run = 1000;
    ASSERT_EQ(pb.WriteSample(run, &check), 0);
    *run = 30;
    ASSERT_EQ(pb.WriteSample(run, &check), 2);
    //std::cout << "Adjusted size: " << pb.getAdjustedSize() << std::endl;
    //ASSERT_TRUE(pb.getIsFull());
}

TEST_F(CircularPacketTest, WriteEmptyCall)
{
    // Check with Jaka if this behaviour should be considered (or if it can be safely ignored)
    PacketBuffer pb;
    void* check;
    size_t bb = 0;
    size_t* tun = &bb;
    *tun = 0;
    ASSERT_EQ(pb.WriteSample(tun, &check), 0);
    //ASSERT_TRUE(pb.getIsFull());
}


TEST_F(CircularPacketTest, ReadEmpty)
{
    PacketBuffer pb;
    size_t bb = 0;
    size_t* gun = &bb;
    *gun = 0;
    ASSERT_EQ(pb.ReadSample(*gun), 1);
}

TEST_F(CircularPacketTest, ReadFromFullBuffer)
{
    PacketBuffer pb;
    void* check;
    size_t bb = 0;
    size_t* wan = &bb;
    *wan = 1024;
    pb.WriteSample(wan, &check);
    //ASSERT_TRUE(pb.getIsFull());
    *wan = 512;
    ASSERT_EQ(pb.ReadSample(*wan), 0);
}

TEST_F(CircularPacketTest, ReadFullBuffer)
{
    PacketBuffer pb;
    void* check;
    size_t bb = 0;
    size_t* hun = &bb;
    *hun = 512;
    pb.WriteSample(hun, &check);
    *hun = 513;
    ASSERT_EQ(pb.ReadSample(*hun), 1);
    //display_write_read_pos(&pb);
    *hun = 256;
    ASSERT_EQ(pb.ReadSample(*hun), 0);
    //display_write_read_pos(&pb);
    *hun = 1000;
    ASSERT_EQ(pb.WriteSample(hun, &check), 2);
    //std::cout << "Adjusted size: " << pb.getAdjustedSize() << std::endl;
    //display_write_read_pos(&pb);
    *hun = 128;
    ASSERT_EQ(pb.WriteSample(hun, &check), 0);
    //display_write_read_pos(&pb);
    *hun = 800;
    ASSERT_EQ(pb.ReadSample(*hun), 0);
    //display_write_read_pos(&pb);
}

TEST_F(CircularPacketTest, ReadPartialWorkflow)
{
    PacketBuffer pb;
    void* check;
    size_t bb = 0;
    size_t* jun = &bb;
    *jun = 512;
    pb.WriteSample(jun, &check);
    *jun = 256;
    ASSERT_EQ(pb.ReadSample(*jun), 0);
    *jun = 128;
    ASSERT_EQ(pb.ReadSample(*jun), 0);
    *jun = 64;
    ASSERT_EQ(pb.ReadSample(*jun), 0);
    *jun = 32;
    ASSERT_EQ(pb.ReadSample(*jun), 0);
    *jun = 16;
    ASSERT_EQ(pb.ReadSample(*jun), 0);
    //display_write_read_pos(&pb);
    *jun = 8;
    ASSERT_EQ(pb.ReadSample(*jun), 0);
    *jun = 4;
    ASSERT_EQ(pb.ReadSample(*jun), 0);
    ASSERT_EQ(pb.ReadSample(*jun), 0);
    *jun = 1;
    ASSERT_EQ(pb.ReadSample(*jun), 1);
}

TEST_F(CircularPacketTest, TestMockPacket)
{
    PacketBuffer pb;
    size_t st = 8;
    //std::cout << pb.getReadPos() << std::endl;
    //std::cout << pb.getWritePos() << std::endl;
    if (1)
    {
        Packet pck = pb.createPacket(&st, 10);
        //std::cout << pb.getWritePos() << std::endl;
        //std::cout << pck.assignedData << std::endl;
        Packet pck2 = pb.createPacket(&st, 100);
        //std::cout << pb.getWritePos() << std::endl;
    }
    //std::cout << pb.getReadPos() << std::endl;
    //std::cout << pb.getWritePos() << std::endl;
}

TEST_F(CircularPacketTest, TestPacketsWithDescriptorsCreate)
{
    auto [descriptor, domain] = generate_building_blocks();

    PacketBuffer pb;
    size_t sampleCount = 100;
    std::cout << pb.getReadPos() << std::endl;
    {
        auto created = pb.createPacket(&sampleCount, descriptor, domain);
        std::cout << created.getPacketId() << std::endl
                  << pb.getWritePos() << std::endl;
    }
    std::cout << pb.getWritePos() << std::endl;
    std::cout << pb.getReadPos() << std::endl;

}


TEST_F(CircularPacketTest, TestFillingUpBuffer)
{
    auto [desc, dom] = generate_building_blocks();

    PacketBuffer pb;
    size_t sampleCount = 100;
    // Here will create a few packets
    {
        daq::DataPacketPtr old_stuff;
        std::vector<daq::DataPacketPtr> a_group;
            try
            {
                for (int i = 0; i < 1000; i++)
                {
                    auto new_pck = pb.createPacket(&sampleCount, desc, dom);
                    a_group.push_back(new_pck);
                }
                ASSERT_FALSE(true);
            }
            catch (...)
            {
                ASSERT_ANY_THROW(pb.createPacket(&sampleCount, desc, dom));
            }
    }

}

TEST_F(CircularPacketTest, TestCleanBufferAfterPacketsDestroyed)
{
    auto [descriptor, domain] = generate_building_blocks();

    PacketBuffer pb;
    size_t sampleCount = 100;
    // Here will create a few packets

    {
        std::cout << "ReadPosition before buffer gets filled: " << pb.getReadPos() << std::endl;
        daq::DataPacketPtr old_stuff;
        std::vector<daq::DataPacketPtr> a_group;
        try
        {
            for (int i = 0; i < 1000; i++)
            {
                auto new_pck = pb.createPacket(&sampleCount, descriptor, domain);
                a_group.push_back(new_pck);
                std::cout << "WritePosition: " << pb.getWritePos() << std::endl
                          << "Buffer is full: " << pb.getIsFull() << std::endl;
            }
        }
        catch (...)
        {
        }
    }
    std::cout << "ReadPosition after full buffer goes out of scope: " << pb.getReadPos() << std::endl;
    ASSERT_EQ(pb.getIsFull(), 0);
}

TEST_F(CircularPacketTest, TestPacketImprovementTest)
// This test will not work until
// I correctly implement the still-alive concept
{
    auto [descriptor, domain] = generate_building_blocks();

    PacketBuffer pb;
    size_t sampleCount = 100;

    std::cout << pb.getWritePos() << std::endl;
    auto old_created = pb.createPacket(&sampleCount, descriptor, domain);
    std::cout << pb.getWritePos() << std::endl;
    std::cout << "ReadPoint: " << pb.getReadPos() << std::endl;
    auto save_point = pb.getReadPos();
    auto mid_point = pb.getWritePos();
    {
        auto new_packet = pb.createPacket(&sampleCount, descriptor, domain);
        mid_point = pb.getWritePos();
        //std::cout << pb.getWritePos() << std::endl;
    }
    std::cout << "ReadPoint: " << pb.getReadPos() << std::endl;
    ASSERT_EQ(pb.getWritePos(), mid_point);
    ASSERT_EQ(pb.getReadPos(), save_point);
}
