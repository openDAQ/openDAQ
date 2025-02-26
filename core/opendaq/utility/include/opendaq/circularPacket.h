#pragma once

#include <opendaq/sample_type.h>
#include <opendaq/data_packet_ptr.h>
#include <opendaq/packet_factory.h>
#include <opendaq/deleter_factory.h>
#include <opendaq/deleter_impl.h>
#include <iostream>                 // This one is a bit overkill and can be removed with no major problems
#include <vector>
#include <memory>
#include <cstdint>
#include <functional>
#include <opendaq/data_descriptor_ptr.h>

enum EnumVariableType
{
    Invalid = 0,
    Int,
    Float,
    Char,
    Structs
};

class PacketBuffer;

class Packet
{
public:
    Packet();
    Packet(size_t desiredNumOfSamples, void* beginningOfData, std::function<void(size_t)> callback);
    ~Packet();
    size_t sampleAmount;
    void* assignedData;

private:
    std::function<void(size_t)> cb = 0;
};


class PacketBuffer
{
public:
    PacketBuffer();
    ~PacketBuffer();

    // int => return code
    int WriteSample(size_t* sampleCount, void** memPos);

    int ReadSample(size_t sampleCount);

    size_t getAvailableSampleCount();

    daq::DataPacketPtr createPacket(size_t* sampleCount, daq::DataDescriptorPtr dataDescriptor, daq::DataPacketPtr& domainPacket);

    Packet createPacket(size_t* sampleCount, size_t dataDescriptor);

    void reset();

//protected:
    // Testing methods
    void setWritePos(size_t offset);
    void setReadPos(size_t offset);

    void* getWritePos();
    void* getReadPos();

    void setIsFull(bool bState);
    bool getIsFull();

    size_t getAdjustedSize();

private:
    bool bIsFull;
    size_t sizeOfMem;
    size_t sizeOfSample;
    void* data;
    void* writePos;
    void* readPos;
    std::function<void(void*)> ff;

    // This is a temporary solution for
    // situation of the sampleCount being to big to fit
    size_t sizeAdjusted;
    bool bAdjustedSize;
};
