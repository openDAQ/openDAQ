#include <coretypes/impl.h>
#include <opendaq/block_reader_impl.h>
#include <opendaq/event_packet_ptr.h>
#include <opendaq/reader_errors.h>
#include <opendaq/reader_factory.h>

BEGIN_NAMESPACE_OPENDAQ

using namespace std::chrono;
using namespace std::chrono_literals;

static SizeT calculateWrittenSamplesCount(SizeT initialWrittenSamplesCount, SizeT finalWrittenSamplesCount)
{
    auto writtenSamplesCount = finalWrittenSamplesCount > initialWrittenSamplesCount
                                   ? finalWrittenSamplesCount - initialWrittenSamplesCount
                                   : initialWrittenSamplesCount - finalWrittenSamplesCount;  // in case of counter wrapped
    return writtenSamplesCount;
};

BlockReaderImpl::BlockReaderImpl(
    const SignalPtr& signal, SizeT blockSize, SizeT overlap, SampleType valueReadType, SampleType domainReadType, ReadMode mode)
    : Super(signal, mode, valueReadType, domainReadType)
    , blockSize(blockSize)
    , overlap(overlap)
{
    if (this->overlap >= 100)
        throw InvalidParameterException("Overlap could not be greater or equal 100%");

    overlappedBlockSize = (blockSize * overlap) / 100;
    overlappedBlockSizeRemainder = blockSize - overlappedBlockSize;

    port.setNotificationMethod(PacketReadyNotification::SameThread);
    readDescriptorFromPort();
}

BlockReaderImpl::BlockReaderImpl(
    IInputPortConfig* port, SizeT blockSize, SizeT overlap, SampleType valueReadType, SampleType domainReadType, ReadMode mode)
    : Super(InputPortConfigPtr(port), mode, valueReadType, domainReadType)
    , blockSize(blockSize)
    , overlap(overlap)
{
    if (this->overlap >= 100)
        throw InvalidParameterException("Overlap could not be greater or equal 100%");

    overlappedBlockSize = (blockSize * overlap) / 100;
    overlappedBlockSizeRemainder = blockSize - overlappedBlockSize;

    this->port.setNotificationMethod(PacketReadyNotification::Scheduler);
}

BlockReaderImpl::BlockReaderImpl(
    const ReaderConfigPtr& readerConfig, SampleType valueReadType, SampleType domainReadType, SizeT blockSize, SizeT overlap, ReadMode mode)
    : Super(readerConfig, mode, valueReadType, domainReadType)
    , blockSize(blockSize)
    , overlap(overlap)
{
    if (this->overlap >= 100)
        throw InvalidParameterException("Overlap could not be greater or equal 100%");

    overlappedBlockSize = (blockSize * overlap) / 100;
    overlappedBlockSizeRemainder = blockSize - overlappedBlockSize;

    readDescriptorFromPort();
}

BlockReaderImpl::BlockReaderImpl(BlockReaderImpl* old, SampleType valueReadType, SampleType domainReadType, SizeT blockSize, SizeT overlap)
    : Super(old, valueReadType, domainReadType)
    , blockSize(blockSize)
    , overlap(overlap)
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
}

SizeT BlockReaderImpl::getAvailableSamples() const
{
    SizeT count{};
    if (info.currentDataPacketIter != info.dataPacketsQueue.end())
        count = info.currentDataPacketIter->getSampleCount() - info.prevSampleIndex;
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
    while (callback.assigned() && getAvailable() && OPENDAQ_SUCCEEDED(errCode))
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
    auto packetSampleCount = info.currentDataPacketIter->getSampleCount();
    auto packetRemainingSampleCount = packetSampleCount - info.prevSampleIndex;
    auto blockRemainingSampleCount = blockSize - info.writtenSampleCount % blockSize;
    SizeT sampleCountToRead = std::min(blockRemainingSampleCount, packetRemainingSampleCount);

    auto* packetData = getValuePacketData(*info.currentDataPacketIter);
    ErrCode errCode = valueReader->readData(packetData, info.prevSampleIndex, &info.values, sampleCountToRead);
    if (OPENDAQ_FAILED(errCode))
    {
        return errCode;
    }
    if (info.domainValues != nullptr)
    {
        auto dataPacket = *info.currentDataPacketIter;
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

    if (info.writtenSampleCount % blockSize == 0)
    {
        // how many data packet need to be stepped back
        const auto rewindSamples = overlappedBlockSize;
        auto rewindPackets = rewindSamples / packetSampleCount;
        auto rewindPacketRemainder = rewindSamples % packetSampleCount;

        if (info.prevSampleIndex < rewindPacketRemainder)
        {
            rewindPackets += 1;
            info.prevSampleIndex = packetSampleCount - rewindPacketRemainder;
        }
        else
        {
            info.prevSampleIndex -= rewindPacketRemainder;
        }

        using IterDiff = BlockReadInfo::DataPacketsQueueType::difference_type;

        info.currentDataPacketIter = std::next(info.currentDataPacketIter, -static_cast<IterDiff>(rewindPackets));

        if (info.remainingSamplesToRead)
            info.remainingSamplesToRead += overlappedBlockSize;
    }

    if (info.prevSampleIndex == packetSampleCount)
    {
        if (++info.currentDataPacketIter == info.dataPacketsQueue.end())
        {
            std::unique_lock lock(notify.mutex);
            notify.dataReady = false;
        }
        info.reset();
        info.trimQueue(packetSampleCount, overlappedBlockSize);
    }

    return OPENDAQ_SUCCESS;
}

ErrCode BlockReaderImpl::readPackets(IReaderStatus** status, SizeT* count)
{
    ErrCode errCode = OPENDAQ_SUCCESS;

    auto initialWrittenSamplesCount = info.writtenSampleCount;
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
    *count = calculateBlockCount(info.remainingSamplesToRead);

    while (info.remainingSamplesToRead != 0)
    {
        PacketPtr packet;

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
                if (info.currentDataPacketIter == info.dataPacketsQueue.end())
                {
                    info.dataPacketsQueue.emplace_back(packet);
                    info.currentDataPacketIter = --info.dataPacketsQueue.end();
                }
                errCode = readPacketData();

                if (OPENDAQ_FAILED(errCode))
                {
                    auto writtenSamplesCount = calculateWrittenSamplesCount(initialWrittenSamplesCount, info.writtenSampleCount);
                    if (status)
                        *status = BlockReaderStatus(nullptr, true, writtenSamplesCount).detach();
                    *count = writtenSamplesCount % blockSize;
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
                    handleDescriptorChanged(eventPacket);
                    auto writtenSamplesCount = calculateWrittenSamplesCount(initialWrittenSamplesCount, info.writtenSampleCount);
                    if (status)
                    {
                        *status = BlockReaderStatus(eventPacket, !invalid, writtenSamplesCount).detach();
                    }
                    *count = writtenSamplesCount / blockSize;
                    info.clean();
                    return errCode;
                }

                if (eventPacket.getEventId() == event_packet_id::IMPLICIT_DOMAIN_GAP_DETECTED)
                {
                    if (status)
                        *status = BlockReaderStatus(eventPacket, !invalid, (samplesToRead - info.remainingSamplesToRead) % blockSize).detach();
                    *count = (samplesToRead - info.remainingSamplesToRead) / blockSize;
                    return errCode;
                }

                break;
            }
            case PacketType::None:
                break;
        }
    }

    if (status && *status == nullptr)
    {
        auto writtenSamplesCount = calculateWrittenSamplesCount(initialWrittenSamplesCount, info.writtenSampleCount);
        *count = writtenSamplesCount / blockSize;
        *status = BlockReaderStatus(nullptr, !invalid, writtenSamplesCount).detach();
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

OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY,
                             BlockReader,
                             ISignal*,
                             signal,
                             SizeT,
                             blockSize,
                             SizeT,
                             overlap,
                             SampleType,
                             valueReadType,
                             SampleType,
                             domainReadType,
                             ReadMode,
                             mode)

template <>
struct ObjectCreator<IBlockReader>
{
    static ErrCode Create(IBlockReader** out,
                          IBlockReader* toCopy,
                          SampleType valueReadType,
                          SampleType domainReadType,
                          SizeT blockSize,
                          SizeT overlap) noexcept
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

OPENDAQ_DEFINE_CUSTOM_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC_OBJ(LIBRARY_FACTORY,
                                                                      IBlockReader,
                                                                      createBlockReaderFromExisting,
                                                                      IBlockReader*,
                                                                      invalidatedReader,
                                                                      SampleType,
                                                                      valueReadType,
                                                                      SampleType,
                                                                      domainReadType,
                                                                      SizeT,
                                                                      blockSize,
                                                                      SizeT,
                                                                      overlap)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(LIBRARY_FACTORY,
                                                           BlockReader,
                                                           IBlockReader,
                                                           createBlockReaderFromPort,
                                                           IInputPortConfig*,
                                                           port,
                                                           SizeT,
                                                           blockSize,
                                                           SizeT,
                                                           overlap,
                                                           SampleType,
                                                           valueReadType,
                                                           SampleType,
                                                           domainReadType,
                                                           ReadMode,
                                                           mode)

END_NAMESPACE_OPENDAQ
