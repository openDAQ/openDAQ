#include <opendaq/reader_errors.h>
#include <opendaq/tail_reader_impl.h>

BEGIN_NAMESPACE_OPENDAQ

TailReaderImpl::TailReaderImpl(ISignal* signal,
                               SizeT historySize,
                               SampleType valueReadType,
                               SampleType domainReadType,
                               ReadMode mode)
    : Super(SignalPtr(signal), mode, valueReadType, domainReadType)
    , historySize(historySize)
    , cachedSamples(0)
{
    port.setNotificationMethod(PacketReadyNotification::SameThread);
    TailReaderImpl::handleDescriptorChanged(connection.dequeue());
}

TailReaderImpl::TailReaderImpl(IInputPortConfig* port,
                               SizeT historySize,
                               SampleType valueReadType,
                               SampleType domainReadType,
                               ReadMode mode)
    : Super(InputPortConfigPtr(port), mode, valueReadType, domainReadType)
    , historySize(historySize)
    , cachedSamples(0)
{
    this->port.setNotificationMethod(PacketReadyNotification::Scheduler);

    if (connection.assigned())
        TailReaderImpl::handleDescriptorChanged(connection.dequeue());
}

TailReaderImpl::TailReaderImpl(const ReaderConfigPtr& readerConfig,
                               SampleType valueReadType,
                               SampleType domainReadType,
                               SizeT historySize,
                               ReadMode mode)
    : Super(readerConfig, mode, valueReadType, domainReadType)
    , historySize(historySize)
    , cachedSamples(0)
{
    readDescriptorFromPort();
}

TailReaderImpl::TailReaderImpl(TailReaderImpl* old,
                               SampleType valueReadType,
                               SampleType domainReadType,
                               SizeT historySize)
    : Super(old, valueReadType, domainReadType)
    , historySize(historySize)
    , cachedSamples(0)
{
    readDescriptorFromPort();
}

ErrCode TailReaderImpl::getAvailableCount(SizeT* count)
{
    OPENDAQ_PARAM_NOT_NULL(count);

    std::unique_lock lock(mutex);

    *count = cachedSamples;
    return OPENDAQ_SUCCESS;
}

ErrCode TailReaderImpl::getHistorySize(SizeT* size)
{
    OPENDAQ_PARAM_NOT_NULL(size);

    *size = historySize;
    return OPENDAQ_SUCCESS;
}

struct TailReaderInfo
{
    void* values{};
    void* domainValues{};
    SizeT remainingToRead{};

    SizeT offset{};
};

ErrCode TailReaderImpl::readPacket(TailReaderInfo& info, const DataPacketPtr& dataPacket)
{
    SizeT sampleCount = dataPacket.getSampleCount();
    if (info.offset > sampleCount)
    {
        info.offset -= sampleCount;
        return OPENDAQ_SUCCESS;
    }

    auto remainingSampleCount = sampleCount - info.offset;
    SizeT toRead = std::min(info.remainingToRead, remainingSampleCount);

    ErrCode errCode = valueReader->readData(getValuePacketData(dataPacket), info.offset, &info.values, toRead);
    if (OPENDAQ_FAILED(errCode))
    {
        return errCode;
    }

    if (info.domainValues != nullptr)
    {
        if (dataPacket.getType() != PacketType::Data)
        {
            return makeErrorInfo(OPENDAQ_ERR_INVALIDSTATE, "Packets must have an associated domain packets to read domain data.");
        }

        auto domainPacket = dataPacket.getDomainPacket();
        errCode = domainReader->readData(domainPacket.getData(), info.offset, &info.domainValues, toRead);
        if (errCode == OPENDAQ_ERR_INVALIDSTATE)
        {
            if (!trySetDomainSampleType(domainPacket))
            {
                return errCode;
            }
            errCode = domainReader->readData(domainPacket.getData(), info.offset, &info.domainValues, toRead);
        }

        if (OPENDAQ_FAILED(errCode))
        {
            return errCode;
        }
    }

    info.offset = 0;

    info.remainingToRead -= toRead;
    return OPENDAQ_SUCCESS;
}

ErrCode TailReaderImpl::readData(TailReaderInfo& info)
{
    if (info.remainingToRead == 0)
    {
        return OPENDAQ_SUCCESS;
    }

    std::unique_lock lock(mutex);

    if (info.remainingToRead > cachedSamples && info.remainingToRead > historySize)
    {
        return makeErrorInfo(OPENDAQ_ERR_SIZETOOLARGE, "The requested sample-count exceeds the reader history size.");
    }

    if (cachedSamples > info.remainingToRead)
        info.offset = cachedSamples - info.remainingToRead;

    ErrCode errCode = OPENDAQ_SUCCESS;
    for (const auto& dataPacket : packets)
    {
        errCode = readPacket(info, dataPacket);
        if (OPENDAQ_FAILED(errCode))
        {
            return errCode;
        }
    }

    return errCode;
}

ErrCode TailReaderImpl::read(void* values, SizeT* count)
{
    OPENDAQ_PARAM_NOT_NULL(values);
    OPENDAQ_PARAM_NOT_NULL(count);

    TailReaderInfo info{values, nullptr, *count};

    ErrCode errCode = readData(info);
    *count = *count - info.remainingToRead;
    return errCode;
}

ErrCode TailReaderImpl::readWithDomain(void* values, void* domain, SizeT* count)
{
    OPENDAQ_PARAM_NOT_NULL(values);
    OPENDAQ_PARAM_NOT_NULL(domain);
    OPENDAQ_PARAM_NOT_NULL(count);

    TailReaderInfo info{values, domain, *count};

    ErrCode errCode = readData(info);
    *count = *count - info.remainingToRead;
    return errCode;
}

void TailReaderImpl::pushPacket(const PacketPtr& packet)
{
    std::unique_lock lock(mutex);
    auto newPacket = packet.asPtrOrNull<IDataPacket>(true);
    SizeT newPacketSampleCount = newPacket.getSampleCount();
    if (cachedSamples < historySize)
    {
        packets.push_back(newPacket);
        cachedSamples += newPacketSampleCount;
    }
    else
    {
        auto availableSamples = cachedSamples + newPacketSampleCount;
        for (auto it = packets.begin(); it != packets.end();)
        {
            SizeT sampleCount = it->getSampleCount();
            if (availableSamples - sampleCount >= historySize)
            {
                it = packets.erase(it);
                availableSamples -= sampleCount;
                continue;
            }
            else
            {
                break;
            }
            ++it;
        }

        packets.push_back(newPacket);
        cachedSamples = availableSamples;
    }
}

ErrCode TailReaderImpl::packetReceived(IInputPort* /*port*/)
{
    PacketPtr packet = readFromConnection();
    while (packet.assigned())
    {
        switch (packet.getType())
        {
            case PacketType::Data:
            {
                pushPacket(packet);
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

        packet = readFromConnection();
    }

    if (readCallback.assigned() && cachedSamples >= historySize)
        readCallback();

    return OPENDAQ_SUCCESS;
}

OPENDAQ_DEFINE_CLASS_FACTORY(
    LIBRARY_FACTORY, TailReader,
    ISignal*, signal,
    SizeT, windowSize,
    SampleType, valueReadType,
    SampleType, domainReadType,
    ReadMode, mode
)

template <>
struct ObjectCreator<ITailReader>
{
    static ErrCode Create(ITailReader** out,
                          ITailReader* toCopy,
                          SizeT historySize,
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
        auto impl = dynamic_cast<TailReaderImpl*>(old.getObject());

        return impl != nullptr
            ? createObject<ITailReader, TailReaderImpl>(out, impl, valueReadType, domainReadType, historySize)
            : createObject<ITailReader, TailReaderImpl>(out, old, valueReadType, domainReadType, historySize, mode);
    }
};

OPENDAQ_DEFINE_CUSTOM_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC_OBJ(
    LIBRARY_FACTORY, ITailReader, createTailReaderFromExisting,
    ITailReader*, invalidatedReader,
    SizeT, historySize,
    SampleType, valueReadType,
    SampleType, domainReadType
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, TailReader,
    ITailReader, createTailReaderFromPort,
    IInputPortConfig*, port,
    SizeT, historySize,
    SampleType, valueReadType,
    SampleType, domainReadType,
    ReadMode, mode
)

END_NAMESPACE_OPENDAQ
