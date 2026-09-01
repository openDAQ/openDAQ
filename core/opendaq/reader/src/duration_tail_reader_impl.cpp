#include <opendaq/duration_tail_reader_impl.h>

#include <coretypes/impl.h>
#include <opendaq/event_packet_utils.h>
#include <opendaq/reader_errors.h>
#include <opendaq/reader_factory.h>

#include <algorithm>

BEGIN_NAMESPACE_OPENDAQ

struct DurationTailReaderInfo
{
    void* values{};
    void* domainValues{};
    SizeT remainingToRead{};

    SizeT offset{};
};

DurationTailReaderImpl::DurationTailReaderImpl(ISignal* signal,
                                               UInt historyDurationMs,
                                               SampleType valueReadType,
                                               SampleType domainReadType,
                                               ReadMode mode,
                                               Bool skipEvents)
    : Super(SignalPtr(signal), mode, valueReadType, domainReadType, skipEvents)
    , historyDurationMs(historyDurationMs)
{
    try
    {
        port.setNotificationMethod(PacketReadyNotification::SameThread);
        packetReceived(port.as<IInputPort>(true));
    }
    catch (...)
    {
        this->releaseWeakRefOnException();
        throw;
    }
}

DurationTailReaderImpl::DurationTailReaderImpl(IInputPortConfig* port,
                                               UInt historyDurationMs,
                                               SampleType valueReadType,
                                               SampleType domainReadType,
                                               ReadMode mode,
                                               Bool skipEvents)
    : Super(InputPortConfigPtr(port), mode, valueReadType, domainReadType, skipEvents)
    , historyDurationMs(historyDurationMs)
{
    try
    {
        const auto portNotifications = this->port.getNotificationMethod();
        if (portNotifications == PacketReadyNotification::None or portNotifications == PacketReadyNotification::Unspecified)
            this->port.setNotificationMethod(PacketReadyNotification::Scheduler);

        if (connection.assigned())
            packetReceived(this->port.as<IInputPort>(true));
    }
    catch (...)
    {
        this->releaseWeakRefOnException();
        throw;
    }
}

std::optional<Float> DurationTailReaderImpl::getPacketStartMs(const PacketPtr& packet) const
{
    if (packet.getType() != PacketType::Data)
        return std::nullopt;

    const auto dataPacket = packet.asPtr<IDataPacket>(true);
    if (!dataPacket.assigned())
        return std::nullopt;

    const auto domainPacket = dataPacket.getDomainPacket();
    if (!domainPacket.assigned())
        return std::nullopt;

    const auto desc = domainPacket.getDataDescriptor();
    auto tickResolution = desc.getTickResolution();
    if (!tickResolution.assigned())
        tickResolution = Ratio(1, 1);

    const auto rule = desc.getRule();
    if (rule.assigned() && rule.getType() == DataRuleType::Explicit)
    {
        Float tick = domainPacket.getValueByIndex(0);
        return static_cast<Float>(Ratio(tick, 1) * tickResolution) * 1000.0;
    }

    const auto offset = domainPacket.getOffset();
    if (!offset.assigned())
        return std::nullopt;

    return static_cast<Float>(Ratio(offset.getIntValue(), 1) * tickResolution) * 1000.0;
}

void DurationTailReaderImpl::trimOldPackets()
{
    if (packets.size() < 2)
        return;

    const auto tLatest = getPacketStartMs(packets.back());
    const auto tFirst = getPacketStartMs(packets.front());

    if (!tLatest.has_value())
        return;

    const Float cutoff = *tLatest - static_cast<Float>(historyDurationMs);

    const auto firstKeep = std::lower_bound(packets.begin(),
                                            packets.end(),
                                            cutoff,
                                            [this](const PacketPtr& packet, Float cut) {
                                                const auto t = getPacketStartMs(packet);
                                                return t.has_value() && *t < cut;
                                            });

    if (firstKeep != packets.begin())
    {
        for (auto it = packets.begin(); it != firstKeep; ++it)
        {
            const auto dataPacket = it->asPtrOrNull<IDataPacket>(true);
            if (dataPacket.assigned())
                bufferedSampleCount -= dataPacket.getSampleCount();
        }
        packets.erase(packets.begin(), firstKeep);
    }
}

Bool DurationTailReaderImpl::hasSufficientHistory() const
{
    if (packets.empty())
        return false;

    const auto tFront = getPacketStartMs(packets.front());
    const auto tBack = getPacketStartMs(packets.back());
    if (!tFront.has_value() || !tBack.has_value())
        return !packets.empty();

    return (*tBack - *tFront) >= static_cast<Float>(historyDurationMs);
}

ErrCode DurationTailReaderImpl::getHistoryDurationMs(UInt* milliseconds)
{
    OPENDAQ_PARAM_NOT_NULL(milliseconds);

    std::unique_lock lock(mutex);
    *milliseconds = historyDurationMs;
    return OPENDAQ_SUCCESS;
}

ErrCode DurationTailReaderImpl::setHistoryDurationMs(UInt milliseconds)
{
    if (milliseconds == 0)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "History duration must be positive");

    std::unique_lock lock(mutex);
    historyDurationMs = milliseconds;
    trimOldPackets();
    return OPENDAQ_SUCCESS;
}

ErrCode DurationTailReaderImpl::getAvailableCount(SizeT* count)
{
    OPENDAQ_PARAM_NOT_NULL(count);

    std::unique_lock lock(mutex);
    *count = bufferedSampleCount;
    return OPENDAQ_SUCCESS;
}

ErrCode DurationTailReaderImpl::getEmpty(Bool* empty)
{
    OPENDAQ_PARAM_NOT_NULL(empty);

    SizeT count = 0;
    getAvailableCount(&count);
    *empty = count == 0;
    return OPENDAQ_SUCCESS;
}

ErrCode DurationTailReaderImpl::readPacket(DurationTailReaderInfo& info, const DataPacketPtr& dataPacket)
{
    const SizeT sampleCount = dataPacket.getSampleCount();
    if (info.offset > sampleCount)
    {
        info.offset -= sampleCount;
        return OPENDAQ_SUCCESS;
    }

    const auto remainingSampleCount = sampleCount - info.offset;
    const SizeT toRead = std::min(info.remainingToRead, remainingSampleCount);

    ErrCode errCode = valueReader->readData(getValuePacketData(dataPacket), info.offset, &info.values, toRead);
    OPENDAQ_RETURN_IF_FAILED(errCode);

    if (info.domainValues != nullptr)
    {
        auto domainPacket = dataPacket.getDomainPacket();
        errCode = domainReader->readData(domainPacket.getData(), info.offset, &info.domainValues, toRead);
        if (errCode == OPENDAQ_ERR_INVALIDSTATE)
        {
            if (!trySetDomainSampleType(domainPacket))
                return DAQ_EXTEND_ERROR_INFO(errCode, "Failed to set domain sample type for packet");

            daqClearErrorInfo();
            errCode = domainReader->readData(domainPacket.getData(), info.offset, &info.domainValues, toRead);
        }

        OPENDAQ_RETURN_IF_FAILED(errCode);
    }

    info.offset = 0;
    info.remainingToRead -= toRead;
    return OPENDAQ_SUCCESS;
}

TailReaderStatusPtr DurationTailReaderImpl::readData(DurationTailReaderInfo& info)
{
    std::unique_lock lock(mutex);

    const SizeT available = bufferedSampleCount;
    if (info.remainingToRead > available)
        return TailReaderStatus(nullptr, !invalid, 0, false);

    if (available > info.remainingToRead)
        info.offset = available - info.remainingToRead;

    NumberPtr offset;
    for (const auto& packet : packets)
    {
        const auto dataPacket = packet.asPtrOrNull<IDataPacket>(true);
        if (!dataPacket.assigned())
            continue;

        if (!offset.assigned() && info.offset < dataPacket.getSampleCount())
            offset = calculateOffset(dataPacket, info.offset);

        readPacket(info, packet);
    }

    return TailReaderStatus(nullptr, !invalid, offset, hasSufficientHistory());
}

ErrCode DurationTailReaderImpl::read(void* values, SizeT* count, ITailReaderStatus** status)
{
    OPENDAQ_PARAM_NOT_NULL(count);
    if (*count != 0)
        OPENDAQ_PARAM_NOT_NULL(values);

    if (invalid)
    {
        if (status != nullptr)
            *status = TailReaderStatus(nullptr, false).detach();
        *count = 0;
        return OPENDAQ_IGNORED;
    }

    DurationTailReaderInfo info{values, nullptr, *count};

    auto statusPtr = readData(info);
    if (status != nullptr)
        *status = statusPtr.detach();

    *count = *count - info.remainingToRead;
    return OPENDAQ_SUCCESS;
}

ErrCode DurationTailReaderImpl::readWithDomain(void* values, void* domain, SizeT* count, ITailReaderStatus** status)
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
            *status = TailReaderStatus(nullptr, false).detach();
        *count = 0;
        return OPENDAQ_IGNORED;
    }

    DurationTailReaderInfo info{values, domain, *count};

    auto statusPtr = readData(info);
    if (status != nullptr)
        *status = statusPtr.detach();

    *count = *count - info.remainingToRead;
    return OPENDAQ_SUCCESS;
}

ErrCode DurationTailReaderImpl::packetReceived(IInputPort* port)
{
    std::unique_lock lock(mutex);

    const auto incoming = connection.dequeueAll();
    if (incoming.assigned() && incoming.getCount() > 0)
    {
        std::optional<Float> tLatest;
        std::vector<PacketPtr> collected;
        collected.reserve(static_cast<size_t>(incoming.getCount()));

        bool done = false;
        for (SizeT i = incoming.getCount(); i > 0 && !done; --i)
        {
            const PacketPtr packet = incoming[i - 1];

            switch (packet.getType())
            {
                case PacketType::Data:
                {
                    if (!tLatest.has_value())
                        tLatest = getPacketStartMs(packet);

                    if (tLatest.has_value())
                    {
                        const auto t = getPacketStartMs(packet);
                        if (t.has_value() && *t < *tLatest - static_cast<Float>(historyDurationMs))
                        {
                            packets.clear();
                            bufferedSampleCount = 0;
                            done = true;
                        }
                        else
                        {
                            collected.push_back(packet);
                        }
                    }
                    else
                    {
                        collected.push_back(packet);
                    }
                    break;
                }
                case PacketType::Event:
                {
                    const auto eventPacket = packet.asPtr<IEventPacket>(true);
                    if (eventPacket.assigned()
                        && eventPacket.getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED)
                    {
                        Super::handleDescriptorChanged(eventPacket);
                    }
                    break;
                }
                case PacketType::None:
                    break;
            }
        }

        for (auto it = collected.rbegin(); it != collected.rend(); ++it)
        {
            packets.push_back(*it);
            const auto dataPacket = it->asPtrOrNull<IDataPacket>(true);
            if (dataPacket.assigned())
                bufferedSampleCount += dataPacket.getSampleCount();
        }

        if (!done)
            trimOldPackets();
    }

    const auto callback = readCallback;
    const Bool sufficientHistory = hasSufficientHistory();
    lock.unlock();

    if (callback.assigned() && sufficientHistory)
        OPENDAQ_RETURN_IF_FAILED(wrapHandler(callback));

    if (externalListener.assigned() && externalListener.getRef().assigned())
        return externalListener.getRef()->packetReceived(port);

    return OPENDAQ_SUCCESS;
}

// Generates: extern "C" ErrCode createDurationTailReader(IDurationTailReader** objTmp, ISignal* signal, ...)
// { return createObject<IDurationTailReader, DurationTailReaderImpl>(objTmp, signal, ...); }
// - the same C-ABI-exported pattern every other reader in this module uses, so DurationTailReader
// is constructible from the C bindings too, not just from C++. `skipEvents` is deliberately not
// part of the public factory surface (matching TailReader) - it stays at its constructor default.
OPENDAQ_DEFINE_CLASS_FACTORY(
    LIBRARY_FACTORY, DurationTailReader,
    ISignal*, signal,
    UInt, historyDurationMs,
    SampleType, valueReadType,
    SampleType, domainReadType,
    ReadMode, mode
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, DurationTailReader,
    IDurationTailReader, createDurationTailReaderFromPort,
    IInputPortConfig*, port,
    UInt, historyDurationMs,
    SampleType, valueReadType,
    SampleType, domainReadType,
    ReadMode, mode
)

END_NAMESPACE_OPENDAQ
