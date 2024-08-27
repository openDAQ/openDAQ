#include <coretypes/impl.h>
#include <opendaq/block_reader_impl.h>
#include <opendaq/event_packet_ptr.h>
#include <opendaq/reader_errors.h>
#include <opendaq/reader_factory.h>

BEGIN_NAMESPACE_OPENDAQ

using namespace std::chrono;
using namespace std::chrono_literals;

BlockReaderImpl::BlockReaderImpl(const SignalPtr& signal,
                                 SizeT blockSize,
                                 SampleType valueReadType,
                                 SampleType domainReadType,
                                 ReadMode mode,
                                 SizeT overlap,
                                 Bool skipEvents)
    : Super(signal, mode, valueReadType, domainReadType, skipEvents)
    , blockSize(blockSize)
    , overlap(overlap)
{
    initOverlap();

    port.setNotificationMethod(PacketReadyNotification::SameThread);
    packetReceived(port.asPtrOrNull<IInputPort>(true));
}

BlockReaderImpl::BlockReaderImpl(IInputPortConfig* port,
                                 SizeT blockSize,
                                 SampleType valueReadType,
                                 SampleType domainReadType,
                                 ReadMode mode,
                                 SizeT overlap,
                                 Bool skipEvents)
    : Super(InputPortConfigPtr(port), mode, valueReadType, domainReadType, skipEvents)
    , blockSize(blockSize)
    , overlap(overlap)
{
    initOverlap();
    this->port.setNotificationMethod(PacketReadyNotification::Scheduler);
}

BlockReaderImpl::BlockReaderImpl(const ReaderConfigPtr& readerConfig,
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

BlockReaderImpl::BlockReaderImpl(BlockReaderImpl* old,
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
    handleDescriptorChanged(DataDescriptorChangedEventPacket(dataDescriptor, domainDescriptor));

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
    return calculateBlockCount(availableSamples);
}

SizeT BlockReaderImpl::getAvailableSamples() const
{
    SizeT count = 0;
    if (info.currentDataPacketIter != info.dataPacketsQueue.end())
        count += info.currentDataPacketIter->getSampleCount() - info.prevSampleIndex;
    if (connection.assigned())
        count += skipEvents ? connection.getSamplesUntilNextGapPacket() : connection.getSamplesUntilNextEventPacket();
    return count;
}

SizeT BlockReaderImpl::getTotalSamples() const
{
    SizeT count = 0;
    if (info.currentDataPacketIter != info.dataPacketsQueue.end())
        count += info.currentDataPacketIter->getSampleCount() - info.prevSampleIndex;
    if (connection.assigned())
        count += connection.getAvailableSamples();
    return count;
}


ErrCode BlockReaderImpl::connected(IInputPort* inputPort)
{
    OPENDAQ_PARAM_NOT_NULL(inputPort);

    std::scoped_lock lock(notify.mutex);
    inputPort->getConnection(&connection);
    return OPENDAQ_SUCCESS;
}

ErrCode BlockReaderImpl::disconnected(IInputPort* inputPort)
{
    OPENDAQ_PARAM_NOT_NULL(inputPort);

    std::scoped_lock lock(notify.mutex);
    connection = nullptr;
    return OPENDAQ_SUCCESS;
}

ErrCode BlockReaderImpl::packetReceived(IInputPort* inputPort)
{
    OPENDAQ_PARAM_NOT_NULL(inputPort);

    ProcedurePtr callback;
    {
        bool triggerCallback = false;
        std::scoped_lock lock(notify.mutex);
        if (connection.hasEventPacket())
        {
            triggerCallback = true;
        }
        else
        {
            triggerCallback = calculateBlockCount(getTotalSamples()) != 0;
        }

        if (triggerCallback)
        {
            callback = readCallback;
            notify.dataReady = true;
        }
    }
   
    notify.condition.notify_one();

    if (callback.assigned())
    {
        return wrapHandler(callback);
    }

    return OPENDAQ_SUCCESS;
}

ErrCode BlockReaderImpl::getEmpty(Bool* empty)
{
    OPENDAQ_PARAM_NOT_NULL(empty);

    std::scoped_lock lock(mutex);
    if (connection.assigned())
    {
        *empty = false;
        if (!skipEvents && connection.hasEventPacket())
        {
            return OPENDAQ_SUCCESS;
        }

        if (skipEvents && connection.hasGapPacket())
        {
            return OPENDAQ_SUCCESS;
        }
    }

    *empty = calculateBlockCount(getTotalSamples()) == 0;
    return OPENDAQ_SUCCESS;
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
            notify.dataReady = false;
        }
        info.resetSampleIndex();
        info.trimQueue(packetSampleCount, overlappedBlockSize);
    }

    return OPENDAQ_SUCCESS;
}

BlockReaderStatusPtr BlockReaderImpl::readPackets()
{
    std::unique_lock notifyLock(notify.mutex);
    auto initialWrittenSamplesCount = info.writtenSampleCount;

    if (info.timeout.count() > 0)
    {
        // if there is no enough samples - wait for the timeout or a full block
        auto condition = [this]
        {
            if (!connection.assigned())
            {
                return false;
            }

            if (skipEvents)
            {
                if (connection.hasGapPacket())
                {
                    return true;
                }
            }
            else
            {
                if (connection.hasEventPacket())
                {
                    return true;
                }
            }

            if (!notify.dataReady)
            {
                return false;
            }
            notify.dataReady = false;
            return getAvailableSamples() >= info.remainingSamplesToRead;
        };
        notify.condition.wait_for(notifyLock, info.timeout, condition);
    }

    NumberPtr offset;
    while (true)    
    {
        PacketPtr packet;

        if (info.currentDataPacketIter != info.dataPacketsQueue.end())
        {
            packet = *info.currentDataPacketIter;
        }
        
        if (!packet.assigned())
        {
            // if no partially-read packet and there are more blocks left in the connection
            if (connection.assigned())
            {
                packet = connection.dequeue();
            }
        }

        if (!packet.assigned())
        {
            break;
        }

        if (packet.getType() == PacketType::Data)
        {
            if (info.currentDataPacketIter == info.dataPacketsQueue.end())
            {
                info.dataPacketsQueue.emplace_back(packet);
                info.currentDataPacketIter = --info.dataPacketsQueue.end();
            }

            if (info.remainingSamplesToRead == 0)
            {
                break;
            }

            if (!offset.assigned())
            {
                offset = calculateOffset(*info.currentDataPacketIter, info.prevSampleIndex);
            }

            ErrCode errCode = readPacketData();
            if (OPENDAQ_FAILED(errCode))
            {
                auto writtenSamplesCount = info.writtenSampleCount - initialWrittenSamplesCount;
                return BlockReaderStatus(nullptr, true, offset, writtenSamplesCount);
            }
        }
        else if (packet.getType() == PacketType::Event)
        {
            // Handle events
            auto eventPacket = packet.asPtr<IEventPacket>(true);
            if (eventPacket.getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED)
            {
                handleDescriptorChanged(eventPacket);
            }

            if (!skipEvents || invalid || eventPacket.getEventId() == event_packet_id::IMPLICIT_DOMAIN_GAP_DETECTED)
            {
                auto writtenSamplesCount = info.writtenSampleCount - initialWrittenSamplesCount;
                info.clean();
                return BlockReaderStatus(eventPacket, !invalid, offset, writtenSamplesCount);
            }
        }
    }

    auto writtenSamplesCount = info.writtenSampleCount - initialWrittenSamplesCount;
    return BlockReaderStatus(nullptr, !invalid, offset, writtenSamplesCount);
}

ErrCode BlockReaderImpl::read(void* blocks, SizeT* count, SizeT timeoutMs, IBlockReaderStatus** status)
{
    OPENDAQ_PARAM_NOT_NULL(count);
    if (*count != 0)
    {
        OPENDAQ_PARAM_NOT_NULL(blocks);
    }

    std::scoped_lock lock(mutex);

    if (invalid)
    {
        if (status)
            *status = BlockReaderStatus(nullptr, !invalid).detach();
        *count = 0;
        return OPENDAQ_IGNORED;
    }

    const SizeT samplesToRead = *count * overlappedBlockSizeRemainder + overlappedBlockSize;
    info.prepare(blocks, samplesToRead, blockSize, milliseconds(timeoutMs));

    auto statusPtr = readPackets();
    *count = statusPtr.getReadSamples() / blockSize;
    if (status)
    {
        *status = statusPtr.detach();
    }
    return OPENDAQ_SUCCESS;
}

ErrCode BlockReaderImpl::readWithDomain(void* dataBlocks, void* domainBlocks, SizeT* count, SizeT timeoutMs, IBlockReaderStatus** status)
{
    OPENDAQ_PARAM_NOT_NULL(count);
    if (*count != 0)
    {
        OPENDAQ_PARAM_NOT_NULL(dataBlocks);
        OPENDAQ_PARAM_NOT_NULL(domainBlocks);
    }

    std::scoped_lock lock(mutex);

    if (invalid)
    {
        if (status)
            *status = BlockReaderStatus(nullptr, !invalid).detach();
        *count = 0;
        return OPENDAQ_IGNORED;
    }

    const SizeT samplesToRead = *count * overlappedBlockSizeRemainder + overlappedBlockSize;
    info.prepareWithDomain(dataBlocks, domainBlocks, samplesToRead, blockSize, milliseconds(timeoutMs));

    auto statusPtr = readPackets();
    *count = statusPtr.getReadSamples() / blockSize;
    if (status)
    {
        *status = statusPtr.detach();
    }
    return OPENDAQ_SUCCESS;
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

        if ((builderPtr.getValueReadType() == SampleType::Undefined || builderPtr.getDomainReadType() == SampleType::Undefined) &&
        builderPtr.getSkipEvents())
        {
            return makeErrorInfo(OPENDAQ_ERR_CREATE_FAILED, "Reader cannot skip events when sample type is undefined", nullptr);
        }

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
                builderPtr.getOverlap(),
                builderPtr.getSkipEvents());
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
                builderPtr.getOverlap(),
                builderPtr.getSkipEvents());
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
