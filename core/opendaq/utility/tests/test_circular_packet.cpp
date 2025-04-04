#include <gtest/gtest.h>
#include <opendaq/circular_packet.h>
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



class Packetet : public daq::PacketBuffer
{
public:

    using PacketBuffer::getReadPos;
    using PacketBuffer::getWritePos;
    using PacketBuffer::setWritePos;
    using PacketBuffer::setReadPos;
    using PacketBuffer::getIsFull;


    bufferReturnCodes::EReturnCodesPacketBuffer callReadSample(void* beginningOfDelegatedSpace, size_t sampleCount)
    {
        return Read(beginningOfDelegatedSpace, sampleCount);
    }

    bufferReturnCodes::EReturnCodesPacketBuffer callWriteSample(size_t* sampleCount, void** memPos)
    {
        return Write(sampleCount, memPos);
    }

    daq::DataPacketPtr callCreatePacket(size_t* sampleCount,
                                        daq::DataDescriptorPtr dataDescriptor,
                                        daq::DataPacketPtr& domainPacket)
    {
        return createPacket(sampleCount, dataDescriptor, domainPacket);
    }

    std::mutex* goForLock()
    {
        return &flip;
    }

    Packetet()
        : PacketBuffer(sizeof(double), 1024, nullptr)
    {
    }

};

class daq::Packet
{
public:
    Packet(size_t desiredNumOfSamples, void* beginningOfData, std::function<void(void*, size_t)> callback)
    {
        cb = std::move(callback);
        // Users code, users memory corruption problems
        sampleAmount = desiredNumOfSamples;
        assignedData = beginningOfData;
    }

    Packet()
    {
        // Failed state
        sampleAmount = 0;
        assignedData = nullptr;
    }

    ~Packet()
    {
        cb(assignedData, sampleAmount);
    }
    size_t sampleAmount;
    void* assignedData;

private:
    std::function<void(void*, size_t)> cb;
};

// This is a test function that was used to help gauge the behaviour of the buffer class
daq::Packet daq::PacketBuffer::cP(size_t* sampleCount, size_t dataDescriptor)
{
    void* startOfSpace = nullptr;
    bufferReturnCodes::EReturnCodesPacketBuffer ret = this->Write(sampleCount, &startOfSpace);
    std::function<void(void*, size_t)> cb = std::bind(&PacketBuffer::Read, this, std::placeholders::_1, std::placeholders::_2);
    if (ret == bufferReturnCodes::EReturnCodesPacketBuffer::Ok)
    {
        return Packet(*sampleCount, startOfSpace, cb);
    }
    else if (ret == bufferReturnCodes::EReturnCodesPacketBuffer::AdjustedSize)
    {
        // The argument of the function needs to be changed to reflect the spec details
        std::cout << "The size of the packet is smaller than requested. It's so JOEVER " << std::endl;
        return Packet(*sampleCount, startOfSpace, cb);
    }
    else
    {
        // Maybe throw here, or something else (who knows)
        return Packet();
    }
}

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
    Packetet pb;
    void* check;
    size_t bb = 0;
    size_t* delique = &bb;
    *delique = 16;
    ASSERT_EQ(pb.callWriteSample(delique, &check), bufferReturnCodes::EReturnCodesPacketBuffer::Ok);
    *delique = 1024;
    ASSERT_EQ(pb.callWriteSample(delique, &check), bufferReturnCodes::EReturnCodesPacketBuffer::AdjustedSize);
}

TEST_F(CircularPacketTest, WriteFullRangeFill)
{
    Packetet pb;
    void* check;
    size_t bb = 0;
    size_t* fun = &bb;
    *fun = 512;
    ASSERT_EQ(pb.callWriteSample(fun, &check), bufferReturnCodes::EReturnCodesPacketBuffer::Ok);
    ASSERT_EQ(pb.callWriteSample(fun, &check), bufferReturnCodes::EReturnCodesPacketBuffer::Ok);
    *fun = 2;
    ASSERT_EQ(pb.callWriteSample(fun, &check), bufferReturnCodes::EReturnCodesPacketBuffer::OutOfMemory);       // This one fails because the buffer is full
}

TEST_F(CircularPacketTest, WriteAdjustedSize)
{
    Packetet pb;
    void* check;
    size_t bb = 0;
    size_t* run = &bb;
    *run = 1000;
    ASSERT_EQ(pb.callWriteSample(run, &check), bufferReturnCodes::EReturnCodesPacketBuffer::Ok);
    *run = 30;
    ASSERT_EQ(pb.callWriteSample(run, &check), bufferReturnCodes::EReturnCodesPacketBuffer::AdjustedSize);
}

TEST_F(CircularPacketTest, WriteEmptyCall)
{
    Packetet pb;
    void* check;
    size_t bb = 0;
    size_t* tun = &bb;
    *tun = 0;
    ASSERT_EQ(pb.callWriteSample(tun, &check), bufferReturnCodes::EReturnCodesPacketBuffer::Ok);
}


TEST_F(CircularPacketTest, ReadEmpty)
{
    Packetet pb;
    size_t bb = 0;
    size_t* gun = &bb;
    *gun = 0;
    ASSERT_EQ(pb.callReadSample(pb.getReadPos(), *gun), bufferReturnCodes::EReturnCodesPacketBuffer::Failure);
}

TEST_F(CircularPacketTest, ReadFromFullBuffer)
{
    Packetet pb;
    void* check;
    size_t bb = 0;
    size_t* wan = &bb;
    *wan = 1024;
    pb.callWriteSample(wan, &check);
    *wan = 512;
    ASSERT_EQ(pb.callReadSample(check, *wan), bufferReturnCodes::EReturnCodesPacketBuffer::Ok);
}

TEST_F(CircularPacketTest, ReadFullBuffer)
{
    Packetet pb;
    void* check;
    size_t bb = 0;
    size_t* hun = &bb;
    *hun = 512;
    pb.callWriteSample(hun, &check);
    *hun = 256;
    ASSERT_EQ(pb.callReadSample(check, *hun), bufferReturnCodes::EReturnCodesPacketBuffer::Ok);
    *hun = 1000;
    ASSERT_EQ(pb.callWriteSample(hun, &check), bufferReturnCodes::EReturnCodesPacketBuffer::AdjustedSize);
    *hun = 128;
    ASSERT_EQ(pb.callWriteSample(hun, &check), bufferReturnCodes::EReturnCodesPacketBuffer::Ok);
    *hun = 800;
    ASSERT_EQ(pb.callReadSample(check, *hun), bufferReturnCodes::EReturnCodesPacketBuffer::Ok);
}

TEST_F(CircularPacketTest, ReadPartialWorkflow)
{
    Packetet pb;
    void* check;
    size_t bb = 0;
    size_t* jun = &bb;
    *jun = 512; 
    pb.callWriteSample(jun, &check);
    *jun = 256;
    ASSERT_EQ(pb.callReadSample(check, *jun), bufferReturnCodes::EReturnCodesPacketBuffer::Ok);
    *jun = 128;
    ASSERT_EQ(pb.callReadSample(pb.getReadPos(), *jun), bufferReturnCodes::EReturnCodesPacketBuffer::Ok);
    *jun = 64;
    ASSERT_EQ(pb.callReadSample(pb.getReadPos(), *jun), bufferReturnCodes::EReturnCodesPacketBuffer::Ok);
    *jun = 32;
    ASSERT_EQ(pb.callReadSample(pb.getReadPos(), *jun), bufferReturnCodes::EReturnCodesPacketBuffer::Ok);
    *jun = 16;
    ASSERT_EQ(pb.callReadSample(pb.getReadPos(), *jun), bufferReturnCodes::EReturnCodesPacketBuffer::Ok);
    *jun = 8;
    ASSERT_EQ(pb.callReadSample(pb.getReadPos(), *jun), bufferReturnCodes::EReturnCodesPacketBuffer::Ok);
    *jun = 4;
    ASSERT_EQ(pb.callReadSample(pb.getReadPos(), *jun), bufferReturnCodes::EReturnCodesPacketBuffer::Ok);
    ASSERT_EQ(pb.callReadSample(pb.getReadPos(), *jun), bufferReturnCodes::EReturnCodesPacketBuffer::Ok);
    *jun = 1;
    ASSERT_EQ(pb.callReadSample(pb.getReadPos(), *jun), bufferReturnCodes::EReturnCodesPacketBuffer::Failure);
}


// Considering that Packet is used nowhere but here, there is a good reason to simply get rid of it,
// for it has no use outside of this custom test
TEST_F(CircularPacketTest, TestMockPacket)
{
    Packetet pb;
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

    Packetet pb;
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

    Packetet pb;
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

    Packetet pb;
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

    Packetet pb;
    size_t sampleCount = 100;

    {
        std::cout << "WritePoint before creation: " << pb.getWritePos() << std::endl;
        auto old_created = pb.createPacket(&sampleCount, descriptor, domain);
        std::cout << "WritePoint after outer scope creation: " << pb.getWritePos() << std::endl;
        std::cout << "ReadPoint after outer scopecreation: " << pb.getReadPos() << std::endl;
        {
            auto new_packet = pb.createPacket(&sampleCount, descriptor, domain);
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
    Packetet pb;
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

void createMultiThreadedPacket(Packetet *pb)
{
    auto [desc, dom] = generate_building_blocks();
    size_t n = 100;
    
    auto r = pb->createPacket(&n, desc, dom);
    std::lock_guard<std::mutex> lock(*(pb->goForLock()));
    std::cout << "Created in the thread: " << r.getRawDataSize() << std::endl;
}


TEST_F(CircularPacketTest, TestMultiThread)
{
    Packetet pb;
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

int createAndWaitPacket(daq::PacketBuffer& pb, daq::DataDescriptorPtr& desc, daq::DataPacketPtr& dom, std::condition_variable& cv)
{
    size_t n = 100;
    auto r = pb.createPacket(&n, desc, dom);
    std::mutex re;
    std::unique_lock<std::mutex> lck(re);
    cv.wait(lck,
             [&]
             {
                 t += 1;
                 return true;
             });
    lck.unlock();
    cv.notify_all();
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    std::cout << "Packet was created. " << std::endl;
    return 1;
}

 void cb(daq::PacketBuffer* pb)
{
   auto r = pb -> reset();
    std::cout << r << std::endl;
}

TEST_F(CircularPacketTest, TestReset)
{
    Packetet pb;
    std::condition_variable start_up_assurance;
    auto [descriptor, domain] = generate_building_blocks();

    std::thread th1 = std::thread(createAndWaitPacket,std::ref(pb), std::ref(descriptor), std::ref(domain), std::ref(start_up_assurance));

    std::thread th2 = std::thread(createAndWaitPacket, std::ref(pb), std::ref(descriptor), std::ref(domain), std::ref(start_up_assurance));

    std::thread th3 = std::thread(createAndWaitPacket, std::ref(pb), std::ref(descriptor), std::ref(domain), std::ref(start_up_assurance));

    std::thread th4 = std::thread(createAndWaitPacket, std::ref(pb), std::ref(descriptor), std::ref(domain), std::ref(start_up_assurance));

    std::mutex gh;
    std::unique_lock<std::mutex> rezan(gh);

    start_up_assurance.wait(rezan, [&] { return t >= 4; });
    std::thread th5(createAndWaitPacket, std::ref(pb), std::ref(descriptor), std::ref(domain), std::ref(start_up_assurance));

    cb(&pb);
    
    th1.join();
    th2.join();
    th3.join();
    th4.join();
    th5.join();
    ASSERT_EQ(pb.getWritePos(), pb.getReadPos());

}
