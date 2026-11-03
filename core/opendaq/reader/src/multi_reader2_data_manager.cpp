#include <coretypes/validation.h>
#include <opendaq/custom_log.h>
#include <opendaq/data_descriptor_factory.h>
#include <opendaq/data_packet_ptr.h>
#include <opendaq/event_packet_ids.h>
#include <opendaq/event_packet_params.h>
#include <opendaq/event_packet_ptr.h>
#include <opendaq/multi_reader2_convert.h>
#include <opendaq/multi_reader2_data_manager.h>
#include <opendaq/multi_reader2_stall.h>
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
    apply(std::move(config));
}

void MultiReader2DataManager::configure(Config config, IMultiReader2Status** status)
{
    std::scoped_lock lock(mutex);
    apply(std::move(config));
    SizeT none = 0;
    readLocked(nullptr, nullptr, false, &none, nullptr, status, nullptr);
}

void MultiReader2DataManager::evaluate(IMultiReader2Status** status)
{
    std::scoped_lock lock(mutex);
    const bool wasValid = valid();
    for (SizeT i = 0; i < slots.size(); ++i)
        rejudge(i);
    settle(wasValid, false);
    SizeT none = 0;
    readLocked(nullptr, nullptr, false, &none, nullptr, status, nullptr);
}

void MultiReader2DataManager::apply(Config config)
{
    loggerComponent = config.loggerComponent;
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

    for (SizeT i = 0; i < slots.size(); ++i)
        ask(i);
    mainSlot = pinnedMain;
    if (!mainSlot)
    {
        relateAll();
        evaluateInputs();
        mainSlot = firstHealthy();
    }
    relateAll();
    evaluateInputs();
    stateDirty = true;
    restart();
}

// -------- Judgement

void MultiReader2DataManager::ask(SizeT i)
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
        const BaseObjectPtr verdict = acceptsDescriptor.call(s.input, s.current.value);
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
void MultiReader2DataManager::rejudge(SizeT i)
{
    Slot& s = slots[i];
    const bool before = s.accepted;
    const auto oldError = s.error;
    ask(i);
    if (s.accepted == before)
        return;
    evaluateInput(i);
    if (s.error == MultiReader2InputError::None && oldError != MultiReader2InputError::None)
        s.restartSync = true;
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
    // A stall is the input's own; the other causes are relative to the main input
    if (s.syncFailed == SyncCause::Stalled)
        return {MultiReader2InputError::SyncFailed, stalledText()};
    if (!alone && s.syncFailed == SyncCause::Ahead)
        return {MultiReader2InputError::SyncFailed,
                fmt::format("not synchronized, more than {} s ahead of the main input", queueLimitSeconds)};
    if (!alone && s.syncFailed == SyncCause::OffGrid)
        return {MultiReader2InputError::SyncFailed, "not synchronized, off the main input's grid"};
    return {MultiReader2InputError::None, {}};
}

void MultiReader2DataManager::evaluateInput(SizeT i)
{
    Slot& s = slots[i];
    s.sampleSize = readSampleSize(s.current.value, readType);
    stateDirty = true;
    std::tie(s.error, s.errorText) = judge(i, false);
}

void MultiReader2DataManager::evaluateInputs()
{
    for (SizeT i = 0; i < slots.size(); ++i)
        evaluateInput(i);
}

void MultiReader2DataManager::relateAll()
{
    const TickDomain* main = mainSlot ? (slots[*mainSlot].ticks ? &*slots[*mainSlot].ticks : nullptr) : nullptr;
    if (main)
        queueLimit = secondsToTicks(*main, queueLimitSeconds);
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
    if (changesDue())
        return 0;
    const SizeT k = alignedCount();
    return k >= minReadCount ? k : 0;
}

ErrCode MultiReader2DataManager::read(void** data, std::int64_t* domain, SizeT* count, SizeT* packetOffset, IMultiReader2Status** status)
{
    return doRead(data, domain, true, count, packetOffset, status);
}

ErrCode MultiReader2DataManager::skipSamples(SizeT* count, SizeT* packetOffset, IMultiReader2Status** status)
{
    return doRead(nullptr, nullptr, false, count, packetOffset, status);
}

ErrCode MultiReader2DataManager::doRead(
    void** data, std::int64_t* domain, bool copy, SizeT* count, SizeT* packetOffset, IMultiReader2Status** status)
{
    OPENDAQ_PARAM_NOT_NULL(count);
    OPENDAQ_PARAM_NOT_NULL(status);

    std::vector<Copy> copies;
    SampleType type;
    {
        std::scoped_lock lock(mutex);
        const SizeT requested = *count;
        if (requested > 0)
        {
            if (requested < minReadCount)
                return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "The count is below MinReadCount");
            // Only the inputs contributing now: one that joins during the read changes the status, which stops the delivery
            for (SizeT i = 0; copy && i < slots.size(); ++i)
                if (contributing(i) && (data == nullptr || data[i] == nullptr))
                    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "A contributing input has no buffer");
        }
        copies.swap(spareCopies);
        type = readType;
        readLocked(data, domain, copy, count, packetOffset, status, &copies);
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

void MultiReader2DataManager::readLocked(void** data,
                                         std::int64_t* domain,
                                         bool copy,
                                         SizeT* count,
                                         SizeT* packetOffset,
                                         IMultiReader2Status** status,
                                         std::vector<Copy>* copies)
{
    applyStalls();
    const SizeT requested = *count;
    // A status with changes is reported before any further sample: the samples after it belong to it
    bool due = changesDue();
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
            const SizeT n = drop ? k : (!due && (k >= minReadCount || delivered > 0) ? std::min(k, requested - delivered) : 0);
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
                    auto* dst = static_cast<std::uint8_t*>(data[i]) + delivered * size;
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
                    if (copy && domain)
                        for (SizeT j = 0; j < n; ++j)
                            domain[delivered + j] = readTick + static_cast<std::int64_t>(j) * delta;
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
        due = due || changesDue();
        if (!due && delivered == requested && alignedCount() > 0)
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

// Stalls the inputs the queue limit waits on, then judges as after an event
void MultiReader2DataManager::applyStalls()
{
    if (!mainSlot || !slots[*mainSlot].ticks || !stallDue(slots, valid(), *slots[*mainSlot].ticks))
        return;
    const bool wasValid = valid();
    stall(slots, wasValid, *slots[*mainSlot].ticks, loggerComponent);
    evaluateInputs();
    settle(wasValid, false);
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
    const auto changed = s.apply(event);
    if (mainSlot && *mainSlot == i)
        relateAll();
    else if (changed.domainChanged)
    {
        const TickDomain* main = mainSlot && slots[*mainSlot].ticks ? &*slots[*mainSlot].ticks : nullptr;
        s.relation = main && s.ticks ? relate(*s.ticks, *main) : std::nullopt;
    }
    if (changed.valueChanged || changed.connectedNow)
        ask(i);

    const auto oldError = s.error;
    evaluateInput(i);
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
                if (s.syncFailed != SyncCause::Stalled)
                    s.syncFailed = SyncCause::None;
        }
    }
    relateAll();
    evaluateInputs();
    restart();
}

// The judged inputs start afresh: every status field is reported again, and synchronization starts over
void MultiReader2DataManager::restart()
{
    phase = !computeValid() ? Phase::Invalid : mainSlot ? Phase::Synchronizing : Phase::Idle;
    afresh = true;
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
    if (loggerComponent.assigned())
    {
        LOG_T("Synchronizing{}, read tick {}", fromStreaming ? " again while streaming" : "", readTick);
    }
    if (fromStreaming)
    {
        // Other inputs never start behind the read position; only the main input's own jump back moves it back
        const std::int64_t delta = mainTicks().delta;
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

// Checks the data in front of each input's first event: finds the common start, or fails the inputs that cannot reach it
void MultiReader2DataManager::trySync()
{
    if (phase != Phase::Synchronizing || !mainSlot)
        return;
    const SizeT main = *mainSlot;
    Slot& m = slots[main];
    if (m.queue.frontCount() == 0)
        return;  // the main input waits for data or for a read to apply its event: nobody is judged
    const std::int64_t delta = mainTicks().delta;
    const std::int64_t mainFirst = m.mainTick(m.queue.firstOwn(delta));
    const std::int64_t latest = m.mainTick(m.queue.lastOwn(delta));

    auto& judged = syncJudged;
    judged.clear();
    for (SizeT i = 0; i < slots.size(); ++i)
        if (i != main && contributing(i) && !slots[i].queue.frontIsEvent())
            judged.push_back(i);
    // While synchronizing, nothing queued is aligned: a failure marker goes in front of the input's data
    const std::int64_t common = judgeSync(
        mainFirst,
        latest,
        queueLimit,
        judged,
        [&](SizeT i) { return slots[i].syncInput(mainFirst, delta); },
        [&](SizeT i, SyncCause cause)
        {
            slots[i].fail(cause);
            if (loggerComponent.assigned())
            {
                LOG_T("{}: synchronization failed, {}, main input queued {} to {}, limit {}",
                      slots[i].input.getLocalId().toStdString(),
                      cause == SyncCause::OffGrid ? "off the grid" : "ahead",
                      mainFirst,
                      latest,
                      queueLimit);
            }
        });

    const auto first = [&](SizeT i) { return slots[i].mainTick(slots[i].queue.firstOwn(slots[i].ticks->delta)); };
    bool atStart = true;
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
            s.queue.drop(static_cast<SizeT>((behind + delta - 1) / delta));
        atStart = atStart && s.queue.frontCount() > 0 && first(i) == common;
    }
    if (!atStart)
        return;
    phase = Phase::Streaming;
    readTick = common;
    if (loggerComponent.assigned())
    {
        LOG_T("Streaming from main tick {}", common);
    }
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
    slots[*i].queueEvent(Event{slots[*i].latest, after});
    return wake(subscribed);
}

bool MultiReader2DataManager::disconnected(const void* key, bool subscribed)
{
    std::scoped_lock lock(mutex);
    const auto i = slotOf(key);
    if (!i)
        return false;
    Announced after = slots[*i].latest;
    after.connected = false;
    slots[*i].queueEvent(Event{slots[*i].latest, after});
    return wake(subscribed);
}

bool MultiReader2DataManager::addPacket(const void* key, const PacketPtr& packet, bool subscribed)
{
    std::scoped_lock lock(mutex);
    const auto i = slotOf(key);
    if (!i)
        return false;
    try
    {
        if (packet.getType() == PacketType::Event)
            ingestEvent(*i, packet.asPtr<IEventPacket>(true));
        else if (const auto data = packet.asPtrOrNull<IDataPacket>(true); data.assigned())
            ingestData(*i, data);
    }
    catch (const std::exception& e)
    {
        // A packet that cannot be ingested is lost; the gap has the next read report the loss as a resynchronization
        if (loggerComponent.assigned())
            LOG_W("{}: a packet could not be ingested and is dropped: {}", slots[*i].input.getLocalId().toStdString(), e.what());
        slots[*i].queueGap();
    }
    return wake(subscribed);
}

void MultiReader2DataManager::ingestEvent(SizeT i, const EventPacketPtr& event)
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
        s.queueEvent(Event{s.latest, after});
    }
    else if (id == event_packet_id::IMPLICIT_DOMAIN_GAP_DETECTED)
        s.queueGap();
}

// Rules in order, each on what the earlier ones left: ticks, the queue limits, gap and queuing, rejoin, synchronization
void MultiReader2DataManager::ingestData(SizeT i, const DataPacketPtr& data)
{
    Slot& s = slots[i];
    const SizeT n = data.getSampleCount();
    const auto domainPacket = data.getDomainPacket();
    if (n == 0 || !domainPacket.assigned() || !s.latestTicks)
        return;
    const auto offset = domainPacket.getOffset();
    if (!offset.assigned())
        return;
    const TickDomain& own = *s.latestTicks;
    // A first tick outside Int64 cannot be aligned: the packet is dropped, and the next one in range is a jump
    const auto first = exactSum(offset.getIntValue(), own.start);
    if (!first)
        return;
    const std::int64_t ownFirst = *first;
    const bool jump = s.nextOwn && *s.nextOwn != ownFirst;
    s.nextOwn = ownFirst + static_cast<std::int64_t>(n) * own.delta;
    const TickDomain* main = mainSlot && slots[*mainSlot].ticks ? &*slots[*mainSlot].ticks : nullptr;
    s.track(ownFirst, n, mainSlot && *mainSlot == i, main);

    const bool contributes = contributing(i);
    bool keep = contributes || s.queue.hasEvent();
    // The queue holds at most queueLimitSeconds of the input's data. While another input has nothing queued it keeps it, and
    // the next read stalls that input. While synchronizing nothing queued is aligned: the oldest packets go, as
    // synchronization trims them. Otherwise the queued data goes and a gap takes its place, so reads go on from the newest data
    const SizeT bytes = data.getRawDataSize() + domainPacket.getRawDataSize();
    const bool limited = keep && s.full(n) && !stallPending(slots, valid());
    while (limited && phase == Phase::Synchronizing && s.full(n) && s.queue.dropOldestPacket())
    {
    }
    if (limited && s.full(n))
    {
        if (loggerComponent.assigned())
            LOG_W("{}: more than {} s queued, the queued samples are dropped", s.input.getLocalId().toStdString(), queueLimitSeconds);
        s.queue.dropData();
        s.queueGap();
    }
    // A gap matters against queued or streamed data and clears an off-grid input
    else if (jump && ((keep && (!s.queue.empty() || phase == Phase::Streaming)) || s.syncFailed == SyncCause::OffGrid))
    {
        s.queueGap();
        keep = true;
    }
    if (keep)
    {
        keepWithinBudget(bytes);
        s.queue.pushData(data, ownFirst, n, bytes);
    }

    if (main)
        checkRejoin(slots, i, *main, loggerComponent);
    if (phase == Phase::Synchronizing && keep && contributes)
        trySync();
}

// The queues of all inputs hold at most queueBudgetBytesPerInput per input together. A packet that would pass it takes the
// data of the input holding the most, by the same rule as the time limit: its oldest packets while synchronizing, else all of
// it and a gap
void MultiReader2DataManager::keepWithinBudget(SizeT bytes)
{
    const SizeT budget = queueBudgetBytesPerInput * slots.size();
    const auto heldNow = [this]
    {
        SizeT sum = 0;
        for (const auto& s : slots)
            sum += s.queue.heldBytes();
        return sum;
    };
    // Summing every queue is needed only once the bound passes the budget; reads and drops leave the bound above the real bytes
    assert(heldNow() <= heldBound);
    if (heldBound + bytes <= budget)
    {
        heldBound += bytes;
        return;
    }
    SizeT held = heldNow();
    while (held + bytes > budget)
    {
        Slot& s = *std::max_element(
            slots.begin(), slots.end(), [](const Slot& a, const Slot& b) { return a.queue.heldBytes() < b.queue.heldBytes(); });
        const SizeT before = s.queue.heldBytes();
        if (before == 0)
            break;
        if (phase != Phase::Synchronizing || !s.queue.dropOldestPacket())
        {
            if (loggerComponent.assigned())
                LOG_W("{}: the inputs hold more than {} MB, its queued samples are dropped",
                      s.input.getLocalId().toStdString(),
                      budget / 1000000);
            s.queue.dropData();
            s.queueGap();
        }
        held -= before - s.queue.heldBytes();
    }
    heldBound = held + bytes;
}

// -------- Triggers

// Schedules a call while subscribed with none pending, when a read would return samples or a status with changes
bool MultiReader2DataManager::wake(bool subscribed)
{
    if (!subscribed || callPending || !hasTrigger())
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

bool MultiReader2DataManager::hasTrigger() const
{
    return readable() || statusOwed();
}

bool MultiReader2DataManager::statusOwed() const
{
    if (changesDue() || (mainSlot && slots[*mainSlot].ticks && stallDue(slots, valid(), *slots[*mainSlot].ticks)))
        return true;
    for (SizeT i = 0; i < slots.size(); ++i)
    {
        const Slot& s = slots[i];
        if (s.queue.frontIsEvent() || (!contributing(i) && s.queue.hasEvent()))
            return true;
    }
    return false;
}

bool MultiReader2DataManager::callFinished(bool failed, bool subscribed)
{
    std::scoped_lock lock(mutex);
    callPending = false;
    // A failed call is not repeated at once: the next ingest schedules it again
    return !failed && wake(subscribed);
}

bool MultiReader2DataManager::inCallback() const
{
    return callbackThread.load() == std::this_thread::get_id();
}

// -------- Status

// In error only a restart and a moved state are changes
MultiReader2DataManager::Changes MultiReader2DataManager::changes(const State* now) const
{
    Changes c;
    c.afresh = afresh;
    c.state = now && !(*now == reported);
    if (!slots.empty() && !valid())
        return c;
    const bool domain = mainSlot && slots[*mainSlot].current.domain.assigned();
    c.domain = domain && (afresh || !sameDescriptor(slots[*mainSlot].current.domain, reported.domain));
    c.resync = domain && (afresh || (lastEnd && readTick != *lastEnd));
    for (SizeT i = 0; i < slots.size() && !c.descriptors; ++i)
        c.descriptors = descriptorReported(i);
    return c;
}

// Healthy, and the previous status gave none or another descriptor
bool MultiReader2DataManager::descriptorReported(SizeT i) const
{
    const Slot& s = slots[i];
    return valid() && s.error == MultiReader2InputError::None &&
           (afresh || i >= reported.inputs.size() || !sameDescriptor(s.current.value, reported.inputs[i].descriptor));
}

// The state moves only by a judgement
std::optional<MultiReader2DataManager::State> MultiReader2DataManager::stateNow() const
{
    return stateDirty ? std::optional<State>(captureState()) : std::nullopt;
}

// A status with changes holds back further samples
bool MultiReader2DataManager::changesDue() const
{
    const auto now = stateNow();
    return changes(now ? &*now : nullptr).any();
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
    auto now = stateNow();
    const Changes c = changes(now ? &*now : nullptr);
    const bool changed = c.any();

    // A read without changes returns the previous status object, so the streaming path allocates no status
    MultiReader2StatusPtr created;
    if (!changed && lastUnchanged.assigned())
        created = lastUnchanged;
    else
    {
        const State& st = now ? *now : reported;
        MultiReader2StatusImpl::Fields f;
        f.hasChanges = changed;
        f.domainDescriptorChanged = c.domain;
        f.resynchronized = c.resync;
        for (SizeT i = 0; i < slots.size(); ++i)
        {
            const auto& in = st.inputs[i];
            f.inputs.pushBack(createWithImplementation<IMultiReader2InputStatus, MultiReader2InputStatusImpl>(
                slots[i].input, in.error, c.descriptors && descriptorReported(i), in.descriptor, in.sampleSize));
        }
        f.status = st.level;
        f.message = st.message;
        f.domainDescriptor = st.domain;
        f.mainInput = st.main;
        created = createWithImplementation<IMultiReader2Status, MultiReader2StatusImpl>(std::move(f));
        lastUnchanged = changed ? nullptr : created;
    }

    // What this status reported is what the next one compares with
    if (now)
        reported = std::move(*now);
    stateDirty = false;
    afresh = false;
    if (c.resync || reported.level == ComponentStatus::Error)
        lastEnd.reset();
    *status = created.detach();
}

END_NAMESPACE_OPENDAQ
