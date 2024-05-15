#include <opendaq/block_reader_impl.h>
#include <coretypes/impl.h>
#include <opendaq/event_packet_ptr.h>
#include <opendaq/reader_errors.h>
#include <opendaq/reader_factory.h>

BEGIN_NAMESPACE_OPENDAQ

using namespace std::chrono;
using namespace std::chrono_literals;

BlockReaderImpl::BlockReaderImpl(const SignalPtr& signal,
                                 SizeT blockSize,
                                 SizeT overlap,
                                 SampleType valueReadType,
                                 SampleType domainReadType,
                                 ReadMode mode)
    : Super(signal, mode, valueReadType, domainReadType)
    , blockSize(blockSize), overlap(overlap)
{
    calculateOverlapSize();

    port.setNotificationMethod(PacketReadyNotification::SameThread);
    readDescriptorFromPort();
}

BlockReaderImpl::BlockReaderImpl(IInputPortConfig* port,
                                 SizeT blockSize,
                                 SizeT overlap,
                                 SampleType valueReadType,
                                 SampleType domainReadType,
                                 ReadMode mode)
    : Super(InputPortConfigPtr(port), mode, valueReadType, domainReadType)
    , blockSize(blockSize), overlap(overlap)
{
    calculateOverlapSize();

    this->port.setNotificationMethod(PacketReadyNotification::Scheduler);
}

BlockReaderImpl::BlockReaderImpl(const ReaderConfigPtr& readerConfig,
                                 SampleType valueReadType,
                                 SampleType domainReadType,
                                 SizeT blockSize,
                                 SizeT overlap,
                                 ReadMode mode)
    : Super(readerConfig, mode, valueReadType, domainReadType)
    , blockSize(blockSize), overlap(overlap)
{
    calculateOverlapSize();

    readDescriptorFromPort();
}

BlockReaderImpl::BlockReaderImpl(BlockReaderImpl* old,
                                 SampleType valueReadType,
                                 SampleType domainReadType,
                                 SizeT blockSize,
                                 SizeT overlap)
    : Super(old, valueReadType, domainReadType)
    , blockSize(blockSize), overlap(overlap)
    , info(old->info)
{
    calculateOverlapSize();

    this->internalAddRef();
    if (portBinder.assigned())
        handleDescriptorChanged(DataDescriptorChangedEventPacket(dataDescriptor, domainDescriptor));
    else
        readDescriptorFromPort();

    notify.dataReady = false;
}

ErrCode BlockReaderImpl::getBlockSize(SizeT* size)
{
    OPENDAQ_PARAM_NOT_NULL(size);

    *size = blockSize;
    return OPENDAQ_SUCCESS;
}

ErrCode BlockReaderImpl::getOverlap(daq::SizeT* size)
{
    OPENDAQ_PARAM_NOT_NULL(size);

    *size = overlap;
    return OPENDAQ_SUCCESS;
}

ErrCode BlockReaderImpl::getAvailableCount(SizeT* count)
{
    OPENDAQ_PARAM_NOT_NULL(count);

    SizeT available{};
    ErrCode errCode = wrapHandlerReturn(this, &BlockReaderImpl::getAvailable, available);
    *count = available;
    return errCode;
}

SizeT BlockReaderImpl::getAvailable() const
{
    // TODO: recalculate in case of overlap
    return getAvailableSamples() / blockSize;
}

SizeT BlockReaderImpl::getAvailableSamples() const
{
    SizeT count{};
    if (info.dataPacket.assigned())
    {
        count = info.dataPacket.getSampleCount() - info.prevSampleIndex;
    }

    count += connection.getAvailableSamples();
    return count;
}

void BlockReaderImpl::calculateOverlapSize()
{
    if (this->overlap >= 100)
        throw InvalidParameterException("Overlap could not be greater or equal 100%");

    overlappedBlockSize = (blockSize * overlap) / 100;
    overlappedBlockSizeRemainder = blockSize - overlappedBlockSize;
}

void BlockReaderImpl::flattenOverlappedBlocks(SizeT count, SizeT evenSamplesCount, SizeT remainingSamplesCount)
{
    if (overlap == 0) {
        return;
    }

    const auto dataSampleSize = dataDescriptor.getRawSampleSize();
    auto outputBlockCount = count + (remainingSamplesCount != 0 ? 1 : 0);
    auto outputPtr = static_cast<uint8_t*>(info.values) + (outputBlockCount - 1) * dataSampleSize;
    auto inputPtr = static_cast<uint8_t*>(info.values) + (evenSamplesCount - blockSize) * dataSampleSize;

    if (remainingSamplesCount) {
        std::memcpy(outputPtr + blockSize * dataSampleSize,
                    inputPtr + blockSize * dataSampleSize,
                    remainingSamplesCount);
        outputPtr -= blockSize * dataSampleSize;
        inputPtr -= blockSize * dataSampleSize;
        --outputBlockCount;
    }

    for (int i = 0; i < outputBlockCount; ++i) {
        std::memcpy(outputPtr, inputPtr, blockSize * dataSampleSize);
        outputPtr -= blockSize * dataSampleSize;
        inputPtr -= blockSize * dataSampleSize;
    }
}

ErrCode BlockReaderImpl::packetReceived(IInputPort* inputPort)
{
    OPENDAQ_PARAM_NOT_NULL(inputPort);

    {
        std::scoped_lock lock(notify.mutex);

        if (getAvailable() != 0)
        {
            notify.dataReady = true;
        }
        else
        {
            return OPENDAQ_SUCCESS;
        }
    }
    notify.condition.notify_one();

    ErrCode errCode = OPENDAQ_SUCCESS;
    std::unique_lock lock(mutex);
    auto callback = readCallback;
    while(callback.assigned() && getAvailable() && OPENDAQ_SUCCEEDED(errCode))
    {
        lock.unlock();
        errCode = wrapHandler(callback);
        lock.lock();
        callback = readCallback;
    }

    return errCode;
}

ErrCode BlockReaderImpl::readPacketData()
{
    auto remainingSampleCount = info.dataPacket.getSampleCount() - info.prevSampleIndex;
    SizeT toRead = std::min(remainingSampleCount, info.remainingSamplesToRead);

    ErrCode errCode = valueReader->readData(getValuePacketData(info.dataPacket), info.prevSampleIndex, &info.values, toRead);
    if (OPENDAQ_FAILED(errCode))
    {
        return errCode;
    }

    if (info.domainValues != nullptr)
    {
        auto dataPacket = info.dataPacket;
        if (!dataPacket.getDomainPacket().assigned())
        {
            return makeErrorInfo(OPENDAQ_ERR_INVALIDSTATE, "Packets must have an associated domain packets to read domain data.");
        }

        auto domainPacket = dataPacket.getDomainPacket();
        errCode = domainReader->readData(domainPacket.getData(), info.prevSampleIndex, &info.domainValues, toRead);
        if (errCode == OPENDAQ_ERR_INVALIDSTATE)
        {
            if (!trySetDomainSampleType(domainPacket))
            {
                return errCode;
            }
            errCode = domainReader->readData(domainPacket.getData(), info.prevSampleIndex, &info.domainValues, toRead);
        }

        if (OPENDAQ_FAILED(errCode))
        {
            return errCode;
        }
    }

    if (toRead < remainingSampleCount)
    {
        info.prevSampleIndex += toRead;
    }
    else
    {
        std::unique_lock lock(notify.mutex);
        notify.dataReady = false;

        info.reset();
    }

    info.remainingSamplesToRead -= toRead;
    return OPENDAQ_SUCCESS;
}

ErrCode BlockReaderImpl::readPackets(IReaderStatus** status, SizeT* count)
{
    ErrCode errCode = OPENDAQ_SUCCESS;

    auto availableSamples = getAvailableSamples();
    if (availableSamples < info.remainingSamplesToRead && info.timeout.count() > 0)
    {
        // if not enough samples wait for the timeout or a full block
        std::unique_lock notifyLock(notify.mutex);
        auto condition = [this, &availableSamples]
        {
            if (!notify.dataReady)
                return false;
            notify.dataReady = false;

            availableSamples = getAvailableSamples();
            return availableSamples >= info.remainingSamplesToRead;
        };
        notify.condition.wait_for(notifyLock, info.timeout, condition);
    }

    info.remainingSamplesToRead = std::min(availableSamples, info.remainingSamplesToRead);
    SizeT samplesToRead = info.remainingSamplesToRead;


    *count = (info.remainingSamplesToRead - blockSize) / overlappedBlockSizeRemainder + 1;

    while (info.remainingSamplesToRead != 0)
    {
        PacketPtr packet = info.dataPacket;

        // if no partially-read packet and there are more blocks left in the connection
        if (!packet.assigned())
        {
            std::unique_lock notifyLock(notify.mutex);

            packet = connection.dequeue();
            notify.dataReady = false;
        }

        switch (packet.getType())
        {
            case PacketType::Data:
            {
                info.dataPacket = packet;
                errCode = readPacketData();

                if (OPENDAQ_FAILED(errCode))
                {
                    const auto samplesHasBeenRead = samplesToRead - info.remainingSamplesToRead;
                    SizeT evenSamplesCount;
                    SizeT remainingSamplesCount;

                    if (samplesHasBeenRead < blockSize)
                    {
                        *count = 0;
                        evenSamplesCount = 0;
                    }
                    else
                    {
                        *count = (samplesHasBeenRead - blockSize) / overlappedBlockSizeRemainder + 1;
                        evenSamplesCount = *count * (blockSize - overlappedBlockSize) + overlappedBlockSize;
                    }

                    remainingSamplesCount = samplesHasBeenRead - evenSamplesCount;
                    flattenOverlappedBlocks(*count, evenSamplesCount, remainingSamplesCount);

                    if (status)
                        *status = BlockReaderStatus(nullptr, true, evenSamplesCount + remainingSamplesCount).detach();

                    return errCode;
                }

                break;
            }
            case PacketType::Event:
            {
                // Handle events
                auto eventPacket = packet.asPtrOrNull<IEventPacket>(true);

                if (eventPacket.getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED)
                {
                    const auto samplesHasBeenRead = samplesToRead - info.remainingSamplesToRead;
                    SizeT evenSamplesCount;
                    SizeT remainingSamplesCount;

                    if (samplesHasBeenRead < blockSize)
                    {
                        *count = 0;
                        evenSamplesCount = 0;
                    }
                    else
                    {
                        *count = (samplesHasBeenRead - blockSize) / overlappedBlockSizeRemainder + 1;
                        evenSamplesCount = *count * (blockSize - overlappedBlockSize) + overlappedBlockSize;
                    }

                    remainingSamplesCount = samplesHasBeenRead - evenSamplesCount;
                    flattenOverlappedBlocks(*count, evenSamplesCount, remainingSamplesCount);

                    handleDescriptorChanged(eventPacket);

                    if (status)
                        *status = BlockReaderStatus(eventPacket, !invalid, evenSamplesCount + remainingSamplesCount).detach();

                    return errCode;
                }
                break;
            }
            case PacketType::None:
                break;
        }
    }

    if (OPENDAQ_SUCCEEDED(errCode)) {
        flattenOverlappedBlocks(*count, blockSize * *count, 0);
    }

    return errCode;
}

ErrCode BlockReaderImpl::read(void* blocks, SizeT* count, SizeT timeoutMs, IReaderStatus** status)
{
    OPENDAQ_PARAM_NOT_NULL(blocks);
    OPENDAQ_PARAM_NOT_NULL(count);

    std::scoped_lock lock(mutex);

    if (invalid)
    {
        if (status)
            *status = BlockReaderStatus(nullptr, !invalid).detach();
        return OPENDAQ_IGNORED;
    }

    if (status)
        *status = nullptr;

    const SizeT samplesToRead = *count * (blockSize - overlappedBlockSize) + overlappedBlockSize;
    info.prepare(blocks, samplesToRead, milliseconds(timeoutMs));

    const ErrCode errCode = readPackets(status, count);
    
    if (status && *status == nullptr)
        *status = BlockReaderStatus(nullptr, !invalid, *count * blockSize).detach();
    
    return errCode;
}

ErrCode BlockReaderImpl::readWithDomain(void* dataBlocks, void* domainBlocks, SizeT* count, SizeT timeoutMs, IReaderStatus** status)
{
    OPENDAQ_PARAM_NOT_NULL(dataBlocks);
    OPENDAQ_PARAM_NOT_NULL(domainBlocks);
    OPENDAQ_PARAM_NOT_NULL(count);

    std::scoped_lock lock(mutex);

    if (invalid)
    {
        if (status)
            *status = BlockReaderStatus(nullptr, !invalid).detach();
        return OPENDAQ_IGNORED;
    }

    if (status)
        *status = nullptr;

    const SizeT samplesToRead = *count * (blockSize - overlappedBlockSize) + overlappedBlockSize;
    info.prepareWithDomain(dataBlocks, domainBlocks, samplesToRead, milliseconds(timeoutMs));

    const ErrCode errCode = readPackets(status, count);

    if (status && *status == nullptr)
        *status = BlockReaderStatus(nullptr, !invalid, *count * blockSize).detach();

    return errCode;
}

OPENDAQ_DEFINE_CLASS_FACTORY(
    LIBRARY_FACTORY, BlockReader,
    ISignal*, signal,
    SizeT, blockSize,
    SizeT, overlap,
    SampleType, valueReadType,
    SampleType, domainReadType,
    ReadMode, mode
)

template <>
struct ObjectCreator<IBlockReader>
{
    static ErrCode Create(IBlockReader** out,
                          IBlockReader* toCopy,
                          SampleType valueReadType,
                          SampleType domainReadType,
                          SizeT blockSize,
                          SizeT overlap
                         ) noexcept
    {
        OPENDAQ_PARAM_NOT_NULL(out);

        if (toCopy == nullptr)
        {
            return makeErrorInfo(OPENDAQ_ERR_ARGUMENT_NULL, "Existing reader must not be null", nullptr);
        }

        ReadMode mode;
        toCopy->getReadMode(&mode);

        auto old = ReaderConfigPtr::Borrow(toCopy);
        auto impl = dynamic_cast<BlockReaderImpl*>(old.getObject());

        return impl != nullptr
            ? createObject<IBlockReader, BlockReaderImpl>(out, impl, valueReadType, domainReadType, blockSize, overlap)
            : createObject<IBlockReader, BlockReaderImpl>(out, old, valueReadType, domainReadType, blockSize, overlap, mode);
    }
};

OPENDAQ_DEFINE_CUSTOM_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC_OBJ(
    LIBRARY_FACTORY, IBlockReader, createBlockReaderFromExisting,
    IBlockReader*, invalidatedReader,
    SampleType, valueReadType,
    SampleType, domainReadType,
    SizeT, blockSize,
    SizeT, overlap
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, BlockReader,
    IBlockReader, createBlockReaderFromPort,
    IInputPortConfig*, port,
    SizeT, blockSize,
    SizeT, overlap,
    SampleType, valueReadType,
    SampleType, domainReadType,
    ReadMode, mode
)


END_NAMESPACE_OPENDAQ
