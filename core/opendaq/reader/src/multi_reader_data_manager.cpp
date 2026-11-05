#include <coretypes/ctutils.h>
#include <coretypes/validation.h>
#include <opendaq/custom_log.h>
#include <opendaq/data_rule_ptr.h>
#include <opendaq/event_packet_ids.h>
#include <opendaq/event_packet_ptr.h>
#include <opendaq/event_packet_utils.h>
#include <opendaq/multi_reader2_status_impl.h>
#include <opendaq/multi_reader_data_manager.h>
#include <opendaq/reader_utils.h>
#include <opendaq/sample_type_traits.h>

#include <algorithm>
#include <cstring>
#include <limits>
#include <numeric>

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

    // Checked int64 arithmetic for the ticks; false on overflow
    constexpr std::int64_t int64Max = std::numeric_limits<std::int64_t>::max();
    constexpr std::int64_t int64Min = std::numeric_limits<std::int64_t>::min();

    bool mulChecked(std::int64_t a, std::int64_t b, std::int64_t& result)
    {
        if (a > 0 ? (b > 0 ? a > int64Max / b : b < int64Min / a) : (b > 0 ? a < int64Min / b : a != 0 && b < int64Max / a))
            return false;
        result = a * b;
        return true;
    }

    bool addChecked(std::int64_t a, std::int64_t b, std::int64_t& result)
    {
        if ((b > 0 && a > int64Max - b) || (b < 0 && a < int64Min - b))
            return false;
        result = a + b;
        return true;
    }

    bool subChecked(std::int64_t a, std::int64_t b, std::int64_t& result)
    {
        if ((b < 0 && a > int64Max + b) || (b > 0 && a < int64Min + b))
            return false;
        result = a - b;
        return true;
    }

    // a - b > limit; a difference beyond int64 exceeds any limit
    bool exceeds(std::int64_t a, std::int64_t b, std::int64_t limit)
    {
        std::int64_t difference;
        if (!subChecked(a, b, difference))
            return a > b;
        return difference > limit;
    }
}

// ---------------------------------------------------------------- configuration

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
        steadyStatus = nullptr;

        // As if newly constructed: no SyncFailed and no main input carried over
        for (auto& slot : slots)
        {
            // Applying the queued events leaves latest*; their data goes with them
            clearQueue(slot);
            slot.connected = slot.latestConnected;
            slot.value = slot.latestValue;
            slot.domain = slot.latestDomain;
            slot.syncFailed = SyncCause::None;
            slot.converter.reset();   // the read type may have changed
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

ErrCode MultiReaderDataManager::reevaluate(IMultiReader2Status** status)
{
    OPENDAQ_PARAM_NOT_NULL(status);

    std::scoped_lock lock(mutex);
    return daqTry([&]
    {
        const ComponentStatus previousLevel = level();
        const bool wasValid = previousLevel != ComponentStatus::Error;
        const auto previousMain = mainSlot;
        const auto previous = reportedErrors();
        steadyStatus = nullptr;

        // Every input that passes the reader's own checks is judged again with its descriptors in force
        for (SizeT i = 0; i < slots.size(); ++i)
            checkInput(i);
        settleMainInput(true);

        if (wasValid != (level() != ComponentStatus::Error) || mainSlot != previousMain)
        {
            // The reader became invalid or valid again, or the automatic main input moved. Only the inputs whose
            // queued events change their descriptors are judged a second time
            internalReconfigure(wasValid);
        }
        else
        {
            // As on a descriptor event; the other inputs keep their queued data, unless an input rejoins with none
            std::vector<SizeT> rejoined;
            for (SizeT i = 0; i < slots.size(); ++i)
            {
                Slot& slot = slots[i];
                if (slot.error == previous[i].first)
                    continue;

                if (slot.error == MultiReader2InputError::None)
                {
                    // Rejoins under Exclude
                    slot.descriptorChanged = true;
                    rejoined.push_back(i);
                }
                else
                {
                    // Set aside under Exclude; data behind a queued event waits for the read that applies it
                    dropFront(slot);
                }
            }
            if (!rejoined.empty())
            {
                dropForRejoin(rejoined);
                startSync();
            }
            synchronize();
        }
        noteChanges(previousLevel, previous);

        *status = buildStatus().detach();
    });
}

// The error the input has on its own, with its descriptors and connection in force, in the order of the error table:
// the owner's verdict once the value descriptor passes the reader's checks, then the domain. The checks against the
// main input are in settleMainInput
void MultiReaderDataManager::checkInput(SizeT i)
{
    Slot& slot = slots[i];
    slot.domainInfo = parseDomain(slot.domain);
    slot.rejectedByOwner = false;

    if (!slot.connected)
        slot.ownError = MultiReader2InputError::Disconnected;
    else if (!slot.value.assigned() || !isConvertible(slot.value, config.readType))
        slot.ownError = MultiReader2InputError::ValueDescriptorInvalid;
    else if (!askAccept(i))
    {
        slot.ownError = MultiReader2InputError::ValueDescriptorInvalid;
        slot.rejectedByOwner = true;
    }
    else if (!slot.domainInfo.valid)
        slot.ownError = MultiReader2InputError::DomainDescriptorInvalid;
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
    catch (const std::exception& e)
    {
        // An exception counts as a rejection
        daqClearErrorInfo();
        const auto& loggerComponent = config.loggerComponent;
        if (loggerComponent.assigned())
            LOG_W("acceptsDescriptor threw for {}: {}", config.inputs[i].getLocalId().toStdString(), e.what());
        return false;
    }
    catch (...)
    {
        daqClearErrorInfo();
        const auto& loggerComponent = config.loggerComponent;
        if (loggerComponent.assigned())
            LOG_W("acceptsDescriptor threw for {}", config.inputs[i].getLocalId().toStdString());
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

    // Origin: ISO 8601 on a full second; a year, or a year and month, alone starts on the first day
    const auto origin = domain.getOrigin();
    if (!origin.assigned() || origin.getLength() == 0)
        return info;
    std::string text = origin.toStdString();
    if (text.size() == 4)
        text += "-01-01";
    else if (text.size() == 7)
        text += "-01";
    bool parsed = false;
    const auto epoch = reader::parseEpoch(text, &parsed);
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

    // seconds / (num / den seconds per tick); mapToMain turns the difference into the input's tick offset. A difference
    // that does not fit in Int64 main ticks matches nothing
    std::int64_t originDifference, scaled;
    if (!subChecked(input.originS, main.originS, originDifference) ||
        !mulChecked(originDifference, main.resolution.getDenominator(), scaled))
        return false;
    return scaled % main.resolution.getNumerator() == 0;
}

// Chooses the main input from the inputs' own errors, then sets every input's reported error and message entry.
// keepMain: an internal reconfigure keeps a healthy automatic main input (Main input)
void MultiReaderDataManager::settleMainInput(bool keepMain)
{
    const auto previousMain = mainSlot;

    for (auto& slot : slots)
        slot.error = slot.ownError;

    // Pinned, kept while healthy, or the first input without an error. A move prefers an input that was synchronized
    // with the main input it replaces, so the inputs read together stay the same as far as they can; SyncFailed is
    // judged against that main input, so an input that failed it is taken only when no other input is healthy
    if (config.mainSlot.has_value())
    {
        mainSlot = config.mainSlot;
    }
    else if (!(keepMain && mainSlot.has_value() && slots[*mainSlot].error == MultiReader2InputError::None))
    {
        const auto firstHealthy = [this](bool synchronized) -> std::optional<SizeT>
        {
            for (SizeT i = 0; i < slots.size(); ++i)
            {
                if (slots[i].error == MultiReader2InputError::None && (!synchronized || slots[i].syncFailed == SyncCause::None))
                    return i;
            }
            return std::nullopt;
        };
        mainSlot = firstHealthy(true);
        if (!mainSlot.has_value())
            mainSlot = firstHealthy(false);
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
}

// The end of every reconfigure, internal or not: everything is reported anew and synchronization starts
void MultiReaderDataManager::restart()
{
    for (auto& slot : slots)
    {
        // A new run on every input; the main input may have changed, and with it every main tick
        dropData(slot);
        slot.nextOffset.reset();
        slot.newestTick.reset();
        slot.latestInfo = parseDomain(slot.latestDomain);
        if (slot.syncFailed == SyncCause::None)
            slot.clearPending = false;
        slot.descriptorChanged = slot.error == MultiReader2InputError::None;
    }
    // Without a main input there is no stream and no packetOffset to continue
    domainChanged = true;
    resynchronized = mainSlot.has_value();
    statusOwed = true;

    mainDelta = 1;
    syncLimitTicks = 0;
    if (mainSlot.has_value() && slots[*mainSlot].domainInfo.valid)
    {
        const DomainInfo& main = slots[*mainSlot].domainInfo;
        mainDelta = main.delta;
        syncLimitTicks = 2 * main.rate * main.delta;   // rate * delta ticks per second
    }
    for (SizeT i = 0; i < slots.size(); ++i)
        mapToMain(i);

    // A reader without inputs is valid and has nothing to synchronize
    state = level() == ComponentStatus::Error ? State::Invalid : State::Syncing;
    readTick = 0;
    positioned = false;
    mainTrimmedFrom.reset();
}

// A synchronization trigger while valid, such as a rejoin under Exclude
void MultiReaderDataManager::startSync()
{
    if (state == State::Invalid)
        return;
    state = State::Syncing;
    mainTrimmedFrom.reset();
}

// The configure with the params in force, from a read (R5): every queued event goes onto its input and the data is
// dropped; the inputs whose descriptors or connection changed are judged again, and a healthy main input is kept, with
// the SyncFailed of the others
void MultiReaderDataManager::internalReconfigure(bool wasValid)
{
    for (SizeT i = 0; i < slots.size(); ++i)
    {
        Slot& slot = slots[i];
        bool judge = false;
        for (const auto& entry : slot.queue)
        {
            if (entry.event.has_value())
                judge |= applyEvent(slot, *entry.event).judge;
        }
        clearQueue(slot);
        if (judge)
            checkInput(i);
    }
    // A reader that was invalid picks its automatic main input afresh, as a configure would: the one kept while it was
    // invalid (Main input) depends on the order the inputs recovered in. Should the fresh choice leave the reader
    // invalid, the kept one stays, with the SyncFailed judged against it, which a move of the main input clears
    if (!wasValid && !config.mainSlot.has_value())
    {
        const auto kept = mainSlot;
        std::vector<SyncCause> keptSyncFailed;
        for (const auto& slot : slots)
            keptSyncFailed.push_back(slot.syncFailed);

        settleMainInput(false);
        if (level() == ComponentStatus::Error)
        {
            mainSlot = kept;
            for (SizeT i = 0; i < slots.size(); ++i)
                slots[i].syncFailed = keptSyncFailed[i];
            settleMainInput(true);
        }
    }
    else
    {
        settleMainInput(true);
    }

    // Unlike a configure, the caller decides from what the status will show whether one is owed
    const bool owed = statusOwed;
    restart();
    statusOwed = owed;
}

// A status is owed when it will show something the previous one did not; nothing of a restart shows under Error
void MultiReaderDataManager::noteChanges(ComponentStatus previousLevel, const std::vector<std::pair<MultiReader2InputError, std::string>>& previous)
{
    const bool valid = level() != ComponentStatus::Error;
    bool changes = level() != previousLevel || (valid && (domainChanged || resynchronized));
    for (SizeT i = 0; i < slots.size(); ++i)
    {
        const Slot& slot = slots[i];
        changes = changes || slot.error != previous[i].first || slot.errorText != previous[i].second ||
                  (valid && slot.error == MultiReader2InputError::None && slot.descriptorChanged);
    }
    if (changes)
        statusOwed = true;
}

std::vector<std::pair<MultiReader2InputError, std::string>> MultiReaderDataManager::reportedErrors() const
{
    std::vector<std::pair<MultiReader2InputError, std::string>> errors;
    for (const auto& slot : slots)
        errors.emplace_back(slot.error, slot.errorText);
    return errors;
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

// Consumes the event fields, which are never set under Error. Every change a status reports sets statusOwed (noteChanges),
// so a status without changes equals the last one and is returned again; the flags it would have consumed are not
// reported under it either
MultiReader2StatusPtr MultiReaderDataManager::buildStatus()
{
    if (!statusOwed && steadyStatus.assigned())
    {
        for (auto& slot : slots)
            slot.descriptorChanged = false;
        domainChanged = false;
        resynchronized = false;
        return steadyStatus;
    }

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

    // An invalid reader keeps a healthy automatic main input internally, but reports none; a pinned one always shows
    const bool hasMainDomain = valid && mainSlot.has_value();
    const bool reportMain = mainSlot.has_value() && (valid || config.mainSlot.has_value());
    auto result = createWithImplementation<IMultiReader2Status, MultiReader2StatusImpl>(
        statusOwed,
        status,
        String(message),
        hasMainDomain && domainChanged,
        hasMainDomain ? slots[*mainSlot].domain : nullptr,
        valid && resynchronized,
        reportMain ? config.inputs[*mainSlot] : nullptr,
        inputs);

    steadyStatus = statusOwed ? nullptr : result;
    statusOwed = false;
    domainChanged = false;
    resynchronized = false;
    return result;
}

// ---------------------------------------------------------------- consumer side

SizeT MultiReaderDataManager::getAvailableCount()
{
    std::scoped_lock lock(mutex);

    // 0 while a status is owed (C1)
    return statusOwed ? 0 : frontAvailable();
}

// The samples every healthy input has in front of its first queued event; 0 while synchronizing or below MinReadCount.
// While streaming, every contributing input's front starts at readTick
SizeT MultiReaderDataManager::frontAvailable() const
{
    if (state != State::Streaming)
        return 0;

    std::optional<SizeT> count;
    for (const auto& slot : slots)
    {
        if (slot.error == MultiReader2InputError::None)
            count = std::min(count.value_or(slot.frontCount), slot.frontCount);
    }
    return count.has_value() && *count >= config.minReadCount ? *count : 0;
}

ErrCode MultiReaderDataManager::read(void** data, std::int64_t* domain, SizeT* count, SizeT timeoutMs, SizeT* packetOffset, IMultiReader2Status** status)
{
    OPENDAQ_PARAM_NOT_NULL(count);
    OPENDAQ_PARAM_NOT_NULL(status);

    // The read takes the samples off the queues under the mutex and converts them after it, so a large read does not
    // hold up ingest, and a conversion that fails leaves every input at the same position. Reused per thread, so a
    // streaming read allocates nothing
    thread_local std::vector<Copy> copies;
    struct ClearCopies
    {
        std::vector<Copy>& copies;
        ~ClearCopies() { copies.clear(); }
    } clearCopies{copies};

    {
        std::scoped_lock lock(mutex);
        const ErrCode errCode = daqTry([&]
        {
            const ErrCode checkErr = checkRead(*count, timeoutMs, data, false);
            OPENDAQ_RETURN_IF_FAILED(checkErr);

            performRead(data, domain, count, packetOffset, status, &copies);
            return OPENDAQ_SUCCESS;
        });
        OPENDAQ_RETURN_IF_FAILED(errCode);
    }

    const ErrCode errCode = daqTry([&]
    {
        for (const auto& copy : copies)
            runCopy(copy);
    });
    if (OPENDAQ_FAILED(errCode))
    {
        releaseRefIfNotNull(*status);
        *status = nullptr;
    }
    return errCode;
}

ErrCode MultiReaderDataManager::skipSamples(SizeT* count, SizeT timeoutMs, SizeT* packetOffset, IMultiReader2Status** status)
{
    OPENDAQ_PARAM_NOT_NULL(count);
    OPENDAQ_PARAM_NOT_NULL(status);

    std::scoped_lock lock(mutex);
    return daqTry([&]
    {
        const ErrCode errCode = checkRead(*count, timeoutMs, nullptr, true);
        OPENDAQ_RETURN_IF_FAILED(errCode);

        performRead(nullptr, nullptr, count, packetOffset, status, nullptr);
        return OPENDAQ_SUCCESS;
    });
}

// count 0 with null data is a status probe; otherwise every contributing input needs a buffer, while the domain buffer
// may be null. skipSamples takes none
ErrCode MultiReaderDataManager::checkRead(SizeT count, SizeT timeoutMs, void** data, bool skip) const
{
    if (timeoutMs != 0)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTIMPLEMENTED, "A read never waits; timeoutMs must be 0");
    if (count == 0)
        return OPENDAQ_SUCCESS;
    if (count < config.minReadCount)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "The count is below MinReadCount");

    // Buffers are needed only for contributing inputs; a reader without any, such as one without inputs, reads count 0
    const bool anyContributes = std::any_of(
        slots.begin(), slots.end(), [](const Slot& slot) { return slot.error == MultiReader2InputError::None; });
    if (skip || level() == ComponentStatus::Error || !anyContributes)
        return OPENDAQ_SUCCESS;
    if (data == nullptr)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "The buffers are missing");

    for (SizeT i = 0; i < slots.size(); ++i)
    {
        if (slots[i].error == MultiReader2InputError::None && data[i] == nullptr)
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "A contributing input has no buffer");
    }
    return OPENDAQ_SUCCESS;
}

// The read (R1): the samples every contributing input has in front of its first event, unless a status is owed, then
// the events the read reached and the status. skipSamples copies nothing; a domain buffer gets the main ticks. The
// samples' copies go to copies, which the caller runs out of the mutex
void MultiReaderDataManager::performRead(void** data, std::int64_t* domain, SizeT* count, SizeT* packetOffset, IMultiReader2Status** status,
                                         std::vector<Copy>* copies)
{
    dropShortRun();

    const SizeT n = statusOwed ? 0 : std::min(*count, frontAvailable());
    if (packetOffset)
        *packetOffset = n > 0 ? static_cast<SizeT>(readTick - slots[*mainSlot].domainInfo.start) : 0;   // R9
    if (n > 0)
    {
        if (domain)
        {
            for (SizeT k = 0; k < n; ++k)
                domain[k] = readTick + static_cast<std::int64_t>(k) * mainDelta;
        }
        for (SizeT i = 0; i < slots.size(); ++i)
        {
            if (slots[i].error == MultiReader2InputError::None)
                consume(slots[i], n, data ? data[i] : nullptr, copies);
        }
        readTick += static_cast<std::int64_t>(n) * mainDelta;
        positioned = true;
    }
    *count = n;

    applyEvents();
    *status = buildStatus().detach();
}

// Fewer than MinReadCount samples in front of an event can never be read (R8): their count, when the contributing
// inputs' shortest front run is such a run. A run without an event behind it can still grow and is kept
std::optional<SizeT> MultiReaderDataManager::shortRun() const
{
    if (state != State::Streaming)
        return std::nullopt;

    std::optional<SizeT> least;
    bool closed = false;
    for (const auto& slot : slots)
    {
        if (slot.error != MultiReader2InputError::None)
            continue;
        if (!least.has_value() || slot.frontCount < *least)
        {
            least = slot.frontCount;
            closed = slot.events > 0;
        }
        else if (slot.frontCount == *least)
            closed = closed || slot.events > 0;
    }
    if (!least.has_value() || *least == 0 || *least >= config.minReadCount || !closed)
        return std::nullopt;
    return least;
}

// The read drops a short run on every contributing input, so it reaches the event behind it
void MultiReaderDataManager::dropShortRun()
{
    const auto count = shortRun();
    if (!count.has_value())
        return;

    for (auto& slot : slots)
    {
        if (slot.error == MultiReader2InputError::None)
            consume(slot, *count, nullptr, nullptr);
    }
    readTick += static_cast<std::int64_t>(*count) * mainDelta;
    resynchronized = true;
    statusOwed = true;
}

// Whether the input's samples count in a read: it has no error and the reader is valid
bool MultiReaderDataManager::contributes(const Slot& slot, bool valid)
{
    return valid && slot.error == MultiReader2InputError::None;
}

// Whether a read applies an event on the input (R1): a contributing input the one in front of it, which the read
// reached; any other input every queued one
bool MultiReaderDataManager::reachesEvent(const Slot& slot, bool contributing)
{
    if (contributing)
        return !slot.queue.empty() && slot.queue.front().event.has_value();
    return slot.events > 0;
}

// Takes count samples off the front run, which holds at least that many, queueing their copy into out unless it is null
void MultiReaderDataManager::consume(Slot& slot, SizeT count, void* out, std::vector<Copy>* copies)
{
    while (count > 0)
    {
        Entry& entry = slot.queue.front();
        const SizeT taken = std::min(count, entry.count);
        if (out && copies)
            queueCopy(slot, entry, taken, out, *copies);

        entry.start += taken;
        entry.count -= taken;
        if (entry.firstTick.has_value())
            *entry.firstTick += static_cast<std::int64_t>(taken) * mainDelta;
        slot.frontCount -= taken;
        slot.queuedSamples -= taken;
        count -= taken;
        if (entry.count == 0)
            popData(slot);
    }
}

// The copy of the entry's next count samples, scaled, in the read type (R7); out moves past them. Under Undefined the
// input's own samples are copied as they are
void MultiReaderDataManager::queueCopy(Slot& slot, const Entry& entry, SizeT count, void*& out, std::vector<Copy>& copies)
{
    Copy copy{entry.packet, entry.start, count, out};
    if (config.readType == SampleType::Undefined)
    {
        copy.sampleSize = slot.value.getSampleSize();
    }
    else
    {
        // The converter is made again only for another descriptor object; comparing descriptors by value would cost
        // an equals per entry. TODO: TypedReadingUtils::readData of openDAQ/openDAQ#1215 can replace the reader here
        // (todo file)
        if (!slot.converter || slot.converterFor.getObject() != slot.value.getObject())
        {
            slot.converter = createReaderForType(config.readType, nullptr);
            DataDescriptorPtr descriptor = slot.value;
            slot.converter->handleDescriptorChanged(descriptor, ReadMode::Scaled);
            slot.converterFor = slot.value;
        }
        copy.converter = slot.converter;
    }
    out = static_cast<std::uint8_t*>(out) + count * sampleSize(slot);
    copies.push_back(std::move(copy));
}

// Runs outside the mutex: the packet and the converter are the copy's own
void MultiReaderDataManager::runCopy(const Copy& copy)
{
    auto* input = static_cast<std::uint8_t*>(copy.packet.getData());
    if (!copy.converter)
    {
        std::memcpy(copy.out, input + copy.start * copy.sampleSize, copy.count * copy.sampleSize);
        return;
    }

    void* out = copy.out;
    checkErrorInfo(copy.converter->readData(input, copy.start, &out, copy.count));
}

// One event onto its input's state in force
MultiReaderDataManager::Applied MultiReaderDataManager::applyEvent(Slot& slot, const Event& event)
{
    Applied applied;
    switch (event.kind)
    {
        case Event::Kind::Descriptor:
        case Event::Kind::Connected:
            applied.judge = true;
            applied.domain = event.domain != slot.domain;
            if (event.value != slot.value)
                slot.descriptorChanged = true;
            slot.value = event.value;
            slot.domain = event.domain;
            if (event.kind == Event::Kind::Connected)
                slot.connected = true;

            // A new domain descriptor or a connect clears an off-grid input (SyncFailed)
            if (slot.syncFailed == SyncCause::OffGrid && (applied.domain || event.kind == Event::Kind::Connected))
                slot.syncFailed = SyncCause::None;
            break;
        case Event::Kind::Disconnected:
            applied.judge = true;
            slot.connected = false;
            break;
        case Event::Kind::Gap:
            // A gap clears an off-grid input too; on any other failed input it changes nothing
            applied.gap = true;
            if (slot.syncFailed == SyncCause::OffGrid)
                slot.syncFailed = SyncCause::None;
            break;
        case Event::Kind::SyncFailed:
            slot.syncFailed = event.cause;
            break;
    }
    return applied;
}

// The samples in front of the first event, after events were taken off the front. Ingest and consume keep frontCount
// up to date otherwise, so only a read that applied an event on the input counts again (C3)
void MultiReaderDataManager::countFront(Slot& slot)
{
    slot.frontCount = 0;
    for (const auto& entry : slot.queue)
    {
        if (entry.event.has_value())
            break;
        slot.frontCount += entry.count;
    }
}

// What a read changes after its samples (R1, R4): the events it reached, the clears it owes, the re-evaluation, and
// the internal reconfigure, set-aside, rejoin or resynchronization that follows from it
void MultiReaderDataManager::applyEvents()
{
    const ComponentStatus previousLevel = level();
    const bool wasValid = previousLevel != ComponentStatus::Error;

    // A read that reaches no event and owes no clear changes no error, so nothing below would: the streaming read
    // returns here without the snapshot of the errors
    const bool nothingToApply = std::none_of(slots.begin(), slots.end(), [wasValid](const Slot& slot)
                                             { return slot.clearPending || reachesEvent(slot, contributes(slot, wasValid)); });
    if (nothingToApply)
        return;

    const auto previousMain = mainSlot;
    const auto previous = reportedErrors();

    bool reconfigure = false;
    bool resync = false;
    for (SizeT i = 0; i < slots.size(); ++i)
    {
        Slot& slot = slots[i];

        // A contributing input applies the events in front of it, which the read reached. Any other input delivers
        // nothing: it applies all of them and keeps only the data behind the last
        const bool contributing = contributes(slot, wasValid);
        bool judge = false;
        bool appliedAny = false;
        while (reachesEvent(slot, contributing))
        {
            while (!slot.queue.front().event.has_value())
                popData(slot);

            const Applied applied = applyEvent(slot, *slot.queue.front().event);
            slot.queue.pop_front();
            --slot.events;
            appliedAny = true;
            judge |= applied.judge;
            if (applied.domain && mainSlot == i)
                reconfigure = true;
            else if (contributing && (applied.domain || applied.gap))
                resync = true;
        }
        if (appliedAny)
            countFront(slot);
        if (judge)
            checkInput(i);

        // A late input marked within the limit clears on this read; the mark goes in any case, as it triggers a call
        if (slot.clearPending)
        {
            if (slot.syncFailed == SyncCause::Limit)
                slot.syncFailed = SyncCause::None;
            slot.clearPending = false;
        }
    }
    settleMainInput(true);

    const bool valid = level() != ComponentStatus::Error;
    if (reconfigure || valid != wasValid || mainSlot != previousMain)
    {
        internalReconfigure(wasValid);
        noteChanges(previousLevel, previous);
        return;
    }

    std::vector<SizeT> rejoined;
    for (SizeT i = 0; i < slots.size(); ++i)
    {
        Slot& slot = slots[i];
        if (slot.error == MultiReader2InputError::None)
        {
            // Rejoins under Exclude, with a synchronization
            if (previous[i].first != MultiReader2InputError::None)
            {
                slot.descriptorChanged = true;
                rejoined.push_back(i);
                resync = true;
            }
        }
        else
        {
            // Set aside, or still in error: no data in front of its events
            dropFront(slot);
        }
    }
    if (!valid)
    {
        for (auto& slot : slots)
            dropFront(slot);
    }

    if (!rejoined.empty())
        dropForRejoin(rejoined);

    // Also after an applied event that held synchronization back while it led a queue
    if (resync)
        startSync();
    synchronize();
    noteChanges(previousLevel, previous);
}

// ---------------------------------------------------------------- producer side

bool MultiReaderDataManager::connected(SizeT slot, const DataDescriptorPtr& value, const DataDescriptorPtr& domain)
{
    std::scoped_lock lock(mutex);
    return claimWake(queueConnect(slot, value, domain));
}

bool MultiReaderDataManager::disconnected(SizeT slot)
{
    std::scoped_lock lock(mutex);
    return claimWake(queueDisconnect(slot));
}

bool MultiReaderDataManager::addPacket(SizeT slot, const PacketPtr& packet)
{
    std::scoped_lock lock(mutex);
    return claimWake(queuePacket(slot, packet));
}

// A batch of one port's packets under one lock, with one wake claim for all of them
bool MultiReaderDataManager::addPackets(SizeT slot, const PacketPtr* packets, SizeT count)
{
    std::scoped_lock lock(mutex);
    bool owed = false;
    for (SizeT i = 0; i < count; ++i)
        owed = queuePacket(slot, packets[i]) || owed;
    return claimWake(owed);
}

bool MultiReaderDataManager::queueConnect(SizeT i, const DataDescriptorPtr& value, const DataDescriptorPtr& domain)
{
    Slot& slot = slots[i];

    // A connect the reader already has, such as one configure read before it took the port over
    if (slot.latestConnected)
        return queueDescriptor(i, value, domain);

    slot.latestConnected = true;
    if (!slot.queue.empty() && slot.queue.back().event.has_value() && slot.queue.back().event->kind == Event::Kind::Disconnected)
    {
        // A disconnect and reconnect with nothing between them cancel out (Error conditions). The run continues, so
        // a jump to the new signal's data is a gap
        slot.queue.pop_back();
        --slot.events;
        return queueDescriptor(i, value, domain);
    }

    // Possibly another signal: a new run, which continues nothing
    slot.latestValue = value;
    slot.latestDomain = domain;
    slot.latestInfo = parseDomain(domain);
    mapToMain(i);
    slot.nextOffset.reset();
    pushEvent(slot, {Event::Kind::Connected, value, domain});
    return true;
}

bool MultiReaderDataManager::queueDisconnect(SizeT i)
{
    Slot& slot = slots[i];
    if (!slot.latestConnected)
        return false;

    slot.latestConnected = false;
    pushEvent(slot, {Event::Kind::Disconnected});
    return true;
}

bool MultiReaderDataManager::queuePacket(SizeT i, const PacketPtr& packet)
{
    switch (packet.getType())
    {
        case PacketType::Event:
        {
            // The gap event packet is ignored with the others: the continuity check finds the same jump
            const auto eventPacket = packet.asPtr<IEventPacket>(true);
            if (eventPacket.getEventId() != event_packet_id::DATA_DESCRIPTOR_CHANGED)
                return false;
            const auto [valueNew, domainNew, value, domain] = parseDataDescriptorEventPacket(eventPacket);
            const Slot& slot = slots[i];
            return queueDescriptor(i, valueNew ? value : slot.latestValue, domainNew ? domain : slot.latestDomain);
        }
        case PacketType::Data:
            return ingestData(i, packet.asPtr<IDataPacket>(true));
        default:
            return false;
    }
}

// Queues a descriptor event unless both descriptors equal the ones last seen
bool MultiReaderDataManager::queueDescriptor(SizeT i, const DataDescriptorPtr& value, const DataDescriptorPtr& domain)
{
    Slot& slot = slots[i];
    if (value == slot.latestValue && domain == slot.latestDomain)
        return false;

    if (domain != slot.latestDomain)
    {
        // A new domain starts a new run
        slot.latestInfo = parseDomain(domain);
        mapToMain(i);
        slot.nextOffset.reset();
    }
    slot.latestValue = value;
    slot.latestDomain = domain;
    pushEvent(slot, {Event::Kind::Descriptor, value, domain});
    return true;
}

bool MultiReaderDataManager::ingestData(SizeT i, const DataPacketPtr& packet)
{
    Slot& slot = slots[i];
    const SizeT count = packet.getSampleCount();
    if (count == 0)
        return false;

    std::optional<std::int64_t> offset;
    const auto domainPacket = packet.getDomainPacket();
    if (domainPacket.assigned() && domainPacket.getOffset().assigned())
        offset = domainPacket.getOffset().getIntValue();

    // Continuity in the input's own ticks, so it holds without a main input
    const bool gap = offset.has_value() && slot.nextOffset.has_value() && *offset != *slot.nextOffset;
    std::int64_t span, next;
    if (offset.has_value() && slot.latestInfo.delta > 0 && mulChecked(static_cast<std::int64_t>(count), slot.latestInfo.delta, span) &&
        addChecked(*offset, span, next))
        slot.nextOffset = next;
    else
        slot.nextOffset.reset();

    const auto firstTick = offset.has_value() ? toMainTick(slot, *offset) : std::nullopt;
    std::int64_t lastSpan, last;
    if (firstTick.has_value() && mulChecked(static_cast<std::int64_t>(count - 1), mainDelta, lastSpan) &&
        addChecked(*firstTick, lastSpan, last))
        slot.newestTick = last;
    else
        slot.newestTick.reset();

    // While the reader is invalid or the input is in error, data with no event ahead of it is dropped
    bool keep = slot.events > 0 || (state != State::Invalid && slot.error == MultiReader2InputError::None);

    // A gap ends the run. On an off-grid input it clears the failure, so it is queued even while the data is dropped,
    // and the data behind it waits for the read that applies it
    bool trigger = false;
    if (gap && (keep || slot.syncFailed == SyncCause::OffGrid))
    {
        pushEvent(slot, {Event::Kind::Gap});
        keep = true;
        trigger = true;
    }

    if (keep)
    {
        const SizeT bytes = packet.getRawDataSize() + (domainPacket.assigned() ? domainPacket.getRawDataSize() : 0);
        trigger |= limitQueue(i, count, bytes);
        if (slot.events == 0)
            slot.frontCount += count;
        Entry entry;
        entry.packet = packet;
        entry.count = count;
        entry.firstTick = firstTick;
        entry.bytes = bytes;
        slot.queue.push_back(std::move(entry));
        slot.queuedSamples += count;
        slot.queuedBytes += bytes;
    }

    trigger |= markClear(i, firstTick);
    trigger |= synchronize();
    return trigger || frontAvailable() > 0;
}

// The map from the input's latest domain to the main input's domain in force; invalid without a main input, or when
// the origin difference is no whole number of main ticks
void MultiReaderDataManager::mapToMain(SizeT i)
{
    Slot& slot = slots[i];
    slot.toMain = TickMap();
    if (!mainSlot.has_value())
        return;

    const DomainInfo& input = slot.latestInfo;
    const DomainInfo& main = slots[*mainSlot].domainInfo;
    if (!input.valid || !main.valid)
        return;

    // (num_i / den_i) / (num_m / den_m), reduced
    std::int64_t p, q, difference;
    if (!mulChecked(input.resolution.getNumerator(), main.resolution.getDenominator(), p) ||
        !mulChecked(input.resolution.getDenominator(), main.resolution.getNumerator(), q))
        return;
    const std::int64_t divisor = std::gcd(p, q);

    // seconds / (num_m / den_m seconds per tick)
    if (!mulChecked(input.originS - main.originS, main.resolution.getDenominator(), difference) ||
        difference % main.resolution.getNumerator() != 0)
        return;

    slot.toMain = {true, p / divisor, q / divisor, difference / main.resolution.getNumerator()};
}

// A packet's first tick in main ticks; unset when it is no whole main tick or out of range, which counts as off grid
std::optional<std::int64_t> MultiReaderDataManager::toMainTick(const Slot& slot, std::int64_t packetOffset)
{
    const TickMap& map = slot.toMain;
    std::int64_t tick, scaled, result;
    if (!map.valid || !addChecked(packetOffset, slot.latestInfo.start, tick) || tick % map.q != 0 ||
        !mulChecked(tick / map.q, map.p, scaled) || !addChecked(scaled, map.offset, result))
        return std::nullopt;
    return result;
}

void MultiReaderDataManager::pushEvent(Slot& slot, Event event)
{
    Entry entry;
    entry.event = std::move(event);
    slot.queue.push_back(std::move(entry));
    ++slot.events;
}

// The data in front of the first event
void MultiReaderDataManager::dropFront(Slot& slot)
{
    while (!slot.queue.empty() && !slot.queue.front().event.has_value())
        popData(slot);
    slot.frontCount = 0;
}

// All queued data; the events stay
void MultiReaderDataManager::dropData(Slot& slot)
{
    slot.queue.erase(std::remove_if(slot.queue.begin(), slot.queue.end(), [](const Entry& entry) { return !entry.event.has_value(); }),
                     slot.queue.end());
    slot.frontCount = 0;
    slot.queuedSamples = 0;
    slot.queuedBytes = 0;
}

// The data entry in front of the queue, with its samples and bytes
void MultiReaderDataManager::popData(Slot& slot)
{
    const Entry& entry = slot.queue.front();
    slot.queuedSamples -= entry.count;
    slot.queuedBytes -= entry.bytes;
    slot.queue.pop_front();
}

// Every queued entry, data and events
void MultiReaderDataManager::clearQueue(Slot& slot)
{
    slot.queue.clear();
    slot.frontCount = 0;
    slot.events = 0;
    slot.queuedSamples = 0;
    slot.queuedBytes = 0;
}

// Whether the input's queue would hold more than queueLimitSeconds of its data, or more than queueLimitBytes, with a packet
// of count samples and bytes more. The time counts the queued samples at the input's own rate, so a gap among them adds none
bool MultiReaderDataManager::overLimit(const Slot& slot, SizeT count, SizeT bytes) const
{
    if (slot.queuedSamples == 0)
        return false;
    if (slot.queuedBytes + bytes > queueLimitBytes)
        return true;
    const std::int64_t rate = slot.latestInfo.valid ? slot.latestInfo.rate : 0;
    return rate > 0 && slot.queuedSamples + count > static_cast<SizeT>(queueLimitSeconds * rate);
}

// Keeps the input's queue within the limits before a packet of count samples and bytes is queued. While synchronizing,
// nothing queued is aligned yet: the oldest packets in front of the first event go, as synchronization would trim them.
// Otherwise every queued sample goes and a gap takes its place, so the reads go on from the newest data and resynchronize.
// Returns whether a gap was queued
bool MultiReaderDataManager::limitQueue(SizeT i, SizeT count, SizeT bytes)
{
    Slot& slot = slots[i];
    if (!overLimit(slot, count, bytes))
        return false;

    while (state == State::Syncing && overLimit(slot, count, bytes) && !slot.queue.empty() && !slot.queue.front().event.has_value())
    {
        // The behind check measures from where the main input's data began, as if the dropped data were still held
        if (mainSlot == i && !mainTrimmedFrom.has_value())
            mainTrimmedFrom = slot.queue.front().firstTick;
        slot.frontCount -= slot.queue.front().count;
        popData(slot);
    }
    if (!overLimit(slot, count, bytes))
        return false;

    if (config.loggerComponent.assigned())
    {
        const auto& loggerComponent = config.loggerComponent;
        LOG_W("{}: more than {} s or {} MB queued, the queued samples are dropped", config.inputs[i].getLocalId().toStdString(),
              queueLimitSeconds, queueLimitBytes / 1000000);
    }
    dropData(slot);
    countFront(slot);
    pushEvent(slot, {Event::Kind::Gap});
    return true;
}

bool MultiReaderDataManager::hasData(const Slot& slot)
{
    return std::any_of(slot.queue.begin(), slot.queue.end(), [](const Entry& entry) { return !entry.event.has_value(); });
}

// An input that rejoins with no data queued has nothing that could align with what the others queued: what it sent while
// set aside was dropped, and what it sends next is newer. Their data goes too, events kept, so the synchronization judges
// only what arrives after the rejoin, and an input is not behind for a backlog its consumer left unread
void MultiReaderDataManager::dropForRejoin(const std::vector<SizeT>& rejoined)
{
    if (std::all_of(rejoined.begin(), rejoined.end(), [this](SizeT i) { return hasData(slots[i]); }))
        return;
    for (auto& slot : slots)
        dropData(slot);
}

// The front run's samples before the tick. The run has one sample every mainDelta ticks: a gap would have ended it
void MultiReaderDataManager::discardBefore(Slot& slot, std::int64_t tick) const
{
    while (!slot.queue.empty() && !slot.queue.front().event.has_value())
    {
        Entry& entry = slot.queue.front();
        std::int64_t distance;
        if (!entry.firstTick.has_value() || *entry.firstTick >= tick)
            return;

        if (subChecked(tick, *entry.firstTick, distance))
        {
            const auto behind = static_cast<SizeT>((distance + mainDelta - 1) / mainDelta);
            if (behind < entry.count)
            {
                entry.start += behind;
                entry.count -= behind;
                *entry.firstTick += static_cast<std::int64_t>(behind) * mainDelta;
                slot.frontCount -= behind;
                slot.queuedSamples -= behind;
                return;
            }
        }
        slot.frontCount -= entry.count;
        popData(slot);
    }
}

// Failed on the sync limit: applied, or found and queued in front of the input's data for the next read
bool MultiReaderDataManager::failedOnLimit(const Slot& slot)
{
    if (slot.syncFailed == SyncCause::Limit)
        return true;
    if (slot.queue.empty() || !slot.queue.front().event.has_value())
        return false;
    const Event& front = *slot.queue.front().event;
    return front.kind == Event::Kind::SyncFailed && front.cause == SyncCause::Limit;
}

// Drops the input's data and queues the failure in front of it, where the next read applies it
void MultiReaderDataManager::failSync(Slot& slot, SyncCause cause)
{
    dropFront(slot);

    Entry entry;
    entry.event = Event{Event::Kind::SyncFailed, nullptr, nullptr, cause};
    slot.queue.push_front(std::move(entry));
    ++slot.events;
    slot.frontCount = 0;
}

// The check mechanism (Synchronization) over the inputs' front runs, on every ingest and after a read applies events,
// while synchronizing. Returns whether it reached the common start or failed an input
bool MultiReaderDataManager::synchronize()
{
    if (state != State::Syncing || !mainSlot.has_value())
        return false;

    // While the main input's front is an event, nobody is judged
    Slot& main = slots[*mainSlot];
    if (main.queue.empty() || main.queue.front().event.has_value() || !main.queue.front().firstTick.has_value())
        return false;
    const auto runEnd = [this](const Slot& slot)
    {
        return *slot.queue.front().firstTick + static_cast<std::int64_t>(slot.frontCount - 1) * mainDelta;
    };
    const std::int64_t mainFirst = *main.queue.front().firstTick;
    const std::int64_t mainLatest = runEnd(main);

    // Inputs with data in front of their first event. A failed input's marker is such an event, so it takes no
    // further part
    bool reported = false;
    std::vector<SizeT> candidates;
    for (SizeT i = 0; i < slots.size(); ++i)
    {
        Slot& slot = slots[i];
        if (i == *mainSlot || slot.error != MultiReader2InputError::None || slot.queue.empty() || slot.queue.front().event.has_value())
            continue;

        // Grid first (step 1), as an off-grid first tick cannot be the common start; then ahead (step 2)
        const auto& first = slot.queue.front().firstTick;
        std::int64_t distance;
        if (!first.has_value() || !subChecked(*first, mainFirst, distance) || distance % mainDelta != 0)
            failSync(slot, SyncCause::OffGrid);
        else if (exceeds(*first, mainLatest, syncLimitTicks))
            failSync(slot, SyncCause::Limit);
        else
        {
            candidates.push_back(i);
            continue;
        }
        reported = true;
    }

    // The common start (step 3)
    std::int64_t common = mainFirst;
    for (const SizeT i : candidates)
        common = std::max(common, *slots[i].queue.front().firstTick);

    // Discard in front of the common start (step 5); an input whose data is all behind it is left empty
    discardBefore(main, common);
    for (const SizeT i : candidates)
        discardBefore(slots[i], common);

    // Behind (step 4): still no data at the common start once the main input holds more than the limit from it,
    // however far behind the input was. Data the queue limit dropped from the main input counts as held
    const std::int64_t behindFrom = mainTrimmedFrom.has_value() ? std::min(common, *mainTrimmedFrom) : common;
    if (exceeds(mainLatest, behindFrom, syncLimitTicks))
    {
        for (SizeT i = 0; i < slots.size(); ++i)
        {
            Slot& slot = slots[i];
            if (i == *mainSlot || slot.error != MultiReader2InputError::None || !slot.queue.empty())
                continue;
            failSync(slot, SyncCause::Limit);
            reported = true;
        }
    }

    // Streaming begins once every contributing input is at the common start or has failed
    for (const auto& slot : slots)
    {
        if (slot.error != MultiReader2InputError::None)
            continue;
        if (slot.queue.empty())
            return reported;
        const Entry& front = slot.queue.front();
        if (front.event.has_value() ? front.event->kind != Event::Kind::SyncFailed : front.firstTick != common)
            return reported;
    }
    // A start other than where the last read that delivered samples ended is a jump in packetOffset (R6); without such a
    // read there is nothing to continue. The restart reports its own
    if (positioned && common != readTick)
    {
        resynchronized = true;
        statusOwed = true;
    }
    state = State::Streaming;
    readTick = common;
    return true;
}

// An input failed on the sync limit whose packet, from its first tick to its newest, meets the main input's latest sample +/- the limit.
// Marked once, until a read clears it
bool MultiReaderDataManager::markClear(SizeT i, const std::optional<std::int64_t>& firstTick)
{
    Slot& slot = slots[i];
    if (!failedOnLimit(slot) || slot.clearPending || !firstTick.has_value() || !slot.newestTick.has_value() ||
        !mainSlot.has_value() || !slots[*mainSlot].newestTick.has_value())
        return false;

    const std::int64_t mainLatest = *slots[*mainSlot].newestTick;
    if (exceeds(mainLatest, *slot.newestTick, syncLimitTicks) || exceeds(*firstTick, mainLatest, syncLimitTicks))
        return false;

    slot.clearPending = true;
    return true;
}

// ---------------------------------------------------------------- wake

bool MultiReaderDataManager::hasTrigger()
{
    std::scoped_lock lock(mutex);
    return triggerHolds();
}

void MultiReaderDataManager::callStarted()
{
    std::scoped_lock lock(mutex);
    arrivedDuringCall = false;
}

bool MultiReaderDataManager::callFinished(bool failed)
{
    std::scoped_lock lock(mutex);
    callPending = false;

    // Calls announce new arrivals only: the handler reads everything available, and what it leaves is not announced
    // again. So another call follows only for an arrival during this one that is still unread
    const bool again = arrivedDuringCall && triggerHolds();

    // After a failed call the marks that wake once per cause are re-armed (B4)
    if (failed)
    {
        for (auto& slot : slots)
            slot.clearPending = false;
    }

    if (again)
        callPending = true;
    return again;
}

void MultiReaderDataManager::callAborted()
{
    std::scoped_lock lock(mutex);
    callPending = false;
}

// An arrival with something new for the user (W1): a call is claimed only when none is pending or running
bool MultiReaderDataManager::claimWake(bool owed)
{
    if (!owed)
        return false;
    arrivedDuringCall = true;
    if (callPending)
        return false;
    callPending = true;
    return true;
}

// The next read would change something (W3): samples, a status with changes, an input that clears, or an event it
// applies. The event part asks the read's own rules, so an arrival the next read does not reach calls no one
bool MultiReaderDataManager::triggerHolds() const
{
    if (statusOwed || frontAvailable() > 0 || shortRun().has_value())
        return true;

    const bool valid = level() != ComponentStatus::Error;
    for (const auto& slot : slots)
    {
        if (slot.clearPending || reachesEvent(slot, contributes(slot, valid)))
            return true;
    }
    return false;
}

bool MultiReaderDataManager::inCallback() const
{
    return callbackThread.load() == std::this_thread::get_id();
}

END_NAMESPACE_OPENDAQ
