#include <opendaq/circular_packet.h>

using namespace daq;

PacketBufferInit::PacketBufferInit(const daq::DataDescriptorPtr& description, size_t sampleAmount, const ContextPtr ctx)
{
    if (ctx != nullptr)
    {
        logger = ctx.getLogger();
    }
    else
    {
        logger = Logger();
    }

    if (description == nullptr)
    {
        throw;
    }
    desc = *description;

    sampleCount = sampleAmount;

    if (sampleAmount == 0)
    {
        if (desc.getUnit().getSymbol() != "s")
            return ;

        auto r = desc.getTickResolution();
        auto newSampleAmount = 2 * (1 / r.getNumerator() / r.getDenominator());  // 2s worth of packets for the buffer
        sampleCount = newSampleAmount;
    }
}

PacketBuffer::PacketBuffer(size_t sampleSize, size_t memSize, const ContextPtr ctx)
{
    if (ctx == nullptr)
    {
        logger = Logger(); // For testing (????)
    }
    else
    {
        logger = ctx.getLogger();
    }

    loggerComponent = logger.getOrAddComponent("CircularBuffer");
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
    logger.removeComponent("CircularBuffer");
    free(data);
}

PacketBuffer::PacketBuffer(const PacketBufferInit& instructions)
{
    logger = instructions.logger;
    loggerComponent = logger.getOrAddComponent("CircularBuffer");
    sizeOfSample = instructions.desc.getRawSampleSize();
    sizeOfMem = instructions.sampleCount;
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

bufferReturnCodes::EReturnCodesPacketBuffer PacketBuffer::Write(size_t* sampleCount, void** memPos)
{
    // We lock the thread outside in createPacket

    auto writePosVirtuallyAdjusted = ((uint8_t*) writePos + sizeOfSample * *sampleCount);

    auto endOfBuffer = ((uint8_t*) data + sizeOfSample * sizeOfMem);

    auto readPosWritePosDiff = ((uint8_t*) writePos - (uint8_t*) readPos)/(int)sizeOfSample;
    auto writePosEndBufferDiff = (endOfBuffer - (uint8_t*) writePos)/(int)sizeOfSample;

    size_t availableSamples;
    bool bSizeAdjusted = false;

    if (readPosWritePosDiff < 0)
    {
        availableSamples = (size_t) std::abs(readPosWritePosDiff);
    }
    else
    {
        if (bIsFull && readPosWritePosDiff == 0)
            return bufferReturnCodes::EReturnCodesPacketBuffer::OutOfMemory;

        availableSamples = (size_t) writePosEndBufferDiff;
    }

    if (availableSamples < *sampleCount)
    {
        // Adjust size
        *memPos = writePos;
        *sampleCount = availableSamples;
        writePos = (void*) ((uint8_t*) writePos + sizeOfSample * availableSamples);
        bSizeAdjusted = true;
    }
    else
    {
        *memPos = writePos;
        writePos = writePosVirtuallyAdjusted;
    }

    if (writePos == readPos)
    {
        bIsFull = true;

        if (bSizeAdjusted)
            return bufferReturnCodes::EReturnCodesPacketBuffer::AdjustedSize;

        return bufferReturnCodes::EReturnCodesPacketBuffer::Ok;
    }
    if (writePos == (void*) endOfBuffer)
    {
        writePos = data;
        if (readPos == writePos)
            bIsFull = true;

        if (bSizeAdjusted)
            return bufferReturnCodes::EReturnCodesPacketBuffer::AdjustedSize;

        return bufferReturnCodes::EReturnCodesPacketBuffer::Ok;
    }

    return bufferReturnCodes::EReturnCodesPacketBuffer::Ok;

}

bufferReturnCodes::EReturnCodesPacketBuffer PacketBuffer::Read(void* beginningOfDelegatedSpace, size_t sampleCount)
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
            return bufferReturnCodes::EReturnCodesPacketBuffer::Ok;
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
            return bufferReturnCodes::EReturnCodesPacketBuffer::Failure;
        }

        // Due to checks in writeSample we cannot go further than WritePos

        // Therefore we only need to check for wraparound and adjust the that

        readPos = (void*) ((uint8_t*) readPos + sizeOfSample * sampleCount);

        if ((uint8_t*) readPos >= (uint8_t*) data + sizeOfSample * sizeOfMem)
            readPos = (void*) ((uint8_t*) readPos - (sizeOfSample * sizeOfMem));

        bIsFull = false;
    }
    cv.notify_all();
    return bufferReturnCodes::EReturnCodesPacketBuffer::Ok;
    
    // I need to think about the full buffer state (when writePos is a start)
   
}

size_t PacketBuffer::getAvailableSampleCount()
{
    auto ff_g = (uint8_t*) data + sizeOfMem * sizeOfSample;

    if (writePos == readPos)
    {
        if (bIsFull)
            return 0;
        return (uint8_t*)data - (uint8_t*)readPos;
    }
    else
    {
        if (writePos > readPos)
            return ff_g - (uint8_t*)writePos;
        return (uint8_t*)readPos - (uint8_t*)writePos;
    }
    
    // This is the recommended way of doing this from the code review
     
    //return (((uint8_t*)data + sizeOfSample * sizeOfMem) - (uint8_t*)writePos);
}

DataPacketPtr PacketBuffer::createPacket(size_t* sampleCount, daq::DataDescriptorPtr dataDescriptor, daq::DataPacketPtr& domainPacket)
{
    std::unique_lock<std::mutex> loa(flip);
    if (bUnderReset)
    {
        LOG_D("Under ongoing reset, cannot create new packets\r\n")
        //std::cerr << "Under ongoing reset, cannot create new packets" << std::endl;
        //this->cv.wait(loa, [&] { return !bUnderReset; });
        return daq::DataPacketPtr();
    }
    sizeOfSample = dataDescriptor.getRawSampleSize();
    void* startOfSpace = nullptr;
    // Here the should be a lock for creation

    bufferReturnCodes::EReturnCodesPacketBuffer ret = this->Write(sampleCount, &startOfSpace);
    deleterFunction = [&, sampleCnt = *sampleCount, startOfSpace = startOfSpace](void*)
         {
             Read(startOfSpace, sampleCnt);
         };
    auto deleter = daq::Deleter(std::move(deleterFunction));
    
    if (ret < bufferReturnCodes::EReturnCodesPacketBuffer::OutOfMemory)
    {
        if (ret == bufferReturnCodes::EReturnCodesPacketBuffer::AdjustedSize)
        {
            LOG_D("The size of the packet is smaller than requested. It's so JOEVER\r\n")
        }
            //std::cout << "The size of the packet is smaller than requested. It's so JOEVER " << std::endl;

        return daq::DataPacketWithExternalMemory(domainPacket, dataDescriptor, (uint64_t) *sampleCount, startOfSpace, deleter);
    }
    else
    {
        if (ret == bufferReturnCodes::EReturnCodesPacketBuffer::OutOfMemory)
        {
            LOG_W("We ran out of memory...\r\n")
        }
        else
        {
            LOG_W("Something went very wrong... \r\n")
        }
        return daq::DataPacketPtr();
    }
}

int PacketBuffer::reset()
{
    std::unique_lock<std::mutex> lock(flip);
    bUnderReset = true;
    this->cv.wait(lock, [&]
            {
                LOG_W("Calling the check in reset. \r\n")
                std::cerr << "Calling the check in reset. " << std::endl;
                return ((readPos == writePos) && (!bIsFull));
            });

    bUnderReset = false;
    cv.notify_all();
    return 0;
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
    sizeOfMem = instructions.sampleCount;
    data = malloc(sizeOfMem * sizeOfSample);
    writePos = data;
    readPos = data;
    bIsFull = false;
    bUnderReset = false;
    bAdjustedSize = false;
    sizeAdjusted = 0;

}
