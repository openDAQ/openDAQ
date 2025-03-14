#pragma once

#include <opendaq/sample_type.h>
#include <opendaq/data_packet_ptr.h>
#include <opendaq/packet_factory.h>
#include <opendaq/deleter_factory.h>
#include <opendaq/deleter_impl.h>
#include <iostream>
#include <mutex>
#include <vector>
#include <queue>
#include <memory>
#include <functional>
#include <opendaq/data_descriptor_ptr.h>


BEGIN_NAMESPACE_OPENDAQ


// Think about what needs also to be included in here...

struct PUBLIC_EXPORT PacketBufferInit
{
    PacketBufferInit(daq::DataDescriptorPtr desc, size_t sA, enum EnumAdjustSize eAdjust);

    daq::DataDescriptorPtr desc;
    size_t sampleAmount;
    enum EnumAdjustSize sizeAdjustment;
};

enum EnumAdjustSize
{
    Invalid = 0,
    AdjustOnEnd,
    Fragment,
    PreMadePackets
};

class PacketBuffer;

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


class PUBLIC_EXPORT PacketBuffer
{
    // When reset is invoked the WriteSample functionality should be locked,
    // we must not lock the entire PacketBuffer itself

public:
    PacketBuffer();

    PacketBuffer(size_t sampleSize, size_t memSize);

    PacketBuffer(const PUBLIC_EXPORT PacketBufferInit& instructions);

    ~PacketBuffer();

    void resize(const PUBLIC_EXPORT PacketBufferInit& instructions);

    // int => return code
    int WriteSample(size_t* sampleCount, void** memPos);

    int ReadSample(void* beginningOfDelegatedSpace, size_t sampleCount);

    size_t getAvailableSampleCount();

    daq::DataPacketPtr createPacket(size_t* sampleCount, daq::DataDescriptorPtr dataDescriptor, daq::DataPacketPtr& domainPacket);

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
