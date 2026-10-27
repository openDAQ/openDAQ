#include <coretypes/ctutils.h>
#include <coretypes/validation.h>
#include <opendaq/data_rule_ptr.h>
#include <opendaq/multi_reader2_status_impl.h>
#include <opendaq/multi_reader_data_manager.h>
#include <opendaq/reader_utils.h>
#include <opendaq/sample_type_traits.h>

#include <algorithm>

BEGIN_NAMESPACE_OPENDAQ

namespace
{
    // As MultiReader2Impl::resolveParams tells inputs apart
    bool sameComponent(const ComponentPtr& a, const ComponentPtr& b)
    {
        return a.getGlobalId() == b.getGlobalId();
    }

    template <typename TReadType>
    bool convertsTo(SampleType from)
    {
        switch (from)
        {
            case SampleType::Float32:
                return std::is_convertible_v<SampleTypeToType<SampleType::Float32>::Type, TReadType>;
            case SampleType::Float64:
                return std::is_convertible_v<SampleTypeToType<SampleType::Float64>::Type, TReadType>;
            case SampleType::UInt8:
                return std::is_convertible_v<SampleTypeToType<SampleType::UInt8>::Type, TReadType>;
            case SampleType::Int8:
                return std::is_convertible_v<SampleTypeToType<SampleType::Int8>::Type, TReadType>;
            case SampleType::UInt16:
                return std::is_convertible_v<SampleTypeToType<SampleType::UInt16>::Type, TReadType>;
            case SampleType::Int16:
                return std::is_convertible_v<SampleTypeToType<SampleType::Int16>::Type, TReadType>;
            case SampleType::UInt32:
                return std::is_convertible_v<SampleTypeToType<SampleType::UInt32>::Type, TReadType>;
            case SampleType::Int32:
                return std::is_convertible_v<SampleTypeToType<SampleType::Int32>::Type, TReadType>;
            case SampleType::UInt64:
                return std::is_convertible_v<SampleTypeToType<SampleType::UInt64>::Type, TReadType>;
            case SampleType::Int64:
                return std::is_convertible_v<SampleTypeToType<SampleType::Int64>::Type, TReadType>;
            case SampleType::RangeInt64:
                return std::is_convertible_v<SampleTypeToType<SampleType::RangeInt64>::Type, TReadType>;
            case SampleType::ComplexFloat32:
                return std::is_convertible_v<SampleTypeToType<SampleType::ComplexFloat32>::Type, TReadType>;
            case SampleType::ComplexFloat64:
                return std::is_convertible_v<SampleTypeToType<SampleType::ComplexFloat64>::Type, TReadType>;
            default:
                return false;
        }
    }

    // The rule of the existing readers (TypedReader::handleDescriptorChanged in scaled mode), whose helper is not
    // reachable from the white-box tests. The value's sample type is the one after post scaling
    bool isConvertible(const DataDescriptorPtr& value, SampleType readType)
    {
        const SampleType from = value.getSampleType();
        switch (readType)
        {
            case SampleType::Undefined:
            case SampleType::Struct:
                return true;
            case SampleType::Float32:
                return convertsTo<SampleTypeToType<SampleType::Float32>::Type>(from);
            case SampleType::Float64:
                return convertsTo<SampleTypeToType<SampleType::Float64>::Type>(from);
            case SampleType::UInt8:
                return convertsTo<SampleTypeToType<SampleType::UInt8>::Type>(from);
            case SampleType::Int8:
                return convertsTo<SampleTypeToType<SampleType::Int8>::Type>(from);
            case SampleType::UInt16:
                return convertsTo<SampleTypeToType<SampleType::UInt16>::Type>(from);
            case SampleType::Int16:
                return convertsTo<SampleTypeToType<SampleType::Int16>::Type>(from);
            case SampleType::UInt32:
                return convertsTo<SampleTypeToType<SampleType::UInt32>::Type>(from);
            case SampleType::Int32:
                return convertsTo<SampleTypeToType<SampleType::Int32>::Type>(from);
            case SampleType::UInt64:
                return convertsTo<SampleTypeToType<SampleType::UInt64>::Type>(from);
            case SampleType::Int64:
                return convertsTo<SampleTypeToType<SampleType::Int64>::Type>(from);
            case SampleType::RangeInt64:
                return convertsTo<SampleTypeToType<SampleType::RangeInt64>::Type>(from);
            case SampleType::ComplexFloat32:
                return convertsTo<SampleTypeToType<SampleType::ComplexFloat32>::Type>(from);
            case SampleType::ComplexFloat64:
                return convertsTo<SampleTypeToType<SampleType::ComplexFloat64>::Type>(from);
            default:
                return false;
        }
    }

    bool isIntegerType(SampleType type)
    {
        switch (type)
        {
            case SampleType::Int8:
            case SampleType::UInt8:
            case SampleType::Int16:
            case SampleType::UInt16:
            case SampleType::Int32:
            case SampleType::UInt32:
            case SampleType::Int64:
            case SampleType::UInt64:
                return true;
            default:
                return false;
        }
    }

    SizeT elementCount(const DataDescriptorPtr& descriptor)
    {
        SizeT count = 1;
        const auto dimensions = descriptor.getDimensions();
        if (dimensions.assigned())
        {
            for (const auto& dimension : dimensions)
                count *= dimension.getSize();
        }
        return count;
    }
}

// ---------------------------------------------------------------- configuration

bool MultiReaderDataManager::isSameConfig(const Config& other)
{
    std::scoped_lock lock(mutex);

    // The params only; the descriptors and connection states are what the reader read off the inputs. The judgement
    // compares by identity, so a new function object reconfigures
    if (!configured || config.inputs.size() != other.inputs.size() || config.mainSlot != other.mainSlot ||
        config.acceptsDescriptor.getObject() != other.acceptsDescriptor.getObject() || config.readType != other.readType ||
        config.minReadCount != other.minReadCount || config.errorPolicy != other.errorPolicy)
        return false;

    for (SizeT i = 0; i < config.inputs.size(); ++i)
    {
        if (!sameComponent(config.inputs[i], other.inputs[i]))
            return false;
    }
    return true;
}

ErrCode MultiReaderDataManager::reconfigure(Config newConfig, IMultiReader2Status** status)
{
    const SizeT inputCount = newConfig.inputs.size();
    if (newConfig.connected.size() != inputCount || newConfig.valueDescriptors.size() != inputCount ||
        newConfig.domainDescriptors.size() != inputCount)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "The connection states and descriptors must have one entry per input");

    std::scoped_lock lock(mutex);
    return daqTry([&]
    {
        // Retained inputs keep what they last saw; new ones start from what the reader read off the input
        std::vector<Slot> newSlots(inputCount);
        for (SizeT i = 0; i < inputCount; ++i)
        {
            const auto it = std::find_if(config.inputs.begin(), config.inputs.end(),
                                         [&](const ComponentPtr& input) { return sameComponent(input, newConfig.inputs[i]); });
            if (it != config.inputs.end())
            {
                newSlots[i] = std::move(slots[it - config.inputs.begin()]);
                continue;
            }

            newSlots[i].latestConnected = newConfig.connected[i];
            newSlots[i].latestValue = newConfig.valueDescriptors[i];
            newSlots[i].latestDomain = newConfig.domainDescriptors[i];
        }

        config = std::move(newConfig);
        slots = std::move(newSlots);
        configured = true;

        // As if newly constructed: no SyncFailed and no main input carried over
        for (auto& slot : slots)
        {
            // TODO(ingest): drop the queued data and events; latest* already holds the result of the events
            slot.connected = slot.latestConnected;
            slot.value = slot.latestValue;
            slot.domain = slot.latestDomain;
            slot.syncFailed = SyncCause::None;
        }
        mainSlot.reset();

        for (SizeT i = 0; i < slots.size(); ++i)
            checkInput(i);
        settleMainInput(false);
        restart();

        if (status)
            *status = buildStatus().detach();
    });
}

ErrCode MultiReaderDataManager::rejudge(IMultiReader2Status** status)
{
    OPENDAQ_PARAM_NOT_NULL(status);

    std::scoped_lock lock(mutex);
    return daqTry([&]
    {
        const bool wasValid = level() != ComponentStatus::Error;
        const auto previousMain = mainSlot;
        std::vector<MultiReader2InputError> previousErrors;
        for (const auto& slot : slots)
            previousErrors.push_back(slot.error);

        // Every input that passes the reader's own checks is judged again with its descriptors in force
        for (SizeT i = 0; i < slots.size(); ++i)
            checkInput(i);
        settleMainInput(true);

        if (wasValid != (level() != ComponentStatus::Error) || mainSlot != previousMain)
        {
            // Internal reconfigure: the reader became invalid or valid again, or the automatic main input moved. The
            // verdicts are fresh, so the inputs are not judged a second time
            // TODO(ingest): apply every queued event first, as the internal reconfigure in read does
            restart();
        }
        else
        {
            // As on a descriptor event; the other inputs keep their queued data
            for (SizeT i = 0; i < slots.size(); ++i)
            {
                Slot& slot = slots[i];
                if (slot.error == previousErrors[i])
                    continue;

                statusOwed = true;
                if (slot.error == MultiReader2InputError::None)
                {
                    // Rejoins under Exclude
                    // TODO(sync): rejoin with a synchronization
                    slot.descriptorChanged = true;
                }
                else
                {
                    // Set aside under Exclude
                    // TODO(ingest): drop the input's queued data
                }
            }
        }

        *status = buildStatus().detach();
    });
}

// The error the input has on its own, with its descriptors and connection in force: the reader's checks in the order
// of the error table, then the owner's verdict once they pass. The checks against the main input are in settleMainInput
void MultiReaderDataManager::checkInput(SizeT i)
{
    Slot& slot = slots[i];
    slot.domainInfo = parseDomain(slot.domain);
    slot.rejectedByOwner = false;

    if (!slot.connected)
        slot.ownError = MultiReader2InputError::Disconnected;
    else if (!slot.value.assigned() || !isConvertible(slot.value, config.readType))
        slot.ownError = MultiReader2InputError::ValueDescriptorInvalid;
    else if (!slot.domainInfo.valid)
        slot.ownError = MultiReader2InputError::DomainDescriptorInvalid;
    else if (!askAccept(i))
    {
        slot.ownError = MultiReader2InputError::ValueDescriptorInvalid;
        slot.rejectedByOwner = true;
    }
    else
        slot.ownError = MultiReader2InputError::None;
}

bool MultiReaderDataManager::askAccept(SizeT i)
{
    if (!config.acceptsDescriptor.assigned())
        return true;

    callbackThread = std::this_thread::get_id();
    Finally leave([this] { callbackThread = std::thread::id(); });
    try
    {
        return config.acceptsDescriptor.call(config.inputs[i], slots[i].value, slots[i].domain);
    }
    catch (...)
    {
        // An exception counts as a rejection
        daqClearErrorInfo();
        return false;
    }
}

// The domain constraints that need only the input itself; those against the main input are in matchesMain
MultiReaderDataManager::DomainInfo MultiReaderDataManager::parseDomain(const DataDescriptorPtr& domain)
{
    DomainInfo info;
    if (!domain.assigned() || !isIntegerType(domain.getSampleType()))
        return info;

    const auto dimensions = domain.getDimensions();
    if (dimensions.assigned() && dimensions.getCount() > 0)
        return info;

    const auto unit = domain.getUnit();
    if (!unit.assigned() || unit.getSymbol() != "s")
        return info;

    const auto rule = domain.getRule();
    if (!rule.assigned() || rule.getType() != DataRuleType::Linear)
        return info;
    const auto parameters = rule.getParameters();
    if (!parameters.assigned() || !parameters.hasKey("delta") || !parameters.hasKey("start"))
        return info;
    const BaseObjectPtr delta = parameters.get("delta");
    if (delta.getCoreType() != ctInt || static_cast<std::int64_t>(delta) <= 0)
        return info;
    // An integer start (Rule row): packetOffset + start is the packet's first tick from the origin
    const BaseObjectPtr start = parameters.get("start");
    if (start.getCoreType() != ctInt)
        return info;
    info.start = start;
    info.delta = delta;

    // Sampling rate: an integer number of samples per second
    info.resolution = domain.getTickResolution();
    if (!info.resolution.assigned() || info.resolution.getNumerator() <= 0 || info.resolution.getDenominator() <= 0)
        return info;
    const std::int64_t secondsPerSampleNum = info.resolution.getNumerator() * info.delta;
    if (info.resolution.getDenominator() % secondsPerSampleNum != 0)
        return info;
    info.rate = info.resolution.getDenominator() / secondsPerSampleNum;

    // Origin: ISO 8601 on a full second
    const auto origin = domain.getOrigin();
    if (!origin.assigned() || origin.getLength() == 0)
        return info;
    bool parsed = false;
    const auto epoch = reader::parseEpoch(origin.toStdString(), &parsed);
    if (!parsed || epoch.time_since_epoch() % std::chrono::seconds(1) != std::chrono::system_clock::duration::zero())
        return info;
    info.originS = std::chrono::duration_cast<std::chrono::seconds>(epoch.time_since_epoch()).count();

    info.valid = true;
    return info;
}

// The domain constraints against the main input's descriptor (DomainDescriptorInvalid): Sampling rate, equal rates,
// and Origin, origins a whole number of main ticks apart. Whether the packets' samples fall on the main input's ticks
// is the Grid constraint, found during synchronization (SyncFailed, off grid)
bool MultiReaderDataManager::matchesMain(const DomainInfo& input, const DomainInfo& main)
{
    if (input.rate != main.rate)
        return false;

    // seconds / (num / den seconds per tick)
    // TODO(sync): the difference becomes the input's tick offset
    const std::int64_t originDifference = input.originS - main.originS;
    return originDifference * main.resolution.getDenominator() % main.resolution.getNumerator() == 0;
}

// Chooses the main input from the inputs' own errors, then sets every input's reported error and message entry.
// keepMain: an internal reconfigure keeps a healthy automatic main input (Main input)
void MultiReaderDataManager::settleMainInput(bool keepMain)
{
    const auto previousMain = mainSlot;

    for (auto& slot : slots)
        slot.error = slot.ownError;

    // Pinned, kept while healthy, or the first input without an error
    if (config.mainSlot.has_value())
    {
        mainSlot = config.mainSlot;
    }
    else if (!(keepMain && mainSlot.has_value() && slots[*mainSlot].error == MultiReader2InputError::None))
    {
        mainSlot.reset();
        for (SizeT i = 0; i < slots.size(); ++i)
        {
            if (slots[i].error == MultiReader2InputError::None)
            {
                mainSlot = i;
                break;
            }
        }
    }

    // Every comparison is against the main input; none is possible while it is in error itself
    if (mainSlot.has_value() && slots[*mainSlot].error == MultiReader2InputError::None)
    {
        const DomainInfo& main = slots[*mainSlot].domainInfo;
        for (SizeT i = 0; i < slots.size(); ++i)
        {
            if (i != *mainSlot && slots[i].error == MultiReader2InputError::None && !matchesMain(slots[i].domainInfo, main))
                slots[i].error = MultiReader2InputError::DomainDescriptorInvalid;
        }
    }

    // SyncFailed survives only while the main input stays, and the main input is never SyncFailed
    for (SizeT i = 0; i < slots.size(); ++i)
    {
        Slot& slot = slots[i];
        if (mainSlot != previousMain || mainSlot == i)
            slot.syncFailed = SyncCause::None;
        if (slot.syncFailed != SyncCause::None && slot.error == MultiReader2InputError::None)
            slot.error = MultiReader2InputError::SyncFailed;
        slot.errorText = describeError(slot);
    }

    // Invalid with an automatic main input: none is assigned, the recovery picks afresh
    if (!config.mainSlot.has_value() && level() == ComponentStatus::Error)
        mainSlot.reset();
}

// The end of every reconfigure, internal or not: everything is reported anew and synchronization starts
void MultiReaderDataManager::restart()
{
    for (auto& slot : slots)
    {
        // TODO(ingest): drop the queued data
        slot.descriptorChanged = slot.error == MultiReader2InputError::None;
    }
    domainChanged = true;
    resynchronized = true;
    statusOwed = true;

    // A reader without inputs is valid and has nothing to synchronize
    state = level() == ComponentStatus::Error ? State::Invalid : State::Syncing;
    // TODO(sync): reset commonStart and readTick, set syncLimitTicks to 2 s of the main input's ticks
}

// ---------------------------------------------------------------- status

ComponentStatus MultiReaderDataManager::level() const
{
    const auto inError = static_cast<SizeT>(std::count_if(
        slots.begin(), slots.end(), [](const Slot& slot) { return slot.error != MultiReader2InputError::None; }));
    if (inError == 0)
        return ComponentStatus::Ok;
    if (config.errorPolicy == MultiReader2ErrorPolicy::Invalidate)
        return ComponentStatus::Error;

    const bool pinnedMainInError = config.mainSlot.has_value() && slots[*config.mainSlot].error != MultiReader2InputError::None;
    return pinnedMainInError || inError == slots.size() ? ComponentStatus::Error : ComponentStatus::Warning;
}

// The text of the input's entry in the status message (Status message)
std::string MultiReaderDataManager::describeError(const Slot& slot) const
{
    switch (slot.error)
    {
        case MultiReader2InputError::Disconnected:
            return "not connected";
        case MultiReader2InputError::ValueDescriptorInvalid:
            if (!slot.value.assigned())
                return "no value descriptor";
            if (slot.rejectedByOwner)
                return "value descriptor rejected by the owner";
            return "value type not convertible to " + convertSampleTypeToString(config.readType);
        case MultiReader2InputError::DomainDescriptorInvalid:
            return "domain descriptor invalid";
        case MultiReader2InputError::SyncFailed:
            if (slot.syncFailed == SyncCause::OffGrid)
                return "not synchronized, off the main input's grid";
            return "not synchronized, more than 2 s from the main input";
        default:
            return "";
    }
}

// Elements times the size of the read type, or the input's own sample size under Undefined
SizeT MultiReaderDataManager::sampleSize(const Slot& slot) const
{
    if (config.readType == SampleType::Undefined)
        return slot.value.getSampleSize();
    return getSampleSize(config.readType) * elementCount(slot.value);
}

// Consumes the event fields, which are never set under Error
MultiReader2StatusPtr MultiReaderDataManager::buildStatus()
{
    const ComponentStatus status = level();
    const bool valid = status != ComponentStatus::Error;

    auto inputs = List<IMultiReader2InputStatus>();
    std::string message;
    for (SizeT i = 0; i < slots.size(); ++i)
    {
        Slot& slot = slots[i];
        const bool reported = valid && slot.error == MultiReader2InputError::None;
        inputs.pushBack(createWithImplementation<IMultiReader2InputStatus, MultiReader2InputStatusImpl>(
            config.inputs[i], slot.error, reported && slot.descriptorChanged, reported ? slot.value : nullptr,
            reported ? sampleSize(slot) : 0));
        slot.descriptorChanged = false;

        if (slot.error != MultiReader2InputError::None)
        {
            if (!message.empty())
                message += "; ";
            message += config.inputs[i].getLocalId().toStdString() + ": " + slot.errorText;
        }
    }

    const bool hasMainDomain = valid && mainSlot.has_value();
    auto result = createWithImplementation<IMultiReader2Status, MultiReader2StatusImpl>(
        statusOwed,
        status,
        String(message),
        hasMainDomain && domainChanged,
        hasMainDomain ? slots[*mainSlot].domain : nullptr,
        valid && resynchronized,
        mainSlot.has_value() ? config.inputs[*mainSlot] : nullptr,
        inputs);

    statusOwed = false;
    domainChanged = false;
    resynchronized = false;
    return result;
}

// ---------------------------------------------------------------- consumer side

SizeT MultiReaderDataManager::getAvailableCount()
{
    std::scoped_lock lock(mutex);

    // TODO(read): the samples every healthy input has in front of its first queued event; 0 while a status is owed
    // (C1), while synchronizing, or below MinReadCount. With no queues yet there are none
    return 0;
}

ErrCode MultiReaderDataManager::read(void** data, SizeT* count, SizeT timeoutMs, SizeT* packetOffset, IMultiReader2Status** status)
{
    OPENDAQ_PARAM_NOT_NULL(count);
    OPENDAQ_PARAM_NOT_NULL(status);

    std::scoped_lock lock(mutex);
    return daqTry([&]
    {
        const ErrCode errCode = checkRead(*count, timeoutMs, data, Buffers::Values);
        OPENDAQ_RETURN_IF_FAILED(errCode);

        // TODO(read): copy the samples into data
        readStatus(count, packetOffset, status);
        return OPENDAQ_SUCCESS;
    });
}

ErrCode MultiReaderDataManager::readWithDomain(void** data, SizeT* count, SizeT timeoutMs, IMultiReader2Status** status)
{
    OPENDAQ_PARAM_NOT_NULL(count);
    OPENDAQ_PARAM_NOT_NULL(status);

    std::scoped_lock lock(mutex);
    return daqTry([&]
    {
        const ErrCode errCode = checkRead(*count, timeoutMs, data, Buffers::WithDomain);
        OPENDAQ_RETURN_IF_FAILED(errCode);

        // TODO(read): fill the leading domain buffer with packetOffset plus the main rule's start, then the samples
        readStatus(count, nullptr, status);
        return OPENDAQ_SUCCESS;
    });
}

ErrCode MultiReaderDataManager::skipSamples(SizeT* count, SizeT timeoutMs, SizeT* packetOffset, IMultiReader2Status** status)
{
    OPENDAQ_PARAM_NOT_NULL(count);
    OPENDAQ_PARAM_NOT_NULL(status);

    std::scoped_lock lock(mutex);
    return daqTry([&]
    {
        const ErrCode errCode = checkRead(*count, timeoutMs, nullptr, Buffers::None);
        OPENDAQ_RETURN_IF_FAILED(errCode);

        readStatus(count, packetOffset, status);
        return OPENDAQ_SUCCESS;
    });
}

// count 0 with null data is a status probe; otherwise every contributing input needs a buffer, and readWithDomain the
// leading domain buffer. skipSamples takes none
ErrCode MultiReaderDataManager::checkRead(SizeT count, SizeT timeoutMs, void** data, Buffers buffers) const
{
    if (timeoutMs != 0)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTIMPLEMENTED, "A read never waits; timeoutMs must be 0");
    if (count == 0)
        return OPENDAQ_SUCCESS;
    if (count < config.minReadCount)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "The count is below MinReadCount");

    if (buffers == Buffers::None || level() == ComponentStatus::Error)
        return OPENDAQ_SUCCESS;
    const bool withDomain = buffers == Buffers::WithDomain;
    if (data == nullptr || (withDomain && data[0] == nullptr))
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "The buffers are missing");

    const SizeT first = withDomain ? 1 : 0;
    for (SizeT i = 0; i < slots.size(); ++i)
    {
        if (slots[i].error == MultiReader2InputError::None && data[first + i] == nullptr)
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "A contributing input has no buffer");
    }
    return OPENDAQ_SUCCESS;
}

void MultiReaderDataManager::readStatus(SizeT* count, SizeT* packetOffset, IMultiReader2Status** status)
{
    // TODO(read): drop fewer than MinReadCount samples in front of a queued event, apply the events the read reaches
    // and re-evaluate their inputs (checkInput, settleMainInput), run an internal reconfigure when one is due, and plan
    // the samples. With no queues yet every read returns count 0 with its status
    *count = 0;
    if (packetOffset)
        *packetOffset = 0;   // TODO(read): readTick
    *status = buildStatus().detach();
}

// ---------------------------------------------------------------- producer side

bool MultiReaderDataManager::connected(SizeT /*slot*/, const DataDescriptorPtr& /*value*/, const DataDescriptorPtr& /*domain*/)
{
    return false;
}

bool MultiReaderDataManager::disconnected(SizeT /*slot*/)
{
    return false;
}

bool MultiReaderDataManager::addPacket(SizeT /*slot*/, const PacketPtr& /*packet*/)
{
    return false;
}

// ---------------------------------------------------------------- wake

bool MultiReaderDataManager::hasTrigger() const
{
    return false;
}

bool MultiReaderDataManager::callFinished(bool /*failed*/)
{
    return false;
}

bool MultiReaderDataManager::inCallback() const
{
    return callbackThread.load() == std::this_thread::get_id();
}

END_NAMESPACE_OPENDAQ
