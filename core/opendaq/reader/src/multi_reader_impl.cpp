#include <coreobjects/ownable_ptr.h>
#include <coreobjects/property_object_factory.h>
#include <coretypes/validation.h>
#include <opendaq/custom_log.h>
#include <opendaq/event_packet_params.h>
#include <opendaq/input_port_factory.h>
#include <opendaq/multi_reader_impl.h>
#include <opendaq/packet_factory.h>
#include <opendaq/reader_errors.h>
#include <opendaq/reader_utils.h>

#include <fmt/ostream.h>
#include <set>
#include <thread>

using namespace std::chrono;

using Milliseconds = duration<double, std::milli>;

template <>
struct fmt::formatter<daq::Comparable> : ostream_formatter
{
};

BEGIN_NAMESPACE_OPENDAQ

MultiReaderImpl::MultiReaderImpl(const ListPtr<IComponent>& list,
                                 SampleType valueReadType,
                                 SampleType domainReadType,
                                 ReadMode mode,
                                 ReadTimeoutType timeoutType,
                                 std::int64_t requiredCommonSampleRate,
                                 Bool startOnFullUnitOfDomain,
                                 SizeT minReadCount)
    : requiredCommonSampleRate(requiredCommonSampleRate)
    , startOnFullUnitOfDomain(startOnFullUnitOfDomain)
    , minReadCount(minReadCount)
{
    this->internalAddRef();
    try
    {
        checkEarlyPreconditionsAndCacheContext(list);
        loggerComponent = context.getLogger().getOrAddComponent("MultiReader");
        bool fromInputPorts;
        auto listener = this->thisPtr<InputPortNotificationsPtr>();
        auto ports = checkPreconditions(list, true, fromInputPorts, listener);

        if (fromInputPorts)
            portBinder = PropertyObject();

        connectPorts(ports, valueReadType, domainReadType, mode);
    }
    catch (...)
    {
        this->releaseWeakRefOnException();
        throw;
    }
}

MultiReaderImpl::MultiReaderImpl(MultiReaderImpl* old, SampleType valueReadType, SampleType domainReadType)
    : loggerComponent(old->loggerComponent)
{
    std::scoped_lock lock(old->mutex);
    old->invalid = true;
    portBinder = old->portBinder;
    startOnFullUnitOfDomain = old->startOnFullUnitOfDomain;
    isActive = old->isActive;
    minReadCount = old->minReadCount;

    bool fromInputPorts;

    auto oldSignals = old->getSignals();
    checkEarlyPreconditionsAndCacheContext(oldSignals);
    checkPreconditions(oldSignals, false, fromInputPorts);
    commonSampleRate = old->commonSampleRate;
    requiredCommonSampleRate = old->requiredCommonSampleRate;

    this->internalAddRef();
    try
    {
        auto listener = this->thisPtr<InputPortNotificationsPtr>();

        for (auto& signal : old->signals)
        {
            signals.emplace_back(signal, listener, valueReadType, domainReadType);
        }

        updateCommonSampleRateAndDividers();
        if (invalid)
            throw InvalidParameterException("Signal sample rate does not match required common sample rate");
    }
    catch (...)
    {
        this->releaseWeakRefOnException();
        throw;
    }
}

MultiReaderImpl::MultiReaderImpl(const ReaderConfigPtr& readerConfig, SampleType valueReadType, SampleType domainReadType, ReadMode mode)
    : minReadCount(1)
{
    if (!readerConfig.assigned())
        throw ArgumentNullException("Existing reader must not be null");

    readerConfig.markAsInvalid();

    this->internalAddRef();
    auto listener = this->thisPtr<InputPortNotificationsPtr>();

    auto ports = readerConfig.getInputPorts();

    checkEarlyPreconditionsAndCacheContext(ports);
    loggerComponent = context.getLogger().getOrAddComponent("MultiReader");
    bool fromInputPorts;
    checkPreconditions(ports, false, fromInputPorts);

    SignalInfo sigInfo{nullptr, readerConfig.getValueTransformFunction(), readerConfig.getDomainTransformFunction(), mode, loggerComponent};

    for (const auto& port : ports)
    {
        sigInfo.port = port;
        signals.emplace_back(sigInfo, listener, valueReadType, domainReadType);
    }

    updateCommonSampleRateAndDividers();
    if (invalid)
        throw InvalidParameterException("Signal sample rate does not match required common sample rate");
}

MultiReaderImpl::MultiReaderImpl(const MultiReaderBuilderPtr& builder)
    : requiredCommonSampleRate(builder.getRequiredCommonSampleRate())
    , startOnFullUnitOfDomain(builder.getStartOnFullUnitOfDomain())
    , minReadCount(builder.getMinReadCount())
{
    internalAddRef();
    try
    {
        auto sourceComponents = builder.getSourceComponents();
        checkEarlyPreconditionsAndCacheContext(sourceComponents);
        loggerComponent = context.getLogger().getOrAddComponent("MultiReader");
        bool fromInputPorts;
        auto listener = this->thisPtr<InputPortNotificationsPtr>();
        auto ports = checkPreconditions(sourceComponents, false, fromInputPorts, listener);

        if (fromInputPorts)
            portBinder = PropertyObject();
        connectPorts(ports, builder.getValueReadType(), builder.getDomainReadType(), builder.getReadMode());
    }
    catch (...)
    {
        this->releaseWeakRefOnException();
        throw;
    }
}

MultiReaderImpl::~MultiReaderImpl()
{
    if (!portBinder.assigned())
        for (const auto& signal : signals)
            signal.port.remove();
}

ListPtr<ISignal> MultiReaderImpl::getSignals() const
{
    auto list = List<ISignal>();
    for (auto& signal : signals)
    {
        list.pushBack(signal.connection.getSignal());
    }
    return list;
}

struct MultiReaderImpl::ReferenceDomainBin
{
    StringPtr id;
    TimeSource timeSource;

    bool operator<(const ReferenceDomainBin& rhs) const
    {
        if (id == rhs.id)
            return timeSource < rhs.timeSource;
        if (id.assigned() && rhs.id.assigned())
            return id < rhs.id;
        if (rhs.id.assigned())
            return true;
        return false;
    }
};

void MultiReaderImpl::isDomainValid(const ListPtr<IInputPortConfig>& list)
{
    StringPtr domainUnitSymbol;
    StringPtr domainQuantity;
    TimeSource timeSource = TimeSource::Unknown;
    std::set<ReferenceDomainBin> bins;

    for (const auto& port : list)
    {
        auto signal = port.getSignal();
        if (!signal.assigned())
        {
            continue;
        }

        auto domain = signal.getDomainSignal();
        if (!domain.assigned())
        {
            throw InvalidParameterException(R"(Signal "{}" does not have a domain signal set.)", signal.getLocalId());
        }

        auto domainDescriptor = domain.getDescriptor();
        if (!domainDescriptor.assigned())
        {
            throw InvalidParameterException(R"(Signal "{}" does not have a domain descriptor set.)", signal.getLocalId());
        }

        auto domainUnit = domainDescriptor.getUnit();
        if (!domainUnit.assigned())
        {
            throw InvalidParameterException(R"(Signal "{}" does not have a domain unit set.)", signal.getLocalId());
        }

        if (!domainQuantity.assigned() || domainQuantity.getLength() == 0)
        {
            domainQuantity = domainUnit.getQuantity();
            domainUnitSymbol = domainUnit.getSymbol();

            if (!domainQuantity.assigned() || domainQuantity.getLength() == 0)
            {
                throw InvalidParameterException(R"(Signal "{}" does not have a domain quantity set.)", signal.getLocalId());
            }

            if (domainQuantity != "time")
            {
                throw NotSupportedException(R"(Signal "{}" domain quantity is not "time" but "{}" which is not currently supported.)",
                                            signal.getLocalId(),
                                            domainQuantity);
            }

            if (domainUnitSymbol != "s")
            {
                throw NotSupportedException(R"(Signal "{}" domain unit is not "s" but "{}" which is not currently supported.)",
                                            signal.getLocalId(),
                                            domainUnitSymbol);
            }
        }
        else
        {
            if (domainQuantity != domainUnit.getQuantity())
            {
                throw InvalidStateException(R"(Signal "{}" domain quantity does not match with others.)", signal.getLocalId());
            }

            if (domainUnitSymbol != domainUnit.getSymbol())
            {
                throw InvalidStateException(R"(Signal "{}" domain unit does not match with others.)", signal.getLocalId());
            }
        }

        auto referenceDomainInfo = domainDescriptor.getReferenceDomainInfo();

        if (!referenceDomainInfo.assigned())
        {
            LOG_D(R"(Domain signal "{}" Reference Domain Info is not assigned.)", domain.getLocalId());
        }
        else
        {
            auto referenceDomainID = referenceDomainInfo.getReferenceDomainId();

            if (!referenceDomainID.assigned() || referenceDomainID.getLength() == 0)
            {
                // This will perhaps be bumped up to a higher severity later on (warning)
                LOG_D(R"(Domain signal "{}" Reference Domain ID not assigned.)", domain.getLocalId());
            }

            if (referenceDomainInfo.getReferenceTimeSource() == TimeSource::Unknown)
            {
                // This will perhaps be bumped up to a higher severity later on (warning)
                LOG_D(R"(Domain signal "{}" Reference Time Source is Unknown.)", domain.getLocalId());
            }
            else
            {
                if (timeSource != TimeSource::Unknown && referenceDomainInfo.getReferenceTimeSource() != timeSource)
                    throw InvalidStateException("Only one known Reference Time Source is allowed per Multi Reader.");
                timeSource = referenceDomainInfo.getReferenceTimeSource();
            }

            ReferenceDomainBin bin = {referenceDomainInfo.getReferenceDomainId(), referenceDomainInfo.getReferenceTimeSource()};
            auto elt = bins.begin();
            while (elt != bins.end())
            {
                // Traverse one group

                bool needsKnownTimeSource = false;
                bool hasKnownTimeSource = false;
                auto groupDomainId = elt->id;

                while (elt != bins.end() && elt->id == groupDomainId)
                {
                    if (groupDomainId.assigned() && bin.id.assigned() && groupDomainId != bin.id)
                    {
                        // Both are assigned, but not matching
                        // Needs at least one known time source
                        needsKnownTimeSource = true;
                    }
                    if (elt->timeSource != TimeSource::Unknown)
                    {
                        // Group (domain signals with identical domain ID) has at least one known time source
                        hasKnownTimeSource = true;
                    }
                    ++elt;
                }

                if (needsKnownTimeSource && !hasKnownTimeSource)
                {
                    throw InvalidStateException("Reference domain is incompatible.");
                }
            }

            bins.insert(bin);
        }
    }
}

void MultiReaderImpl::updateCommonSampleRateAndDividers()
{
    if (requiredCommonSampleRate > 0)
    {
        commonSampleRate = requiredCommonSampleRate;
    }
    else
    {
        commonSampleRate = 1;
        for (const auto& signal : signals)
        {
            commonSampleRate = std::lcm<std::int64_t>(signal.sampleRate, commonSampleRate);
        }
    }

    for (auto& signal : signals)
    {
        signal.setCommonSampleRate(commonSampleRate);
        if (signal.invalid)
        {
            invalid = true;
            return;
        }
    }

    sampleRateDividerLcm = 1;
    for (const auto& signal : signals)
    {
        if (!signal.connection.assigned())
        {
            return;
        }
        sampleRateDividerLcm = std::lcm(signal.sampleRateDivider, sampleRateDividerLcm);
    }
}

void MultiReaderImpl::checkEarlyPreconditionsAndCacheContext(const ListPtr<IComponent>& list)
{
    if (!list.assigned())
        throw NotAssignedException("List of inputs is not assigned");
    if (list.getCount() == 0)
        throw InvalidParameterException("Need at least one signal.");
    context = list[0].getContext();
}

ListPtr<IInputPortConfig> MultiReaderImpl::checkPreconditions(const ListPtr<IComponent>& list,
                                                              bool overrideMethod,
                                                              bool& fromInputPorts,
                                                              InputPortNotificationsPtr listener)
{
    bool haveInputPorts = false;
    bool haveSignals = false;

    auto portList = List<IInputPortConfig>();
    for (const auto& el : list)
    {
        if (auto signal = el.asPtrOrNull<ISignal>(); signal.assigned())
        {
            if (haveInputPorts)
                throw InvalidParameterException("Cannot pass both input ports and signals as items");
            auto port = InputPort(context, nullptr, fmt::format("multi_reader_signal_{}", signal.getLocalId()));
            if (listener.assigned())
                port.setListener(listener);
            port.setNotificationMethod(PacketReadyNotification::SameThread);
            port.connect(signal);
            portList.pushBack(port);

            haveSignals = true;
        }
        else if (auto port = el.asPtrOrNull<IInputPortConfig>(); port.assigned())
        {
            if (haveSignals)
                throw InvalidParameterException("Cannot pass both input ports and signals as items");

            if (overrideMethod && port.getConnection().assigned())
                throw InvalidParameterException("Signal has been connected to the port before the reader is created");

            if (overrideMethod)
                port.setNotificationMethod(PacketReadyNotification::Scheduler);
            portList.pushBack(port);

            haveInputPorts = true;
        }
        else
        {
            throw InvalidParameterException("One of the elements of input list is not signal or input port");
        }
    }

    fromInputPorts = haveInputPorts;

    isDomainValid(portList);
    return portList;
}

void MultiReaderImpl::setStartInfo()
{
    LOG_T("<----")
    LOG_T("Setting start info:")

    RatioPtr maxResolution = signals[0].domainInfo.resolution;
    system_clock::time_point minEpoch = signals[0].domainInfo.epoch;
    for (auto& sigInfo : signals)
    {
        if (sigInfo.domainInfo.epoch < minEpoch)
        {
            minEpoch = sigInfo.domainInfo.epoch;
        }

        if (static_cast<double>(sigInfo.domainInfo.resolution) < static_cast<double>(maxResolution))
        {
            maxResolution = sigInfo.domainInfo.resolution;
        }
    }

    auto systemClockResolution = system_clock::period::num / static_cast<double>(system_clock::period::den);
    if (systemClockResolution < static_cast<double>(maxResolution))
    {
        maxResolution = Ratio(system_clock::period::num, system_clock::period::den);
    }

    readResolution = maxResolution;
    readOrigin = date::format("%FT%TZ", minEpoch);

    LOG_T("MaxResolution: {}", maxResolution)
    LOG_T("MinEpoch: {}", minEpoch)

    for (auto& sigInfo : signals)
    {
        sigInfo.setStartInfo(minEpoch, maxResolution);
    }
}

void MultiReaderImpl::connectPorts(const ListPtr<IInputPortConfig>& inputPorts, SampleType valueRead, SampleType domainRead, ReadMode mode)
{
    auto listener = this->thisPtr<InputPortNotificationsPtr>();

    for (const auto& port : inputPorts)
    {
        if (portBinder.assigned())
            port.asPtr<IOwnable>().setOwner(portBinder);
        port.setListener(listener);

        signals.emplace_back(port, valueRead, domainRead, mode, loggerComponent);
    }
    portConnected = true;
}

ErrCode MultiReaderImpl::setOnDataAvailable(IProcedure* callback)
{
    std::scoped_lock lock(mutex);

    readCallback = callback;
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReaderImpl::getValueReadType(SampleType* sampleType)
{
    OPENDAQ_PARAM_NOT_NULL(sampleType);

    *sampleType = signals[0].valueReader->getReadType();
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReaderImpl::getDomainReadType(SampleType* sampleType)
{
    OPENDAQ_PARAM_NOT_NULL(sampleType);

    *sampleType = signals[0].domainReader->getReadType();
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReaderImpl::setValueTransformFunction(IFunction* transform)
{
    std::scoped_lock lock(mutex);

    for (auto& signal : signals)
    {
        signal.valueReader->setTransformFunction(transform);
    }

    return OPENDAQ_SUCCESS;
}

ErrCode MultiReaderImpl::setDomainTransformFunction(IFunction* transform)
{
    std::scoped_lock lock(mutex);

    for (auto& signal : signals)
    {
        signal.domainReader->setTransformFunction(transform);
    }

    return OPENDAQ_SUCCESS;
}

ErrCode MultiReaderImpl::getReadMode(ReadMode* mode)
{
    OPENDAQ_PARAM_NOT_NULL(mode);

    *mode = signals[0].readMode;
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReaderImpl::getAvailableCount(SizeT* count)
{
    OPENDAQ_PARAM_NOT_NULL(count);

    std::lock_guard lock(mutex);

    std::lock_guard lockNotify(notify.mutex);

    SizeT min{};
    SyncStatus syncStatus{};
    ErrCode errCode = synchronize(min, syncStatus);
    if (OPENDAQ_FAILED(errCode))
    {
        return errCode;
    }

    SizeT cnt = 0;
    if (syncStatus == SyncStatus::Synchronized)
    {
        cnt = (min / sampleRateDividerLcm) * sampleRateDividerLcm;
        if (cnt < minReadCount)
            cnt = 0;
    }

    *count = cnt;
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReaderImpl::read(void* samples, SizeT* count, SizeT timeoutMs, IMultiReaderStatus** status)
{
    OPENDAQ_PARAM_NOT_NULL(count);
    if (*count != 0)
    {
        OPENDAQ_PARAM_NOT_NULL(samples);

        if (minReadCount > *count)
            return makeErrorInfo(OPENDAQ_ERR_INVALIDPARAMETER, "Count parameter has to be either 0 or larger than minReadCount.");
    }

    std::scoped_lock lock(mutex);

    if (invalid)
    {
        if (status)
            *status = createReaderStatus().detach();
        *count = 0;
        return OPENDAQ_IGNORED;
    }

    SizeT samplesToRead = (*count / sampleRateDividerLcm) * sampleRateDividerLcm;
    prepare((void**) samples, samplesToRead, milliseconds(timeoutMs));

    auto statusPtr = readPackets();
    if (status)
        *status = statusPtr.detach();

    SizeT samplesRead = samplesToRead - remainingSamplesToRead;
    *count = samplesRead;
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReaderImpl::readWithDomain(void* samples, void* domain, SizeT* count, SizeT timeoutMs, IMultiReaderStatus** status)
{
    OPENDAQ_PARAM_NOT_NULL(count);
    if (*count != 0)
    {
        OPENDAQ_PARAM_NOT_NULL(samples);
        OPENDAQ_PARAM_NOT_NULL(domain);

        if (minReadCount > *count)
            return makeErrorInfo(OPENDAQ_ERR_INVALIDPARAMETER, "Count parameter has to be either 0 or larger than minReadCount.");
    }

    std::scoped_lock lock(mutex);

    if (invalid)
    {
        if (status)
            *status = createReaderStatus().detach();
        *count = 0;
        return OPENDAQ_IGNORED;
    }

    SizeT samplesToRead = (*count / sampleRateDividerLcm) * sampleRateDividerLcm;
    prepareWithDomain((void**) samples, (void**) domain, samplesToRead, milliseconds(timeoutMs));

    auto statusPtr = readPackets();
    if (status)
        *status = statusPtr.detach();

    SizeT samplesRead = samplesToRead - remainingSamplesToRead;
    *count = samplesRead;

    return OPENDAQ_SUCCESS;
}

ErrCode MultiReaderImpl::skipSamples(SizeT* count, IMultiReaderStatus** status)
{
    OPENDAQ_PARAM_NOT_NULL(count);

    std::scoped_lock lock(mutex);

    if (invalid)
    {
        if (status)
            *status = createReaderStatus().detach();
        *count = 0;
        return OPENDAQ_IGNORED;
    }

    if (minReadCount > *count)
        return makeErrorInfo(OPENDAQ_ERR_INVALIDPARAMETER, "Count parameter has to be larger than minReadCount.");

    const SizeT samplesToRead = *count;
    prepare(nullptr, samplesToRead, milliseconds(0));

    auto statusPtr = readPackets();
    if (status)
        *status = statusPtr.detach();

    const SizeT samplesRead = samplesToRead - remainingSamplesToRead;
    *count = samplesRead;

    return OPENDAQ_SUCCESS;
}

SizeT MultiReaderImpl::getMinSamplesAvailable(bool acrossDescriptorChanges) const
{
    SizeT min = std::numeric_limits<SizeT>::max();
    for (const auto& signal : signals)
    {
        auto sigSamples = signal.getAvailable(acrossDescriptorChanges);
        if (sigSamples < min)
        {
            min = sigSamples;
            if (min == 0)
            {
                return min;
            }
        }
    }

    return min;
}

MultiReaderStatusPtr MultiReaderImpl::createReaderStatus(const DictPtr<IString, IEventPacket>& eventPackets, const NumberPtr& offset)
{
    auto mainDescriptor = DataDescriptorChangedEventPacket(mainValueDescriptor, mainDomainDescriptor);
    return MultiReaderStatus(mainDescriptor, eventPackets, !invalid, offset);
}

SyncStatus MultiReaderImpl::getSyncStatus() const
{
    SyncStatus status = SyncStatus::Unsynchronized;
    for (const auto& signal : signals)
    {
        switch (signal.synced)
        {
            case SyncStatus::Unsynchronized:
                return signal.synced;
            case SyncStatus::Synchronizing:
                status = signal.synced;
                break;
            case SyncStatus::Synchronized:
                if (status == SyncStatus::Unsynchronized)
                    status = signal.synced;
                break;
        }
    }
    return status;
}

DictPtr<IString, IEventPacket> MultiReaderImpl::readUntilFirstDataPacket()
{
    auto packets = Dict<IString, EventPacketPtr>();

    for (size_t i = 0; i < signals.size(); i++)
    {
        auto& signal = signals[i];
        auto packet = signal.readUntilNextDataPacket();
        invalid |= signal.invalid;
        if (packet.assigned())
        {
            packets.set(signal.port.getGlobalId(), packet);
        }

        if (i == 0 && packet.assigned() && packet.getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED)
        {
            auto params = packet.getParameters();
            DataDescriptorPtr newValueDescriptor = params[event_packet_param::DATA_DESCRIPTOR];
            DataDescriptorPtr newDomainDescriptor = params[event_packet_param::DOMAIN_DATA_DESCRIPTOR];
            if (newValueDescriptor.assigned())
            {
                mainValueDescriptor = newValueDescriptor;
            }
            if (newDomainDescriptor.assigned())
            {
                mainDomainDescriptor = newDomainDescriptor;
            }
        }
    }
    return packets.detach();
}

ErrCode MultiReaderImpl::synchronize(SizeT& min, SyncStatus& syncStatus)
{
    min = getMinSamplesAvailable();
    syncStatus = getSyncStatus();

    if ((min != 0) && (syncStatus != SyncStatus::Synchronized))
    {
        try
        {
            // set info data packet
            for (auto& signal : signals)
            {
                signal.isFirstPacketEvent();
            }

            if (syncStatus != SyncStatus::Synchronizing)
            {
                setStartInfo();
                readDomainStart();
            }
            sync();

            // Re-check samples available after dropping them to sync
            syncStatus = getSyncStatus();
            if (syncStatus == SyncStatus::Synchronized)
            {
                min = getMinSamplesAvailable();
            }
        }
        catch (const DaqException& e)
        {
            return errorFromException(e, nullptr);
        }
        catch (const std::exception& e)
        {
            return errorFromException(e);
        }
        catch (...)
        {
            return OPENDAQ_ERR_GENERALERROR;
        }
    }
    return OPENDAQ_SUCCESS;
}

bool MultiReaderImpl::hasEventOrGapInQueue()
{
    auto it = signals.begin();
    for (; it != signals.end(); ++it)
    {
        if (it->connection.hasEventPacket() || it->connection.hasGapPacket())
            break;
    }

    return it != signals.end();
}

MultiReaderStatusPtr MultiReaderImpl::readPackets()
{
    std::unique_lock notifyLock(notify.mutex);
    SizeT availableSamples{};
    SyncStatus syncStatus{};
    const bool zeroDataRead = remainingSamplesToRead == 0;

    if (timeout.count() > 0)
    {
        MultiReaderStatusPtr status;
        auto condition = [this, &status, &availableSamples, &syncStatus]
        {
            if (notify.packetReady == false)
            {
                return false;
            }
            notify.packetReady = false;

            if (auto eventPackets = readUntilFirstDataPacket(); eventPackets.getCount() != 0)
            {
                status = createReaderStatus(eventPackets);
                return true;
            }

            ErrCode errCode = synchronize(availableSamples, syncStatus);
            if (OPENDAQ_FAILED(errCode))
            {
                status = createReaderStatus();
                return true;
            }

            if (syncStatus == SyncStatus::Synchronized && availableSamples > 0u)
            {
                if (availableSamples >= remainingSamplesToRead)
                {
                    return true;
                }
                for (auto& signal : signals)
                {
                    if (signal.connection.hasEventPacket())
                    {
                        return true;
                    }
                }
            }

            return false;
        };

#if (OPENDAQ_LOG_LEVEL <= OPENDAQ_LOG_LEVEL_TRACE)
        auto start = std::chrono::steady_clock::now();
#endif

        [[maybe_unused]] bool ok = notify.condition.wait_for(notifyLock, timeout, condition);

#if (OPENDAQ_LOG_LEVEL <= OPENDAQ_LOG_LEVEL_TRACE)
        auto end = std::chrono::steady_clock::now();
        LOG_T("Waited {} ms {} success", std::chrono::duration_cast<Milliseconds>(end - start).count(), ok ? "with" : "without")
#endif

        if (status.assigned() && portConnected)
        {
            updateCommonSampleRateAndDividers();
            portConnected = false;
            return status;
        }
    }

    if (portDisconnected)
    {
        return createReaderStatus();
    }

    if (syncStatus != SyncStatus::Synchronized)
    {
        auto eventPackets = readUntilFirstDataPacket();

        if (portConnected && eventPackets.getCount() != 0)
        {
            updateCommonSampleRateAndDividers();
            portConnected = false;
        }

        ErrCode errCode = synchronize(availableSamples, syncStatus);
        if (OPENDAQ_FAILED(errCode) || eventPackets.getCount() != 0)
        {
            return createReaderStatus(eventPackets);
        }

        if (remainingSamplesToRead == 0)
        {
            if (zeroDataRead && (availableSamples < minReadCount) && hasEventOrGapInQueue())
            {
                // skip remaining samples
                readSamples(availableSamples);
            }

            return createReaderStatus();
        }
    }

    NumberPtr offset = 0;
    if (syncStatus == SyncStatus::Synchronized && availableSamples > 0u)
    {
        auto domainPacket = signals[0].info.dataPacket.getDomainPacket();
        if (domainPacket.assigned() && domainPacket.getOffset().assigned())
        {
            Int delta = signals[0].packetDelta;
            offset = domainPacket.getOffset().getIntValue() + (signals[0].info.prevSampleIndex * delta);
        }

        SizeT toRead = std::min(remainingSamplesToRead, availableSamples);

#if (OPENDAQ_LOG_LEVEL <= OPENDAQ_LOG_LEVEL_TRACE)
        SizeT samplesToRead = remainingSamplesToRead;
        auto start = std::chrono::steady_clock::now();
#endif
        readSamplesAndSetRemainingSamples(toRead);

#if (OPENDAQ_LOG_LEVEL <= OPENDAQ_LOG_LEVEL_TRACE)
        auto end = std::chrono::steady_clock::now();
        LOG_T("Read {} / {} [{} left] for {} ms",
              toRead,
              samplesToRead,
              remainingSamplesToRead,
              std::chrono::duration_cast<Milliseconds>(end - start).count())
#endif
    }

    return createReaderStatus(nullptr, offset);
}

// Listener

ErrCode MultiReaderImpl::acceptsSignal(IInputPort* port, ISignal* signal, Bool* accept)
{
    OPENDAQ_PARAM_NOT_NULL(port);
    OPENDAQ_PARAM_NOT_NULL(signal);
    OPENDAQ_PARAM_NOT_NULL(accept);

    *accept = true;
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReaderImpl::connected(IInputPort* port)
{
    OPENDAQ_PARAM_NOT_NULL(port);

    auto findSigByPort = [port](const SignalReader& signal) { return signal.port == port; };

    std::scoped_lock lock(notify.mutex);
    if (signals.empty())
        return OPENDAQ_SUCCESS;

    auto sigInfo = std::find_if(signals.begin(), signals.end(), findSigByPort);
    if (sigInfo != signals.end())
    {
        sigInfo->connection = sigInfo->port.getConnection();

        // check new signals
        auto portList = List<IInputPortConfig>();
        for (const auto& signalReader : signals)
        {
            if (signalReader.connection.assigned())
                portList.pushBack(signalReader.port);
        }

        isDomainValid(portList);
        portConnected = true;
    }

    portDisconnected = false;
    for (auto& signal : signals)
    {
        if (!signal.connection.assigned())
        {
            portDisconnected = true;
            break;
        }
    }
    if (!portDisconnected)
    {
        for (auto& signal : signals)
        {
            signal.port.setActive(isActive);
        }
        portConnected = true;
    }
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReaderImpl::disconnected(IInputPort* port)
{
    OPENDAQ_PARAM_NOT_NULL(port);
    auto findSigByPort = [port](const SignalReader& signal) { return signal.port == port; };

    std::scoped_lock lock(notify.mutex);
    if (signals.empty())
        return OPENDAQ_SUCCESS;

    auto sigInfo = std::find_if(signals.begin(), signals.end(), findSigByPort);

    if (sigInfo != signals.end())
    {
        sigInfo->connection = nullptr;
        if (portDisconnected == false)
        {
            portDisconnected = true;
            for (auto& signal : signals)
            {
                signal.port.setActive(false);
            }
        }
    }
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReaderImpl::getEmpty(Bool* empty)
{
    OPENDAQ_PARAM_NOT_NULL(empty);
    bool hasDataPacket = true;

    std::scoped_lock lock(mutex);
    for (auto& signal : signals)
    {
        if (signal.isFirstPacketEvent())
        {
            *empty = false;
            return OPENDAQ_SUCCESS;
        }

        hasDataPacket &= (signal.getAvailable(true) != 0);
    }

    *empty = !hasDataPacket;
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReaderImpl::packetReceived(IInputPort* inputPort)
{
    // data are ready
    // if any of signals has event packet
    // or all signals have data packet
    bool hasEventPacket = false;
    bool hasDataPacket = true;

    std::unique_lock lock(notify.mutex);
    if (portDisconnected)
    {
        notify.packetReady = false;
        return OPENDAQ_SUCCESS;
    }

    for (auto& signal : signals)
    {
        if (signal.isFirstPacketEvent())
        {
            hasEventPacket = true;
            break;
        }
        hasDataPacket &= (signal.getAvailable(true) != 0);
    }

    if (hasEventPacket || hasDataPacket)
    {
        notify.packetReady = true;
        ProcedurePtr callback = readCallback;
        lock.unlock();
        notify.condition.notify_one();
        if (callback.assigned())
        {
            return wrapHandler(callback);
        }
    }

    return OPENDAQ_SUCCESS;
}

#pragma region MultiReaderInfo

void MultiReaderImpl::prepare(void** outValues, SizeT count, std::chrono::milliseconds timeoutTime)
{
    remainingSamplesToRead = count;
    values = outValues;

    domainValues = nullptr;

    timeout = std::chrono::duration_cast<Duration>(timeoutTime);
    startTime = std::chrono::steady_clock::now();

    const SizeT alignedCount = (count / sampleRateDividerLcm) * sampleRateDividerLcm;

    const auto signalsNum = signals.size();
    for (SizeT i = 0u; i < signalsNum; ++i)
    {
        const auto outPtr = outValues != nullptr ? outValues[i] : nullptr;
        signals[i].prepare(outPtr, alignedCount);
    }
}

void MultiReaderImpl::prepareWithDomain(void** outValues, void** domain, SizeT count, std::chrono::milliseconds timeoutTime)
{
    remainingSamplesToRead = count;
    values = outValues;

    domainValues = domain;

    timeout = std::chrono::duration_cast<Duration>(timeoutTime);
    startTime = std::chrono::steady_clock::now();

    const SizeT alignedCount = (count / sampleRateDividerLcm) * sampleRateDividerLcm;

    auto signalsNum = signals.size();
    for (SizeT i = 0u; i < signalsNum; ++i)
    {
        signals[i].prepareWithDomain(outValues[i], domain[i], alignedCount);
    }
}

MultiReaderImpl::Duration MultiReaderImpl::durationFromStart() const
{
    return std::chrono::duration_cast<Duration>(Clock::now() - startTime);
}

void MultiReaderImpl::readSamples(SizeT samples)
{
    auto signalsNum = signals.size();
    for (SizeT i = 0u; i < signalsNum; ++i)
    {
        signals[i].info.remainingToRead = samples / signals[i].sampleRateDivider;
        signals[i].readPackets();
    }
}

void MultiReaderImpl::readSamplesAndSetRemainingSamples(SizeT samples)
{
    readSamples(samples);

    remainingSamplesToRead -= samples;
}

void MultiReaderImpl::readDomainStart()
{
    assert(getSyncStatus() != SyncStatus::Synchronized);

    LOG_T("---\n");

    for (auto& signal : signals)
    {
        auto sigStart = signal.readStartDomain();
        if (!commonStart)
        {
            commonStart = std::move(sigStart);
        }
        else if (*commonStart < *sigStart)
        {
            commonStart = std::move(sigStart);
        }
    }

    LOG_T("---");
    LOG_T("DomainStart: {}", *commonStart);

    if (startOnFullUnitOfDomain)
    {
        commonStart->roundUpOnUnitOfDomain();
        LOG_T("Rounded DomainStart: {}", *commonStart);
    }
    else
    {
        const RatioPtr interval = Ratio(sampleRateDividerLcm, commonSampleRate).simplify();
        commonStart->roundUpOnDomainInterval(interval);
        LOG_T("Aligned DomainStart: {}", *commonStart);
    }
}

void MultiReaderImpl::sync()
{
    bool synced = true;
    for (auto& signal : signals)
    {
        synced = signal.sync(*commonStart) && synced;
    }

    LOG_T("Synced: {}", synced);
}

#pragma endregion MultiReaderInfo

ErrCode MultiReaderImpl::getTickResolution(IRatio** resolution)
{
    if (resolution == nullptr)
    {
        return OPENDAQ_ERR_ARGUMENT_NULL;
    }

    *resolution = readResolution.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReaderImpl::getOrigin(IString** origin)
{
    if (origin == nullptr)
    {
        return OPENDAQ_ERR_ARGUMENT_NULL;
    }

    *origin = readOrigin.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReaderImpl::getOffset(void* domainStart)
{
    if (domainStart == nullptr)
    {
        return OPENDAQ_ERR_ARGUMENT_NULL;
    }

    if (commonStart)
    {
        commonStart->getValue(domainStart);
        return OPENDAQ_SUCCESS;
    }

    return OPENDAQ_IGNORED;
}

ErrCode INTERFACE_FUNC MultiReaderImpl::getCommonSampleRate(Int* commonSampleRate)
{
    if (commonSampleRate == nullptr)
    {
        return OPENDAQ_ERR_ARGUMENT_NULL;
    }

    *commonSampleRate = this->commonSampleRate;

    return OPENDAQ_SUCCESS;
}

ErrCode MultiReaderImpl::getIsSynchronized(Bool* isSynchronized)
{
    OPENDAQ_PARAM_NOT_NULL(isSynchronized);

    *isSynchronized = static_cast<bool>(commonStart);

    return OPENDAQ_SUCCESS;
}

ErrCode MultiReaderImpl::setActive(Bool isActive)
{
    std::scoped_lock lock{mutex, notify.mutex};

    bool modified = this->isActive != static_cast<bool>(isActive);
    this->isActive = isActive;

    for (auto& signalReader : signals)
    {
        if (modified)
            signalReader.synced = SyncStatus::Unsynchronized;

        if (signalReader.port.assigned())
            signalReader.port.setActive(this->isActive);

        if (modified && !this->isActive)
            signalReader.skipUntilLastEventPacket();
    }

    return OPENDAQ_SUCCESS;
}

ErrCode MultiReaderImpl::getActive(Bool* isActive)
{
    OPENDAQ_PARAM_NOT_NULL(isActive);

    std::lock_guard lock{mutex};
    *isActive = this->isActive;

    return OPENDAQ_SUCCESS;
}

#pragma region ReaderConfig

ErrCode MultiReaderImpl::getValueTransformFunction(IFunction** transform)
{
    OPENDAQ_PARAM_NOT_NULL(transform);
    std::scoped_lock lock(mutex);

    *transform = signals[0].valueReader->getTransformFunction().addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReaderImpl::getDomainTransformFunction(IFunction** transform)
{
    OPENDAQ_PARAM_NOT_NULL(transform);
    std::scoped_lock lock(mutex);

    *transform = signals[0].domainReader->getTransformFunction().addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReaderImpl::getInputPorts(IList** ports)
{
    OPENDAQ_PARAM_NOT_NULL(ports);

    auto list = List<IInputPortConfig>();
    for (auto& signal : signals)
    {
        list.pushBack(signal.port);
    }

    *ports = list.detach();
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReaderImpl::getReadTimeoutType(ReadTimeoutType* timeoutType)
{
    OPENDAQ_PARAM_NOT_NULL(timeoutType);

    *timeoutType = ReadTimeoutType::All;
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReaderImpl::markAsInvalid()
{
    std::unique_lock lock(mutex);
    invalid = true;

    return OPENDAQ_SUCCESS;
}

#pragma endregion ReaderConfig

OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY,
                             MultiReader,
                             IList*,
                             signals,
                             SampleType,
                             valueReadType,
                             SampleType,
                             domainReadType,
                             ReadMode,
                             mode,
                             ReadTimeoutType,
                             timeoutType)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC_OBJ(
    LIBRARY_FACTORY, MultiReaderImpl, IMultiReader, createMultiReaderEx,
    IList*, signals,
    SampleType, valueReadType,
    SampleType, domainReadType,
    ReadMode, mode,
    ReadTimeoutType, timeoutType,
    Int, requiredCommonSampleRate,
    Bool, startOnFullUnitOfDomain,
    SizeT, minReadCount
)

template <>
struct ObjectCreator<IMultiReader>
{
    static ErrCode Create(IMultiReader** out, IMultiReader* toCopy, SampleType valueReadType, SampleType domainReadType) noexcept
    {
        OPENDAQ_PARAM_NOT_NULL(out);

        if (toCopy == nullptr)
        {
            return makeErrorInfo(OPENDAQ_ERR_ARGUMENT_NULL, "Existing reader must not be null", nullptr);
        }

        ReadMode mode;
        toCopy->getReadMode(&mode);

        auto old = ReaderConfigPtr::Borrow(toCopy);
        auto impl = dynamic_cast<MultiReaderImpl*>(old.getObject());

        return impl != nullptr ? createObject<IMultiReader, MultiReaderImpl>(out, impl, valueReadType, domainReadType)
                               : createObject<IMultiReader, MultiReaderImpl>(out, old, valueReadType, domainReadType, mode);
    }
};

OPENDAQ_DEFINE_CUSTOM_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC_OBJ(LIBRARY_FACTORY,
                                                                      IMultiReader,
                                                                      createMultiReaderFromExisting,
                                                                      IMultiReader*,
                                                                      invalidatedReader,
                                                                      SampleType,
                                                                      valueReadType,
                                                                      SampleType,
                                                                      domainReadType)

extern "C" daq::ErrCode PUBLIC_EXPORT createMultiReaderFromBuilder(IMultiReader** objTmp, IMultiReaderBuilder* builder)
{
    return daq::createObject<IMultiReader, MultiReaderImpl>(objTmp, builder);
}

END_NAMESPACE_OPENDAQ
