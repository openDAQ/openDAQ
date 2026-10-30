#include <coretypes/validation.h>
#include <opendaq/custom_log.h>
#include <opendaq/data_descriptor_factory.h>
#include <opendaq/data_packet_ptr.h>
#include <opendaq/event_packet_ids.h>
#include <opendaq/event_packet_params.h>
#include <opendaq/event_packet_ptr.h>
#include <opendaq/multi_reader2_convert.h>
#include <opendaq/multi_reader2_data_manager.h>
#include <opendaq/multi_reader2_status_impl.h>
#include <opendaq/sample_type_traits.h>

#include <cassert>

BEGIN_NAMESPACE_OPENDAQ

using namespace multi_reader2;

bool MultiReader2DataManager::State::operator==(const State& other) const
{
    if (level != other.level || message != other.message || main.getObject() != other.main.getObject() ||
        !sameDescriptor(domain, other.domain) || inputs.size() != other.inputs.size())
        return false;
    for (SizeT i = 0; i < inputs.size(); ++i)
        if (inputs[i].error != other.inputs[i].error || !sameDescriptor(inputs[i].descriptor, other.inputs[i].descriptor) ||
            inputs[i].sampleSize != other.inputs[i].sampleSize)
            return false;
    return true;
}

// -------- Configuration

void MultiReader2DataManager::construct(Config config)
{
    std::scoped_lock lock(mutex);
    apply(std::move(config), true);
}

void MultiReader2DataManager::configure(Config config, IMultiReader2Status** status)
{
    std::scoped_lock lock(mutex);
    bool same = configured && config.inputs.size() == slots.size() && config.mainSlot == pinnedMain &&
                config.acceptsDescriptor.getObject() == acceptsDescriptor.getObject() && config.readType == readType &&
                config.minReadCount == minReadCount && config.errorPolicy == errorPolicy;
    for (SizeT i = 0; same && i < slots.size(); ++i)
        same = config.keys[i] == slots[i].key;
    apply(std::move(config), !same);
    SizeT none = 0;
    readLocked(nullptr, false, false, &none, nullptr, status, nullptr);
}

void MultiReader2DataManager::apply(Config config, bool reinitialize)
{
    loggerComponent = config.loggerComponent;
    if (!reinitialize)
    {
        // Equal params: every input is judged again
        const bool wasValid = valid();
        const auto values = valuesInForce();
        for (SizeT i = 0; i < slots.size(); ++i)
            rejudge(i, values);
        settle(wasValid, false);
        return;
    }

    std::vector<Slot> next(config.inputs.size());
    for (SizeT i = 0; i < next.size(); ++i)
    {
        Slot& s = next[i];
        const auto it = std::find_if(slots.begin(), slots.end(), [&](const Slot& old) { return old.key == config.keys[i]; });
        if (it != slots.end())
        {
            s.latest = it->latest;
            s.latestTicks = it->latestTicks;
        }
        else
        {
            s.latest = config.current[i];
            s.latestTicks = parseDomain(s.latest.domain);
        }
        s.input = config.inputs[i];
        s.key = config.keys[i];
        s.current = s.latest;
        s.ticks = s.latestTicks;
    }
    slots = std::move(next);
    acceptsDescriptor = config.acceptsDescriptor;
    readType = config.readType;
    minReadCount = config.minReadCount;
    errorPolicy = config.errorPolicy;
    pinnedMain = config.mainSlot;
    configured = true;

    const auto values = valuesInForce();
    for (SizeT i = 0; i < slots.size(); ++i)
        ask(i, values);
    mainSlot = pinnedMain;
    if (!mainSlot)
    {
        relateAll();
        evaluateAll();
        mainSlot = firstHealthy();
    }
    relateAll();
    evaluateAll();
    owed.force = true;
    stateDirty = true;
    restart();
}

// -------- Judgement

void MultiReader2DataManager::ask(SizeT i, const ListPtr<IDataDescriptor>& values)
{
    Slot& s = slots[i];
    if (!s.current.connected || readSampleSize(s.current.value, readType) == 0)
        return;
    if (!acceptsDescriptor.assigned())
    {
        s.accepted = true;
        return;
    }
    callbackThread = std::this_thread::get_id();
    std::optional<std::string> thrown;
    try
    {
        const BaseObjectPtr verdict = acceptsDescriptor.call(s.input, s.current.value, s.current.domain, values);
        const auto boolean = verdict.asPtrOrNull<IBoolean>();
        s.accepted = boolean.assigned() && static_cast<bool>(boolean);
    }
    catch (const std::exception& e)
    {
        thrown = e.what();
    }
    catch (...)
    {
        thrown = "an unknown exception";
    }
    daqClearErrorInfo();
    callbackThread = std::thread::id();
    // An exception is a rejection, logged so that a bug in the owner does not pass for a verdict
    if (thrown)
    {
        s.accepted = false;
        if (loggerComponent.assigned())
            LOG_W("acceptsDescriptor threw for {}: {}", s.input.getLocalId().toStdString(), *thrown);
    }
}

// Asked again; an input whose verdict moved is set aside or rejoins as on a descriptor event
void MultiReader2DataManager::rejudge(SizeT i, const ListPtr<IDataDescriptor>& values)
{
    Slot& s = slots[i];
    const bool before = s.accepted;
    const auto oldError = s.error;
    ask(i, values);
    if (s.accepted == before)
        return;
    evaluate(i);
    if (s.error == MultiReader2InputError::None && oldError != MultiReader2InputError::None)
    {
        s.descriptorChanged = true;
        s.restartSync = true;
    }
}

// Every input's value descriptor in force, in slot order, null for an input that is not connected; what a verdict sees
ListPtr<IDataDescriptor> MultiReader2DataManager::valuesInForce() const
{
    if (!acceptsDescriptor.assigned())
        return nullptr;
    auto values = List<IDataDescriptor>();
    for (const auto& s : slots)
        values.pushBack(s.current.connected ? s.current.value : DataDescriptorPtr());
    return values;
}

// The input's error, first cause first. Alone, it is judged without the main input: no relation to it and no SyncFailed
std::pair<MultiReader2InputError, std::string> MultiReader2DataManager::judge(SizeT i, bool alone) const
{
    const Slot& s = slots[i];
    const bool relative = !alone && mainSlot && *mainSlot != i && slots[*mainSlot].ticks;
    if (!s.current.connected)
        return {MultiReader2InputError::Disconnected, "not connected"};
    const char* fault = nullptr;
    if (!s.ticks)
        parseDomain(s.current.domain, &fault);
    else if (relative && !s.relation)
        relate(*s.ticks, *slots[*mainSlot].ticks, &fault);
    if (fault)
        return {MultiReader2InputError::DomainDescriptorInvalid, std::string("domain descriptor invalid, ") + fault};
    if (!s.current.value.assigned())
        return {MultiReader2InputError::ValueDescriptorInvalid, "no value descriptor"};
    if (readSampleSize(s.current.value, readType) == 0)
        return {MultiReader2InputError::ValueDescriptorInvalid, "value type not convertible to " + convertSampleTypeToString(readType)};
    if (!s.accepted)
        return {MultiReader2InputError::ValueDescriptorInvalid, "value descriptor rejected by the owner"};
    if (!alone && s.syncFailed == SyncCause::Late)
        return {MultiReader2InputError::SyncFailed, "not synchronized, more than 2 s from the main input"};
    if (!alone && s.syncFailed == SyncCause::OffGrid)
        return {MultiReader2InputError::SyncFailed, "not synchronized, off the main input's grid"};
    return {MultiReader2InputError::None, {}};
}

void MultiReader2DataManager::evaluate(SizeT i)
{
    Slot& s = slots[i];
    s.sampleSize = readSampleSize(s.current.value, readType);
    stateDirty = true;
    std::tie(s.error, s.errorText) = judge(i, false);
}

void MultiReader2DataManager::evaluateAll()
{
    for (SizeT i = 0; i < slots.size(); ++i)
        evaluate(i);
}

void MultiReader2DataManager::relateAll()
{
    const TickDomain* main = mainSlot ? (slots[*mainSlot].ticks ? &*slots[*mainSlot].ticks : nullptr) : nullptr;
    if (main)
        syncLimit = syncLimitTicks(*main);
    for (SizeT i = 0; i < slots.size(); ++i)
    {
        Slot& s = slots[i];
        if (mainSlot && *mainSlot == i)
            s.relation = s.ticks ? std::optional<Relation>(Relation{}) : std::nullopt;
        else
            s.relation = main && s.ticks ? relate(*s.ticks, *main) : std::nullopt;
    }
}

bool MultiReader2DataManager::contributing(SizeT i) const
{
    return valid() && slots[i].error == MultiReader2InputError::None;
}

bool MultiReader2DataManager::computeValid() const
{
    if (slots.empty())
        return true;
    if (pinnedMain && slots[*pinnedMain].error != MultiReader2InputError::None)
        return false;
    const auto healthy = [](const Slot& s) { return s.error == MultiReader2InputError::None; };
    if (errorPolicy == MultiReader2ErrorPolicy::Invalidate)
        return std::all_of(slots.begin(), slots.end(), healthy);
    return std::any_of(slots.begin(), slots.end(), healthy);
}

// The first input without an error; failing that, the first healthy on its own. Errors relative to the main input,
// SyncFailed and a rate other than its own, are cleared by a new main input, and a reader that lost its automatic main
// input with only such inputs left would otherwise have nothing to clear them against
std::optional<SizeT> MultiReader2DataManager::firstHealthy() const
{
    for (SizeT i = 0; i < slots.size(); ++i)
        if (slots[i].error == MultiReader2InputError::None)
            return i;
    for (SizeT i = 0; i < slots.size(); ++i)
        if (judge(i, true).first == MultiReader2InputError::None)
            return i;
    return std::nullopt;
}

// -------- Consumer side

SizeT MultiReader2DataManager::getAvailableCount()
{
    std::scoped_lock lock(mutex);
    if (pendingEventFields())
        return 0;
    const SizeT k = alignedCount();
    return k >= minReadCount ? k : 0;
}

ErrCode MultiReader2DataManager::read(void** data, SizeT* count, SizeT timeoutMs, SizeT* packetOffset, IMultiReader2Status** status)
{
    return doRead(data, false, true, count, timeoutMs, packetOffset, status);
}

ErrCode MultiReader2DataManager::readWithDomain(void** data, SizeT* count, SizeT timeoutMs, IMultiReader2Status** status)
{
    return doRead(data, true, true, count, timeoutMs, nullptr, status);
}

ErrCode MultiReader2DataManager::skipSamples(SizeT* count, SizeT timeoutMs, SizeT* packetOffset, IMultiReader2Status** status)
{
    return doRead(nullptr, false, false, count, timeoutMs, packetOffset, status);
}

ErrCode MultiReader2DataManager::doRead(
    void** data, bool withDomain, bool copy, SizeT* count, SizeT timeoutMs, SizeT* packetOffset, IMultiReader2Status** status)
{
    OPENDAQ_PARAM_NOT_NULL(count);
    OPENDAQ_PARAM_NOT_NULL(status);
    if (timeoutMs != 0)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTIMPLEMENTED, "A read with a timeout is not implemented");

    std::vector<Copy> copies;
    SampleType type;
    {
        std::scoped_lock lock(mutex);
        const SizeT requested = *count;
        const SizeT first = withDomain ? 1 : 0;
        if (requested > 0)
        {
            if (requested < minReadCount)
                return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "The count is below MinReadCount");
            if (copy && withDomain && (data == nullptr || data[0] == nullptr))
                return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "The read has no domain buffer");
            // Only the inputs contributing now: one that joins during the read changes the status, which stops the delivery
            for (SizeT i = 0; copy && i < slots.size(); ++i)
                if (contributing(i) && (data == nullptr || data[first + i] == nullptr))
                    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "A contributing input has no buffer");
        }
        copies.swap(spareCopies);
        type = readType;
        readLocked(data, withDomain, copy, count, packetOffset, status, &copies);
    }

    // The samples left the queues under the mutex; converting them outside it keeps a large read from holding up ingest
    for (const auto& c : copies)
        copySamples(c.packet, c.from, c.count, c.dst, type);
    if (copies.capacity() > 0)
    {
        copies.clear();
        std::scoped_lock lock(mutex);
        if (copies.capacity() > spareCopies.capacity())
            spareCopies.swap(copies);
    }
    return OPENDAQ_SUCCESS;
}

void MultiReader2DataManager::readLocked(
    void** data, bool withDomain, bool copy, SizeT* count, SizeT* packetOffset, IMultiReader2Status** status, std::vector<Copy>* copies)
{
    const SizeT requested = *count;
    const SizeT first = withDomain ? 1 : 0;
    // A status with changes is reported before any further sample: the samples after it belong to it
    bool owed = pendingEventFields();
    SizeT delivered = 0;
    std::optional<std::int64_t> firstTick;
    std::int64_t firstStart = 0;  // the main rule's start of the delivered samples, before any change this read applies
    for (;;)
    {
        if (phase == Phase::Streaming)
        {
            const SizeT k = alignedCount();
            const std::int64_t delta = mainTicks().delta;
            const bool drop = k > 0 && k < minReadCount && barrierBounded(k);
            const SizeT n = drop ? k : (!owed && (k >= minReadCount || delivered > 0) ? std::min(k, requested - delivered) : 0);
            if (n > 0)
            {
                for (SizeT i = 0; i < slots.size(); ++i)
                {
                    if (!contributing(i))
                        continue;
                    if (drop || !copy)
                    {
                        slots[i].queue.drop(n);
                        continue;
                    }
                    const SizeT size = slots[i].sampleSize;
                    auto* dst = static_cast<std::uint8_t*>(data[first + i]) + delivered * size;
                    slots[i].queue.consume(n,
                                           [&](const DataPacketPtr& packet, SizeT from, SizeT k2)
                                           {
                                               copies->push_back({packet, from, k2, dst});
                                               dst += k2 * size;
                                           });
                }
                if (!drop)
                {
                    if (!firstTick)
                    {
                        firstTick = readTick;
                        firstStart = mainTicks().start;
                    }
                    if (copy && withDomain)
                        for (SizeT j = 0; j < n; ++j)
                            static_cast<std::int64_t*>(data[0])[delivered + j] = readTick + static_cast<std::int64_t>(j) * delta;
                    delivered += n;
                }
                readTick += static_cast<std::int64_t>(n) * delta;
                if (!drop)
                    lastEnd = readTick;
            }
        }
        // Events reached with no data between them are applied together; a change stops the delivery
        if (!applyReached())
            break;
        owed = owed || pendingEventFields() || stateChanged();
        if (!owed && delivered == requested && alignedCount() > 0)
            break;
    }

    *count = delivered;
    if (packetOffset)
    {
        const std::int64_t start = mainSlot && slots[*mainSlot].ticks ? slots[*mainSlot].ticks->start : 0;
        // With nothing delivered, the next sample's position exists only while streaming
        *packetOffset = static_cast<SizeT>(firstTick ? *firstTick - firstStart : phase == Phase::Streaming ? readTick - start : 0);
    }
    buildStatus(status);
}

SizeT MultiReader2DataManager::alignedCount() const
{
    if (phase != Phase::Streaming)
        return 0;
    std::optional<SizeT> k;
    for (SizeT i = 0; i < slots.size(); ++i)
        if (contributing(i))
            k = std::min(k.value_or(slots[i].queue.frontCount()), slots[i].queue.frontCount());
    return k.value_or(0);
}

bool MultiReader2DataManager::barrierBounded(SizeT k) const
{
    for (SizeT i = 0; i < slots.size(); ++i)
        if (contributing(i) && slots[i].queue.frontCount() == k && slots[i].queue.hasEvent())
            return true;
    return false;
}

// Applies the event in front of each contributing input with no data before it, and every event of the other inputs
bool MultiReader2DataManager::applyReached()
{
    const bool wasValid = valid();
    const auto oldMain = mainSlot;
    bool applied = false;
    bool mainDomainChanged = false;
    for (SizeT i = 0; i < slots.size(); ++i)
    {
        Slot& s = slots[i];
        for (;;)
        {
            const bool contributes = wasValid && s.error == MultiReader2InputError::None;
            if (!contributes)
                s.queue.drop(s.queue.frontCount());
            if (!s.queue.frontIsEvent())
                break;
            const Event event = s.queue.popEvent();
            if (oldMain && *oldMain == i && !sameDescriptor(event.after.domain, s.current.domain))
                mainDomainChanged = true;
            applyEvent(i, event);
            applied = true;
            if (wasValid && s.error == MultiReader2InputError::None)
                break;
        }
    }
    if (applied)
        settle(wasValid, mainDomainChanged);
    return applied;
}

void MultiReader2DataManager::applyEvent(SizeT i, const Event& event)
{
    Slot& s = slots[i];
    const bool wasConnected = s.current.connected;
    const auto changed = s.apply(event);
    if (mainSlot && *mainSlot == i)
        relateAll();
    else if (changed.domainChanged)
    {
        const TickDomain* main = mainSlot && slots[*mainSlot].ticks ? &*slots[*mainSlot].ticks : nullptr;
        s.relation = main && s.ticks ? relate(*s.ticks, *main) : std::nullopt;
    }
    // A verdict may depend on every input's value descriptor, so a change to one judges the others again too
    const bool valuesMoved = changed.valueChanged || s.current.connected != wasConnected;
    const auto values = valuesMoved || changed.domainChanged ? valuesInForce() : nullptr;
    if (changed.valueChanged || changed.domainChanged || changed.connectedNow)
        ask(i, values);
    for (SizeT j = 0; valuesMoved && j < slots.size(); ++j)
        if (j != i)
            rejudge(j, values);

    const auto oldError = s.error;
    evaluate(i);
    if (s.error == MultiReader2InputError::None &&
        (changed.valueChanged || changed.connectedNow || oldError != MultiReader2InputError::None))
        s.descriptorChanged = true;
    if (s.error == MultiReader2InputError::None &&
        (oldError != MultiReader2InputError::None || changed.domainChanged || changed.connectedNow || event.gap))
        s.restartSync = true;
}

void MultiReader2DataManager::settle(bool wasValid, bool mainDomainChanged)
{
    bool mainMoved = false;
    if (!pinnedMain && (!mainSlot || slots[*mainSlot].error != MultiReader2InputError::None))
    {
        const auto candidate = firstHealthy();
        mainMoved = candidate.has_value() && candidate != mainSlot;
    }
    const bool nowValid = computeValid();
    if (wasValid != nowValid || (nowValid && (mainMoved || mainDomainChanged)))
    {
        internalReconfigure(!wasValid && nowValid);
        return;
    }
    if (!nowValid)
    {
        for (auto& s : slots)
            s.restartSync = false;
        return;
    }

    bool restart = false;
    for (SizeT i = 0; i < slots.size(); ++i)
    {
        Slot& s = slots[i];
        if (s.error != MultiReader2InputError::None)
            s.queue.drop(s.queue.frontCount());  // set aside: its data goes
        else if (s.restartSync)
            restart = true;
        s.restartSync = false;
    }
    if (restart)
        startSync(phase == Phase::Streaming);
    else if (phase == Phase::Synchronizing)
        trySync();
}

void MultiReader2DataManager::internalReconfigure(bool recovered)
{
    // Every queued event is applied first, so the last announced state is what is evaluated; the data goes
    for (SizeT i = 0; i < slots.size(); ++i)
    {
        Slot& s = slots[i];
        for (;;)
        {
            s.queue.drop(s.queue.frontCount());
            if (!s.queue.frontIsEvent())
                break;
            applyEvent(i, s.queue.popEvent());
        }
        s.restartSync = false;
    }

    if (!pinnedMain && (recovered || !mainSlot || slots[*mainSlot].error != MultiReader2InputError::None))
    {
        const auto candidate = firstHealthy();
        if (candidate && candidate != mainSlot)
        {
            // A new main input: every input is judged afresh against it
            mainSlot = candidate;
            if (loggerComponent.assigned())
            {
                LOG_T("Main input moved to {}", slots[*candidate].input.getLocalId().toStdString());
            }
            for (auto& s : slots)
            {
                s.syncFailed = SyncCause::None;
                s.late = Slot::Late::None;
            }
        }
    }
    relateAll();
    evaluateAll();
    restart();
}

// The judged inputs start afresh: every status field is reported again, and synchronization starts over
void MultiReader2DataManager::restart()
{
    phase = !computeValid() ? Phase::Invalid : mainSlot ? Phase::Synchronizing : Phase::Idle;
    for (auto& s : slots)
        s.descriptorChanged = true;
    owed.domain = owed.resync = true;
    lastEnd.reset();
    if (phase == Phase::Synchronizing)
        startSync(false);
}

// -------- Synchronization

// A synchronizing or streaming reader always has a main input with a valid domain
const TickDomain& MultiReader2DataManager::mainTicks() const
{
    assert(mainSlot && slots[*mainSlot].ticks);
    return *slots[*mainSlot].ticks;
}

void MultiReader2DataManager::startSync(bool fromStreaming)
{
    phase = Phase::Synchronizing;
    syncMainFirst.reset();
    if (loggerComponent.assigned())
    {
        LOG_T("Synchronizing{}, read tick {}", fromStreaming ? " again while streaming" : "", readTick);
    }
    if (fromStreaming)
    {
        // Other inputs never start behind the read position; only the main input's own jump back moves it back
        const std::int64_t delta = mainTicks().delta;
        // The main input's unread backlog arrived before the start and does not count toward the no-sample limit
        if (const auto newest = slots[*mainSlot].newestTick)
            syncMainFirst = *newest + delta;
        for (SizeT i = 0; i < slots.size(); ++i)
        {
            Slot& s = slots[i];
            if (i == *mainSlot || !contributing(i) || s.queue.frontCount() == 0)
                continue;
            const std::int64_t behind = readTick - s.mainTick(s.queue.firstOwn(s.ticks->delta));
            if (behind > 0)
                s.queue.drop(static_cast<SizeT>((behind + delta - 1) / delta));
        }
    }
    trySync();
}

// Checks the data in front of each input's first event; true when a read now has a start, a failure or an event to apply
bool MultiReader2DataManager::trySync()
{
    if (phase != Phase::Synchronizing || !mainSlot)
        return false;
    const SizeT main = *mainSlot;
    Slot& m = slots[main];
    if (m.queue.frontCount() == 0)
        return false;  // the main input waits for data or for a read to apply its event: nobody is judged
    const std::int64_t delta = mainTicks().delta;
    const std::int64_t mainFirst = m.mainTick(m.queue.firstOwn(delta));
    const std::int64_t latest = m.mainTick(m.queue.lastOwn(delta));
    if (!syncMainFirst)
        syncMainFirst = mainFirst;

    auto& judged = syncJudged;
    judged.clear();
    for (SizeT i = 0; i < slots.size(); ++i)
        if (i != main && contributing(i) && !slots[i].queue.frontIsEvent())
            judged.push_back(i);
    bool failed = false;
    // While synchronizing, nothing queued is aligned: a failure marker goes in front of the input's data
    const std::int64_t common = judgeSync(
        mainFirst,
        latest,
        latest - *syncMainFirst,
        syncLimit,
        judged,
        [&](SizeT i) { return slots[i].syncInput(mainFirst, delta); },
        [&](SizeT i, SyncCause cause, bool ahead)
        {
            slots[i].fail(cause, ahead, false);
            failed = true;
            if (loggerComponent.assigned())
            {
                LOG_T("{}: synchronization failed, {}, main input queued {} to {}, limit {}",
                      slots[i].input.getLocalId().toStdString(),
                      cause == SyncCause::OffGrid ? "off the grid"
                      : ahead                     ? "late and ahead"
                                                  : "late and behind",
                      mainFirst,
                      latest,
                      syncLimit);
            }
        });

    const auto first = [&](SizeT i) { return slots[i].mainTick(slots[i].queue.firstOwn(slots[i].ticks->delta)); };
    bool atStart = true;
    bool exposed = false;
    for (SizeT i = 0; i < slots.size(); ++i)
    {
        Slot& s = slots[i];
        if (!contributing(i) || s.queue.frontCount() == 0)
        {
            atStart = atStart && !contributing(i);
            continue;
        }
        const std::int64_t behind = common - first(i);
        if (behind > 0)
        {
            s.queue.drop(static_cast<SizeT>((behind + delta - 1) / delta));
            // An event the drop uncovered waits for a read, and no later packet would call one
            exposed = exposed || s.queue.frontIsEvent();
        }
        atStart = atStart && s.queue.frontCount() > 0 && first(i) == common;
    }
    if (!atStart)
        return failed || exposed;
    phase = Phase::Streaming;
    readTick = common;
    if (loggerComponent.assigned())
    {
        LOG_T("Streaming from main tick {}", common);
    }
    return true;
}

// -------- Ingest

std::optional<SizeT> MultiReader2DataManager::slotOf(const void* key)
{
    // Ingest hands over a port's packets in batches, so the previous slot is the usual answer
    if (lastSlot < slots.size() && slots[lastSlot].key == key)
        return lastSlot;
    for (SizeT i = 0; i < slots.size(); ++i)
        if (slots[i].key == key)
            return lastSlot = i;
    return std::nullopt;
}

bool MultiReader2DataManager::connected(const void* key, const Announced& announced, bool subscribed)
{
    std::scoped_lock lock(mutex);
    const auto i = slotOf(key);
    if (!i)
        return false;
    Announced after = announced;
    after.connected = true;
    return trigger(slots[*i].queueEvent(Event{slots[*i].latest, after}) && eventReached(*i), subscribed);
}

bool MultiReader2DataManager::disconnected(const void* key, bool subscribed)
{
    std::scoped_lock lock(mutex);
    const auto i = slotOf(key);
    if (!i)
        return false;
    Announced after = slots[*i].latest;
    after.connected = false;
    return trigger(slots[*i].queueEvent(Event{slots[*i].latest, after}) && eventReached(*i), subscribed);
}

bool MultiReader2DataManager::addPacket(const void* key, const PacketPtr& packet, bool subscribed)
{
    std::scoped_lock lock(mutex);
    const auto i = slotOf(key);
    if (!i)
        return false;
    if (packet.getType() == PacketType::Event)
        return trigger(ingestEvent(*i, packet.asPtr<IEventPacket>(true)), subscribed);
    const auto data = packet.asPtrOrNull<IDataPacket>(true);
    return trigger(data.assigned() && ingestData(*i, data), subscribed);
}

bool MultiReader2DataManager::ingestEvent(SizeT i, const EventPacketPtr& event)
{
    Slot& s = slots[i];
    const auto id = event.getEventId();
    if (id == event_packet_id::DATA_DESCRIPTOR_CHANGED)
    {
        const auto params = event.getParameters();
        Announced after = s.latest;
        const DataDescriptorPtr value = params.get(event_packet_param::DATA_DESCRIPTOR);
        const DataDescriptorPtr domain = params.get(event_packet_param::DOMAIN_DATA_DESCRIPTOR);
        if (value.assigned())
            after.value = value != NullDataDescriptor() ? value : nullptr;
        if (domain.assigned())
            after.domain = domain != NullDataDescriptor() ? domain : nullptr;
        return s.queueEvent(Event{s.latest, after}) && eventReached(i);
    }
    if (id == event_packet_id::IMPLICIT_DOMAIN_GAP_DETECTED)
        return s.queueGap() && eventReached(i);
    return false;
}

// Rules in order, each on what the earlier ones left: ticks, the memory limit, gap and queuing, late checks, synchronization
bool MultiReader2DataManager::ingestData(SizeT i, const DataPacketPtr& data)
{
    Slot& s = slots[i];
    const SizeT n = data.getSampleCount();
    const auto domainPacket = data.getDomainPacket();
    if (n == 0 || !domainPacket.assigned() || !s.latestTicks)
        return false;
    const auto offset = domainPacket.getOffset();
    if (!offset.assigned())
        return false;
    const TickDomain& own = *s.latestTicks;
    // A first tick outside Int64 cannot be aligned: the packet is dropped, and the next one in range is a jump
    const auto first = exactSum(offset.getIntValue(), own.start);
    if (!first)
        return false;
    const std::int64_t ownFirst = *first;
    const bool jump = s.nextOwn && *s.nextOwn != ownFirst;
    s.nextOwn = ownFirst + static_cast<std::int64_t>(n) * own.delta;
    const auto relation =
        s.track(ownFirst, n, mainSlot && *mainSlot == i, mainSlot && slots[*mainSlot].ticks ? &*slots[*mainSlot].ticks : nullptr);

    bool cause = false;
    const bool contributes = contributing(i);
    bool keep = contributes || s.queue.hasEvent();
    // Bounded memory behind a silent main input or a slow consumer: a packet past the limit is dropped, the next kept one is a gap
    const SizeT bytes = data.getRawDataSize() + domainPacket.getRawDataSize();
    if (keep && s.full(bytes))
    {
        if (!std::exchange(s.capped, true) && loggerComponent.assigned())
            LOG_W(
                "{}: more than {} MiB queued, newer samples are dropped", s.input.getLocalId().toStdString(), Slot::queueLimitBytes >> 20);
        keep = false;
    }
    // A gap matters against queued or streamed data and clears an off-grid input; the cap ends here whether or not the packet jumps
    else if ((std::exchange(s.capped, false) || jump) &&
             ((keep && (!s.queue.empty() || phase == Phase::Streaming)) || s.syncFailed == SyncCause::OffGrid))
    {
        cause = s.queueGap() && eventReached(i);
        keep = true;
    }
    if (keep)
        s.queue.pushData(data, ownFirst, n, bytes);

    if (relation && s.trackLate(*relation, ownFirst))
        cause = checkLate(i) || cause;
    // A jump or domain change queued on the main input puts its newest tick in a frame the other inputs have not reached
    if (mainSlot && *mainSlot == i && !s.queue.hasEvent())
        cause = checkAgainstMain() || cause;

    // Synchronization reaching the common start or failing always triggers; after a failed call, a queued clear does once more
    const bool synced = phase == Phase::Synchronizing && keep && contributes && trySync();
    const bool retry =
        rearmed && std::any_of(slots.begin(), slots.end(), [](const Slot& other) { return other.late == Slot::Late::ClearQueued; });
    rearmed = rearmed && !retry;
    return cause || synced || retry || (keep && readable());
}

// On the main input's ingest: late inputs within the limit again are cleared, and while streaming an input whose newest
// sample is more than the limit behind the main input's is late. One that stopped sending would otherwise hold back every
// read and every event queued behind data
bool MultiReader2DataManager::checkAgainstMain()
{
    bool cause = false;
    for (SizeT j = 0; j < slots.size(); ++j)
        cause = checkLate(j) || cause;
    if (phase != Phase::Streaming)
        return cause;
    const std::int64_t newest = *slots[*mainSlot].newestTick;
    for (SizeT j = 0; j < slots.size(); ++j)
    {
        const Slot& other = slots[j];
        if (j != *mainSlot && contributing(j) && !other.queue.hasEvent() && other.newestTick && newest - *other.newestTick > syncLimit)
        {
            slots[j].fail(SyncCause::Late, false, true);
            if (loggerComponent.assigned())
            {
                LOG_T("{}: late while streaming, {} main ticks behind, limit {}",
                      other.input.getLocalId().toStdString(),
                      newest - *other.newestTick,
                      syncLimit);
            }
            cause = true;
        }
    }
    return cause;
}

bool MultiReader2DataManager::checkLate(SizeT i)
{
    Slot& s = slots[i];
    if (!mainSlot || !slots[*mainSlot].newestTick || !s.lateWithin(*slots[*mainSlot].newestTick, syncLimit))
        return false;
    switch (s.rejoin())
    {
        case Slot::Rejoin::Withdrawn:
            return true;
        case Slot::Rejoin::Queued:
            return eventReached(i);
        default:
            return false;
    }
}

// -------- Triggers

bool MultiReader2DataManager::trigger(bool cause, bool subscribed)
{
    if (!cause)
        return false;
    ++triggers;
    if (!subscribed || callPending)
        return false;
    callPending = true;
    return true;
}

// Enough samples, or samples too few to read in front of an event, which a read drops
bool MultiReader2DataManager::readable() const
{
    const SizeT k = alignedCount();
    return k > 0 && (k >= minReadCount || barrierBounded(k));
}

// The event is applied by the next read: in front of the data, on a non-contributing input, or behind readable samples
bool MultiReader2DataManager::eventReached(SizeT i) const
{
    return !contributing(i) || slots[i].queue.frontIsEvent() || readable();
}

bool MultiReader2DataManager::hasTrigger() const
{
    return readable() || statusOwed();
}

bool MultiReader2DataManager::statusOwed() const
{
    if (pendingEventFields())
        return true;
    for (SizeT i = 0; i < slots.size(); ++i)
    {
        const Slot& s = slots[i];
        if (s.queue.frontIsEvent() || (!contributing(i) && s.queue.hasEvent()))
            return true;
    }
    return false;
}

std::uint64_t MultiReader2DataManager::triggerCount()
{
    std::scoped_lock lock(mutex);
    return triggers;
}

bool MultiReader2DataManager::callFinished(bool failed, std::uint64_t triggersAtStart, bool subscribed)
{
    std::scoped_lock lock(mutex);
    callPending = false;
    bool again;
    if (failed)
    {
        rearmed = true;
        again = triggers != triggersAtStart;
    }
    else
        again = hasTrigger();
    if (!again || !subscribed)
        return false;
    callPending = true;
    return true;
}

bool MultiReader2DataManager::inCallback() const
{
    return callbackThread.load() == std::this_thread::get_id();
}

// -------- Status

// What the next status reports as changed, apart from a moved state. In error only a new configuration is a change
MultiReader2DataManager::Changes MultiReader2DataManager::pendingChanges() const
{
    Changes c;
    c.force = owed.force;
    if (!slots.empty() && !valid())
        return c;
    const bool domain = mainSlot && slots[*mainSlot].current.domain.assigned();
    c.domain = domain && owed.domain;
    c.resync = domain && (owed.resync || (lastEnd && readTick != *lastEnd));
    c.descriptors = std::any_of(slots.begin(), slots.end(), [this](const Slot& s) { return descriptorReported(s); });
    return c;
}

bool MultiReader2DataManager::descriptorReported(const Slot& s) const
{
    return valid() && s.error == MultiReader2InputError::None && s.descriptorChanged;
}

// A pending change, or a queued clear whose rejoin the status owes, holds back further samples
bool MultiReader2DataManager::pendingEventFields() const
{
    return pendingChanges().any() ||
           std::any_of(slots.begin(), slots.end(), [](const Slot& s) { return s.late == Slot::Late::ClearQueued; });
}

bool MultiReader2DataManager::stateChanged() const
{
    return !lastState || (stateDirty && !(captureState() == *lastState));
}

MultiReader2DataManager::State MultiReader2DataManager::captureState() const
{
    State st;
    st.inputs.reserve(slots.size());
    const bool anyError = std::any_of(slots.begin(), slots.end(), [](const Slot& s) { return s.error != MultiReader2InputError::None; });
    st.level = slots.empty() ? ComponentStatus::Ok
               : !valid()    ? ComponentStatus::Error
               : anyError    ? ComponentStatus::Warning
                             : ComponentStatus::Ok;
    const bool error = st.level == ComponentStatus::Error;
    for (const auto& s : slots)
    {
        if (s.error != MultiReader2InputError::None)
            st.message += (st.message.empty() ? "" : "; ") + s.input.getLocalId().toStdString() + ": " + s.errorText;
        const bool healthy = !error && s.error == MultiReader2InputError::None;
        st.inputs.push_back({s.error, healthy ? s.current.value : nullptr, healthy ? s.sampleSize : 0});
    }
    if (mainSlot && (!error || pinnedMain))
        st.main = slots[*mainSlot].input;
    if (mainSlot && !error)
        st.domain = slots[*mainSlot].current.domain;
    return st;
}

void MultiReader2DataManager::buildStatus(IMultiReader2Status** status)
{
    // The state is captured again only after a judgement, so a streaming read allocates nothing for it
    bool moved = false;
    if (stateDirty || !lastState)
    {
        State captured = captureState();
        moved = !lastState || !(captured == *lastState);
        lastState = std::move(captured);
        stateDirty = false;
    }
    const State& st = *lastState;
    const Changes pending = pendingChanges();
    const bool changes = pending.any() || moved;
    if (pending.resync || st.level == ComponentStatus::Error)
        lastEnd.reset();
    owed = {};

    // A read without changes returns the previous status object, so the streaming path allocates no status
    const bool synchronizing = phase == Phase::Synchronizing;
    if (!changes && lastUnchanged.assigned() && static_cast<bool>(lastUnchanged.getSynchronizing()) == synchronizing)
    {
        for (auto& s : slots)
            s.descriptorChanged = false;
        *status = lastUnchanged.addRefAndReturn();
        return;
    }
    MultiReader2StatusImpl::Fields f;
    f.hasChanges = changes;
    f.domainDescriptorChanged = pending.domain;
    f.resynchronized = pending.resync;
    f.synchronizing = synchronizing;
    for (SizeT i = 0; i < slots.size(); ++i)
    {
        const auto& in = st.inputs[i];
        f.inputs.pushBack(createWithImplementation<IMultiReader2InputStatus, MultiReader2InputStatusImpl>(
            slots[i].input, in.error, descriptorReported(slots[i]), in.descriptor, in.sampleSize));
        slots[i].descriptorChanged = false;
    }
    f.status = st.level;
    f.message = st.message;
    f.domainDescriptor = st.domain;
    f.mainInput = st.main;
    auto created = createWithImplementation<IMultiReader2Status, MultiReader2StatusImpl>(std::move(f));
    lastUnchanged = changes ? nullptr : created;
    *status = created.detach();
}

END_NAMESPACE_OPENDAQ
