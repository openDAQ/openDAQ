#include <opendaq/circularPacket.h>

using namespace daq;

PacketBufferInit::PacketBufferInit(daq::DataDescriptorPtr description, EnumAdjustSize eAdjust, size_t sA)
{
    desc = description;
    sizeAdjustment = eAdjust;
    sampleAmount = sA;
    bUsingBuffer = true;
    if (sA == 0)
    {
        auto r = desc.getTickResolution();
        auto newSampleAmount = 2 * (1 / r.getNumerator() / r.getDenominator());  // 2s worth of packets for the buffer
        sampleAmount = newSampleAmount;
    }
}


PacketBuffer::PacketBuffer()
{
    sizeOfMem = 1024;
    sizeOfSample = sizeof(double);
    data = malloc(sizeOfMem * sizeOfSample);
    writePos = data;
    readPos = data;
    bIsFull = false;
    bAdjustedSize = false;
    bUnderReset = false;
    sizeAdjusted = 0;
}

PacketBuffer::PacketBuffer(size_t sampleSize, size_t memSize)
{
    sizeOfMem = memSize;
    sizeOfSample = sampleSize;
    data = malloc(sizeOfMem * sizeOfSample);
    writePos = data;
    readPos = data;
    bIsFull = false;
    bAdjustedSize = false;
    bUnderReset = false;
    sizeAdjusted = 0;
}

PacketBuffer::~PacketBuffer()
{
    free(data);
}

PacketBuffer::PacketBuffer(const PacketBufferInit& instructions)
{
    sizeOfSample = instructions.desc.getRawSampleSize();
    sizeOfMem = instructions.sampleAmount;
    data = malloc(sizeOfMem * sizeOfSample);
    writePos = data;
    readPos = data;
    bIsFull = false;
    bUnderReset = false;
    bAdjustedSize = false;
    sizeAdjusted = 0;
    // enumAdjustSize we will come back to this when,
    // alternative implementation is made
    // (for now we will ignore it)
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

bool PacketBuffer::isEmpty()
{
    return writePos == readPos && !bIsFull;
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
    //std::lock_guard<std::mutex> lock(flip);
    if (writePos >= readPos)
    {
        if (writePos == readPos && bIsFull)
        {
            return 1;
        }
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

int PacketBuffer::ReadSample(void* beginningOfDelegatedSpace, size_t sampleCount)
{
    {
        std::lock_guard<std::mutex> lock(flip);
        if (beginningOfDelegatedSpace != readPos)
        {
            // If the OOS packet falls into space between the beginning of memory and readPos
            // we can artificially add it at the end and simulate
            // an extension of the circular buffer up to the writePos

            // |_____WP______RP___|-----SWP

            if (beginningOfDelegatedSpace < readPos)
                oos_packets.push(
                    std::make_pair(((uint8_t*) data + sizeOfMem) + ((uint8_t*) beginningOfDelegatedSpace - (uint8_t*) data), sampleCount));

            oos_packets.push(std::make_pair(beginningOfDelegatedSpace, sampleCount));
            return 0;
        }
        else
        {
            // Here we just add to sampleCount until we hit either writePos
            // or simulatedWritePos
            while (!oos_packets.empty())
            {
                auto mm = (void*) ((uint8_t*) beginningOfDelegatedSpace + sizeOfSample * sampleCount);
                if (oos_packets.top().first == mm)
                {
                    sampleCount += oos_packets.top().second;
                    oos_packets.pop();
                }
                else
                {
                    // This could maybe use a nicer way of concluding...
                    break;
                }
            }
        }

        // Empty exit
        if (readPos == writePos && !bIsFull)
        {
            return 1;
        }

        // Due to checks in writeSample we cannot go further than WritePos

        // Therefore we only need to check for wraparound and adjust the that

        if ((uint8_t*) readPos + sizeOfSample * sampleCount >= (uint8_t*) data + sizeOfSample * sizeOfMem)
            readPos = (void*) ((uint8_t*) data +
                               ((uint8_t*) readPos + sizeOfSample * sampleCount - ((uint8_t*) data + sizeOfSample * sizeOfMem)));
        else
            readPos = (void*) ((uint8_t*) readPos + sizeOfSample * sampleCount);

        bIsFull = false;
    }
    //std::cout << "We messing with memory... " << std::endl;
    cv.notify_all();
    return 0;
    
    // I need to think about the full buffer state (when writePos is a start)
   
}

size_t PacketBuffer::getAvailableSampleCount()
{
    return (((uint8_t*)data + sizeOfSample * sizeOfMem) - (uint8_t*)writePos);
}

DataPacketPtr PacketBuffer::createPacket(size_t* sampleCount, daq::DataDescriptorPtr dataDescriptor, daq::DataPacketPtr& domainPacket)
{
    std::unique_lock<std::mutex> loa(flip);
    if (bUnderReset)
    {
        std::cerr << "Under ongoing reset, cannot create new packets" << std::endl;
        cv.wait(loa, [&] { return !bUnderReset; });
        return daq::DataPacketPtr();
    }
    sizeOfSample = dataDescriptor.getRawSampleSize();
    void* startOfSpace = nullptr;
    // Here the should be a lock for creation
    //std::cout << "Packet was created. " << std::endl;
    int ret = this->WriteSample(sampleCount, &startOfSpace);
    ff = [&, sampleCnt = *sampleCount, startOfSpace = startOfSpace](void*)
         {
             ReadSample(startOfSpace, sampleCnt);
         };
    auto deleter = daq::Deleter(std::move(ff));
    
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
        std::cerr << "You are trying to create an empty packet (or you are trying to create a packet while the buffer is full)." \
                     " \r\n We have returned an empty packet, therefore wherever this is called, this needs to be checked for"\
                     "\r\n" << std::endl; 

        return daq::DataPacketPtr();
    }
}

int PacketBuffer::reset()
{
    std::unique_lock<std::mutex> lock(flip);
    bUnderReset = true;
    cv.wait(lock, [&]
            {
                std::cerr << "Calling the check in reset. " << std::endl;
            return ((readPos == writePos) && (!bIsFull)); });

    bUnderReset = false;
    cv.notify_all();
    return 0;
 }

Packet::Packet(size_t desiredNumOfSamples, void* beginningOfData, std::function<void(void*, size_t)> callback)
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
    cb(assignedData,sampleAmount);
}

// This is a test function that was used to help gauge the behaviour of the buffer class
Packet PacketBuffer::cP(size_t* sampleCount, size_t dataDescriptor)
{

    void* startOfSpace = nullptr;
    int ret = this->WriteSample(sampleCount, &startOfSpace);
    std::function<void(void*,size_t)> cb = std::bind(&PacketBuffer::ReadSample, this, std::placeholders::_1, std::placeholders::_2);
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

void PacketBuffer::resize(const PacketBufferInit& instructions)
{
    // If I were to give a new PacketBufferInit into here then a new malloc could be called
    // similarly to the way buffer gets created in the constructor, we can create a new version in this function
    // 

    reset();

    // crude, but effective
    if (!this->isEmpty())
        throw;

    free(data);

    sizeOfSample = instructions.desc.getRawSampleSize();
    sizeOfMem = instructions.sampleAmount;
    data = malloc(sizeOfMem * sizeOfSample);
    writePos = data;
    readPos = data;
    bIsFull = false;
    bUnderReset = false;
    bAdjustedSize = false;
    sizeAdjusted = 0;

}
