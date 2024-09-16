#include <opendaq/reader_errors.h>
#include <opendaq/tail_reader_impl.h>

BEGIN_NAMESPACE_OPENDAQ

TailReaderImpl::TailReaderImpl(ISignal* signal,
                               SizeT historySize,
                               SampleType valueReadType,
                               SampleType domainReadType,
                               ReadMode mode,
                               Bool skipEvents)
    : Super(SignalPtr(signal), mode, valueReadType, domainReadType, skipEvents)
    , historySize(historySize)
    , cachedSamples(0)
{
    port.setNotificationMethod(PacketReadyNotification::SameThread);
    packetReceived(port.as<IInputPort>(true));
}

TailReaderImpl::TailReaderImpl(IInputPortConfig* port,
                               SizeT historySize,
                               SampleType valueReadType,
                               SampleType domainReadType,
                               ReadMode mode,
                               Bool skipEvents)
    : Super(InputPortConfigPtr(port), mode, valueReadType, domainReadType, skipEvents)
    , historySize(historySize)
    , cachedSamples(0)
{
    this->port.setNotificationMethod(PacketReadyNotification::Scheduler);
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
}

TailReaderImpl::TailReaderImpl(TailReaderImpl* old,
                               SampleType valueReadType,
                               SampleType domainReadType,
                               SizeT historySize)
    : Super(old, valueReadType, domainReadType)
    , historySize(historySize)
    , cachedSamples(old->cachedSamples)
    , packets(old->packets)
    
{
    handleDescriptorChanged(DataDescriptorChangedEventPacket(dataDescriptor, domainDescriptor));
}

ErrCode TailReaderImpl::getAvailableCount(SizeT* count)
{
    OPENDAQ_PARAM_NOT_NULL(count);

    std::unique_lock lock(mutex);

    *count = cachedSamples;
    return OPENDAQ_SUCCESS;
}

ErrCode TailReaderImpl::getEmpty(Bool* empty)
{
    OPENDAQ_PARAM_NOT_NULL(empty);
    SizeT count;
    getAvailableCount(&count);
    *empty = count == 0;
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

TailReaderStatusPtr TailReaderImpl::readData(TailReaderInfo& info)
{
    std::unique_lock lock(mutex);

    if (info.remainingToRead > cachedSamples && info.remainingToRead > historySize)
    {
        return TailReaderStatus(nullptr, !invalid, 0, false);
    }

    if (cachedSamples > info.remainingToRead)
        info.offset = cachedSamples - info.remainingToRead;

    size_t readCachedSamples = 0;

    NumberPtr offset;
    for (auto it = packets.begin(); it != packets.end();)
    {
        const auto packet = *it;
        if (packet.getType() == PacketType::Event)
        {
            auto eventPacket = packet.asPtr<IEventPacket>(true);
            handleDescriptorChanged(eventPacket);
            it = packets.erase(packets.begin(), it + 1);
            cachedSamples -= readCachedSamples;

            if (!skipEvents || invalid || eventPacket.getEventId() == event_packet_id::IMPLICIT_DOMAIN_GAP_DETECTED)
            {
                return TailReaderStatus(packet, !invalid, offset);
            }
        } 
        else
        {
            auto dataPacket = packet.asPtrOrNull<IDataPacket>(true);
            if (dataPacket.assigned())
            {
                if (!offset.assigned() && info.offset < dataPacket.getSampleCount())
                {
                    offset = calculateOffset(dataPacket, info.offset);
                }
                readPacket(info, packet);
                readCachedSamples += dataPacket.getSampleCount();
            }            
            it++;
        }
    }

    return TailReaderStatus(nullptr, !invalid, offset);
}

ErrCode TailReaderImpl::read(void* values, SizeT* count, ITailReaderStatus** status)
{
    OPENDAQ_PARAM_NOT_NULL(count);
    if (*count != 0)
    {
        OPENDAQ_PARAM_NOT_NULL(values);
    }

    if (invalid)
    {
        if (status != nullptr)
        {
            *status = TailReaderStatus(nullptr, false).detach();
        }
        *count = 0;
        return OPENDAQ_IGNORED;
    }

    TailReaderInfo info{values, nullptr, *count};

    auto statusPtr = readData(info);
    if (status != nullptr)
    {
        *status = statusPtr.detach();
    }
    *count = *count - info.remainingToRead;
    return OPENDAQ_SUCCESS;
}

ErrCode TailReaderImpl::readWithDomain(void* values, void* domain, SizeT* count, ITailReaderStatus** status)
{
    OPENDAQ_PARAM_NOT_NULL(count);
    if (*count != 0)
    {
        OPENDAQ_PARAM_NOT_NULL(values);
        OPENDAQ_PARAM_NOT_NULL(domain);
    }

    if (invalid)
    {
        if (status != nullptr)
        {
            *status = TailReaderStatus(nullptr, false).detach();
        }
        *count = 0;
        return OPENDAQ_IGNORED;
    }

    TailReaderInfo info{values, domain, *count};

    auto statusPtr = readData(info);
    if (status != nullptr)
    {
        *status = statusPtr.detach();
    }
    *count = *count - info.remainingToRead;
    return OPENDAQ_SUCCESS;
}

ErrCode TailReaderImpl::packetReceived(IInputPort* /*port*/)
{
    std::unique_lock lock(mutex);
    bool hasEventPacket = false;
    PacketPtr packet = connection.dequeue();
    while (packet.assigned())
    {
        switch (packet.getType())
        {
            case PacketType::Data:
            {
                auto newPacket = packet.asPtr<IDataPacket>(true);
                SizeT newPacketSampleCount = newPacket.getSampleCount();
                if (cachedSamples < historySize)
                {
                    packets.push_back(packet);
                    cachedSamples += newPacketSampleCount;
                }
                else
                {
                    auto availableSamples = cachedSamples + newPacketSampleCount;
                    for (auto it = packets.begin(); it != packets.end();)
                    {
                        if (it->getType() == PacketType::Event)
                        {
                            ++it;
                            continue;
                        }
                
                        auto tmpPacket = it->asPtr<IDataPacket>(true);
                        SizeT sampleCount = tmpPacket.getSampleCount();
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
                break;
            }
            case PacketType::Event:
            {
                hasEventPacket = true;
                packets.push_back(packet);
                break;
            }
            case PacketType::None:
                break;
        }

        packet = connection.dequeue();
    }

    auto callback = readCallback;
    if (callback.assigned() && (hasEventPacket || (cachedSamples >= historySize)))
    {
        lock.unlock();
        return wrapHandler(callback);
    }
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

extern "C"
daq::ErrCode PUBLIC_EXPORT createTailReaderFromBuilder(ITailReader** objTmp, ITailReaderBuilder* builder)
{
    OPENDAQ_PARAM_NOT_NULL(builder);

    auto builderPtr = TailReaderBuilderPtr::Borrow(builder);

    if ((builderPtr.getValueReadType() == SampleType::Undefined || builderPtr.getDomainReadType() == SampleType::Undefined) &&
        builderPtr.getSkipEvents())
    {
        return makeErrorInfo(OPENDAQ_ERR_CREATE_FAILED, "Reader cannot skip events when sample type is undefined", nullptr);
    }

    if (auto port = builderPtr.getInputPort(); port.assigned())
    {
        return createObject<ITailReader, TailReaderImpl>(objTmp,
                                                         port.as<IInputPortConfig>(true),
                                                         builderPtr.getHistorySize(),
                                                         builderPtr.getValueReadType(),
                                                         builderPtr.getDomainReadType(),
                                                         builderPtr.getReadMode(),
                                                         builderPtr.getSkipEvents());
    }
    else if (auto signal = builderPtr.getSignal(); signal.assigned())
    {
        return createObject<ITailReader, TailReaderImpl>(objTmp,
                                                         signal,
                                                         builderPtr.getHistorySize(),
                                                         builderPtr.getValueReadType(),
                                                         builderPtr.getDomainReadType(),
                                                         builderPtr.getReadMode(),
                                                         builderPtr.getSkipEvents());
    }

    return makeErrorInfo(OPENDAQ_ERR_ARGUMENT_NULL, "Neither signal nor input port is not set in TailReader builder", nullptr);
}

END_NAMESPACE_OPENDAQ
