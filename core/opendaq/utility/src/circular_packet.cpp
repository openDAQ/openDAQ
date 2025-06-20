#include <opendaq/circular_packet.h>

using namespace daq;

PacketBufferInit::PacketBufferInit(const DataDescriptorPtr& descriptor, const ContextPtr& context)
   // : descriptor(descriptor)
   // , context(context)
{
    if (!context.assigned())
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Context must be assigned on packet buffer creation!");

    if (!descriptor.assigned())
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Data descriptor cannot be empty.");

    // Is this a worth while constructor??
}

PacketBufferInit::PacketBufferInit(size_t memSize, const ContextPtr& context)
    : context(context)
    , sampleCount(memSize)
{
    // This might not be a good way of assigning this, becouse memSize is a product of 2 different values by default
    // I really don't like the way this is structured
}

PacketBufferInit::PacketBufferInit(size_t sampleSize, size_t sampleSizeInMilliseconds, const ContextPtr& context)
    : context(context)
    , sampleCount(sampleSize)
{
    // The amount of space that will be initialy taken up by the data std::vector (sampleSize * sampleSizInMilliseconds * 2000)
}

PacketBufferInit::PacketBufferInit(size_t sampleSize, std::chrono::milliseconds duration, const ContextPtr& context)
    : context(context)
    , sampleCount(sampleSize)
{
    // The amount of space initialy taken up by the vector (I guess the assumption is that a single sample will take up to 1 millisecond (??))
    // Here the space calculation will probably need some engineering magic
}

PacketBuffer::~PacketBuffer()
{
    logger.removeComponent("CircularBuffer");
}

PacketBuffer::PacketBuffer(const PacketBufferInit& instructions)
    : bIsFull(false)
    , bUnderReset(false)
    , sizeOfMem(instructions.sampleCount) // Is this actually just sample count? memSize is the norm usually.
    , rawSampleSize(instructions.descriptor.getRawSampleSize()) // Why not rawSampleSize
    , data(std::vector<uint8_t>(sizeOfMem * rawSampleSize))
    , logger(instructions.context.getLogger())
    , context(instructions.context)
    , sizeAdjusted(0)
    , bAdjustedSize(false)
{
    writePos = &data;
    readPos = &data;
}

PacketBuffer::PacketBuffer()
{
}


bufferReturnCodes::ReturnCodesPacketBuffer PacketBuffer::Write(size_t* sampleCount, void** memPos)
{
    // We lock the thread outside in createPacket

    auto writePosVirtuallyAdjusted = (static_cast<uint8_t*>(writePos) + rawSampleSize * *sampleCount);

    auto endOfBuffer = (reinterpret_cast<uint8_t*>(&data) + rawSampleSize * sizeOfMem);

    auto readPosWritePosDiff = (static_cast<uint8_t*>(writePos) - static_cast<uint8_t*>(readPos))/static_cast<int>(rawSampleSize);
    auto writePosEndBufferDiff = (endOfBuffer - static_cast<uint8_t*>(writePos))/static_cast<int>(rawSampleSize);

    size_t availableSamples;
    bool bSizeAdjusted = false;

    if (readPosWritePosDiff < 0)
    {
        availableSamples = static_cast<size_t>(std::abs(readPosWritePosDiff));
    }
    else
    {
        if (bIsFull && readPosWritePosDiff == 0)
            return bufferReturnCodes::ReturnCodesPacketBuffer::OutOfMemory;

        availableSamples = static_cast<size_t>(writePosEndBufferDiff);
    }

    if (availableSamples < *sampleCount)
    {
        // Adjust size
        *memPos = writePos;
        *sampleCount = availableSamples;
        writePos = static_cast<void*>(static_cast<uint8_t*>(writePos) + rawSampleSize * availableSamples);
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
            return bufferReturnCodes::ReturnCodesPacketBuffer::AdjustedSize;

        return bufferReturnCodes::ReturnCodesPacketBuffer::Ok;
    }
    if (writePos == (void*) endOfBuffer)
    {
        writePos = &data;
        if (readPos == writePos)
            bIsFull = true;

        if (bSizeAdjusted)
            return bufferReturnCodes::ReturnCodesPacketBuffer::AdjustedSize;

        return bufferReturnCodes::ReturnCodesPacketBuffer::Ok;
    }

    return bufferReturnCodes::ReturnCodesPacketBuffer::Ok;

}

bufferReturnCodes::ReturnCodesPacketBuffer PacketBuffer::Read(void* beginningOfDelegatedSpace, size_t sampleCount, size_t rawSize)
{
    {
        std::lock_guard<std::mutex> lock(mxFlip);
        if (beginningOfDelegatedSpace != readPos)
        {
            // If the OOS packet falls into space between the beginning of memory and readPos
            // we can artificially add it at the end and simulate
            // an extension of the circular buffer up to the writePos

            // |_____WritePos______ReadPos___|-----SimulatedWritePos
            bool scenario = beginningOfDelegatedSpace < readPos;
            if (scenario)
                oos_packets.push(
                    std::make_pair(((uint8_t*) &data + sizeOfMem) + ((uint8_t*) beginningOfDelegatedSpace - (uint8_t*) &data), sampleCount * rawSize));

            oos_packets.push(std::make_pair(beginningOfDelegatedSpace, sampleCount * rawSize));
            return bufferReturnCodes::ReturnCodesPacketBuffer::Ok;
        }
        else
        {
            // Here we just add to sampleCount until we hit either writePos
            // or simulatedWritePos
            sampleCount *= rawSize;
            auto mm = static_cast<void*>(static_cast<uint8_t*>(beginningOfDelegatedSpace) + sampleCount);

            while (!oos_packets.empty())
            {
                mm = static_cast<void*>(static_cast<uint8_t*>(beginningOfDelegatedSpace) + sampleCount);
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
            return bufferReturnCodes::ReturnCodesPacketBuffer::Failure;
        }

        // Due to checks in writeSample we cannot go further than WritePos

        // Therefore we only need to check for wraparound and adjust for that

        readPos = (void*) ((uint8_t*) readPos + rawSize * sampleCount);

        // Re-think if all calculations should contain the rawSize*sizeOfMem part of the calculation

        // Notes on reconfiguration change the sizeOfMem to already have internally calculate rawSize*sampleAmount
        // when instaciating the object

        // <-- Look if anywhere the sizeOfMem is being used on it's own

        if ((uint8_t*) readPos >= (uint8_t*) &data + rawSize * sizeOfMem)
            readPos = (void*) ((uint8_t*) readPos - (rawSize * sizeOfMem));

        bIsFull = false;
    }
    cv.notify_all();
    return bufferReturnCodes::ReturnCodesPacketBuffer::Ok;
    
    // I need to think about the full buffer state (when writePos is a start)
   
}

size_t PacketBuffer::getAvailableSampleCount() const
{
    auto ff_g = (uint8_t*) &data + sizeOfMem * rawSampleSize;    // All samples (wrong calculation (works only under the assumption that rawSampleSize does not change))

    if (writePos == readPos)
    {
        if (bIsFull)
            return 0;                                           // Buffer is full
        return (uint8_t*)&data - (uint8_t*)readPos;             // Rpos and Wpos are allined, but buffer is empty
                                                                // this needs to check if the area b4 or after the allignment is bigger
    }
    else
    {
        if (writePos > readPos)                                 // |_____Rpos_____Wpos|__this_space__|
            return ff_g - (uint8_t*)writePos;                   // All minus the Wpos
        return (uint8_t*)readPos - (uint8_t*)writePos;          // |_____Wpos|__this_space__|Rpos___|
    }
        
    // This is the recommended way of doing this from the code review     
    // return (((uint8_t*)data + rawSampleSize * sizeOfMem) - (uint8_t*)writePos);

}

DataPacketPtr PacketBuffer::createPacket(size_t* sampleCount, daq::DataDescriptorPtr dataDescriptor, daq::DataPacketPtr& domainPacket)
{
    if (dataDescriptor.getRule().getType() == daq::DataRuleType::Linear)
    {
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Circular buffer does not support Linear Data Rule Type packets.");
    }

    std::unique_lock<std::mutex> lock(mxFlip);
    if (bUnderReset)
    {
        LOG_D("Under ongoing reset, cannot create new packets")
        //std::cerr << "Under ongoing reset, cannot create new packets" << std::endl;
        //this->cv.wait(loa, [&] { return !bUnderReset; });
        return daq::DataPacketPtr();
    }
    rawSampleSize = dataDescriptor.getRawSampleSize();
    void* startOfSpace = nullptr;
    // Here the should be a lock for creation

    bufferReturnCodes::ReturnCodesPacketBuffer ret = this->Write(sampleCount, &startOfSpace);
    deleterFunction = [&, sampleCnt = *sampleCount, startOfSpace = startOfSpace, rawSizeOfSample = rawSampleSize](void*)
         {
             Read(startOfSpace, sampleCnt, rawSizeOfSample);
         };
    auto deleter = daq::Deleter(std::move(deleterFunction));
    
    if (ret < bufferReturnCodes::ReturnCodesPacketBuffer::OutOfMemory)
    {
        if (ret == bufferReturnCodes::ReturnCodesPacketBuffer::AdjustedSize)
        {
            LOG_W("The size of the packet is smaller than requested. It's so JOEVER")
        }

        return daq::DataPacketWithExternalMemory(domainPacket, dataDescriptor, (uint64_t) *sampleCount, startOfSpace, deleter);
    }
    else
    {
        if (ret == bufferReturnCodes::ReturnCodesPacketBuffer::OutOfMemory)
        {
            LOG_E("We ran out of memory...")
        }
        else
        {
            LOG_E("Something went very wrong...")
        }
        return daq::DataPacketPtr();
    }
}

int PacketBuffer::reset()
{
    // When reset is invoked the WriteSample functionality should be locked,
    // we must not lock the entire PacketBuffer itself

    std::unique_lock<std::mutex> lock(mxFlip);
    bUnderReset = true;
    this->cv.wait(lock, [&]
            {
                LOG_D("Calling the check in reset.")
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

    reset();

    // crude, but effective
    if (!this->isEmpty())
        DAQ_THROW_EXCEPTION(InvalidStateException, "Reset procedure has failed.");

    rawSampleSize = instructions.descriptor.getRawSampleSize();
    sizeOfMem = instructions.sampleCount;
    data = std::vector<uint8_t> (sizeOfMem * rawSampleSize);
    writePos = &data;
    readPos = &data;
    bIsFull = false;
    bUnderReset = false;
    bAdjustedSize = false;
    sizeAdjusted = 0;

}

bool PacketBuffer::isEmpty() const
{
    return ((readPos == writePos) && !bIsFull);
}
