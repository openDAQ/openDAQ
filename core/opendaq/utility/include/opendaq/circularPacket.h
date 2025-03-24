#pragma once

#include <opendaq/sample_type.h>
#include <opendaq/data_packet_ptr.h>
#include <opendaq/packet_factory.h>
#include <opendaq/deleter_factory.h>
#include <opendaq/deleter_impl.h>
#include <iostream>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <queue>
#include <memory>
#include <functional>
#include <opendaq/data_descriptor_ptr.h>


BEGIN_NAMESPACE_OPENDAQ


// Think about what needs also to be included in here...
/* enum EnumAdjustSize
{
    Invalid = 0,
    AdjustOnEnd,
    Fragment,
    PreMadePackets
};*/

struct PacketBufferInit
{

    PUBLIC_EXPORT PacketBufferInit(daq::DataDescriptorPtr description, /* EnumAdjustSize eAdjust,*/ size_t sA = 0);

    daq::DataDescriptorPtr desc;
    size_t sampleAmount;
    //EnumAdjustSize sizeAdjustment;

    // sampleAmount will be relative to the acqloop and the amount of time that is required
    // to go through the aquisition loop of data.

    // This boolean below is supposed to be a switch for using either a buffer
    // or 'classic malloc' when creating new packets
    //bool bUsingBuffer;

    // Check on the return codes and allow them to be
    // handled how the user wants them to be handled

    // I need to add a bool that can be used to change between packetBuffer and
    // malloc packet aquisition

};


class PacketBuffer;

// Move this into the test_suite (it isn't required here)
// (* from here

class Packet
{
public:
    Packet();
    Packet(size_t desiredNumOfSamples, void* beginningOfData, std::function<void(void*, size_t)> callback);
    ~Packet();
    size_t sampleAmount;
    void* assignedData;

private:
    std::function<void(void*, size_t)> cb = 0;
};
// to here *) 

class PacketBuffer
{
    // When reset is invoked the WriteSample functionality should be locked,
    // we must not lock the entire PacketBuffer itself

public:
    PUBLIC_EXPORT PacketBuffer();

    PUBLIC_EXPORT PacketBuffer(size_t sampleSize, size_t memSize);

    PUBLIC_EXPORT PacketBuffer(const PacketBufferInit& instructions);

    PUBLIC_EXPORT ~PacketBuffer();

    PUBLIC_EXPORT void resize(const PacketBufferInit& instructions);

    // int => return code
    int WriteSample(size_t* sampleCount, void** memPos);

    int ReadSample(void* beginningOfDelegatedSpace, size_t sampleCount);

    PUBLIC_EXPORT size_t getAvailableSampleCount();

    PUBLIC_EXPORT daq::DataPacketPtr createPacket(size_t* sampleCount,
                                                 daq::DataDescriptorPtr dataDescriptor,
                                                 daq::DataPacketPtr& domainPacket);

    Packet cP(size_t* sampleCount, size_t dataDescriptor);

    bool isEmpty();

    int reset();

//protected:
    // Testing methods
    void setWritePos(size_t offset);
    void setReadPos(size_t offset);

    void* getWritePos();
    void* getReadPos();

    void setIsFull(bool bState);
    bool getIsFull();

    size_t getAdjustedSize();

    std::mutex flip;
    std::condition_variable cv;
private:
    bool bIsFull;
    bool bUnderReset;
    size_t sizeOfMem;
    size_t sizeOfSample;
    void* data;
    void* writePos;
    void* readPos;
    std::priority_queue<std::pair<void*, size_t>, std::vector<std::pair<void*, size_t>>, std::greater<std::pair<void*, size_t>>> oos_packets;
    std::vector<std::weak_ptr<daq::DataPacketPtr>> dd; // This is one of the ways
    std::function<void(void*)> ff;


    // This is a temporary solution for
    // situation of the sampleCount being to big to fit
    size_t sizeAdjusted;
    bool bAdjustedSize;
};


END_NAMESPACE_OPENDAQ
