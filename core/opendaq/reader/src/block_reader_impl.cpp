#include <coretypes/impl.h>
#include <opendaq/block_reader_impl.h>
#include <opendaq/event_packet_ptr.h>
#include <opendaq/reader_errors.h>
#include <opendaq/reader_factory.h>

BEGIN_NAMESPACE_OPENDAQ

using namespace std::chrono;
using namespace std::chrono_literals;

BlockReaderImpl::BlockReaderImpl(
    const SignalPtr& signal,
    SizeT blockSize,
    SampleType valueReadType,
    SampleType domainReadType,
    ReadMode mode,
    SizeT overlap)
    : Super(signal, mode, valueReadType, domainReadType)
    , blockSize(blockSize)
    , overlap(overlap)
{
    initOverlap();

    port.setNotificationMethod(PacketReadyNotification::SameThread);
    readDescriptorFromPort();
}

BlockReaderImpl::BlockReaderImpl(
    IInputPortConfig* port,
    SizeT blockSize,
    SampleType valueReadType,
    SampleType domainReadType,
    ReadMode mode,
    SizeT overlap)
    : Super(InputPortConfigPtr(port), mode, valueReadType, domainReadType)
    , blockSize(blockSize)
    , overlap(overlap)
{
    initOverlap();

    this->port.setNotificationMethod(PacketReadyNotification::Scheduler);
}

BlockReaderImpl::BlockReaderImpl(
    const ReaderConfigPtr& readerConfig,
    SampleType valueReadType,
    SampleType domainReadType,
    SizeT blockSize,
    ReadMode mode,
    SizeT overlap)
    : Super(readerConfig, mode, valueReadType, domainReadType)
    , blockSize(blockSize)
    , overlap(overlap)
{
    initOverlap();

    readDescriptorFromPort();
}

BlockReaderImpl::BlockReaderImpl(
    BlockReaderImpl* old,
    SampleType valueReadType,
    SampleType domainReadType,
    SizeT blockSize,
    SizeT overlap)
    : Super(old, valueReadType, domainReadType)
    , blockSize(blockSize)
    , overlap(overlap)
    , info(old->info)
{
    initOverlap();

    this->internalAddRef();
    if (portBinder.assigned())
    {
        auto eventPacket = DataDescriptorChangedEventPacket(dataDescriptor, domainDescriptor);
        handleDescriptorChanged(eventPacket);
    }
    else
    {
        readDescriptorFromPort();
    }

    notify.dataReady = false;
}

ErrCode BlockReaderImpl::getBlockSize(SizeT* size)
{
    OPENDAQ_PARAM_NOT_NULL(size);

    *size = blockSize;
    return OPENDAQ_SUCCESS;
}

ErrCode BlockReaderImpl::getOverlap(daq::SizeT* overlap)
{
    OPENDAQ_PARAM_NOT_NULL(overlap);

    *overlap = this->overlap;
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
            errCode = domainReader->readData(domainData, info.prevSampleIndex, &info.domainValues, sampleCountToRead);
        }

        if (OPENDAQ_FAILED(errCode))
        {
            return errCode;
        }
    }

    info.writtenSampleCount += sampleCountToRead;
    info.prevSampleIndex += sampleCountToRead;
    info.remainingSamplesToRead -= sampleCountToRead;

    if (overlap)
        info.rewindQueue(blockSize, overlappedBlockSize);

    if (info.prevSampleIndex == packetSampleCount)
    {
        if (++info.currentDataPacketIter == info.dataPacketsQueue.end())
        {
            std::unique_lock lock(notify.mutex);
            notify.dataReady = false;
        }
        info.resetSampleIndex();
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
                    auto writtenSamplesCount = info.writtenSampleCount - initialWrittenSamplesCount;
                    if (status)
                        *status = BlockReaderStatus(nullptr, true, writtenSamplesCount).detach();
                    *count = writtenSamplesCount / blockSize;
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
                    auto writtenSamplesCount = info.writtenSampleCount - initialWrittenSamplesCount;
                    if (status)
                        *status = BlockReaderStatus(eventPacket, !invalid, writtenSamplesCount).detach();
                    *count = writtenSamplesCount / blockSize;
                    info.clean();
                    return errCode;
                }

                if (eventPacket.getEventId() == event_packet_id::IMPLICIT_DOMAIN_GAP_DETECTED)
                {
                    auto writtenSamplesCount = info.writtenSampleCount - initialWrittenSamplesCount;
                    if (status)
                        *status = BlockReaderStatus(eventPacket, !invalid, writtenSamplesCount).detach();
                    *count = writtenSamplesCount / blockSize;
                    info.clean();
                    return errCode;
                }

                break;
            }
            case PacketType::None:
                break;
        }
    }

    if (status)
    {
        auto writtenSamplesCount = info.writtenSampleCount - initialWrittenSamplesCount;
        *status = BlockReaderStatus(nullptr, !invalid, writtenSamplesCount).detach();
        *count = writtenSamplesCount / blockSize;
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
    return errCode;
}

void BlockReaderImpl::initOverlap()
{
    if (overlap >= 100)
        throw InvalidParameterException("Overlap could not be greater or equal 100%");

    overlappedBlockSize = (blockSize * overlap) / 100;
    overlappedBlockSizeRemainder = blockSize - overlappedBlockSize;
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
    SampleType, valueReadType,
    SampleType, domainReadType,
    ReadMode, mode
)

template <>
struct ObjectCreator<IBlockReader>
{
    using Self = ObjectCreator<IBlockReader>;

    static ErrCode Create(IBlockReader** out,
                          IBlockReader* toCopy,
                          SampleType valueReadType,
                          SampleType domainReadType,
                          SizeT blockSize) noexcept
    {
        OPENDAQ_PARAM_NOT_NULL(out);

        if (toCopy == nullptr)
        {
            return makeErrorInfo(OPENDAQ_ERR_ARGUMENT_NULL, "Existing reader must not be null", nullptr);
        }

        ReadMode readMode;
        toCopy->getReadMode(&readMode);

        SizeT overlap;
        toCopy->getOverlap(&overlap);

        return Self::CreateImpl(out, toCopy, readMode, valueReadType, domainReadType, blockSize, overlap);
    }

    static ErrCode Create(IBlockReader** out,
                          IBlockReaderBuilder* builder) noexcept
    {
        OPENDAQ_PARAM_NOT_NULL(out);

        if (builder == nullptr)
            return makeErrorInfo(OPENDAQ_ERR_ARGUMENT_NULL, "Builder must not be null", nullptr);

        auto builderPtr = BlockReaderBuilderPtr::Borrow(builder);
        auto signal = builderPtr.getSignal();
        auto inputPort = builderPtr.getInputPort();
        auto oldBlockReader = builderPtr.getOldBlockReader();

        auto assignedCount = signal.assigned() + inputPort.assigned() + oldBlockReader.assigned();
        if (assignedCount > 1)
            return makeErrorInfo(OPENDAQ_ERR_CREATE_FAILED, "Only old block reader instance or signal or input port should be used in builder to construct new instance", nullptr);

        if (builderPtr.getBlockSize() == 0)
            return makeErrorInfo(OPENDAQ_ERR_CREATE_FAILED, "Block size cannot be 0", nullptr);

        ErrCode errCode;

        if (oldBlockReader.assigned())
        {
            auto oldObject = oldBlockReader.getObject();
            errCode = Self::CreateImpl(
                out,
                oldObject,
                builderPtr.getReadMode(),
                builderPtr.getValueReadType(),
                builderPtr.getDomainReadType(),
                builderPtr.getBlockSize(),
                builderPtr.getOverlap());
        }
        else if (signal.assigned())
        {
            errCode = createObject<IBlockReader, BlockReaderImpl>(
                out,
                signal,
                builderPtr.getBlockSize(),
                builderPtr.getValueReadType(),
                builderPtr.getDomainReadType(),
                builderPtr.getReadMode(),
                builderPtr.getOverlap());
        }
        else if (inputPort.assigned())
        {
            errCode = createObject<IBlockReader, BlockReaderImpl>(
                out,
                inputPort.as<IInputPortConfig>(true),
                builderPtr.getBlockSize(),
                builderPtr.getValueReadType(),
                builderPtr.getDomainReadType(),
                builderPtr.getReadMode(),
                builderPtr.getOverlap());
        }
        else
        {
            errCode = makeErrorInfo(OPENDAQ_ERR_CREATE_FAILED, "Signal, input port or old Block reader must be assigned to builder", nullptr);
        }

        return errCode;
    }

private:
    static ErrCode CreateImpl(IBlockReader** out,
                              IBlockReader* toCopy,
                              ReadMode readMode,
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

        auto old = ReaderConfigPtr::Borrow(toCopy);
        auto impl = dynamic_cast<BlockReaderImpl*>(old.getObject());

        return impl != nullptr
                   ? createObject<IBlockReader, BlockReaderImpl>(out, impl, valueReadType, domainReadType, blockSize, overlap)
                   : createObject<IBlockReader, BlockReaderImpl>(out, old, valueReadType, domainReadType, blockSize, readMode, overlap);
    }
};

OPENDAQ_DEFINE_CUSTOM_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC_OBJ(
    LIBRARY_FACTORY, IBlockReader, createBlockReaderFromExisting,
    IBlockReader*, invalidatedReader,
    SampleType, valueReadType,
    SampleType, domainReadType,
    SizeT, blockSize
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, BlockReader, IBlockReader, createBlockReaderFromPort,
    IInputPortConfig*, port,
    SizeT, blockSize,
    SampleType, valueReadType,
    SampleType, domainReadType,
    ReadMode, mode
)

OPENDAQ_DEFINE_CUSTOM_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC_OBJ(
    LIBRARY_FACTORY, IBlockReader, createBlockReaderFromBuilder,
    IBlockReaderBuilder*, builder
)

END_NAMESPACE_OPENDAQ
