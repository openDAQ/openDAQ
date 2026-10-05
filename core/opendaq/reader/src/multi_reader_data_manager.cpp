#include <coreobjects/unit_ptr.h>
#include <coretypes/ratio_ptr.h>
#include <coretypes/validation.h>
#include <opendaq/data_rule_ptr.h>
#include <opendaq/dimension_ptr.h>
#include <opendaq/event_packet_ids.h>
#include <opendaq/event_packet_ptr.h>
#include <opendaq/event_packet_utils.h>
#include <opendaq/multi_reader2_status_impl.h>
#include <opendaq/multi_reader_data_manager.h>
#include <opendaq/reader_utils.h>
#include <opendaq/sample_type_traits.h>

#include <algorithm>
#include <cctype>
#include <cstring>
#include <limits>
#include <numeric>

BEGIN_NAMESPACE_OPENDAQ

namespace
{
    bool isNumeric(SampleType type)
    {
        switch (type)
        {
            case SampleType::Float32:
            case SampleType::Float64:
            case SampleType::UInt8:
            case SampleType::Int8:
            case SampleType::UInt16:
            case SampleType::Int16:
            case SampleType::UInt32:
            case SampleType::Int32:
            case SampleType::UInt64:
            case SampleType::Int64:
                return true;
            default:
                return false;
        }
    }

    bool isInteger(SampleType type)
    {
        return isNumeric(type) && type != SampleType::Float32 && type != SampleType::Float64;
    }

    template <typename Dst, typename Src>
    void convertElements(Dst* dst, const void* src, SizeT elements)
    {
        const auto source = static_cast<const Src*>(src);
        for (SizeT i = 0; i < elements; i++)
            dst[i] = static_cast<Dst>(source[i]);
    }

    template <typename Dst>
    void convertFrom(Dst* dst, const void* src, SampleType srcType, SizeT elements)
    {
        switch (srcType)
        {
            case SampleType::Float32: return convertElements<Dst, float>(dst, src, elements);
            case SampleType::Float64: return convertElements<Dst, double>(dst, src, elements);
            case SampleType::UInt8: return convertElements<Dst, uint8_t>(dst, src, elements);
            case SampleType::Int8: return convertElements<Dst, int8_t>(dst, src, elements);
            case SampleType::UInt16: return convertElements<Dst, uint16_t>(dst, src, elements);
            case SampleType::Int16: return convertElements<Dst, int16_t>(dst, src, elements);
            case SampleType::UInt32: return convertElements<Dst, uint32_t>(dst, src, elements);
            case SampleType::Int32: return convertElements<Dst, int32_t>(dst, src, elements);
            case SampleType::UInt64: return convertElements<Dst, uint64_t>(dst, src, elements);
            case SampleType::Int64: return convertElements<Dst, int64_t>(dst, src, elements);
            default: break;
        }
    }

    void convertSamples(void* dst, SampleType dstType, const void* src, SampleType srcType, SizeT elements)
    {
        switch (dstType)
        {
            case SampleType::Float32: return convertFrom(static_cast<float*>(dst), src, srcType, elements);
            case SampleType::Float64: return convertFrom(static_cast<double*>(dst), src, srcType, elements);
            case SampleType::UInt8: return convertFrom(static_cast<uint8_t*>(dst), src, srcType, elements);
            case SampleType::Int8: return convertFrom(static_cast<int8_t*>(dst), src, srcType, elements);
            case SampleType::UInt16: return convertFrom(static_cast<uint16_t*>(dst), src, srcType, elements);
            case SampleType::Int16: return convertFrom(static_cast<int16_t*>(dst), src, srcType, elements);
            case SampleType::UInt32: return convertFrom(static_cast<uint32_t*>(dst), src, srcType, elements);
            case SampleType::Int32: return convertFrom(static_cast<int32_t*>(dst), src, srcType, elements);
            case SampleType::UInt64: return convertFrom(static_cast<uint64_t*>(dst), src, srcType, elements);
            case SampleType::Int64: return convertFrom(static_cast<int64_t*>(dst), src, srcType, elements);
            default: break;
        }
    }

    bool sameDescriptor(const DataDescriptorPtr& a, const DataDescriptorPtr& b)
    {
        if (!a.assigned() || !b.assigned())
            return a.assigned() == b.assigned();
        if (static_cast<IDataDescriptor*>(a) == static_cast<IDataDescriptor*>(b))
            return true;
        return a == b;
    }

    bool sameComponent(const ComponentPtr& a, const ComponentPtr& b)
    {
        if (!a.assigned() || !b.assigned())
            return a.assigned() == b.assigned();
        return a.getGlobalId() == b.getGlobalId();
    }
}

MultiReaderDataManager::MultiReaderDataManager() = default;

// ---------------------------------------------------------------- configuration

bool MultiReaderDataManager::isSameConfig(const Config& other)
{
    std::scoped_lock lock(mutex);
    if (slots.empty() && config.inputs.empty())
        return false;
    if (other.inputs.size() != config.inputs.size() || other.mainSlot != config.mainSlot || other.used != config.used ||
        other.readType != config.readType || other.minReadCount != config.minReadCount || other.errorPolicy != config.errorPolicy)
        return false;
    for (SizeT i = 0; i < config.inputs.size(); i++)
    {
        if (!sameComponent(other.inputs[i], config.inputs[i]) || other.inputUsed[i] != config.inputUsed[i])
            return false;
    }
    return true;
}

void MultiReaderDataManager::reconfigure(Config newConfig)
{
    std::scoped_lock lock(mutex);

    // Retained inputs keep what ingest last saw on them; new ones start from the connection state given
    std::vector<Slot> newSlots(newConfig.inputs.size());
    for (SizeT i = 0; i < newConfig.inputs.size(); i++)
    {
        Slot& target = newSlots[i];
        bool retained = false;
        for (SizeT j = 0; j < config.inputs.size(); j++)
        {
            if (sameComponent(newConfig.inputs[i], config.inputs[j]))
            {
                target.latestValue = slots[j].latestValue;
                target.latestDomain = slots[j].latestDomain;
                retained = true;
                break;
            }
        }
        if (!retained)
        {
            if (i < newConfig.valueDescriptors.size())
                target.latestValue = newConfig.valueDescriptors[i];
            if (i < newConfig.domainDescriptors.size())
                target.latestDomain = newConfig.domainDescriptors[i];
        }
        target.latestConnected = newConfig.connected[i];
        target.used = newConfig.inputUsed[i];
        target.reportedUsed = target.used;
    }

    config = std::move(newConfig);
    slots = std::move(newSlots);
    firstStatus = true;
    internalReconfigure();
    wakeGeneration++;  // the first status after a configure carries changes, so a wake is owed
}

void MultiReaderDataManager::internalReconfigure()
{
    for (auto& slot : slots)
    {
        slot.clear();
        slot.value = slot.latestValue;
        slot.domain = slot.latestDomain;
        slot.connected = slot.latestConnected;
        slot.descriptorChanged = true;
    }
    evaluate();
    domainChanged = true;
    resynchronized = true;
    if (state != State::Invalid)
        startSync();
}

void MultiReaderDataManager::setSyncLimits(std::chrono::milliseconds newDeadline, std::chrono::milliseconds newMaxDistance)
{
    std::scoped_lock lock(mutex);
    deadline = newDeadline;
    maxDistance = newMaxDistance;
    if (state == State::Syncing)
        syncDeadline = std::chrono::steady_clock::now() + deadline;
}

std::uint64_t MultiReaderDataManager::getWakeGeneration()
{
    std::scoped_lock lock(mutex);
    return wakeGeneration;
}

StringPtr MultiReaderDataManager::getMainInputId()
{
    std::scoped_lock lock(mutex);
    // A pinned main is configuration and always named; an automatic one leads nothing while the reader is invalid
    if (mainSlot.has_value() && (config.mainSlot.has_value() || state != State::Invalid))
        return config.inputs[*mainSlot].getGlobalId();
    return String("");
}

// ---------------------------------------------------------------- evaluation

bool MultiReaderDataManager::valueConvertible(const DataDescriptorPtr& descriptor, SampleType readType)
{
    if (!descriptor.assigned())
        return false;
    try
    {
        if (readType == SampleType::Undefined)
            return descriptor.getSampleType() != SampleType::Invalid && descriptor.getSampleType() != SampleType::Undefined;
        return isNumeric(descriptor.getSampleType());
    }
    catch (...)
    {
        return false;
    }
}

void MultiReaderDataManager::parseDomain(Slot& slot)
{
    DomainInfo info;
    const auto& descriptor = slot.domain;
    slot.dom = info;
    if (!descriptor.assigned())
        return;

    try
    {
        if (!isInteger(descriptor.getSampleType()))
            return;

        const auto dimensions = descriptor.getDimensions();
        if (dimensions.assigned() && dimensions.getCount() > 0)
            return;

        const auto unit = descriptor.getUnit();
        if (!unit.assigned() || unit.getSymbol() != "s")
            return;

        const auto rule = descriptor.getRule();
        if (!rule.assigned() || rule.getType() != DataRuleType::Linear)
            return;
        const auto params = rule.getParameters();
        const Float rawDelta = params.get("delta");
        const Float rawStart = params.hasKey("start") ? static_cast<Float>(params.get("start")) : 0.0;
        info.delta = static_cast<std::int64_t>(rawDelta);
        info.start = static_cast<std::int64_t>(rawStart);
        if (info.delta <= 0 || static_cast<Float>(info.delta) != rawDelta || static_cast<Float>(info.start) != rawStart)
            return;

        const auto resolution = descriptor.getTickResolution();
        info.resNum = resolution.assigned() ? static_cast<std::int64_t>(resolution.getNumerator()) : 1;
        info.resDen = resolution.assigned() ? static_cast<std::int64_t>(resolution.getDenominator()) : 1;
        if (info.resNum <= 0 || info.resDen <= 0)
            return;

        // The rate is den / (num * delta) and has to be a whole number of samples per second
        if (info.resDen % (info.resNum * info.delta) != 0)
            return;
        info.rate = info.resDen / (info.resNum * info.delta);

        const StringPtr origin = descriptor.getOrigin();
        if (origin.assigned() && origin.getLength() > 0)
        {
            // A date alone is an origin on a full second too: "1970" and "1970-01" pad to the first day
            std::string text = origin.toStdString();
            if (text.size() == 4 && std::all_of(text.begin(), text.end(), ::isdigit))
                text += "-01-01";
            else if (text.size() == 7 && text[4] == '-')
                text += "-01";
            bool ok = false;
            const auto epoch = reader::parseEpoch(text, &ok);
            if (!ok)
                return;
            const auto sinceEpoch = epoch.time_since_epoch();
            const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(sinceEpoch);
            if (std::chrono::duration_cast<std::chrono::system_clock::duration>(seconds) != sinceEpoch)
                return;  // only full-second origins
            info.hasOrigin = true;
            info.originSec = seconds.count();
        }
    }
    catch (...)
    {
        return;
    }

    info.valid = true;
    slot.dom = info;
}

void MultiReaderDataManager::relateToMain(SizeT index)
{
    Slot& slot = slots[index];
    DomainInfo& dom = slot.dom;
    dom.scaleNum = 1;
    dom.scaleDen = 1;
    dom.originOffset = 0;
    if (!mainSlot.has_value() || *mainSlot == index)
        return;

    const DomainInfo& main = slots[*mainSlot].dom;
    if (!dom.valid || !main.valid)
        return;

    if (dom.rate != main.rate || dom.hasOrigin != main.hasOrigin)
    {
        dom.valid = false;
        return;
    }

    // Same rate in possibly different tick units: scale this input's ticks onto the main grid
    std::int64_t num = dom.resNum * main.resDen;
    std::int64_t den = dom.resDen * main.resNum;
    const auto g = std::gcd(num, den);
    dom.scaleNum = num / g;
    dom.scaleDen = den / g;

    // An origin difference becomes a whole number of main ticks
    if (dom.hasOrigin)
    {
        const std::int64_t diffSec = dom.originSec - main.originSec;
        const std::int64_t scaled = diffSec * main.resDen;
        if (scaled % main.resNum != 0)
        {
            dom.valid = false;
            return;
        }
        dom.originOffset = scaled / main.resNum;
    }
}

void MultiReaderDataManager::chooseMain()
{
    if (config.mainSlot.has_value())
    {
        mainSlot = config.mainSlot;
        return;
    }
    mainSlot.reset();
    for (SizeT i = 0; i < slots.size(); i++)
    {
        const Slot& slot = slots[i];
        if (config.used && slot.used && slot.connected && slot.valueValid && slot.dom.valid && !slot.offGrid && !slot.syncFailed)
        {
            mainSlot = i;
            return;
        }
    }
}

void MultiReaderDataManager::evaluate()
{
    for (auto& slot : slots)
    {
        slot.valueValid = valueConvertible(slot.value, config.readType);
        parseDomain(slot);
    }

    chooseMain();

    bool anyUsed = false;
    bool anyUsedHealthy = false;
    bool allUsedHealthy = true;
    for (SizeT i = 0; i < slots.size(); i++)
    {
        Slot& slot = slots[i];
        relateToMain(i);
        if (!slot.connected)
            slot.error = MultiReader2InputError::Disconnected;
        else if (!slot.valueValid)
            slot.error = MultiReader2InputError::ValueDescriptorInvalid;
        else if (!slot.dom.valid || slot.offGrid)
            slot.error = MultiReader2InputError::DomainDescriptorInvalid;
        else if (slot.syncFailed)
            slot.error = MultiReader2InputError::SyncFailed;
        else
            slot.error = MultiReader2InputError::None;

        if (config.used && slot.used)
        {
            anyUsed = true;
            if (slot.error == MultiReader2InputError::None)
                anyUsedHealthy = true;
            else
                allUsedHealthy = false;
        }
    }

    bool valid;
    if (!anyUsed)
        valid = true;
    else if (config.errorPolicy == MultiReader2ErrorPolicy::Invalidate)
        valid = allUsedHealthy;
    else
        valid = anyUsedHealthy && (!config.mainSlot.has_value() || slots[*config.mainSlot].error == MultiReader2InputError::None);

    if (valid && mainSlot.has_value() && slots[*mainSlot].error != MultiReader2InputError::None)
        valid = false;

    if (!valid)
        state = State::Invalid;
    else if (state == State::Invalid)
        state = State::Syncing;
}

bool MultiReaderDataManager::contributes(SizeT index) const
{
    const Slot& slot = slots[index];
    return config.used && slot.used && slot.error == MultiReader2InputError::None;
}

bool MultiReaderDataManager::dropsData(SizeT index) const
{
    const Slot& slot = slots[index];
    if (!config.used || !slot.used)
        return true;
    // A synchronization failure clears when data shows up again, so that data has to get in
    if (slot.error == MultiReader2InputError::SyncFailed)
        return false;
    return state == State::Invalid || slot.error != MultiReader2InputError::None;
}

// ---------------------------------------------------------------- queue geometry

std::optional<std::int64_t> MultiReaderDataManager::toMain(SizeT index, std::int64_t tick) const
{
    const DomainInfo& dom = slots[index].dom;
    const std::int64_t scaled = tick * dom.scaleNum;
    if (scaled % dom.scaleDen != 0)
        return std::nullopt;
    return scaled / dom.scaleDen + dom.originOffset;
}

void MultiReaderDataManager::Slot::push(Entry entry)
{
    if (queue.empty())
    {
        runCount = entry.isBoundary ? 0 : entry.count;
        runOpen = !entry.isBoundary;
    }
    else if (runOpen)
    {
        const Entry& back = queue.back();
        const bool continues = !entry.isBoundary && entry.delta == back.delta &&
                               entry.start == back.start + static_cast<std::int64_t>(back.count) * back.delta;
        if (continues)
            runCount += entry.count;
        else
            runOpen = false;
    }
    queue.push_back(std::move(entry));
}

void MultiReaderDataManager::Slot::popFront()
{
    const bool wasBoundary = queue.front().isBoundary;
    if (!wasBoundary)
        runCount -= queue.front().count;
    queue.pop_front();
    frontOffset = 0;
    // A boundary or the last packet of a closed run (a gap follows) was popped: what is next is a new run
    if (wasBoundary || (runCount == 0 && !queue.empty()))
        recomputeRun();
}

void MultiReaderDataManager::Slot::clear()
{
    queue.clear();
    frontOffset = 0;
    runCount = 0;
    runOpen = false;
}

// Walks the run behind a popped boundary once; boundaries are rare next to data
void MultiReaderDataManager::Slot::recomputeRun()
{
    runCount = 0;
    runOpen = false;
    if (queue.empty() || queue.front().isBoundary)
        return;
    const Entry& front = queue.front();
    runCount = front.count;
    std::int64_t expected = front.start + static_cast<std::int64_t>(front.count) * front.delta;
    for (SizeT i = 1; i < queue.size(); i++)
    {
        const Entry& entry = queue[i];
        if (entry.isBoundary || entry.start != expected || entry.delta != front.delta)
            return;
        runCount += entry.count;
        expected += static_cast<std::int64_t>(entry.count) * entry.delta;
    }
    runOpen = true;
}

MultiReaderDataManager::Run MultiReaderDataManager::runOf(SizeT index) const
{
    Run run;
    const Slot& slot = slots[index];
    if (slot.queue.empty())
        return run;

    const Entry& front = slot.queue.front();
    if (front.isBoundary)
    {
        run.endsAtBoundary = true;
        return run;
    }

    const auto first = toMain(index, front.start + static_cast<std::int64_t>(slot.frontOffset) * front.delta);
    if (!first.has_value())
    {
        run.offGrid = true;
        return run;
    }

    run.hasData = true;
    run.first = *first;
    run.samples = slot.runCount - slot.frontOffset;
    run.endsAtBoundary = !slot.runOpen;

    const auto mainDelta = mainSlot.has_value() ? slots[*mainSlot].dom.delta : 1;
    run.last = run.first + static_cast<std::int64_t>(run.samples - 1) * mainDelta;
    return run;
}

void MultiReaderDataManager::advance(SizeT index, SizeT samples)
{
    Slot& slot = slots[index];
    while (samples > 0 && !slot.queue.empty())
    {
        Entry& front = slot.queue.front();
        const SizeT left = front.count - slot.frontOffset;
        if (samples >= left)
        {
            samples -= left;
            slot.popFront();
        }
        else
        {
            slot.frontOffset += samples;
            samples = 0;
        }
    }
}

void MultiReaderDataManager::discardBefore(SizeT index, std::int64_t mainTick)
{
    Slot& slot = slots[index];
    while (!slot.queue.empty() && !slot.queue.front().isBoundary)
    {
        const Entry& front = slot.queue.front();
        const auto first = toMain(index, front.start + static_cast<std::int64_t>(slot.frontOffset) * front.delta);
        if (!first.has_value() || *first >= mainTick)
            return;
        const auto mainDelta = slots[*mainSlot].dom.delta;
        const std::int64_t behind = (mainTick - *first) / mainDelta;
        const SizeT left = front.count - slot.frontOffset;
        if (static_cast<std::int64_t>(left) <= behind)
        {
            slot.popFront();
        }
        else
        {
            slot.frontOffset += static_cast<SizeT>(behind);
            return;
        }
    }
}

void MultiReaderDataManager::copyRun(SizeT index, void* buffer, SizeT samples) const
{
    const Slot& slot = slots[index];
    auto dst = static_cast<uint8_t*>(buffer);
    SizeT offset = slot.frontOffset;
    for (const Entry& entry : slot.queue)
    {
        if (samples == 0)
            break;
        const SizeT take = std::min(samples, entry.count - offset);
        const auto descriptor = entry.packet.getDataDescriptor();
        const SampleType srcType = descriptor.getSampleType();
        const SizeT sampleSize = descriptor.getSampleSize();
        const auto src = static_cast<const uint8_t*>(entry.packet.getData()) + offset * sampleSize;
        if (config.readType == SampleType::Undefined)
        {
            std::memcpy(dst, src, take * sampleSize);
            dst += take * sampleSize;
        }
        else
        {
            const SizeT elements = take * (sampleSize / getSampleSize(srcType));
            convertSamples(dst, config.readType, src, srcType, elements);
            dst += elements * getSampleSize(config.readType);
        }
        samples -= take;
        offset = 0;
    }
}

SizeT MultiReaderDataManager::availableLocked() const
{
    if (state != State::Streaming)
        return 0;
    SizeT available = std::numeric_limits<SizeT>::max();
    bool any = false;
    for (SizeT i = 0; i < slots.size(); i++)
    {
        if (!contributes(i))
            continue;
        any = true;
        const Run run = runOf(i);
        const SizeT samples = run.hasData && run.first == readTick ? run.samples : 0;
        available = std::min(available, samples);
    }
    if (!any || available < config.minReadCount)
        return 0;
    return available;
}

// ---------------------------------------------------------------- boundaries and state

void MultiReaderDataManager::applyBoundary(SizeT index, const Boundary& boundary)
{
    Slot& slot = slots[index];
    switch (boundary.kind)
    {
        case Boundary::Kind::Descriptor:
            if (boundary.hasValue && !sameDescriptor(boundary.value, slot.value))
            {
                slot.value = boundary.value;
                slot.descriptorChanged = true;
            }
            if (boundary.hasDomain && !sameDescriptor(boundary.domain, slot.domain))
            {
                slot.domain = boundary.domain;
                if (mainSlot.has_value() && *mainSlot == index)
                    domainChanged = true;
                else if (contributes(index))
                    resynchronized = true;  // an input that gates nothing moves nothing
            }
            slot.syncFailed = false;
            slot.offGrid = false;
            break;
        case Boundary::Kind::Connected:
            slot.connected = true;
            if (boundary.hasValue && !sameDescriptor(boundary.value, slot.value))
            {
                slot.value = boundary.value;
                slot.descriptorChanged = true;
            }
            if (boundary.hasDomain && !sameDescriptor(boundary.domain, slot.domain))
            {
                slot.domain = boundary.domain;
                if (mainSlot.has_value() && *mainSlot == index)
                    domainChanged = true;
                else if (contributes(index))
                    resynchronized = true;
            }
            slot.syncFailed = false;
            slot.offGrid = false;
            break;
        case Boundary::Kind::Disconnected:
            slot.connected = false;
            slot.syncFailed = false;
            slot.offGrid = false;
            break;
        case Boundary::Kind::Gap:
            if (contributes(index) && state != State::Invalid)
                resynchronized = true;
            break;
    }
}

void MultiReaderDataManager::consumeBoundaries()
{
    const bool wasValid = state != State::Invalid;
    const auto oldMain = mainSlot;
    const bool domainWasPending = domainChanged;
    const bool resyncWasPending = resynchronized;
    std::vector<MultiReader2InputError> oldErrors(slots.size());
    for (SizeT i = 0; i < slots.size(); i++)
        oldErrors[i] = slots[i].error;

    bool applied = false;
    for (SizeT i = 0; i < slots.size(); i++)
    {
        Slot& slot = slots[i];

        // Data showing up on a failed input is its way back in
        if (slot.syncFailed && !slot.queue.empty() && !slot.queue.front().isBoundary)
        {
            slot.syncFailed = false;
            applied = true;
        }

        // A data run in front of a boundary gates it on a contributing input; while streaming a run that does not
        // start at the read position is a gap
        while (!slot.queue.empty())
        {
            const Entry& front = slot.queue.front();
            if (front.isBoundary)
            {
                const Boundary boundary = front.boundary;
                slot.popFront();
                applyBoundary(i, boundary);
                applied = true;
                continue;
            }
            if (contributes(i) && state == State::Streaming)
            {
                const auto first = toMain(i, front.start + static_cast<std::int64_t>(slot.frontOffset) * front.delta);
                if (!first.has_value())
                {
                    slot.offGrid = true;  // off the main grid: the input cannot align under its current domain
                    applied = true;
                }
                else if (*first != readTick)
                {
                    resynchronized = true;
                    applied = true;
                }
            }
            break;
        }
    }

    if (!applied)
        return;

    settle(wasValid, oldMain, oldErrors, domainWasPending, resyncWasPending);
}

// Moves the reader after boundaries were applied or synchronization failed: evaluate, then recover, resynchronize
// or just drop what inputs that no longer contribute still hold
void MultiReaderDataManager::settle(bool wasValid,
                                    std::optional<SizeT> oldMain,
                                    const std::vector<MultiReader2InputError>& oldErrors,
                                    bool domainWasPending,
                                    bool resyncWasPending)
{
    evaluate();
    const bool nowValid = state != State::Invalid;

    bool rejoined = false;
    for (SizeT i = 0; i < slots.size(); i++)
    {
        if (oldErrors[i] != MultiReader2InputError::None && slots[i].error == MultiReader2InputError::None)
        {
            slots[i].descriptorChanged = true;
            if (contributes(i))
                rejoined = true;
        }
    }

    const bool mainDomainChanged = domainChanged && !domainWasPending;
    if (wasValid != nowValid || (nowValid && mainSlot != oldMain) || (nowValid && mainDomainChanged))
    {
        internalReconfigure();
        return;
    }

    if (nowValid && state == State::Streaming && (rejoined || (resynchronized && !resyncWasPending)))
        startSync();

    // Stale data on inputs that no longer contribute is dropped so their later boundaries surface
    for (SizeT i = 0; i < slots.size(); i++)
    {
        if (contributes(i) && state != State::Invalid)
            continue;
        Slot& slot = slots[i];
        while (!slot.queue.empty() && !slot.queue.front().isBoundary)
        {
            slot.popFront();
        }
    }
}

void MultiReaderDataManager::startSync()
{
    state = State::Syncing;
    resynchronized = true;
    syncDeadline = std::chrono::steady_clock::now() + deadline;
}

bool MultiReaderDataManager::trySync()
{
    if (state != State::Syncing || !mainSlot.has_value())
        return false;

    const bool deadlinePassed = std::chrono::steady_clock::now() > syncDeadline;
    const std::int64_t mainDelta = slots[*mainSlot].dom.delta;
    const std::int64_t mainTicksPerSecond = slots[*mainSlot].dom.resDen / slots[*mainSlot].dom.resNum;
    const std::int64_t distanceTicks = std::max<std::int64_t>(1, maxDistance.count() * mainTicksPerSecond / 1000);

    std::vector<Run> runs(slots.size());
    bool waiting = false;
    std::int64_t target = std::numeric_limits<std::int64_t>::min();
    bool failed = false;
    for (SizeT i = 0; i < slots.size(); i++)
    {
        if (!contributes(i))
            continue;
        runs[i] = runOf(i);
        if (runs[i].offGrid)
        {
            slots[i].offGrid = true;
            failed = true;
            continue;
        }
        if (!runs[i].hasData)
        {
            if (deadlinePassed)
            {
                slots[i].syncFailed = true;
                failed = true;
            }
            else
            {
                waiting = true;
            }
            continue;
        }
        target = std::max(target, runs[i].first);
    }

    if (!failed && !waiting && target != std::numeric_limits<std::int64_t>::min())
    {
        for (SizeT i = 0; i < slots.size(); i++)
        {
            if (!contributes(i) || !runs[i].hasData)
                continue;
            // Every input has to sit on the main lattice
            if (((target - runs[i].first) % mainDelta + mainDelta) % mainDelta != 0)
            {
                slots[i].offGrid = true;
                failed = true;
                continue;
            }
            if (runs[i].last < target)
            {
                // Everything queued lies before the common start; too far behind means it cannot catch up
                if (target - runs[i].last > distanceTicks)
                {
                    slots[i].syncFailed = true;
                    failed = true;
                }
                else
                {
                    discardBefore(i, target);
                    waiting = true;
                }
                continue;
            }
            discardBefore(i, target);
        }
    }

    if (failed)
    {
        std::vector<MultiReader2InputError> oldErrors(slots.size());
        for (SizeT i = 0; i < slots.size(); i++)
            oldErrors[i] = slots[i].error;
        settle(true, mainSlot, oldErrors, domainChanged, resynchronized);
        return true;
    }

    if (waiting || target == std::numeric_limits<std::int64_t>::min())
        return false;

    state = State::Streaming;
    readTick = target;
    return true;
}

// ---------------------------------------------------------------- read path

SizeT MultiReaderDataManager::getAvailableCount()
{
    std::scoped_lock lock(mutex);
    return availableLocked();
}

ErrCode MultiReaderDataManager::read(void** data, SizeT* count, SizeT* packetOffset, IMultiReader2Status** status)
{
    OPENDAQ_PARAM_NOT_NULL(count);
    OPENDAQ_PARAM_NOT_NULL(status);
    return doRead(data, count, packetOffset, status, false, true);
}

ErrCode MultiReaderDataManager::readWithDomain(void** data, SizeT* count, IMultiReader2Status** status)
{
    OPENDAQ_PARAM_NOT_NULL(count);
    OPENDAQ_PARAM_NOT_NULL(status);
    return doRead(data, count, nullptr, status, true, true);
}

ErrCode MultiReaderDataManager::skipSamples(SizeT* count, IMultiReader2Status** status)
{
    OPENDAQ_PARAM_NOT_NULL(count);
    OPENDAQ_PARAM_NOT_NULL(status);
    return doRead(nullptr, count, nullptr, status, false, false);
}

ErrCode MultiReaderDataManager::doRead(void** data, SizeT* count, SizeT* packetOffset, IMultiReader2Status** status, bool withDomain, bool copy)
{
    std::scoped_lock lock(mutex);

    const SizeT requested = *count;
    if (requested != 0 && requested < config.minReadCount)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "The requested count is below MinReadCount");
    if (copy && requested != 0 && data == nullptr)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "No buffers were given for a non-zero count");

    *count = 0;
    if (packetOffset != nullptr)
        *packetOffset = 0;

    trySync();

    // Changes not yet reported go out before the data that follows them, so the consumer processes every sample
    // under descriptors it has already seen
    bool pendingChanges = firstStatus || domainChanged || resynchronized;
    for (SizeT i = 0; i < slots.size() && !pendingChanges; i++)
        pendingChanges = slots[i].descriptorChanged && slots[i].error == MultiReader2InputError::None;

    if (state == State::Streaming && requested > 0 && !pendingChanges)
    {
        // The aligned run: the shortest run over the contributing inputs, and whether that input stopped at a boundary
        SizeT plan = requested;
        bool stoppedAtBoundary = false;
        bool any = false;
        for (SizeT i = 0; i < slots.size(); i++)
        {
            if (!contributes(i))
                continue;
            any = true;
            const Run run = runOf(i);
            const bool aligned = run.hasData && run.first == readTick;
            const SizeT samples = aligned ? run.samples : 0;
            const bool atBoundary = !aligned || run.endsAtBoundary;
            if (samples < plan)
            {
                plan = samples;
                stoppedAtBoundary = atBoundary;
            }
            else if (samples == plan)
            {
                stoppedAtBoundary = stoppedAtBoundary || atBoundary;
            }
        }
        if (!any)
            plan = 0;

        if (plan > 0 && plan < config.minReadCount)
        {
            // A leading run shorter than MinReadCount in front of a boundary is dropped so the boundary can be consumed
            if (stoppedAtBoundary)
            {
                for (SizeT i = 0; i < slots.size(); i++)
                    if (contributes(i))
                        advance(i, plan);
                readTick += static_cast<std::int64_t>(plan) * slots[*mainSlot].dom.delta;
            }
            plan = 0;
        }

        if (plan > 0)
        {
            const SizeT base = withDomain ? 1 : 0;
            if (copy)
            {
                for (SizeT i = 0; i < slots.size(); i++)
                {
                    if (contributes(i) && data[base + i] == nullptr)
                        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "A used input was given no buffer");
                }
                if (withDomain)
                {
                    if (data[0] == nullptr)
                        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "No buffer was given for the domain");
                    const auto timestamps = static_cast<std::int64_t*>(data[0]);
                    const std::int64_t mainDelta = slots[*mainSlot].dom.delta;
                    for (SizeT k = 0; k < plan; k++)
                        timestamps[k] = readTick + static_cast<std::int64_t>(k) * mainDelta;
                }
                for (SizeT i = 0; i < slots.size(); i++)
                {
                    if (contributes(i))
                        copyRun(i, data[base + i], plan);
                }
            }
            for (SizeT i = 0; i < slots.size(); i++)
            {
                if (contributes(i))
                    advance(i, plan);
            }
            if (packetOffset != nullptr)
                *packetOffset = static_cast<SizeT>(readTick - slots[*mainSlot].dom.start);
            readTick += static_cast<std::int64_t>(plan) * slots[*mainSlot].dom.delta;
            *count = plan;
        }
    }

    // The boundaries the run stopped at, and those on inputs that gate nothing, are evaluated and reported now
    consumeBoundaries();
    trySync();

    *status = makeStatus().detach();
    return OPENDAQ_SUCCESS;
}

ObjectPtr<IMultiReader2Status> MultiReaderDataManager::makeStatus()
{
    const bool valid = state != State::Invalid;

    bool changes = firstStatus || valid != reportedValid;
    if (valid && (domainChanged || resynchronized))
        changes = true;

    for (const auto& slot : slots)
    {
        const bool healthy = valid && slot.error == MultiReader2InputError::None;
        if (slot.error != slot.reportedError || slot.used != slot.reportedUsed || (healthy && slot.descriptorChanged))
            changes = true;
    }
    if (!changes && lastStatus.assigned())
        return lastStatus;

    auto inputs = List<IMultiReader2InputStatus>();
    for (auto& slot : slots)
    {
        const bool healthy = valid && slot.error == MultiReader2InputError::None;
        const bool descriptorChanged = healthy && slot.descriptorChanged;
        inputs.pushBack(createWithImplementation<IMultiReader2InputStatus, MultiReader2InputStatusImpl>(
            config.inputs[&slot - slots.data()], slot.used && config.used, slot.error, descriptorChanged, healthy ? slot.value : nullptr));
        slot.reportedError = slot.error;
        slot.reportedUsed = slot.used;
        if (healthy)
            slot.descriptorChanged = false;
    }

    DataDescriptorPtr domain;
    if (valid && mainSlot.has_value())
        domain = slots[*mainSlot].domain;

    auto status = createWithImplementation<IMultiReader2Status, MultiReader2StatusImpl>(
        changes, valid, valid && domainChanged, domain, valid && resynchronized, inputs);

    if (valid)
    {
        domainChanged = false;
        resynchronized = false;
    }
    reportedValid = valid;
    firstStatus = false;
    lastStatus = changes ? nullptr : status;  // only a status without changes can serve again
    return status;
}

// ---------------------------------------------------------------- producer side

MultiReaderDataManager::NotifyOwed MultiReaderDataManager::connected(SizeT index, const DataDescriptorPtr& value, const DataDescriptorPtr& domain)
{
    std::scoped_lock lock(mutex);
    if (index >= slots.size())
        return {};
    Slot& slot = slots[index];
    if (slot.latestConnected)
        return {};
    slot.latestConnected = true;
    Entry entry;
    entry.isBoundary = true;
    entry.boundary.kind = Boundary::Kind::Connected;
    entry.boundary.hasValue = value.assigned() && !sameDescriptor(value, slot.latestValue);
    entry.boundary.hasDomain = domain.assigned() && !sameDescriptor(domain, slot.latestDomain);
    entry.boundary.value = value;
    entry.boundary.domain = domain;
    if (entry.boundary.hasValue)
        slot.latestValue = value;
    if (entry.boundary.hasDomain)
        slot.latestDomain = domain;
    slot.push(std::move(entry));
    wakeGeneration++;
    return {true};
}

MultiReaderDataManager::NotifyOwed MultiReaderDataManager::disconnected(SizeT index)
{
    std::scoped_lock lock(mutex);
    if (index >= slots.size())
        return {};
    Slot& slot = slots[index];
    if (!slot.latestConnected)
        return {};
    slot.latestConnected = false;
    Entry entry;
    entry.isBoundary = true;
    entry.boundary.kind = Boundary::Kind::Disconnected;
    slot.push(std::move(entry));
    wakeGeneration++;
    return {true};
}

MultiReaderDataManager::NotifyOwed MultiReaderDataManager::setConnected(SizeT index, bool isConnected, const DataDescriptorPtr& value, const DataDescriptorPtr& domain)
{
    return isConnected ? connected(index, value, domain) : disconnected(index);
}

MultiReaderDataManager::NotifyOwed MultiReaderDataManager::addPacket(SizeT index, const PacketPtr& packet)
{
    std::scoped_lock lock(mutex);
    if (index >= slots.size() || !packet.assigned())
        return {};
    Slot& slot = slots[index];

    if (packet.getType() == PacketType::Event)
    {
        const auto eventPacket = packet.asPtrOrNull<IEventPacket>(true);
        if (!eventPacket.assigned())
            return {};
        const auto eventId = eventPacket.getEventId();
        Entry entry;
        entry.isBoundary = true;
        if (eventId == event_packet_id::DATA_DESCRIPTOR_CHANGED)
        {
            const auto [valueChanged, domainChangedFlag, newValue, newDomain] = parseDataDescriptorEventPacket(eventPacket);
            // Only what differs from the last announcement is a boundary
            entry.boundary.kind = Boundary::Kind::Descriptor;
            entry.boundary.hasValue = valueChanged && !sameDescriptor(newValue, slot.latestValue);
            entry.boundary.hasDomain = domainChangedFlag && !sameDescriptor(newDomain, slot.latestDomain);
            entry.boundary.value = newValue;
            entry.boundary.domain = newDomain;
            if (!entry.boundary.hasValue && !entry.boundary.hasDomain)
                return {};
            if (entry.boundary.hasValue)
                slot.latestValue = newValue;
            if (entry.boundary.hasDomain)
                slot.latestDomain = newDomain;
        }
        else if (eventId == event_packet_id::IMPLICIT_DOMAIN_GAP_DETECTED)
        {
            entry.boundary.kind = Boundary::Kind::Gap;
        }
        else
        {
            return {};
        }
        slot.push(std::move(entry));
        wakeGeneration++;
        return {true};
    }

    if (packet.getType() != PacketType::Data || dropsData(index))
        return {};

    const auto dataPacket = packet.asPtrOrNull<IDataPacket>(true);
    if (!dataPacket.assigned())
        return {};
    const auto domainPacket = dataPacket.getDomainPacket();
    if (!domainPacket.assigned())
        return {};

    Entry entry;
    entry.isBoundary = false;
    entry.packet = dataPacket;
    entry.count = dataPacket.getSampleCount();
    if (entry.count == 0)
        return {};
    try
    {
        const auto domainDescriptor = domainPacket.getDataDescriptor();
        if (domainDescriptor.getObject() != slot.packetDomain.getObject())
        {
            const auto params = domainDescriptor.getRule().getParameters();
            slot.packetDelta = static_cast<std::int64_t>(static_cast<Int>(params.get("delta")));
            slot.packetStart = params.hasKey("start") ? static_cast<std::int64_t>(static_cast<Int>(params.get("start"))) : 0;
            slot.packetDomain = domainDescriptor;
        }
        entry.delta = slot.packetDelta;
        entry.start = slot.packetStart + static_cast<std::int64_t>(domainPacket.getOffset().getIntValue());
    }
    catch (...)
    {
        return {};  // not a linear domain; the descriptor evaluation reports it
    }
    slot.push(std::move(entry));

    // Synchronization progresses as packets arrive: it only looks at data in front of the first boundary, which
    // belongs to the descriptors in force. Boundaries themselves wait for a read.
    const bool moved = trySync();
    const bool wake = moved || slot.error == MultiReader2InputError::SyncFailed ||
                      (state == State::Streaming && availableLocked() >= config.minReadCount);
    if (wake)
        wakeGeneration++;
    return {wake};
}

END_NAMESPACE_OPENDAQ
