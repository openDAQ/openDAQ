#include <opendaq/packet_buffer_impl.h>

BEGIN_NAMESPACE_OPENDAQ

PacketBufferImpl::PacketBufferImpl(const PacketBufferBuilderPtr& builder)
{
    // Here comes the init of the buffer (that means the fresh std::vector gets called here)
    // also looks into the builder for the class
    sizeInBytes = builder.getSizeInBytes();
    context = builder.getContext();

    data = std::vector<uint8_t>(sizeInBytes);
    readPos = data.data();
    writePos = data.data();
    isFull = false;
    underReset = false;
}


ErrCode PacketBufferImpl::Write(size_t sampleCount, size_t rawSampleSize, void** memPos)
{
    auto writePosVirtuallyAdjusted = (static_cast<uint8_t*>(writePos) + rawSampleSize * sampleCount);
    auto endOfBuffer = (reinterpret_cast<uint8_t*>(data.data()) + sizeInBytes);
    auto readPosWritePosDiff = (static_cast<uint8_t*>(writePos) - static_cast<uint8_t*>(readPos)) / static_cast<uint8_t>(rawSampleSize);
    size_t availableSamples;

    bool sizeAdjusted = false;

    if (readPosWritePosDiff < 0)
    {
        // This one needs to be changed to calculate the difference between the end and writePos
        availableSamples = static_cast<size_t>(std::abs(readPosWritePosDiff));
    }
    else
    {
        if (isFull && readPosWritePosDiff == 0)
        {
            
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_BUFFERFULL, "The packet buffer is full");
            // Return problem
        }
        else if (readPosWritePosDiff == 0)
        {
            availableSamples = static_cast<size_t>(endOfBuffer - static_cast<uint8_t*>(writePos))/ static_cast<size_t>(rawSampleSize);
        }
        else
        {
            availableSamples = static_cast<size_t>(endOfBuffer - static_cast<uint8_t*>(writePos)) / static_cast<size_t>(rawSampleSize);
        }

    }

    if (availableSamples < sampleCount)
    {
        size_t beginningReadPosDiff = (static_cast<uint8_t*>(readPos) - data.data())/rawSampleSize;
        if (sampleCount <= beginningReadPosDiff)
        {
            // Dummy packet
            oosPackets.push(std::make_pair(static_cast<uint8_t*>(writePos), (static_cast<uint8_t*>(data.data()) + sizeInBytes) - static_cast<uint8_t*>(writePos)));

            // The main packet at the beginning
            *memPos = data.data();
            writePos = static_cast<void*>(data.data() + sampleCount * rawSampleSize); 
            if (writePos == readPos)
            {
                isFull = true;
                return OPENDAQ_SUCCESS;
            }
            return OPENDAQ_SUCCESS;
        }

        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "The requested packet size is no longer available.");
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
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDSTATE, "The size of the requested packet has been adjusted.");
        }
        return OPENDAQ_SUCCESS;
    }
    if (writePos == (void*) endOfBuffer)
    {
        writePos = data.data();
        if (readPos == writePos)
        {
            isFull = true;
        }
        if (sizeAdjusted)
        {
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDSTATE, "The size of the requested packet has been adjusted.");
        }
        return OPENDAQ_SUCCESS;
    }

    return OPENDAQ_SUCCESS;
}

ErrCode PacketBufferImpl::Read(void* beginningOfDelegatedSpace, size_t sampleCount, size_t rawSize)
{
    {
        std::unique_lock<std::mutex> lock(readWriteMutex);
        if (beginningOfDelegatedSpace != readPos)
        {
            bool scenario = beginningOfDelegatedSpace < readPos;
            if (scenario)
            {
                oosPackets.push(std::make_pair((data.data() + sizeInBytes) + (static_cast<uint8_t*>(beginningOfDelegatedSpace) - data.data()),
                                                sampleCount * rawSize));
            }
            else
            {
                oosPackets.push(std::make_pair(static_cast<uint8_t*>(beginningOfDelegatedSpace), sampleCount * rawSize));
            }
            lock.unlock();
            return OPENDAQ_SUCCESS;
        }
        else
        {
            sampleCount = sampleCount * rawSize;
            auto mm = static_cast<void*>(static_cast<uint8_t*>(beginningOfDelegatedSpace) + sampleCount);

            while (!oosPackets.empty())
            {
                mm = static_cast<void*>(static_cast<uint8_t*>(beginningOfDelegatedSpace) + sampleCount);
                if (oosPackets.top().first == mm)
                {
                    sampleCount += oosPackets.top().second;
                    oosPackets.pop();
                }
                else
                {
                    break;
                }
            }
        }
        if (readPos == writePos && !isFull)
        {
            lock.unlock();
            
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDSTATE, "You are trying to empty an already empty buffer.");
        }

        readPos = static_cast<void*>(static_cast<uint8_t*>(readPos) + sampleCount);

        if (static_cast<uint8_t*>(readPos) >= static_cast<uint8_t*>(data.data()) + sizeInBytes)
            readPos = static_cast<void*>(static_cast<uint8_t*>(readPos) - sizeInBytes);

        isFull = false;
        lock.unlock();
    }
    resizeSync.notify_all();
    return OPENDAQ_SUCCESS;
}

ErrCode PacketBufferImpl::createPacket(SizeT sampleCount, IDataDescriptor* desc, IPacket* domainPacket, IDataPacket** packet)
{
    daq::DataRulePtr rule;
    ErrCode err = desc->getRule(&rule);
    OPENDAQ_RETURN_IF_FAILED(err);

    DataRuleType type;
    err = rule->getType(&type);
    if (type == daq::DataRuleType::Linear)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "Packet Buffer does not support Linear Data Rule Type packets.");

    std::lock_guard<std::mutex> lock(readWriteMutex);
    if (underReset)
    {
        // Here there should be a reintroduction of Logging stuff
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDSTATE, "Trying to create packets while the reset procedure is underway.");
        //DAQ_THROW_EXCEPTION(InvalidStateException);  // Aka you should not create packets under reset
        // This maybe needs to be changed to reflect that a packet was trying to be created
    }

    size_t rawSampleSize;
    err = desc->getRawSampleSize(&rawSampleSize);
    OPENDAQ_RETURN_IF_FAILED(err);

    void* startOfSpace = nullptr;

    this->Write(sampleCount, rawSampleSize, &startOfSpace);
    auto deleter = daq::Deleter([this, sampleCnt = sampleCount, startOfSpace = startOfSpace, rawSizeOfSample = rawSampleSize](void*)mutable
                                {
                                    Read(startOfSpace, sampleCnt, rawSizeOfSample);
                                });

    *packet = daq::DataPacketWithExternalMemory(domainPacket, desc, sampleCount, startOfSpace, deleter).detach();
    return OPENDAQ_SUCCESS;
}

ErrCode PacketBufferImpl::getAvailableMemory(SizeT* count)
{
    SizeT check1 = static_cast<uint8_t*>(readPos) - static_cast<uint8_t*>(data.data());
    SizeT check2 = (data.data() + sizeInBytes) - static_cast<uint8_t*>(writePos);

    if (!((writePos == readPos) && isFull))
        *count = (check1 <= check2) ? check2 : check1;
    else
        *count = 0;

    return OPENDAQ_SUCCESS;
}

ErrCode PacketBufferImpl::getAvailableSampleCount(IDataDescriptor* desc, SizeT* count)
{
    OPENDAQ_PARAM_NOT_NULL(desc);
    OPENDAQ_PARAM_NOT_NULL(count);

    auto allAvailableSamples = static_cast<uint8_t*>(data.data()) + sizeInBytes;  // End of buffer

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
    std::unique_lock<std::mutex> lock(readWriteMutex);
    underReset = true;

    this->resizeSync.wait(lock, [&] { return ((readPos == writePos) && (!isFull)); });

    underReset = false;
    data = std::vector<uint8_t>(sizeInBytes);
    sizeInBytes = sizeInBytes;

    resizeSync.notify_all();
    return OPENDAQ_SUCCESS;
}

ErrCode PacketBufferImpl::getMaxAvailableContinousSampleCount(IDataDescriptor* desc, SizeT* count)
{
    OPENDAQ_PARAM_NOT_NULL(desc);
    OPENDAQ_PARAM_NOT_NULL(count);

    auto allAvailableSamples = static_cast<uint8_t*>(data.data()) + sizeInBytes;  // End of buffer

    SizeT rawSampleSize;
    desc->getRawSampleSize(&rawSampleSize);

    if (writePos == readPos)
    {
        if (isFull)
        {
            *count = 0;
        }
        auto fromEndToPos = allAvailableSamples - static_cast<uint8_t*>(readPos);
        auto fromStartToPos = static_cast<uint8_t*>(readPos) - static_cast<uint8_t*>(data.data());

        *count = (fromStartToPos <= fromEndToPos) ? (fromEndToPos/rawSampleSize) : (fromStartToPos/rawSampleSize);
    }
    else
    {
        if (writePos > readPos)
        {
            auto fromEndToPos = allAvailableSamples - static_cast<uint8_t*>(writePos);
            auto fromStartToPos = static_cast<uint8_t*>(readPos) - static_cast<uint8_t*>(data.data());


            *count = (fromStartToPos <= fromEndToPos) ? (fromEndToPos/rawSampleSize) : (fromStartToPos/rawSampleSize);
        }
        *count = (static_cast<uint8_t*>(readPos) - static_cast<uint8_t*>(writePos))/rawSampleSize;
    }

    return OPENDAQ_SUCCESS;
}

ErrCode PacketBufferImpl::getAvailableContinousSampleRight(IDataDescriptor* desc, SizeT* count)
{
    OPENDAQ_PARAM_NOT_NULL(desc);
    OPENDAQ_PARAM_NOT_NULL(count);

    SizeT rawSampleSize;
    desc->getRawSampleSize(&rawSampleSize);

    auto allAvailableSamples = static_cast<uint8_t*>(data.data()) + sizeInBytes;

    if (writePos == readPos)
    {
        if (isFull)
        {
            *count = 0;
        }
        auto fromEndToPos = allAvailableSamples - static_cast<uint8_t*>(readPos);
        *count = fromEndToPos/rawSampleSize;
    }
    else
    {
        if (writePos > readPos)
        {
            auto fromEndToPos = allAvailableSamples - static_cast<uint8_t*>(writePos);

            *count = fromEndToPos/rawSampleSize;
        }
        *count = (static_cast<uint8_t*>(readPos) - static_cast<uint8_t*>(writePos)) / rawSampleSize;
    }

    return OPENDAQ_SUCCESS;
}

ErrCode PacketBufferImpl::getAvailableContinousSampleLeft(IDataDescriptor* desc, SizeT* count)
{
    OPENDAQ_PARAM_NOT_NULL(desc);
    OPENDAQ_PARAM_NOT_NULL(count);

    SizeT rawSampleSize;
    desc->getRawSampleSize(&rawSampleSize);

    if (writePos == readPos)
    {
        if (isFull)
        {
            *count = 0;
        }
        auto fromStartToPos = static_cast<uint8_t*>(readPos) - static_cast<uint8_t*>(data.data());
        *count = fromStartToPos / rawSampleSize;
    }
    else
    {
        if (writePos > readPos)
        {
            auto fromStartToPos = static_cast<uint8_t*>(readPos) - static_cast<uint8_t*>(data.data());

            *count = fromStartToPos / rawSampleSize;
        }
        *count = (static_cast<uint8_t*>(readPos) - static_cast<uint8_t*>(writePos)) / rawSampleSize;
    }

    return OPENDAQ_SUCCESS;
}

OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY, PacketBuffer, IPacketBufferBuilder*, builder)

END_NAMESPACE_OPENDAQ
