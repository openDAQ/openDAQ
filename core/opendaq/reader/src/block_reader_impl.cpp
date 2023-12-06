#include <opendaq/block_reader_impl.h>
#include <coretypes/impl.h>
#include <opendaq/event_packet_ptr.h>
#include <opendaq/reader_errors.h>

BEGIN_NAMESPACE_OPENDAQ

using namespace std::chrono;
using namespace std::chrono_literals;

BlockReaderImpl::BlockReaderImpl(const SignalPtr& signal,
                                 SizeT blockSize,
                                 SampleType valueReadType,
                                 SampleType domainReadType,
                                 ReadMode mode)
    : Super(signal, mode, valueReadType, domainReadType)
    , blockSize(blockSize)
{
    port.setNotificationMethod(PacketReadyNotification::SameThread);
    BlockReaderImpl::handleDescriptorChanged(connection.dequeue());
}

BlockReaderImpl::BlockReaderImpl(IInputPortConfig* port,
                                 SizeT blockSize,
                                 SampleType valueReadType,
                                 SampleType domainReadType,
                                 ReadMode mode)
    : Super(InputPortConfigPtr(port), mode, valueReadType, domainReadType)
    , blockSize(blockSize)
{
    this->port.setNotificationMethod(PacketReadyNotification::SameThread);
    BlockReaderImpl::handleDescriptorChanged(connection.dequeue());
}

BlockReaderImpl::BlockReaderImpl(const ReaderConfigPtr& readerConfig,
                                 SampleType valueReadType,
                                 SampleType domainReadType,
                                 SizeT blockSize,
                                 ReadMode mode)
    : Super(readerConfig, mode, valueReadType, domainReadType)
    , blockSize(blockSize)
{
    readDescriptorFromPort();
}

BlockReaderImpl::BlockReaderImpl(BlockReaderImpl* old,
                                 SampleType valueReadType,
                                 SampleType domainReadType)
    : Super(old, valueReadType, domainReadType)
    , blockSize(old->blockSize)
    , info(old->info)
{
    this->internalAddRef();

    readDescriptorFromPort();
    notify.dataReady = false;
}

ErrCode BlockReaderImpl::getBlockSize(SizeT* size)
{
    OPENDAQ_PARAM_NOT_NULL(size);

    *size = blockSize;
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

ErrCode BlockReaderImpl::packetReceived(IInputPort* inputPort)
{
    OPENDAQ_PARAM_NOT_NULL(inputPort);

    {
        std::unique_lock lock(notify.mutex);

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
    return OPENDAQ_SUCCESS;
}

ErrCode BlockReaderImpl::readPacketData()
{
    auto remainingSampleCount = info.dataPacket.getSampleCount() - info.prevSampleIndex;
    SizeT toRead = std::min(remainingSampleCount, blockSize);

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

ErrCode BlockReaderImpl::readPackets()
{
    ErrCode errCode = OPENDAQ_SUCCESS;

    BlockReadInfo::Duration remainingTime = info.timeout;
    auto shouldReadMore = getAvailable() > 0 || remainingTime.count() > 0;

    while (info.remainingSamplesToRead > 0 && shouldReadMore)
    {
        PacketPtr packet = info.dataPacket;

        // if no partially-read packet and there are more blocks left in the connection
        if (getAvailable() > 0 && !packet.assigned())
        {
            std::unique_lock lock(notify.mutex);

            packet = connection.dequeue();
            notify.dataReady = false;
        }
        else if (!packet.assigned())
        {
            // if not enough samples wait for the timeout or a full block

            std::unique_lock notifyLock(notify.mutex);
            if (notify.condition.wait_for(notifyLock, remainingTime, [this]
            {
                return notify.dataReady && getAvailable() != 0;
            }))
            {
                packet = connection.dequeue();
                notify.dataReady = false;
            }
            else
            {
                break;
            }
        }

        switch (packet.getType())
        {
            case PacketType::Data:
            {
                info.dataPacket = packet;
                errCode = readPacketData();

                if (OPENDAQ_FAILED(errCode))
                {
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
                    if (invalid)
                    {
                        return this->makeErrorInfo(OPENDAQ_ERR_INVALID_DATA, "Packet samples are no longer convertible to the read type");
                    }
                }
                break;
            }
            case PacketType::None:
                break;
        }

        if (info.timeout.count() != 0)
            remainingTime = info.timeout - info.durationFromStart();

        auto available = getAvailableSamples();
        auto remaining = remainingTime.count();

        shouldReadMore = info.remainingSamplesToRead <= available ||
                         remaining > 0;
    }

    return errCode;
}

ErrCode BlockReaderImpl::read(void* blocks, SizeT* count, SizeT timeoutMs)
{
    OPENDAQ_PARAM_NOT_NULL(blocks);
    OPENDAQ_PARAM_NOT_NULL(count);

    std::scoped_lock lock(mutex);

    if (invalid)
        return makeErrorInfo(OPENDAQ_ERR_INVALID_DATA, "Packet samples are no longer convertible to the read type", nullptr);

    SizeT samplesToRead = *count * blockSize;
    info.prepare(blocks, samplesToRead, milliseconds(timeoutMs));

    ErrCode errCode = readPackets();

    SizeT samplesRead = samplesToRead - info.remainingSamplesToRead;
    *count = samplesRead / blockSize;
    return errCode;
}

ErrCode BlockReaderImpl::readWithDomain(void* dataBlocks, void* domainBlocks, SizeT* count, SizeT timeoutMs)
{
    OPENDAQ_PARAM_NOT_NULL(dataBlocks);
    OPENDAQ_PARAM_NOT_NULL(domainBlocks);
    OPENDAQ_PARAM_NOT_NULL(count);

    std::scoped_lock lock(mutex);

    if (invalid)
        return makeErrorInfo(OPENDAQ_ERR_INVALID_DATA, "Packet samples are no longer convertible to the read type.", nullptr);

    SizeT samplesToRead = *count * blockSize;
    info.prepareWithDomain(dataBlocks, domainBlocks, samplesToRead, milliseconds(timeoutMs));

    ErrCode errCode = readPackets();

    SizeT samplesRead = samplesToRead - info.remainingSamplesToRead;
    *count = samplesRead / blockSize;
    return errCode;
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
    static ErrCode Create(IBlockReader** out,
                          IBlockReader* toCopy,
                          SampleType valueReadType,
                          SampleType domainReadType
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

        SizeT blockSize;
        checkErrorInfo(toCopy->getBlockSize(&blockSize));

        return impl != nullptr
            ? createObject<IBlockReader, BlockReaderImpl>(out, impl, valueReadType, domainReadType)
            : createObject<IBlockReader, BlockReaderImpl>(out, old, valueReadType, domainReadType, blockSize, mode);
    }
};

OPENDAQ_DEFINE_CUSTOM_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC_OBJ(
    LIBRARY_FACTORY, IBlockReader, createBlockReaderFromExisting,
    IBlockReader*, invalidatedReader,
    SampleType, valueReadType,
    SampleType, domainReadType
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, BlockReader,
    IBlockReader, createBlockReaderFromPort,
    IInputPortConfig*, port,
    SizeT, blockSize,
    SampleType, valueReadType,
    SampleType, domainReadType,
    ReadMode, mode
)


END_NAMESPACE_OPENDAQ
