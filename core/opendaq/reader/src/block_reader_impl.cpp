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
    if (this->overlap >= 100)
        throw InvalidParameterException("Overlap could not be greater or equal 100%");

    overlappedBlockSize = (blockSize * overlap) / 100;
    overlappedBlockSizeRemainder = blockSize - overlappedBlockSize;

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
    if (this->overlap >= 100)
        throw InvalidParameterException("Overlap could not be greater or equal 100%");

    overlappedBlockSize = (blockSize * overlap) / 100;
    overlappedBlockSizeRemainder = blockSize - overlappedBlockSize;

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
    if (this->overlap >= 100)
        throw InvalidParameterException("Overlap could not be greater or equal 100%");

    overlappedBlockSize = (blockSize * overlap) / 100;
    overlappedBlockSizeRemainder = blockSize - overlappedBlockSize;

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
    if (this->overlap >= 100)
        throw InvalidParameterException("Overlap could not be greater or equal 100%");

    overlappedBlockSize = (blockSize * overlap) / 100;
    overlappedBlockSizeRemainder = blockSize - overlappedBlockSize;

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
    const auto availableSamples = getAvailableSamples();
    return availableSamples < blockSize ? 0 : calculateBlockCount(availableSamples);
//    if (availableSamples < blockSize)
//        return 0;
//    else
//        return (availableSamples - blockSize) / overlappedBlockSizeRemainder + 1;
}

SizeT BlockReaderImpl::getAvailableSamples() const
{
    SizeT count{};
//    if (info.dataPacket.assigned())
//    {
//        count = info.dataPacket.getSampleCount() - info.prevSampleIndex;
//    }
    if (info.currentDataPacketIter != info.dataPacketsQueue.end())
        count = info.currentDataPacketIter->getSampleCount() - info.prevSampleIndex;
    count += info.overlappedSamplesToRead;
//    count += info.getCachedSampleCount() > 0 ? overlappedBlockSize : 0;
    count += connection.getAvailableSamples();
    return count;
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

ErrCode BlockReaderImpl::readPacketDataNew()
{
    auto packetSampleCount = info.currentDataPacketIter->getSampleCount();
    auto packetRemainingSampleCount = packetSampleCount - info.prevSampleIndex;
    auto blockRemainingSampleCount = info.writtenSampleCount % blockSize == 0 ? blockSize : info.writtenSampleCount % blockSize;
    SizeT sampleCountToRead = std::min(blockRemainingSampleCount, packetRemainingSampleCount);

    auto* packetData = getValuePacketData(*info.currentDataPacketIter);
    ErrCode errCode = valueReader->readData(packetData,info.prevSampleIndex,&info.values,sampleCountToRead);
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
        auto* domainData = domainPacket.getData();
        errCode = domainReader->readData(domainData, info.prevSampleIndex, &info.domainValues, sampleCountToRead);
        if (errCode == OPENDAQ_ERR_INVALIDSTATE)
        {
            if (!trySetDomainSampleType(domainPacket))
            {
                return errCode;
            }
            errCode = domainReader->readData(domainPacket.getData(), info.prevSampleIndex, &info.domainValues, sampleCountToRead);
        }

        if (OPENDAQ_FAILED(errCode))
        {
            return errCode;
        }
    }

    info.writtenSampleCount += sampleCountToRead;
    info.prevSampleIndex += sampleCountToRead;
    info.remainingSamplesToRead -= sampleCountToRead;

    info.actualSampleRead += sampleCountToRead;
    info.remainingSamplesToReadNew -= sampleCountToRead;

    if (info.writtenSampleCount % blockSize == 0)
    {
        // how many data packet need to be stepped back
        const auto rewindSamples = overlappedBlockSize;
        auto rewindBlocks = rewindSamples / blockSize;
        auto rewindRemainder = rewindSamples % blockSize;

        if (info.prevSampleIndex < rewindRemainder)
        {
            rewindBlocks += 1;
            info.prevSampleIndex = blockSize - rewindRemainder;
        }
        else
        {
            info.prevSampleIndex -= rewindRemainder;
        }

        using IterDiff = BlockReadInfo::DataPacketsQueueType::difference_type;

        info.currentDataPacketIter = std::next(
            info.currentDataPacketIter,
            -static_cast<IterDiff>(rewindBlocks));

        if (info.remainingSamplesToRead)
        {
            info.actualSampleRead -= overlappedBlockSize;
            info.remainingSamplesToReadNew += overlappedBlockSize;
        }
    }
    else
    {
        if (info.prevSampleIndex == packetSampleCount) {
            std::unique_lock lock(notify.mutex);
            notify.dataReady = false;
            info.currentDataPacketIter = info.dataPacketsQueue.end();
            info.reset();
        }
    }

    return OPENDAQ_SUCCESS;
}

[[deprecated]]
ErrCode BlockReaderImpl::readPacketData()
{
    auto remainingSampleCount = info.dataPacket.getSampleCount() - info.prevSampleIndex;
    SizeT toRead = std::min(remainingSampleCount, info.remainingSamplesToRead);

    auto* packetData = getValuePacketData(info.dataPacket);
    ErrCode errCode = valueReader->readData(packetData, info.prevSampleIndex, &info.values, toRead);
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
        auto* domainData = domainPacket.getData();
        errCode = domainReader->readData(domainData, info.prevSampleIndex, &info.domainValues, toRead);
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
    *count = calculateBlockCount(info.remainingSamplesToRead);

    // output samples count with overlaps. will be greater, than input non-overlapped samples count.
    info.remainingSamplesToRead = *count * blockSize;

    while (info.remainingSamplesToReadNew != 0)
    {
        PacketPtr packet = nullptr;

        if (info.currentDataPacketIter != info.dataPacketsQueue.end())
        {
            packet = *info.currentDataPacketIter;
        }
        else
        {
            // if no partially-read packet and there are more blocks left in the connection
            std::unique_lock notifyLock(notify.mutex);

            packet = connection.dequeue();
            notify.dataReady = false;
        }

        switch (packet.getType())
        {
            case PacketType::Data:
            {
//                info.dataPacket = packet;

                if (info.currentDataPacketIter == info.dataPacketsQueue.end()) {
                    info.dataPacketsQueue.push_back(packet);
                    info.currentDataPacketIter = std::next(info.dataPacketsQueue.end(), -1);
                }
                errCode = readPacketDataNew();

                if (OPENDAQ_FAILED(errCode))
                {
                    if (status)
                        *status = BlockReaderStatus(nullptr, true, samplesToRead - info.remainingSamplesToRead).detach();
//                    *count = (samplesToRead - info.remainingSamplesToRead) / blockSize;
                    *count = calculateBlockCount(samplesToRead - info.remainingSamplesToRead);
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
                    if (status)
                    {
                        *status = BlockReaderStatus(eventPacket, !invalid, samplesToRead - info.remainingSamplesToRead).detach();
                    }
//                    *count = (samplesToRead - info.remainingSamplesToRead) / blockSize;
                    *count = calculateBlockCount(samplesToRead - info.remainingSamplesToRead);

                    handleDescriptorChanged(eventPacket);
                    return errCode;
                }
                break;
            }
            case PacketType::None:
                break;
        }
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

    const SizeT samplesToRead = *count * overlappedBlockSizeRemainder + overlappedBlockSize;
    info.prepare(blocks, samplesToRead, blockSize, milliseconds(timeoutMs));

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

    const SizeT samplesToRead = *count * overlappedBlockSizeRemainder + overlappedBlockSize;
    info.prepareWithDomain(dataBlocks, domainBlocks, samplesToRead, blockSize, milliseconds(timeoutMs));

    const ErrCode errCode = readPackets(status, count);

    if (status && *status == nullptr)
        *status = BlockReaderStatus(nullptr, !invalid, *count * blockSize).detach();

    return errCode;
}

SizeT BlockReaderImpl::calculateBlockCount(SizeT sampleCount) const
{
    if (sampleCount < blockSize)
        return 0;
    else
        return (sampleCount - blockSize) / overlappedBlockSizeRemainder + 1;
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
