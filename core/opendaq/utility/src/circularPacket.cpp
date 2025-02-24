#include <opendaq/circularPacket.h>

PacketBuffer::PacketBuffer()
{
    // Here I need to check for what the type of the PackageBuffer we will use (double, float and so on...)
    // The size will have to be adjusted when 
    sizeOfMem = 1024;
    sizeOfSample = sizeof(double);
    data = malloc(sizeOfMem * sizeOfSample);
    writePos = data;
    readPos = data;
    bIsFull = false;
    bAdjustedSize = false;
    sizeAdjusted = 0;
}

PacketBuffer::~PacketBuffer()
{
    free(data);
}

void PacketBuffer::setWritePos(size_t offset)
{
    writePos = (uint8_t*)writePos + sizeOfSample * offset;
}

void PacketBuffer::setReadPos(size_t offset)
{
    readPos = (size_t*)readPos + sizeOfSample * offset;
}

void* PacketBuffer::getWritePos()
{
    return writePos;
}

void* PacketBuffer::getReadPos()
{
    return readPos;
}

void PacketBuffer::setIsFull(bool bState)
{
    bIsFull = bState;
}

bool PacketBuffer::getIsFull()
{
    return bIsFull;
}

size_t PacketBuffer::getAdjustedSize()
{
    if (bAdjustedSize)
        return sizeAdjusted;

    return 0;
}

int PacketBuffer::WriteSample(size_t* sampleCount, void** memPos)
{
    // check if sampleCount is not out of scope (so check if readPos if ahead and check if the size does not reach over the sizeofMem)

    // malloc ensures contiguous memory
    if (writePos >= readPos)
    {
        if (writePos == readPos && bIsFull)
            return 1;
        // Here the writePos also needs to be moved
        // (check if the wanted size does not fit try putting it at the beginning)
        if (((uint8_t*) writePos + sizeOfSample * *sampleCount) < ((uint8_t*) data + sizeOfSample * sizeOfMem))
        {
            *memPos = writePos;
            writePos = (void*) ((uint8_t*) writePos + sizeOfSample * *sampleCount);
            if (writePos == readPos)
            {
                bIsFull = true;
            }
            return 0;
        }
        else if (((uint8_t*) writePos + sizeOfSample * *sampleCount) == ((uint8_t*) data + sizeOfSample * sizeOfMem))
        {
            *memPos = writePos;
            writePos = data;
            if (writePos == readPos)
            {
                bIsFull = true;
            }
            return 0;
        }
        else
        {
            // The amount has been changed (be careful)
            *memPos = writePos;
            bAdjustedSize = true;
            *sampleCount = ((sizeOfSample * *sampleCount) - (((uint8_t*) writePos + sizeOfSample * *sampleCount) - ((uint8_t*) data + sizeOfSample * sizeOfMem))) / sizeOfSample;
            sizeAdjusted = *sampleCount;
            writePos = data;
            if (writePos == readPos)
            {
                bIsFull = true;
            }
            return 2;
        }
    }
    else
    {
        if (((uint8_t*) writePos + sizeOfSample * *sampleCount) < (uint8_t*)readPos)
        {
            *memPos = writePos;
            writePos = (void*) ((uint8_t*) writePos + sizeOfSample * *sampleCount);
            return 0;
        }
        return 1;
    }

}

int PacketBuffer::ReadSample(size_t sampleCount)
{
    // I need to check if there is space between the writepos and readpos (with a wrap around)
    if (readPos >= writePos)
    {
        // Trying to empty an empty buffer
        if (readPos == writePos && !bIsFull)
            return 1;

        if ((uint8_t*)readPos + sizeOfSample * sampleCount < (uint8_t*)data + sizeOfSample * sizeOfMem)
        {
            bIsFull = false;
            // Everything fits up until the end of buffer
            readPos = (void*) ((uint8_t*) readPos + sizeOfSample * sampleCount);
            return 0;

        }
        else
        {
            auto remainder = ((uint8_t*) readPos + sizeOfSample * sampleCount) - ((uint8_t*) data + sizeOfSample * sizeOfMem);
            if ((uint8_t*)data + remainder < writePos)
            {
                bAdjustedSize = false;
                bIsFull = false;
                readPos = (void*)((uint8_t*) data + remainder);
                return 0;
            }
            // Is the edge case of equal important or no ?? (think about this one at home)
            // We need to wrap around the end of the buffer
            return 1;
        }
    }
    else
    {
        if ((uint8_t*)readPos + sizeOfSample * sampleCount < writePos)
        {
            bIsFull = false;
            readPos = (void*) ((uint8_t*) readPos + sizeOfSample * sampleCount);
            return 0;
        }
        else if ((uint8_t*) readPos + sizeOfSample * sampleCount == writePos)
        {
            readPos = writePos;
            bIsFull = false;
            return 0;
        }
        
        return 1;
    }

}

size_t PacketBuffer::getAvailableSampleCount()
{
    return (((uint8_t*)data + sizeOfSample * sizeOfMem) - (uint8_t*)writePos);
}

daq::DataPacketPtr PacketBuffer::createPacket(size_t* sampleCount, daq::DataDescriptorPtr dataDescriptor, daq::DataPacketPtr& domainPacket)
{
    sizeOfSample = dataDescriptor.getRawSampleSize();
    void* startOfSpace = nullptr;
    ff = [&, sampleCnt = *sampleCount](void*)
         {
             ReadSample(sampleCnt);
         };
    int ret = this->WriteSample(sampleCount, &startOfSpace);
    auto deleter = daq::Deleter(std::move(ff));
    // Here move is required because there is a smart pointer
    // somewhere in there (I think...)
    
    if (!ret)
    {
        return daq::DataPacketWithExternalMemory(domainPacket,
                                                 dataDescriptor,
                                                 (uint64_t)*sampleCount,
                                                 startOfSpace,
                                                 deleter);
    }
    else if (ret == 2)
    {
        // The argument of the function needs to be changed to reflect the spec details
        std::cout << "The size of the packet is smaller than requested. It's so JOEVER " << std::endl;
        return daq::DataPacketWithExternalMemory(domainPacket,
                                                 dataDescriptor,
                                                 (uint64_t) *sampleCount,
                                                 startOfSpace,
                                                 deleter);
    }
    else
    {
        // Maybe throw here, or something else (who knows)
        // This needs to be checked
        // (This might even throw (or in other way explain that this does not work fine))
        // If we get stuck here, it means the allocation of memory failed
        throw 1;
    }
}

Packet::Packet(size_t desiredNumOfSamples, void* beginningOfData, std::function<void(size_t)> callback)
{
    cb = std::move(callback);
    // Users code, users memory corruption problems
    sampleAmount = desiredNumOfSamples;
    assignedData = beginningOfData;
    
}

Packet::Packet()
{
    // Failed state
    sampleAmount = 0;
    assignedData = nullptr;

}

Packet::~Packet()
{
    cb(sampleAmount);
}


// This is a test function that was used to help gauge the behaviour of the buffer class
Packet PacketBuffer::createPacket(size_t* sampleCount, size_t dataDescriptor)
{
    void* startOfSpace = nullptr;
    int ret = this->WriteSample(sampleCount, &startOfSpace);
    std::function<void(size_t)> cb = std::bind(&PacketBuffer::ReadSample, this, std::placeholders::_1);
    if (!ret)
    {
        return Packet(*sampleCount, startOfSpace, cb);
    }
    else if (ret == 2)
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


void PacketBuffer::reset()
{
    // 

}
