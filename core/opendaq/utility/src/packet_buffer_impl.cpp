#include <opendaq/packet_buffer_impl.h>
#include <iostream>

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

ErrCode PacketBufferImpl::CleanOosPackets()
{
    while (!oosPackets.empty())
    { 
        if (readPos != oosPackets.top().first)
            break;

        isFull = false;
        readPos = static_cast<void*>(static_cast<uint8_t*>(readPos) + oosPackets.top().second);
        oosPackets.pop();
    }

    auto bufSize = static_cast<uint8_t*>(data.data()) + sizeInBytes;
    if (readPos >= bufSize)
    {
        auto delta = static_cast<uint8_t*>(readPos) - bufSize;
        readPos = data.data() + delta;
    }

    return OPENDAQ_SUCCESS;
}


ErrCode PacketBufferImpl::Write(size_t sizeOfPackets, void** memPos)
{
    auto endOfBuffer = (reinterpret_cast<uint8_t*>(data.data()) + sizeInBytes);
    size_t availableSamples;

    // Check whether R>W; 
    auto readPosWritePosDiff = (static_cast<uint8_t*>(writePos) - static_cast<uint8_t*>(readPos));
    if (readPosWritePosDiff < 0)
    {
        availableSamples = static_cast<size_t>(std::abs(readPosWritePosDiff));
    }
    else
    {
        if (isFull && readPosWritePosDiff == 0)
        {
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_BUFFERFULL, "The packet buffer is full");
        }
        else
        {
            availableSamples = static_cast<size_t>(endOfBuffer - static_cast<uint8_t*>(writePos));
        }
    }

    // Handle case when W>R and not enough space is available at buffer right-hand-side
    // Create dummy packet that fills up unused buffer space; Actual write will happen on left-hand-side
    if (availableSamples < sizeOfPackets)
    {
         // R>W && availableSamples < sampleCount
        if (readPosWritePosDiff < 0)
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "The requested packet size is not available.");

        size_t beginningReadPosDiff = (static_cast<uint8_t*>(readPos) - data.data());
        if (sizeOfPackets > beginningReadPosDiff)
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "The requested packet size is not available.");

        oosPackets.push(std::make_pair(static_cast<uint8_t*>(writePos), endOfBuffer - static_cast<uint8_t*>(writePos)));
        CleanOosPackets();

        *memPos = data.data();
        writePos = static_cast<void*>(data.data() + sizeOfPackets);
        if (writePos == readPos)
            isFull = true;
    }
    else
    {
        // Case when enough space is available on the right side of WritePos
        *memPos = writePos;
        writePos = static_cast<uint8_t*>(writePos) + sizeOfPackets;

        if (writePos == (void*) endOfBuffer)
            writePos = data.data();

        if (writePos == readPos)
            isFull = true;
    }

    return OPENDAQ_SUCCESS;
}

// TODO: Names of input arguments don't match the ones used to call the method. Should probably just be called startMemPos
ErrCode PacketBufferImpl::Read(void* startMemPos, size_t sizeOfPackets)
{
    {
        std::unique_lock<std::mutex> lock(readWriteMutex);

        auto memSize = sizeOfPackets;
        if (startMemPos < readPos)
        {
            oosPackets.push(std::make_pair(static_cast<uint8_t*>(startMemPos) + sizeInBytes, memSize));
        }
        else
        {
            oosPackets.push(std::make_pair(static_cast<uint8_t*>(startMemPos), memSize));
        }

        CleanOosPackets();
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
    OPENDAQ_RETURN_IF_FAILED(err);


    if (type != daq::DataRuleType::Explicit)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "Packet Buffer supports only Explicit Rule Type packets.");

    std::lock_guard<std::mutex> lock(readWriteMutex);
    if (underReset)
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDSTATE, "Trying to create packets while the reset procedure is underway.");
    }

    size_t rawSampleSize;
    err = desc->getRawSampleSize(&rawSampleSize);
    OPENDAQ_RETURN_IF_FAILED(err);

    void* startOfSpace = nullptr;

    this->Write(sampleCount * rawSampleSize, &startOfSpace);
    DeleterPtr deleter;

    deleter = daq::Deleter([this, sampleCnt = (sampleCount * rawSampleSize), startMemPos = startOfSpace](void*) mutable
                           {
                                Read(startMemPos, sampleCnt);
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

    auto fromEndToPos = allAvailableSamples - static_cast<uint8_t*>(writePos);
    auto fromStartToPos = static_cast<uint8_t*>(readPos) - static_cast<uint8_t*>(data.data());

    *count = (fromStartToPos <= fromEndToPos) ? fromEndToPos : fromStartToPos;

    if (writePos < readPos)
        *count = static_cast<uint8_t*>(readPos) - static_cast<uint8_t*>(writePos);

    if (isFull && (writePos == readPos))
            *count = 0;

    return OPENDAQ_SUCCESS;
}

ErrCode PacketBufferImpl::resize(SizeT sizeInBytes)
{
    std::unique_lock<std::mutex> lock(readWriteMutex);
    underReset = true;

    this->resizeSync.wait(lock,
                          [this]
                          {
                            std::this_thread::sleep_for(std::chrono::milliseconds(20));
                            return ((readPos == writePos) && (!isFull));
                          });

    data = std::vector<uint8_t>(sizeInBytes);
    this->sizeInBytes = sizeInBytes;
    readPos = data.data();
    writePos = data.data();

    resizeSync.notify_all();
    underReset = false;
    lock.unlock();
    return OPENDAQ_SUCCESS;
}

ErrCode PacketBufferImpl::getMaxAvailableContinousSampleCount(IDataDescriptor* desc, SizeT* count)
{
    OPENDAQ_PARAM_NOT_NULL(desc);
    OPENDAQ_PARAM_NOT_NULL(count);

    auto allAvailableSamples = static_cast<uint8_t*>(data.data()) + sizeInBytes;  // End of buffer
    auto fromEndToPos = allAvailableSamples - static_cast<uint8_t*>(writePos);
    auto fromStartToPos = static_cast<uint8_t*>(readPos) - static_cast<uint8_t*>(data.data());

    SizeT rawSampleSize;
    ErrCode err = desc->getRawSampleSize(&rawSampleSize);
    OPENDAQ_RETURN_IF_FAILED(err);

    *count = (fromStartToPos <= fromEndToPos) ? (fromEndToPos/rawSampleSize) : (fromStartToPos/rawSampleSize);

    if (writePos == readPos && isFull)
        *count = 0;

    if (writePos < readPos)
        *count = (static_cast<uint8_t*>(readPos) - static_cast<uint8_t*>(writePos))/rawSampleSize;

    return OPENDAQ_SUCCESS;
}

ErrCode PacketBufferImpl::getAvailableContinousSampleRight(IDataDescriptor* desc, SizeT* count)
{
    OPENDAQ_PARAM_NOT_NULL(desc);
    OPENDAQ_PARAM_NOT_NULL(count);

    SizeT rawSampleSize;
    ErrCode err = desc->getRawSampleSize(&rawSampleSize);
    OPENDAQ_RETURN_IF_FAILED(err);

    auto allAvailableSamples = static_cast<uint8_t*>(data.data()) + sizeInBytes;
    auto fromEndToPos = allAvailableSamples - static_cast<uint8_t*>(readPos);

    *count = fromEndToPos/rawSampleSize;

    if (writePos == readPos && isFull)
        *count = 0;

    if (writePos < readPos)
        *count = (static_cast<uint8_t*>(readPos) - static_cast<uint8_t*>(writePos)) / rawSampleSize;

    return OPENDAQ_SUCCESS;
}

ErrCode PacketBufferImpl::getAvailableContinousSampleLeft(IDataDescriptor* desc, SizeT* count)
{
    OPENDAQ_PARAM_NOT_NULL(desc);
    OPENDAQ_PARAM_NOT_NULL(count);

    SizeT rawSampleSize;
    ErrCode err = desc->getRawSampleSize(&rawSampleSize);
    OPENDAQ_RETURN_IF_FAILED(err);

    auto fromStartToPos = static_cast<uint8_t*>(readPos) - static_cast<uint8_t*>(data.data());

    *count = fromStartToPos / rawSampleSize;

    if (writePos == readPos && isFull)
        *count = 0;

    if (writePos < readPos)
        *count = (static_cast<uint8_t*>(readPos) - static_cast<uint8_t*>(writePos)) / rawSampleSize;

    return OPENDAQ_SUCCESS;
}

OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY, PacketBuffer, IPacketBufferBuilder*, builder)

END_NAMESPACE_OPENDAQ
