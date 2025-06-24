#include <opendaq/packet_buffer_impl.h>

BEGIN_NAMESPACE_OPENDAQ

PacketBufferImpl::PacketBufferImpl(IPacketBufferBuilder* builder)
{
    // Here comes the init of the buffer (that means the fresh std::vector gets called here)
    // also looks into the builder for the class
    SizeT stuff;
    builder->getSizeInBytes(&stuff);
    data = std::vector<uint8_t>(stuff);
    sizeOfMem = stuff;
    readPos = data.data();
    writePos = data.data();
    isFull = false;
    underReset = false;
    rawSampleSize = 1;
    sampleCount = stuff;
    builder->getContext(&context);
}

ErrCode PacketBufferImpl::Write(size_t* sampleCount, void** memPos)
{
    auto writePosVirtuallyAdjusted = (static_cast<uint8_t*>(writePos) + rawSampleSize * *sampleCount);
    auto endOfBuffer = (reinterpret_cast<uint8_t*>(&data) + rawSampleSize * sizeOfMem);
    auto readPosWritePosDiff = (static_cast<uint8_t*>(writePos) - static_cast<uint8_t*>(readPos)) / static_cast<uint8_t>(rawSampleSize);
    size_t availableSamples;

    bool sizeAdjusted = false;

    if (readPosWritePosDiff < 0)
    {
        availableSamples = static_cast<size_t>(std::abs(readPosWritePosDiff));
    }
    else
    {
        if (isFull && readPosWritePosDiff == 0)
        {
            DAQ_THROW_EXCEPTION(InvalidStateException);
            // Return problem
        }
        availableSamples = static_cast<size_t>(readPosWritePosDiff);
    }

    if (availableSamples < *sampleCount)
    {
        auto beginningReadPosDiff = (static_cast<uint8_t*>(readPos) - data.data());
        if (static_cast<long long>(*sampleCount) <= beginningReadPosDiff)
        {
            // I need to create an empty packet that will fill up the last part of the buffer,
            // and then create the requested size of packet at the beginning of the buffer
        }
        // Check if at the end of the buffer for write ahead (so write at the beginning of the buffer if the wated size does not fit)
    }
    else
    {
        *memPos = writePos;
        writePos = writePosVirtuallyAdjusted;
    }

    if (writePos == readPos)
    {
        isFull = true;
        if (sizeAdjusted)
        {
            // Is this endpoint still necessary
        }
        return OPENDAQ_SUCCESS;
    }
    if (writePos == (void*) endOfBuffer)
    {
        writePos = &data;
        if (readPos == writePos)
        {
            isFull = true;
        }
        if (sizeAdjusted)
        {
            // This endpoint might also be redundant
        }
        return OPENDAQ_SUCCESS;
    }

    return OPENDAQ_SUCCESS;
}

ErrCode PacketBufferImpl::Read(void* beginningOfDelegatedSpace, size_t sampleCount, size_t rawSize)
{
    {
        std::lock_guard<std::mutex> lock(mxFlip);
        if (beginningOfDelegatedSpace != readPos)
        {
            bool scenario = beginningOfDelegatedSpace < readPos;
            if (scenario)
            {
                oos_packets.push(std::make_pair(((uint8_t*) &data + sizeOfMem) + ((uint8_t*) beginningOfDelegatedSpace - (uint8_t*) &data),
                                                sampleCount * rawSize));
            }
            oos_packets.push(std::make_pair(beginningOfDelegatedSpace, sampleCount * rawSize));
            return OPENDAQ_SUCCESS;
        }
        else
        {
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
                    break;
                }
            }
        }
        if (readPos == writePos && !isFull)
        {
            // Return error codes
            DAQ_THROW_EXCEPTION(InvalidStateException);
        }

        readPos = static_cast<void*>(static_cast<uint8_t*>(readPos) + rawSize * sampleCount);

        if (static_cast<uint8_t*>(readPos) >= static_cast<uint8_t*>(data.data()) + rawSize * sizeOfMem)
            readPos = static_cast<void*>(static_cast<uint8_t*>(readPos) - (rawSize * sizeOfMem));

        isFull = false;
    }
    cv.notify_all();
    return OPENDAQ_SUCCESS;
}

ErrCode PacketBufferImpl::createPacket(SizeT SampleCount, IDataDescriptor* desc, IPacket* domainPacket, IDataPacket** packet)
{
    daq::DataRulePtr rule;
    desc->getRule(&rule);
    if (rule.getType() == daq::DataRuleType::Linear)
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Packet Buffer does not support Linear Data Rule Type packets.");

    std::unique_lock<std::mutex> lock(mxFlip);
    if (underReset)
    {
        // Here there should be a reintroduction of Logging stuff
        DAQ_THROW_EXCEPTION(InvalidStateException);  // Aka you should not create packets under reset
        // This maybe needs to be changed to reflect that a packet was trying to be created
    }
    desc->getRawSampleSize(&rawSampleSize);
    void* startOfSpace = nullptr;

    this->Write(&sampleCount, &startOfSpace);
    deleterFunction = [&, sampleCnt = sampleCount, startOfSpace = startOfSpace, rawSizeOfSample = rawSampleSize](void*)
    { Read(startOfSpace, sampleCnt, rawSizeOfSample); };
    auto deleter = daq::Deleter(std::move(deleterFunction));

    *packet = daq::DataPacketWithExternalMemory(domainPacket, desc, sampleCount, startOfSpace, deleter);

    return OPENDAQ_SUCCESS;
}

ErrCode PacketBufferImpl::getAvailableMemory(SizeT* count)
{
    *count = ((static_cast<uint8_t*>(writePos) - static_cast<uint8_t*>(data.data())) <=
              (static_cast<uint8_t*>(data.data()) + sizeOfMem - static_cast<uint8_t*>(readPos)))
                 ? (static_cast<uint8_t*>(data.data()) + sizeOfMem - static_cast<uint8_t*>(readPos))
                 : (static_cast<uint8_t*>(writePos) - static_cast<uint8_t*>(data.data()));
    return OPENDAQ_SUCCESS;
}

ErrCode PacketBufferImpl::getAvailableSampleCount(IDataDescriptor* desc, SizeT* count)
{
    auto allAvailableSamples = static_cast<uint8_t*>(data.data()) + sizeOfMem;  // End of buffer

    if (writePos == readPos)
    {
        if (isFull)
        {
            *count = 0;
        }
        auto fromEndToPos = allAvailableSamples - static_cast<uint8_t*>(readPos);
        auto fromStartToPos = static_cast<uint8_t*>(readPos) - static_cast<uint8_t*>(data.data());

        *count = (fromStartToPos <= fromEndToPos) ? fromEndToPos : fromStartToPos;
    }
    else
    {
        if (writePos > readPos)
        {
            auto fromEndToPos = allAvailableSamples - static_cast<uint8_t*>(writePos);
            auto fromStartToPos = static_cast<uint8_t*>(readPos) - static_cast<uint8_t*>(data.data());

            *count = (fromStartToPos <= fromEndToPos) ? fromEndToPos : fromStartToPos;
        }
        *count = static_cast<uint8_t*>(readPos) - static_cast<uint8_t*>(writePos);
    }
    return OPENDAQ_SUCCESS;
}

ErrCode PacketBufferImpl::resize(SizeT sizeInBytes)
{
    std::unique_lock<std::mutex> lock(mxFlip);
    underReset = true;

    this->cv.wait(lock, [&] { return ((readPos == writePos) && (!isFull)); });

    underReset = false;
    data = std::vector<uint8_t>(sizeInBytes);
    sizeOfMem = sizeInBytes;

    cv.notify_all();
    return OPENDAQ_SUCCESS;
}

OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY, PacketBuffer, IPacketBufferBuilder*, builder)

END_NAMESPACE_OPENDAQ
