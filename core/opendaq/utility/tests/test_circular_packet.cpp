#include <gtest/gtest.h>
#include <opendaq/circularPacket.h>
#include <opendaq/data_descriptor_factory.h>
#include <opendaq/data_rule_factory.h>
#include <opendaq/dimension_factory.h>
#include <opendaq/data_rule_calc_private.h>
#include <opendaq/packet_factory.h>
#include <thread>
#include <opendaq/reusable_data_packet_ptr.h>
#include <opendaq/sample_type_traits.h>
#include <opendaq/scaling_factory.h>
#include <opendaq/scaling_ptr.h>
#include <iostream>

using CircularPacketTest = testing::Test;

void display_write_read_pos(daq::PacketBuffer* pb)
{
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
    daq::PacketBuffer pb;
    void* check;
    size_t bb = 0;
    size_t* delique = &bb;
    *delique = 16;
    ASSERT_EQ(pb.WriteSample(delique, &check), 0);
    *delique = 1024;
    ASSERT_EQ(pb.WriteSample(delique, &check), 2);
}

TEST_F(CircularPacketTest, WriteFullRangeFill)
{
    daq::PacketBuffer pb;
    void* check;
    size_t bb = 0;
    size_t* fun = &bb;
    *fun = 512;
    ASSERT_EQ(pb.WriteSample(fun, &check), 0);
    ASSERT_EQ(pb.WriteSample(fun, &check), 0);
    *fun = 2;
    ASSERT_EQ(pb.WriteSample(fun, &check), 1);       // This one fails because the buffer is full
}

TEST_F(CircularPacketTest, WriteAdjustedSize)
{
    daq::PacketBuffer pb;
    void* check;
    size_t bb = 0;
    size_t* run = &bb;
    *run = 1000;
    ASSERT_EQ(pb.WriteSample(run, &check), 0);
    *run = 30;
    ASSERT_EQ(pb.WriteSample(run, &check), 2);
}

TEST_F(CircularPacketTest, WriteEmptyCall)
{
    daq::PacketBuffer pb;
    void* check;
    size_t bb = 0;
    size_t* tun = &bb;
    *tun = 0;
    ASSERT_EQ(pb.WriteSample(tun, &check), 0);
}


TEST_F(CircularPacketTest, ReadEmpty)
{
    daq::PacketBuffer pb;
    size_t bb = 0;
    size_t* gun = &bb;
    *gun = 0;
    ASSERT_EQ(pb.ReadSample(pb.getReadPos(), *gun), 1);
}

TEST_F(CircularPacketTest, ReadFromFullBuffer)
{
    daq::PacketBuffer pb;
    void* check;
    size_t bb = 0;
    size_t* wan = &bb;
    *wan = 1024;
    pb.WriteSample(wan, &check);
    *wan = 512;
    ASSERT_EQ(pb.ReadSample(check, *wan), 0);
}

TEST_F(CircularPacketTest, ReadFullBuffer)
{
    daq::PacketBuffer pb;
    void* check;
    size_t bb = 0;
    size_t* hun = &bb;
    *hun = 512;
    pb.WriteSample(hun, &check);
    *hun = 256;
    ASSERT_EQ(pb.ReadSample(check, *hun), 0);
    *hun = 1000;
    ASSERT_EQ(pb.WriteSample(hun, &check), 2);
    *hun = 128;
    ASSERT_EQ(pb.WriteSample(hun, &check), 0);
    *hun = 800;
    ASSERT_EQ(pb.ReadSample(check, *hun), 0);
}

TEST_F(CircularPacketTest, ReadPartialWorkflow)
{
    daq::PacketBuffer pb;
    void* check;
    size_t bb = 0;
    size_t* jun = &bb;
    *jun = 512;
    pb.WriteSample(jun, &check);
    *jun = 256;
    ASSERT_EQ(pb.ReadSample(check, *jun), 0);
    *jun = 128;
    ASSERT_EQ(pb.ReadSample(pb.getReadPos(), *jun), 0);
    *jun = 64;
    ASSERT_EQ(pb.ReadSample(pb.getReadPos(), *jun), 0);
    *jun = 32;
    ASSERT_EQ(pb.ReadSample(pb.getReadPos(), *jun), 0);
    *jun = 16;
    ASSERT_EQ(pb.ReadSample(pb.getReadPos(), *jun), 0);
    *jun = 8;
    ASSERT_EQ(pb.ReadSample(pb.getReadPos(), *jun), 0);
    *jun = 4;
    ASSERT_EQ(pb.ReadSample(pb.getReadPos(), *jun), 0);
    ASSERT_EQ(pb.ReadSample(pb.getReadPos(), *jun), 0);
    *jun = 1;
    ASSERT_EQ(pb.ReadSample(pb.getReadPos(), *jun), 1);
}

TEST_F(CircularPacketTest, TestMockPacket)
{
    daq::PacketBuffer pb;
    size_t st = 8;

    {
        daq::Packet pck = pb.cP(&st, 10);
        daq::Packet pck2 = pb.cP(&st, 100);
    }
    ASSERT_EQ(pb.getReadPos(), pb.getWritePos());

}

TEST_F(CircularPacketTest, TestPacketsWithDescriptorsCreate)
{
    auto [descriptor, domain] = generate_building_blocks();

    daq::PacketBuffer pb;
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

    daq::PacketBuffer pb;
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
                    if (new_pck == nullptr)
                    {
                        throw 1;
                    }
                    a_group.push_back(new_pck);
                }
                ASSERT_FALSE(true);
            }
            catch (...)
            {
                ASSERT_EQ(pb.createPacket(&sampleCount, desc, dom), nullptr);
            }
    }

}

TEST_F(CircularPacketTest, TestCleanBufferAfterPacketsDestroyed)
{
    auto [descriptor, domain] = generate_building_blocks();

    daq::PacketBuffer pb;
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

    daq::PacketBuffer pb;
    size_t sampleCount = 100;
    //void* mid_point = NULL;
    //void* save_point = NULL;

    {
        std::cout << "WritePoint before creation: " << pb.getWritePos() << std::endl;
        auto old_created = pb.createPacket(&sampleCount, descriptor, domain);
        std::cout << "WritePoint after outer scope creation: " << pb.getWritePos() << std::endl;
        std::cout << "ReadPoint after outer scopecreation: " << pb.getReadPos() << std::endl;
        //save_point = pb.getReadPos();
        //mid_point = pb.getWritePos();
        {
            auto new_packet = pb.createPacket(&sampleCount, descriptor, domain);
            //mid_point = pb.getWritePos();
            //std::cout << "WritePoint after inner scope creation: " << pb.getWritePos() << std::endl;
        }
        std::cout << "ReadPoint after going out of inner scope: " << pb.getReadPos() << std::endl;
    }
    std::cout << "WritePoint after going out of outer scopr: " << pb.getWritePos() << std::endl;
    std::cout << "ReadPoint after going out of outer scope: " << pb.getReadPos() << std::endl;
    ASSERT_EQ(pb.getWritePos(), pb.getReadPos());
}

TEST_F(CircularPacketTest, TestPacketReadPartial)
{
    auto [descriptor, domain] = generate_building_blocks();
    daq::PacketBuffer pb;
    size_t sampleCount = 100;
    {
        std::cout << "WritePos before any declarations: " << pb.getWritePos() << std::endl;
        daq::DataPacketPtr pp;
        {
            auto a = pb.createPacket(&sampleCount, descriptor, domain);
            std::cout << "WritePos after first declare: " << pb.getWritePos() << std::endl;
            auto b = pb.createPacket(&sampleCount, descriptor, domain);
            std::cout << "WritePos after second declare: " << pb.getWritePos() << std::endl;
            {
                pp = pb.createPacket(&sampleCount, descriptor, domain);
                std::cout << "WritePos after third declare: " << pb.getWritePos() << std::endl;
                auto c = pb.createPacket(&sampleCount, descriptor, domain);
                std::cout << "WritePos after fourth declare: " << pb.getWritePos() << std::endl;
                auto d = pb.createPacket(&sampleCount, descriptor, domain);
                std::cout << "WritePos after fifth declare: " << pb.getWritePos() << std::endl;
            }
            std::cout << "ReadPos after fourth and fifth declare go OOS: " << pb.getReadPos() << std::endl;
        }
        std::cout << "ReadPos after the first and second declare go OOS: " << pb.getReadPos() << std::endl;
    }
    std::cout << "ReadPos after the third declare goes OOS: " << pb.getReadPos() << std::endl;
    ASSERT_EQ(pb.getWritePos(), pb.getReadPos());
}

void createMultiThreadedPacket(daq::PacketBuffer *pb)
{
    auto [desc, dom] = generate_building_blocks();
    size_t n = 100;
    
    auto r = pb->createPacket(&n, desc, dom);
    std::lock_guard<std::mutex> lock(pb->flip);
    std::cout << "WritePos: " << pb->getWritePos() << std::endl;
    std::cout << "Created in the thread: " << r.getRawDataSize() << std::endl;
}


TEST_F(CircularPacketTest, TestMultiThread)
{
    daq::PacketBuffer pb;
    auto [descriptor, domain] = generate_building_blocks();
    std::condition_variable cv;

    std::thread th1(createMultiThreadedPacket, &pb);
    std::thread th2(createMultiThreadedPacket, &pb);
    std::thread th3(createMultiThreadedPacket, &pb);
    std::thread th4(createMultiThreadedPacket, &pb);
    th4.join();
    th3.join();
    th1.join();
    th2.join();

    ASSERT_EQ(pb.getWritePos(), pb.getReadPos());
}

// This is a very inelegant solution,
// but it allows the test to run without waiting
// in the test thread itself
int t = 0;

void createAndWaitPacket(daq::PacketBuffer *pb, daq::DataDescriptorPtr desc, daq::DataPacketPtr &dom, std::condition_variable *cv)
{
    
    size_t n = 100;
    auto r = pb->createPacket(&n, desc, dom);
    std::mutex re;
    std::unique_lock<std::mutex> lck(re);
    cv->wait(lck,
             [&]
             {
                 t += 1;
                 return true;
             });
    lck.unlock();
    cv->notify_all();
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    std::cout << "Packet was created. " << std::endl;
}

 void cb(daq::PacketBuffer* pb)
{
   auto r = pb -> reset();
    std::cout << r << std::endl;
}

TEST_F(CircularPacketTest, TestReset)
{
    daq::PacketBuffer pb;
    std::condition_variable start_up_assurance;
    auto [descriptor, domain] = generate_building_blocks();

    std::thread th1(createAndWaitPacket, &pb, descriptor, domain, &start_up_assurance);

    std::thread th2(createAndWaitPacket, &pb, descriptor, domain, &start_up_assurance);

    std::thread th3(createAndWaitPacket, &pb, descriptor, domain, &start_up_assurance);

    std::thread th4(createAndWaitPacket, &pb, descriptor, domain, &start_up_assurance);

    std::mutex gh;
    std::unique_lock<std::mutex> rezan(gh);

    start_up_assurance.wait(rezan, [&] { return t >= 4; });
    std::thread th5(createAndWaitPacket, &pb, descriptor, domain, &start_up_assurance);

    cb(&pb);
    
    th1.join();
    th2.join();
    th3.join();
    th4.join();
    th5.join();
    ASSERT_EQ(pb.getWritePos(), pb.getReadPos());

}
