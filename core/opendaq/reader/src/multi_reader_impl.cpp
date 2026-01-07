#include <coreobjects/ownable_ptr.h>
#include <coreobjects/property_object_factory.h>
#include <coretypes/validation.h>
#include <opendaq/custom_log.h>
#include <opendaq/event_packet_utils.h>
#include <opendaq/input_port_factory.h>
#include <opendaq/multi_reader_impl.h>
#include <opendaq/packet_factory.h>
#include <opendaq/reader_errors.h>
#include <opendaq/reader_utils.h>
#include <opendaq/data_descriptor_factory.h>
#include <opendaq/tags_private_ptr.h>
#include <opendaq/input_port_private_ptr.h>

#include <fmt/ostream.h>
#include <set>
#include <chrono>
#include <optional>

using namespace std::chrono;

using Milliseconds = duration<double, std::milli>;

template <>
struct fmt::formatter<daq::Comparable> : ostream_formatter
{
};

BEGIN_NAMESPACE_OPENDAQ

struct MultiReaderImpl::ReferenceDomainBin
{
    StringPtr id;
    TimeProtocol timeProtocol;

    bool operator<(const ReferenceDomainBin& rhs) const
    {
        if (id == rhs.id)
            return timeProtocol < rhs.timeProtocol;
        if (id.assigned() && rhs.id.assigned())
            return id < rhs.id;
        if (rhs.id.assigned())
            return true;
        return false;
    }
};

// Non-builder constructor
MultiReaderImpl::MultiReaderImpl(const ListPtr<IComponent>& list,
                                 SampleType valueReadType,
                                 SampleType domainReadType,
                                 ReadMode mode,
                                 ReadTimeoutType,// Why is this unused?
                                 std::int64_t requiredCommonSampleRate,
                                 Bool startOnFullUnitOfDomain,
                                 SizeT minReadCount)
    : tickOffsetTolerance(nullptr)
    , requiredCommonSampleRate(requiredCommonSampleRate)
    , startOnFullUnitOfDomain(startOnFullUnitOfDomain)
    , minReadCount(minReadCount)
    , notificationMethod(PacketReadyNotification::None)
    , notificationMethodsList(List<PacketReadyNotification>())
{
    this->internalAddRef();
    try
    {
        checkListSizeAndCacheContext(list);
        loggerComponent = context.getLogger().getOrAddComponent("MultiReader");
        notificationMethod = list[0].supportsInterface(ISignal::Id)
                                 ? PacketReadyNotification::SameThread
                                 : PacketReadyNotification::Scheduler;

        auto ports = createOrAdoptPorts(list);
        configureAndStorePorts(ports, valueReadType, domainReadType, mode);

        auto err = isDomainValid(ports);
        if (OPENDAQ_FAILED(err))
        {
            invalid = true;
            LOG_D("Multi reader signal domains are not valid: {}", getErrorInfoMessage(err));
            clearErrorInfo();
        }
    }
    catch (...)
    {
        this->releaseWeakRefOnException();
        throw;
    }
}

// From old
MultiReaderImpl::MultiReaderImpl(MultiReaderImpl* old, SampleType valueReadType, SampleType domainReadType)
    : loggerComponent(old->loggerComponent)
{
    std::scoped_lock lock(old->mutex);
    old->invalid = true;
    portBinder = old->portBinder;
    startOnFullUnitOfDomain = old->startOnFullUnitOfDomain;
    isActive = old->isActive;
    minReadCount = old->minReadCount;
    tickOffsetTolerance = old->tickOffsetTolerance;
    commonSampleRate = old->commonSampleRate;
    requiredCommonSampleRate = old->requiredCommonSampleRate;
    mainValueDescriptor = old->mainValueDescriptor;
    mainDomainDescriptor = old->mainDomainDescriptor;
    allowDifferentRates = old->allowDifferentRates;
    notificationMethod = old->notificationMethod;
    notificationMethodsList = old->notificationMethodsList;
    context = old->context;
    portsConnected = old->portsConnected;
    externalListener = old->externalListener;

    this->internalAddRef();
    try
    {
        auto listener = this->thisPtr<InputPortNotificationsPtr>();
        for (auto& signal : old->signals)
        {
            signals.emplace_back(signal, listener, valueReadType, domainReadType);
        }

        updateCommonSampleRateAndDividers();
        readCallback = std::move(old->readCallback);
    }
    catch (...)
    {
        this->releaseWeakRefOnException();
        throw;
    }
}

// From builder
MultiReaderImpl::MultiReaderImpl(const MultiReaderBuilderPtr& builder)
    : tickOffsetTolerance(builder.getTickOffsetTolerance())
    , requiredCommonSampleRate(builder.getRequiredCommonSampleRate())
    , allowDifferentRates(builder.getAllowDifferentSamplingRates())
    , startOnFullUnitOfDomain(builder.getStartOnFullUnitOfDomain())
    , minReadCount(builder.getMinReadCount())
    , notificationMethod(builder.getInputPortNotificationMethod())
    , notificationMethodsList(builder.getInputPortNotificationMethods())
{
    internalAddRef();
    try
    {

        auto sourceComponents = builder.getSourceComponents();
        checkListSizeAndCacheContext(sourceComponents);

        if (notificationMethodsList.getCount() > 0 && sourceComponents.getCount() != notificationMethodsList.getCount())
            DAQ_THROW_EXCEPTION(InvalidParameterException, "The list of source components is not of same size than the list of notification methods.");

        loggerComponent = context.getLogger().getOrAddComponent("MultiReader");

        auto ports = createOrAdoptPorts(sourceComponents);
        configureAndStorePorts(ports, builder.getValueReadType(), builder.getDomainReadType(), builder.getReadMode());

        auto err = isDomainValid(ports);
        if (OPENDAQ_FAILED(err))
        {
            invalid = true;
            LOG_D("Multi reader signal domains are not valid: {}", getErrorInfoMessage(err));
            clearErrorInfo();
        }
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

void MultiReaderImpl::checkListSizeAndCacheContext(const ListPtr<IComponent>& list)
{
    if (!list.assigned())
        DAQ_THROW_EXCEPTION(NotAssignedException, "List of inputs is not assigned");
    if (list.getCount() == 0)
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Need at least one signal.");
    context = list[0].getContext();
}

ErrCode MultiReaderImpl::checkDomainUnits(const ListPtr<InputPortConfigPtr>& ports)
{
    for (const auto& port : ports)
    {
        const auto signal = port.getSignal();
        if (!signal.assigned())
        {
            continue;
        }

        auto domain = signal.getDomainSignal();
        if (!domain.assigned())
        {
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, fmt::format(R"(Signal "{}" does not have a domain signal set.)", signal.getLocalId()));
        }

        auto domainDescriptor = domain.getDescriptor();
        if (!domainDescriptor.assigned())
        {
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, fmt::format(R"(Signal "{}" does not have a domain descriptor set.)", signal.getLocalId()));
        }

        auto domainUnit = domainDescriptor.getUnit();
        if (!domainUnit.assigned())
        {
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, fmt::format(R"(Signal "{}" does not have a domain unit set.)", signal.getLocalId()));
        }

        const auto domainQuantity = domainUnit.getQuantity();
        const auto domainUnitSymbol = domainUnit.getSymbol();

        if (!domainQuantity.assigned() || domainQuantity.getLength() == 0)
        {
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, fmt::format(R"(Signal "{}" does not have a domain quantity set.)", signal.getLocalId()));
        }

        if (domainQuantity != "time")
        {
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOT_SUPPORTED,
                                       fmt::format(R"(Signal "{}" domain quantity is not "time" but "{}" which is not currently supported.)",
                                       signal.getLocalId(),
                                       domainQuantity));
        }

        if (domainUnitSymbol != "s")
        {
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOT_SUPPORTED,
                                       fmt::format(R"(Signal "{}" domain unit is not "s" but "{}" which is not currently supported.)",
                                       signal.getLocalId(),
                                       domainUnitSymbol));
        }
    }

    return OPENDAQ_SUCCESS;
}

ErrCode MultiReaderImpl::checkReferenceDomainInfo(const ListPtr<InputPortConfigPtr>& ports) const
{
    TimeProtocol TimeProtocol = TimeProtocol::Unknown;
    std::set<ReferenceDomainBin> bins;

    for (const auto& port : ports)
    {
        const auto signal = port.getSignal();
        if (!signal.assigned())
        {
            continue;
        }

        auto domain = signal.getDomainSignal();
        if (!domain.assigned())
        {
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER,
                                       fmt::format(R"(Signal "{}" does not have a domain signal set.)", signal.getLocalId()));
        }

        auto referenceDomainInfo = domain.getDescriptor().getReferenceDomainInfo();

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

            if (referenceDomainInfo.getReferenceTimeProtocol() == TimeProtocol::Unknown)
            {
                // This will perhaps be bumped up to a higher severity later on (warning)
                LOG_D(R"(Domain signal "{}" Reference Time Source is Unknown.)", domain.getLocalId());
            }
            else
            {
                if (TimeProtocol != TimeProtocol::Unknown && referenceDomainInfo.getReferenceTimeProtocol() != TimeProtocol)
                    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDSTATE,
                                               "Only one known Reference Time Source is allowed per Multi Reader.");
                TimeProtocol = referenceDomainInfo.getReferenceTimeProtocol();
            }

            ReferenceDomainBin bin = {referenceDomainInfo.getReferenceDomainId(), referenceDomainInfo.getReferenceTimeProtocol()};
            auto elt = bins.begin();
            while (elt != bins.end())
            {
                // Traverse one group

                bool needsKnownTimeProtocol = false;
                bool hasKnownTimeProtocol = false;
                auto groupDomainId = elt->id;

                while (elt != bins.end() && elt->id == groupDomainId)
                {
                    if (groupDomainId.assigned() && bin.id.assigned() && groupDomainId != bin.id)
                    {
                        // Both are assigned, but not matching
                        // Needs at least one known time source
                        needsKnownTimeProtocol = true;
                    }
                    if (elt->timeProtocol != TimeProtocol::Unknown)
                    {
                        // Group (domain signals with identical domain ID) has at least one known time source
                        hasKnownTimeProtocol = true;
                    }
                    ++elt;
                }

                if (needsKnownTimeProtocol && !hasKnownTimeProtocol)
                {
                    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDSTATE, "Reference domain is incompatible.");
                }
            }

            bins.insert(bin);
        }
    }

    return OPENDAQ_SUCCESS;
}

ErrCode MultiReaderImpl::isDomainValid(const ListPtr<IInputPortConfig>& list) const
{
    OPENDAQ_RETURN_IF_FAILED(checkDomainUnits(list));
    OPENDAQ_RETURN_IF_FAILED(checkReferenceDomainInfo(list));
    return OPENDAQ_SUCCESS;
}

ListPtr<IInputPortConfig> MultiReaderImpl::createOrAdoptPorts(const ListPtr<IComponent>& list) const
{
    if (list[0].supportsInterface(IInputPort::Id) && notificationMethodsList.getCount() > 0)
    {
        if (notificationMethod == PacketReadyNotification::Unspecified)
            DAQ_THROW_EXCEPTION(InvalidParameterException, "Multi reader created from signals cannot have an unspecified input port notification method.");
    }

    bool hasInputPorts = false;
    bool hasSignals = false;

    auto portList = List<IInputPortConfig>();
    size_t cnt = 0;
    for (const auto& el : list)
    {
        if (auto signal = el.asPtrOrNull<ISignal>(); signal.assigned())
        {
            if (hasInputPorts)
                DAQ_THROW_EXCEPTION(InvalidParameterException, "Cannot pass both input ports and signals as items");

            if (notificationMethodsList.getCount() > 0 && notificationMethodsList[cnt] == PacketReadyNotification::Unspecified)
                DAQ_THROW_EXCEPTION(InvalidParameterException, "Multi reader created from signals cannot have an unspecified input port notification method.");

            auto port = InputPort(context, nullptr, fmt::format("multi_reader_signal_{}", signal.getLocalId()));
            port.getTags().asPtr<ITagsPrivate>().add("MultiReaderInternalPort");

            hasSignals = true;
            port.connect(signal);
            portList.pushBack(port);
        }
        else if (auto port = el.asPtrOrNull<IInputPortConfig>(); port.assigned())
        {
            if (hasSignals)
                DAQ_THROW_EXCEPTION(InvalidParameterException, "Cannot pass both input ports and signals as items");

            hasInputPorts = true;
            portList.pushBack(port);
        }
        else
        {
            DAQ_THROW_EXCEPTION(InvalidParameterException, "One of the elements of input list is not signal or input port");
        }

        cnt++;
    }

    return portList;
}

bool MultiReaderImpl::allPortsConnected() const
{
    return std::all_of(signals.cbegin(),
                       signals.cend(),
                       [](const SignalReader& signal)
                       {
                           return signal.port.getConnection().assigned();
                       });
}

void MultiReaderImpl::configureAndStorePorts(const ListPtr<IInputPortConfig>& inputPorts,
                                             SampleType valueRead,
                                             SampleType domainRead,
                                             ReadMode mode)
{
    auto listener = this->thisPtr<InputPortNotificationsPtr>();

    size_t cnt = 0;
    for (const auto& port : inputPorts)
    {
        if (!port.getTags().contains("MultiReaderInternalPort"))
        {
            if (!portBinder.assigned())
                portBinder = PropertyObject();
            port.asPtr<IOwnable>().setOwner(portBinder);
        }

        port.setListener(listener);
        auto portNotificationMethod = notificationMethodsList.getCount() > 0 ? notificationMethodsList[cnt] : notificationMethod;

        if (portNotificationMethod != PacketReadyNotification::Unspecified)
            port.setNotificationMethod(portNotificationMethod);

        signals.emplace_back(port, valueRead, domainRead, mode, loggerComponent);
        cnt++;
    }

    portsConnected = allPortsConnected();
    if (!portsConnected)
    {
        setPortsActiveState(false);
    }
    else
    {
        setPortsActiveState(true);
    }
}

void MultiReaderImpl::updateCommonSampleRateAndDividers()
{
    std::optional<std::int64_t> lastSampleRate = std::nullopt;

    sameSampleRates = true;

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

            if (!lastSampleRate.has_value())
                lastSampleRate = signal.sampleRate;

            if (lastSampleRate.value() != signal.sampleRate)
            {
                sameSampleRates = false;
                if (!allowDifferentRates)
                {
                    LOG_D("Signal sample rates differ. AllowDifferentSamplingRates must be set to True to allow such configurations.")
                    invalid = true;
                    return;
                }

                if (tickOffsetTolerance.assigned())
                {
                    LOG_D("Signal sample rates differ. Currently, tick offset tolerance can only be applied to signals with identical sample rates.");
                    invalid = true;
                    return;
                }
            }
        }
    }

    for (auto& signal : signals)
    {
        signal.setCommonSampleRate(commonSampleRate);
        if (signal.invalid)
        {
            LOG_D("Signal sample rates differ. Signal sample rate does not match required common sample rate.")
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

    readResolution = maxResolution;
    readOrigin = date::format("%FT%TZ", minEpoch);

    LOG_T("MaxResolution: {}", maxResolution)
    LOG_T("MinEpoch: {}", minEpoch)

    for (auto& sigInfo : signals)
    {
        sigInfo.setStartInfo(minEpoch, maxResolution);
    }
}

ErrCode MultiReaderImpl::setOnDataAvailable(IProcedure* callback)
{
    std::scoped_lock lock(mutex);

    readCallback = callback;
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReaderImpl::setExternalListener(IInputPortNotifications* listener)
{
    this->externalListener = listener;
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
    OPENDAQ_RETURN_IF_FAILED(errCode);

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
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "Count parameter has to be either 0 or larger than minReadCount.");
    }

    std::scoped_lock lock(mutex);

    MultiReaderStatusPtr earlyReturnStatus;
    if (nextPacketIsEvent)
    {
        earlyReturnStatus = readPackets();
    }
    else if (invalid)
    {
        earlyReturnStatus = createReaderStatus();
    }

    if (earlyReturnStatus.assigned())
    {
        if (status)
            *status = earlyReturnStatus.detach();

        *count = 0;
        return OPENDAQ_SUCCESS;
    }

    SizeT samplesToRead = (*count / sampleRateDividerLcm) * sampleRateDividerLcm;
    prepare(static_cast<void**>(samples), samplesToRead, milliseconds(timeoutMs));

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
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "Count parameter has to be either 0 or larger than minReadCount.");
    }

    std::scoped_lock lock(mutex);

    MultiReaderStatusPtr earlyReturnStatus;
    if (nextPacketIsEvent)
    {
        earlyReturnStatus = readPackets();
    }

    if (invalid)
    {
        earlyReturnStatus = createReaderStatus();
    }

    if (earlyReturnStatus.assigned())
    {
        if (status)
            *status = earlyReturnStatus.detach();

        *count = 0;
        return OPENDAQ_SUCCESS;
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
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "Count parameter has to be larger than minReadCount.");

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

        if (!signal.info.dataPacket.assigned())
             sigSamples = 0;

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

MultiReaderStatusPtr MultiReaderImpl::createReaderStatus(const DictPtr<IString, IEventPacket>& eventPackets, const NumberPtr& offset) const
{
    auto mainDescriptor = DataDescriptorChangedEventPacket(descriptorToEventPacketParam(mainValueDescriptor),
                                                           descriptorToEventPacketParam(mainDomainDescriptor));
    return MultiReaderStatus(mainDescriptor, eventPackets, !invalid, offset);
}

SyncStatus MultiReaderImpl::getSyncStatus() const
{
    SyncStatus status = SyncStatus::Unsynchronized;
    for (const auto& signal : signals)
    {
        switch (signal.synced)
        {
            case SyncStatus::SynchronizationFailed:
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

DictPtr<IString, IEventPacket> MultiReaderImpl::readUntilFirstDataPacketAndGetEvents()
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
            const auto [valueDescriptorChanged, domainDescriptorChanged, newValueDescriptor, newDomainDescriptor] =
                parseDataDescriptorEventPacket(packet);

            if (valueDescriptorChanged)
                mainValueDescriptor = newValueDescriptor;
            if (domainDescriptorChanged)
                mainDomainDescriptor = newDomainDescriptor;
        }
    }
    return packets.detach();
}

ErrCode MultiReaderImpl::synchronize(SizeT& min, SyncStatus& syncStatus)
{
    // Get the minimum amount of time samples are available for in unit of 1/commonSamplingRate
    min = getMinSamplesAvailable();
    // Get the worst status among signals. If Synchronized is returned, all signals are in this state. For other states,
    // at least one is in that non-Synchronized state.
    syncStatus = getSyncStatus();

    if (min == 0 || syncStatus == SyncStatus::Synchronized)
        return OPENDAQ_SUCCESS;

    const ErrCode errCode = daqTry([&]()
    {
        // set info data packet
        for (auto& signal : signals)
            // Dequeue first data packet if available
            signal.isFirstPacketEvent();

        if (syncStatus != SyncStatus::Synchronizing)
        {
            setStartInfo();
            readDomainStart();
        }

        sync();

        syncStatus = getSyncStatus();
        if (syncStatus == SyncStatus::Synchronized)
            min = getMinSamplesAvailable();
        if (syncStatus == SyncStatus::SynchronizationFailed)
            setActiveInternal(false);
    });
    OPENDAQ_RETURN_IF_FAILED(errCode, "Failed to synchronize MultiReaderImpl");
    return errCode;
}

bool MultiReaderImpl::eventOrGapInQueue() const
{
    return std::any_of(signals.cbegin(),
                   signals.cend(),
                   [](const SignalReader& signal)
                   {
                       return signal.connection.hasEventPacket() || signal.connection.hasGapPacket();
                   });
}

bool MultiReaderImpl::dataPacketsOrEventReady()
{
    bool hasEventPacket = false;
    bool hasDataPacket = true;

    for (auto& signal : signals)
    {
        if (signal.isFirstPacketEvent())
        {
            return true;
        }

        hasDataPacket &= (signal.getAvailable(true) != 0);
    }

    return hasEventPacket || hasDataPacket;
}

NumberPtr MultiReaderImpl::calculateOffset() const
{
    auto domainPacket = signals[0].info.dataPacket.getDomainPacket();
    if (domainPacket.assigned() && domainPacket.getOffset().assigned())
    {
        Int delta = signals[0].packetDelta;
        return domainPacket.getOffset().getIntValue() + (signals[0].info.prevSampleIndex * delta);
    }

    return 0;
}

MultiReaderStatusPtr MultiReaderImpl::readAndSynchronize(bool zeroDataRead, SizeT& availableSamples, SyncStatus& syncStatus)
{
    auto eventPackets = readUntilFirstDataPacketAndGetEvents();
    if (eventPackets.getCount() != 0)
    {
        updateCommonSampleRateAndDividers();
    }

    ErrCode errCode = synchronize(availableSamples, syncStatus);
    if (OPENDAQ_FAILED(errCode) || eventPackets.getCount() != 0)
    {
        return createReaderStatus(eventPackets);
    }

    if (syncStatus == SyncStatus::SynchronizationFailed)
    {
        return createReaderStatus();
    }

    if (zeroDataRead)
    {
        if (availableSamples < minReadCount && eventOrGapInQueue())
        {
            // skip remaining samples
            readSamples(availableSamples);
        }

        return createReaderStatus();
    }

    return nullptr;
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
        auto condition = [this, zeroDataRead, &status, &availableSamples, &syncStatus]
        {
            if (!portsConnected || !dataPacketsOrEventReady())
            {
                return false;
            }

            status = readAndSynchronize(zeroDataRead, availableSamples, syncStatus);
            if (status.assigned())
            {
                return true;
            }

            if (syncStatus == SyncStatus::Synchronized && availableSamples >= remainingSamplesToRead)
            {
                return true;
            }

            return false;
        };

        notify.condition.wait_for(notifyLock, timeout, condition);

        if (status.assigned())
        {
            return status;
        }
    }

    if (!portsConnected)
    {
        return createReaderStatus();
    }

    if (syncStatus != SyncStatus::Synchronized)
    {
        auto status = readAndSynchronize(zeroDataRead, availableSamples, syncStatus);
        if (status.assigned())
            return status;
    }

    NumberPtr offset = 0;
    if (syncStatus == SyncStatus::Synchronized && availableSamples > 0u)
    {
        offset = calculateOffset();
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

    if (externalListener.assigned() && externalListener.getRef().assigned())
        return externalListener.getRef()->acceptsSignal(port, signal, accept);

    *accept = true;
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReaderImpl::connected(IInputPort* port)
{
    OPENDAQ_PARAM_NOT_NULL(port);

    auto findSigByPort = [port](const SignalReader& signal) { return signal.port == port; };

    {
        std::scoped_lock lock(notify.mutex);
        if (signals.empty())
            return OPENDAQ_SUCCESS;

        auto sigInfo = std::find_if(signals.begin(), signals.end(), findSigByPort);
        if (sigInfo != signals.end())
        {
            sigInfo->connection = sigInfo->port.getConnection();

            // check new signal
            auto portList = List<IInputPortConfig>();
            portList.pushBack(port);

            if (OPENDAQ_FAILED(isDomainValid(portList)))
            {
                invalid = true;
            }
        }

        portsConnected = allPortsConnected();
        if (portsConnected)
        {
            for (auto& signal : signals)
            {
                signal.port.setActive(isActive);
            }

        }
    }

    if (externalListener.assigned() && externalListener.getRef().assigned())
        return externalListener.getRef()->connected(port);
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReaderImpl::disconnected(IInputPort* port)
{
    OPENDAQ_PARAM_NOT_NULL(port);
    auto findSigByPort = [port](const SignalReader& signal) { return signal.port == port; };

    {
        std::scoped_lock lock(notify.mutex);
        if (signals.empty())
            return OPENDAQ_SUCCESS;

        auto sigInfo = std::find_if(signals.begin(), signals.end(), findSigByPort);

        if (sigInfo != signals.end())
        {
            sigInfo->connection = nullptr;
            if (portsConnected)
            {
                portsConnected = false;
                setPortsActiveState(false);
            }
        }

    }

    if (externalListener.assigned() && externalListener.getRef().assigned())
        return externalListener.getRef()->disconnected(port);
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
    std::scoped_lock lockPacketReceived(packetReceivedMutex);
    std::unique_lock lock(notify.mutex);

    if (!portsConnected)
    {
        for (auto& signal : signals)
        {
            if (signal.port == inputPort)
            {
                signal.skipUntilLastEventPacket();
            }
        }

        return OPENDAQ_SUCCESS;
    }

    if (invalid)
    {
        for (auto& signal : signals)
        {
            if (signal.port == inputPort)
            {
                nextPacketIsEvent = signal.skipUntilLastEventPacket();
                break;
            }
        }
    }

    if ((invalid && nextPacketIsEvent) || (!invalid && dataPacketsOrEventReady()))
    {
        ProcedurePtr callback = readCallback;
        lock.unlock();
        notify.condition.notify_one();

        if (callback.assigned())
            OPENDAQ_RETURN_IF_FAILED(wrapHandler(callback));
    }

    if (externalListener.assigned() && externalListener.getRef().assigned())
        return externalListener.getRef()->packetReceived(inputPort);
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
        // Get timestamps for the first available sample
        // Timestamps are transformed to number of max resolution ticks
        // since the most ancient epoch (descriptor origin)
        // Comparable class does the transformation and it enables
        // seamles comparison (between two integers, at that point)
        // It is problematic that readStartDomain evaluates entire packets
        // worth of linear data rule to get the first timestamp (just packet offset).
        // => Conversion could be more explicit
        // => Comparable is more important for the conversion and validation that types match, comparison of integers is trivial in any
        // case.
        // => SignalReader should be able to handle two domain settings - one native to the signal it is reading and
        // another, "common", that will be set from Multireader parent. Ideally, getting these common domain information,
        // it should be trivial to compare starts.
        auto sigStart = signal.readStartDomain();
        if (!commonStart || *commonStart < *sigStart)
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
    system_clock::rep earliestTime = std::numeric_limits<system_clock::rep>::max();
    system_clock::rep latestTime = 0;

    for (auto& signal : signals)
    {
        system_clock::rep firstSampleAbsoluteTime;
        synced = signal.sync(*commonStart, &firstSampleAbsoluteTime) && synced;

        if (synced)
        {
            if (earliestTime > firstSampleAbsoluteTime)
                earliestTime = firstSampleAbsoluteTime;
            if (latestTime < firstSampleAbsoluteTime)
                latestTime = firstSampleAbsoluteTime;
        }
    }

    if (synced && tickOffsetTolerance.assigned())
    {
        auto tickOffsetToleranceSysTicks =
            (system_clock::period::den * tickOffsetTolerance.getNumerator()) /
            (system_clock::period::num * tickOffsetTolerance.getDenominator());

        auto diff = latestTime - earliestTime;

        if (diff > tickOffsetToleranceSysTicks)
        {
            for (auto& signal: signals)
                signal.synced = SyncStatus::SynchronizationFailed;

            synced = false;
        }
    }

    LOG_T("Synced: {}", synced);
}

#pragma endregion MultiReaderInfo

ErrCode MultiReaderImpl::getTickResolution(IRatio** resolution)
{
    OPENDAQ_PARAM_NOT_NULL(resolution);

    *resolution = readResolution.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReaderImpl::getOrigin(IString** origin)
{
    OPENDAQ_PARAM_NOT_NULL(origin);

    *origin = readOrigin.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReaderImpl::getOffset(void* domainStart)
{
    OPENDAQ_PARAM_NOT_NULL(domainStart);

    if (commonStart)
    {
        commonStart->getValue(domainStart);
        return OPENDAQ_SUCCESS;
    }

    return OPENDAQ_IGNORED;
}

ErrCode INTERFACE_FUNC MultiReaderImpl::getCommonSampleRate(Int* commonSampleRate)
{
    OPENDAQ_PARAM_NOT_NULL(commonSampleRate);

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

    setActiveInternal(isActive);

    return OPENDAQ_SUCCESS;
}

ErrCode MultiReaderImpl::getActive(Bool* isActive)
{
    OPENDAQ_PARAM_NOT_NULL(isActive);

    std::lock_guard lock{mutex};
    *isActive = this->isActive;

    return OPENDAQ_SUCCESS;
}

void MultiReaderImpl::internalDispose(bool)
{
    this->portBinder = nullptr;
    this->signals.clear();
    this->externalListener = nullptr;
    this->readCallback = nullptr;
    this->invalid = true;
    this->isActive = false;
    this->portsConnected = false;
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

ErrCode MultiReaderImpl::getIsValid(Bool* isValid)
{
    OPENDAQ_PARAM_NOT_NULL(isValid);

    std::unique_lock lock(mutex);
    *isValid = !invalid;
    return OPENDAQ_SUCCESS;
}

void MultiReaderImpl::setActiveInternal(Bool isActive)
{
    setPortsActiveState(isActive);
    this->isActive = isActive;
}

void MultiReaderImpl::setPortsActiveState(Bool active)
{
    bool modified = this->isActive != static_cast<bool>(active);
    for (auto& signalReader : signals)
    {
        if (modified)
            signalReader.synced = SyncStatus::Unsynchronized;

        if (signalReader.port.assigned())
            signalReader.port.setActive(active);

        if (modified && !active)
            signalReader.skipUntilLastEventPacket();
    }
}

#pragma endregion ReaderConfig

OPENDAQ_DEFINE_CLASS_FACTORY(
    LIBRARY_FACTORY, MultiReader,
    IList*, signals,
    SampleType, valueReadType,
    SampleType, domainReadType,
    ReadMode, mode,
    ReadTimeoutType, timeoutType
)

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
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ARGUMENT_NULL, "Existing reader must not be null");
        }

        auto old = ReaderConfigPtr::Borrow(toCopy);
        auto impl = dynamic_cast<MultiReaderImpl*>(old.getObject());

        if (impl == nullptr)
        {
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "MultiReader from existing can only be used with the base multi reader implementation");
        }

        return createObject<IMultiReader, MultiReaderImpl>(out, impl, valueReadType, domainReadType);
    }
};

OPENDAQ_DEFINE_CUSTOM_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC_OBJ(
    LIBRARY_FACTORY, IMultiReader, createMultiReaderFromExisting,
    IMultiReader*, invalidatedReader,
    SampleType, valueReadType,
    SampleType, domainReadType
)

extern "C" daq::ErrCode PUBLIC_EXPORT createMultiReaderFromBuilder(IMultiReader** objTmp, IMultiReaderBuilder* builder)
{
    return daq::createObject<IMultiReader, MultiReaderImpl>(objTmp, builder);
}

END_NAMESPACE_OPENDAQ
