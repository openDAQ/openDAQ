#include <coretypes/dictobject_factory.h>
#include <coretypes/integer_factory.h>
#include <coretypes/validation.h>
#include <opendaq/data_packet_ptr.h>
#include <opendaq/event_packet_utils.h>
#include <opendaq/multi_reader2_status_impl.h>
#include <opendaq/multi_reader_data_manager.h>

#include <algorithm>
#include <cassert>
#include <limits>

BEGIN_NAMESPACE_OPENDAQ

MultiReaderDataManager::SpscPacketQueue::SpscPacketQueue()
    : head(new Node())
    , tail(head)
{
}

MultiReaderDataManager::SpscPacketQueue::~SpscPacketQueue()
{
    while (Node* node = head)
    {
        head = node->next.load(std::memory_order_relaxed);
        delete node;
    }
}

void MultiReaderDataManager::SpscPacketQueue::push(PacketPtr packet)
{
    auto node = new Node();
    node->packet = std::move(packet);
    tail->next.store(node, std::memory_order_release);
    tail = node;
}

PacketPtr MultiReaderDataManager::SpscPacketQueue::pop()
{
    Node* next = head->next.load(std::memory_order_acquire);
    if (!next)
        return nullptr;

    PacketPtr packet = std::move(next->packet);
    delete head;
    head = next;
    return packet;
}

MultiReaderDataManager::State::State(SizeT slotCount)
{
    slots.reserve(slotCount);
    for (SizeT i = 0; i < slotCount; i++)
        slots.push_back(std::make_unique<SlotCell>());

    const auto fullMask = slotCount >= 64 ? ~uint64_t(0) : (uint64_t(1) << slotCount) - 1;
    usedMask.store(fullMask, std::memory_order_relaxed);
    connectedMask.store(fullMask, std::memory_order_relaxed);
}

MultiReaderDataManager::State::~State()
{
    for (auto& slot : slots)
    {
        if (IPacket* packet = slot->lastEventPacket.exchange(nullptr, std::memory_order_acquire))
            packet->releaseRef();
    }
}

bool MultiReaderDataManager::deliverable(const State& state)
{
    if (state.parked.load(std::memory_order_acquire))
        return false;

    // A disconnected used input silences everything until all used inputs are connected again
    const auto usedForConnect = state.usedMask.load(std::memory_order_acquire);
    if ((state.connectedMask.load(std::memory_order_acquire) & usedForConnect) != usedForConnect)
        return false;

    if (state.pendingEvents.load(std::memory_order_acquire) > 0)
        return true;

    // An inactive reader delivers only events
    if (!state.active.load(std::memory_order_acquire))
        return false;

    const auto used = state.usedMask.load(std::memory_order_acquire);
    return used != 0 && (state.readyMask.load(std::memory_order_acquire) & used) == used;
}

bool MultiReaderDataManager::matchesDescriptor(const DataDescriptorPtr& committed, const PacketPtr& packet)
{
    const auto dataPacket = packet.asPtrOrNull<IDataPacket>(true);
    if (!dataPacket.assigned() || !committed.assigned())
        return false;

    const auto descriptor = dataPacket.getDataDescriptor();
    // Pointer equality is the common case: signals reuse one descriptor object until it changes
    if (static_cast<IDataDescriptor*>(descriptor) == static_cast<IDataDescriptor*>(committed))
        return true;
    return descriptor.assigned() && committed == descriptor;
}

bool MultiReaderDataManager::convertibleValue(const DataDescriptorPtr& descriptor)
{
    if (!descriptor.assigned())
        return false;

    try
    {
        switch (descriptor.getSampleType())
        {
            case SampleType::Float32:
            case SampleType::Float64:
            case SampleType::Int8:
            case SampleType::Int16:
            case SampleType::Int32:
            case SampleType::Int64:
            case SampleType::UInt8:
            case SampleType::UInt16:
            case SampleType::UInt32:
            case SampleType::UInt64:
                break;
            default:
                return false;
        }
        const auto dimensions = descriptor.getDimensions();
        return !dimensions.assigned() || dimensions.getCount() == 0;
    }
    catch (...)
    {
        return false;
    }
}

void MultiReaderDataManager::reconfigure(Config config)
{
    std::scoped_lock lock(consumerMutex);

    // The masks are single words for now; widen to word arrays past 64 inputs
    assert(config.inputIds.size() <= 64);

    this->config = std::move(config);

    auto newState = std::make_shared<State>(this->config.inputIds.size());
    if (!this->config.usedFlags.empty())
    {
        uint64_t mask = 0;
        for (SizeT i = 0; i < this->config.usedFlags.size(); i++)
        {
            if (this->config.usedFlags[i])
                mask |= uint64_t(1) << i;
        }
        newState->usedMask.store(mask, std::memory_order_relaxed);
    }
    if (!this->config.connectedFlags.empty())
    {
        uint64_t mask = 0;
        for (SizeT i = 0; i < this->config.connectedFlags.size(); i++)
        {
            if (this->config.connectedFlags[i])
                mask |= uint64_t(1) << i;
        }
        newState->connectedMask.store(mask, std::memory_order_relaxed);
    }
    syncDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    newState->syncing.store(true, std::memory_order_relaxed);
    newState->syncDeadlineTicks.store(syncDeadline.time_since_epoch().count(), std::memory_order_relaxed);
    std::atomic_store(&state, std::move(newState));

    views.assign(this->config.inputIds.size(), SlotView{});
    pendingStatus = nullptr;
    reportedDisconnectMask = 0;
    synced = false;
    syncStarted = false;
    nextReadTick = 0;
    mainIndex = 0;
    for (SizeT i = 0; i < this->config.inputIds.size(); i++)
    {
        if (this->config.inputIds[i] == this->config.mainInputId)
            mainIndex = i;
    }
}

void MultiReaderDataManager::clear()
{
    std::scoped_lock lock(consumerMutex);

    const auto current = std::atomic_load(&state);
    if (!current)
        return;

    // Fresh state block drops the queues; used, connected, and active flags survive a clear
    auto fresh = std::make_shared<State>(current->slots.size());
    fresh->usedMask.store(current->usedMask.load(std::memory_order_acquire), std::memory_order_relaxed);
    fresh->connectedMask.store(current->connectedMask.load(std::memory_order_acquire), std::memory_order_relaxed);
    fresh->active.store(current->active.load(std::memory_order_acquire), std::memory_order_relaxed);
    std::atomic_store(&state, std::move(fresh));

    for (auto& view : views)
    {
        view.staged.clear();
        view.stagedSamples = 0;
        view.frontOffset = 0;
        view.deliveredVersion = 0;
    }
    pendingStatus = nullptr;
}

void MultiReaderDataManager::drainSlots(State& state)
{
    for (SizeT i = 0; i < state.slots.size(); i++)
    {
        auto& cell = *state.slots[i];
        auto& view = views[i];

        SizeT moved = 0;
        for (;;)
        {
            auto packet = cell.queue.pop();
            if (!packet.assigned())
                break;
            moved++;

            // A failed input sits out until a new descriptor arrives; its data would only pile up
            if (view.failed)
                continue;

            if (matchesDescriptor(view.valueDescriptor, packet))
                view.stagedSamples += packet.asPtrOrNull<IDataPacket>(true).getSampleCount();
            view.staged.push_back(std::move(packet));
        }
        // The ready bit means fresh queued data: consuming the queue lowers it again
        if (moved > 0)
        {
            cell.dataPacketCount.fetch_sub(moved, std::memory_order_acq_rel);
            settleReadyBit(state, i);
        }
    }
}

void MultiReaderDataManager::settleReadyBit(State& state, SizeT index)
{
    auto& cell = *state.slots[index];
    if (cell.dataPacketCount.load(std::memory_order_acquire) > 0)
        return;

    const auto bit = uint64_t(1) << index;
    state.readyMask.fetch_and(~bit, std::memory_order_acq_rel);
    // A producer may have pushed between the check and the clear; its 0->1 edge was consumed by us
    if (cell.dataPacketCount.load(std::memory_order_acquire) > 0)
        state.readyMask.fetch_or(bit, std::memory_order_acq_rel);
}

void MultiReaderDataManager::discardSlotData(State& state, SizeT index)
{
    views[index].staged.clear();
    views[index].stagedSamples = 0;
    views[index].frontOffset = 0;
    settleReadyBit(state, index);
}

void MultiReaderDataManager::discardAllData(State& state)
{
    drainSlots(state);
    for (SizeT i = 0; i < state.slots.size(); i++)
        discardSlotData(state, i);
}

// Extracts the pinned domain constraints: integer samples, seconds unit, implicit linear rule
void MultiReaderDataManager::parseDomain(SlotView& view)
{
    view.domainValid = false;
    view.domainError = MultiReader2InputError::InvalidDescriptor;

    const auto& descriptor = view.domainDescriptor;
    if (!descriptor.assigned())
        return;

    view.domainError = MultiReader2InputError::InvalidDomain;
    try
    {
        switch (descriptor.getSampleType())
        {
            case SampleType::Int8:
            case SampleType::Int16:
            case SampleType::Int32:
            case SampleType::Int64:
            case SampleType::UInt8:
            case SampleType::UInt16:
            case SampleType::UInt32:
            case SampleType::UInt64:
                break;
            default:
                return;
        }

        const auto unit = descriptor.getUnit();
        if (!unit.assigned() || unit.getQuantity() != "time" || unit.getSymbol() != "s")
            return;

        const auto rule = descriptor.getRule();
        if (!rule.assigned() || rule.getType() != DataRuleType::Linear)
            return;
        const Float rawDelta = rule.getParameters().get("delta");
        view.delta = static_cast<Int>(rawDelta);
        if (view.delta <= 0 || static_cast<Float>(view.delta) != rawDelta)
            return;

        const auto resolution = descriptor.getTickResolution();
        view.resolutionNum = resolution.assigned() ? static_cast<Int>(resolution.getNumerator()) : 1;
        view.resolutionDen = resolution.assigned() ? static_cast<Int>(resolution.getDenominator()) : 1;
        if (view.resolutionNum <= 0 || view.resolutionDen <= 0)
            return;

        // The sample rate den / (num * delta) must divide out to a whole number of samples per second
        if (view.resolutionDen % (view.resolutionNum * view.delta) != 0)
            return;

        view.origin = descriptor.getOrigin();
    }
    catch (...)
    {
        // A malformed descriptor stays invalid instead of tearing down the read path
        return;
    }
    view.domainValid = true;
}

// Timestamp of the next readable sample; false while nothing interpretable is staged
bool MultiReaderDataManager::nextTimestamp(SlotView& view, Int& timestamp)
{
    while (!view.staged.empty())
    {
        const auto& front = view.staged.front();
        // A mismatching front is data behind an undelivered boundary: not readable yet
        if (!matchesDescriptor(view.valueDescriptor, front))
            return false;

        const auto dataPacket = front.asPtrOrNull<IDataPacket>(true);
        const auto domainPacket = dataPacket.getDomainPacket();
        if (!domainPacket.assigned())
        {
            // Uninterpretable without a domain packet
            view.stagedSamples -= dataPacket.getSampleCount() - view.frontOffset;
            view.frontOffset = 0;
            view.staged.pop_front();
            continue;
        }

        timestamp = static_cast<Int>(domainPacket.getOffset()) + static_cast<Int>(view.frontOffset) * view.delta;
        return true;
    }
    return false;
}

// Drops staged samples with timestamps below the target
void MultiReaderDataManager::discardBefore(SlotView& view, Int target)
{
    Int timestamp;
    while (nextTimestamp(view, timestamp) && timestamp < target)
    {
        const auto dataPacket = view.staged.front().asPtrOrNull<IDataPacket>(true);
        const auto count = dataPacket.getSampleCount();
        const Int start = static_cast<Int>(dataPacket.getDomainPacket().getOffset());
        const Int end = start + static_cast<Int>(count - 1) * view.delta;
        if (end < target)
        {
            view.stagedSamples -= count - view.frontOffset;
            view.frontOffset = 0;
            view.staged.pop_front();
            continue;
        }

        const auto skip = static_cast<SizeT>((target - start + view.delta - 1) / view.delta);
        view.stagedSamples -= skip - view.frontOffset;
        view.frontOffset = skip;
        return;
    }
}

bool MultiReaderDataManager::deliverEvents(State& state, uint64_t& deliveredMask)
{
    bool delivered = false;
    deliveredMask = 0;
    for (SizeT i = 0; i < state.slots.size(); i++)
    {
        auto& cell = *state.slots[i];
        auto& view = views[i];

        // Snapshot before the exchange: the adopted content is always at least as new as the version
        const auto version = cell.eventVersion.load(std::memory_order_acquire);
        if (version == view.deliveredVersion)
            continue;

        if (IPacket* raw = cell.lastEventPacket.exchange(nullptr, std::memory_order_acq_rel))
        {
            const auto adopted = PacketPtr::Adopt(raw);
            const auto eventPacket = adopted.asPtr<IEventPacket>(true);
            const auto [valueChanged, domainChanged, value, domain] = parseDataDescriptorEventPacket(eventPacket);
            if (valueChanged)
                view.valueDescriptor = value;
            if (domainChanged)
                view.domainDescriptor = domain;
            parseDomain(view);
            view.valueValid = convertibleValue(view.valueDescriptor);
            // A fresh descriptor lets a failed input rejoin synchronization
            view.failed = false;

            // Data staged under a superseded descriptor is dropped; newer data behind it is recounted
            while (!view.staged.empty() && !matchesDescriptor(view.valueDescriptor, view.staged.front()))
                view.staged.pop_front();
            view.stagedSamples = 0;
            view.frontOffset = 0;
            for (const auto& packet : view.staged)
            {
                if (matchesDescriptor(view.valueDescriptor, packet))
                    view.stagedSamples += packet.asPtrOrNull<IDataPacket>(true).getSampleCount();
            }
            settleReadyBit(state, i);
        }
        view.deliveredVersion = version;
        deliveredMask |= uint64_t(1) << i;
        delivered = true;
    }

    return delivered;
}

// Every gap is a boundary; the gapped input's staged data predates a discontinuity and is dropped
uint64_t MultiReaderDataManager::deliverGaps(State& state)
{
    uint64_t gapMask = 0;
    for (SizeT i = 0; i < state.slots.size(); i++)
    {
        const auto gaps = state.slots[i]->gapCount.load(std::memory_order_acquire);
        if (gaps == views[i].deliveredGaps)
            continue;
        views[i].deliveredGaps = gaps;
        discardSlotData(state, i);
        gapMask |= uint64_t(1) << i;
    }
    return gapMask;
}

void MultiReaderDataManager::resetPendingEvents(State& state)
{
    // Conservative reset: anything bumped mid-delivery re-raises the pending flag
    state.pendingEvents.store(0, std::memory_order_release);
    for (SizeT i = 0; i < state.slots.size(); i++)
    {
        if (state.slots[i]->eventVersion.load(std::memory_order_acquire) != views[i].deliveredVersion ||
            state.slots[i]->gapCount.load(std::memory_order_acquire) != views[i].deliveredGaps)
        {
            state.pendingEvents.fetch_add(1, std::memory_order_acq_rel);
            break;
        }
    }
}

// One incremental synchronization pass; true = a failure event was parked
// Staged data is assumed gap-free: a slot's range is its next timestamp plus its staged sample count
bool MultiReaderDataManager::runSync(State& state)
{
    syncStarted = true;
    const auto used = state.usedMask.load(std::memory_order_acquire);
    const bool deadlinePassed = std::chrono::steady_clock::now() > syncDeadline;
    auto errors = Dict<IString, IInteger>();
    const auto failSlot = [this, &state, &errors](SizeT i, MultiReader2InputError error)
    {
        views[i].failed = true;
        discardSlotData(state, i);
        errors.set(config.inputIds[i], Integer(static_cast<Int>(error)));
    };

    // Everything anchors to the main input; without its domain and a readable value nothing proceeds
    auto& main = views[mainIndex];
    if (!main.domainValid || !main.valueValid)
    {
        if (!deadlinePassed)
            return false;
        const auto error = main.valueValid ? main.domainError : MultiReader2InputError::InvalidDescriptor;
        errors.set(config.inputIds[mainIndex], Integer(static_cast<Int>(error)));
        park(state, MultiReader2StatusType::Event, errors);
        return true;
    }

    // Relational validation against the main input; a broken relation fails immediately
    const auto mainOrigin = main.origin.assigned() ? main.origin.toStdString() : std::string();
    for (SizeT i = 0; i < views.size(); i++)
    {
        if (i == mainIndex || ((used >> i) & 1) == 0 || views[i].failed)
            continue;

        auto& view = views[i];
        if (!view.valueDescriptor.assigned() && !view.domainDescriptor.assigned())
            continue;  // nothing announced yet; the deadline below covers it
        if (!view.valueValid)
        {
            failSlot(i, MultiReader2InputError::InvalidDescriptor);
            continue;
        }
        if (!view.domainValid)
        {
            failSlot(i, view.domainError);
            continue;
        }
        // Dividers arrive in a later phase: the effective rate and origin must equal the main input's.
        // Equal fields are the fast path; delta=2 at resolution 1/2 equals delta=1 at resolution 1/1.
        if (view.delta == main.delta && view.resolutionNum == main.resolutionNum && view.resolutionDen == main.resolutionDen)
        {
            view.tickNum = 1;
            view.tickDen = 1;
        }
        else if (view.delta * view.resolutionNum * main.resolutionDen == main.delta * main.resolutionNum * view.resolutionDen)
        {
            // Same rate in different tick units: scale this input's ticks into main ticks
            view.tickNum = view.resolutionNum * main.resolutionDen;
            view.tickDen = view.resolutionDen * main.resolutionNum;
        }
        else
        {
            failSlot(i, MultiReader2InputError::InvalidDomain);
            continue;
        }

        const auto origin = view.origin.assigned() ? view.origin.toStdString() : std::string();
        if (origin != mainOrigin)
            failSlot(i, MultiReader2InputError::InvalidDomain);
    }

    const Int delta = main.delta;
    // The 2 second sync window expressed in domain ticks bounds how far ranges may sit apart
    const Int maxDistance = 2 * main.resolutionDen / main.resolutionNum;

    bool waiting = false;
    for (int pass = 0; pass < 1000 && !waiting && !synced; pass++)
    {
        Int stamps[64];
        Int rawStamps[64];
        uint64_t haveMask = 0;
        Int target = std::numeric_limits<Int>::min();

        for (SizeT i = 0; i < views.size(); i++)
        {
            if (((used >> i) & 1) == 0 || views[i].failed)
                continue;

            Int timestamp;
            if (!nextTimestamp(views[i], timestamp))
            {
                // The main input never fails: at worst it stays synced only to itself
                if (i != mainIndex && deadlinePassed)
                    failSlot(i, MultiReader2InputError::SyncFailed);
                else
                    waiting = true;
                continue;
            }

            // Scale into main ticks; a tick off the main lattice can never align
            const Int scaled = timestamp * views[i].tickNum;
            if (scaled % views[i].tickDen != 0)
            {
                failSlot(i, MultiReader2InputError::InvalidDomain);
                continue;
            }
            stamps[i] = scaled / views[i].tickDen;
            rawStamps[i] = timestamp;
            haveMask |= uint64_t(1) << i;
            target = std::max(target, stamps[i]);
        }
        if (waiting || ((haveMask >> mainIndex) & 1) == 0)
            break;

        const Int mainLast = stamps[mainIndex] + static_cast<Int>(main.stagedSamples - 1) * delta;
        for (SizeT i = 0; i < views.size(); i++)
        {
            if (((haveMask >> i) & 1) == 0 || i == mainIndex)
                continue;

            // The input must share the main grid and its range must be close enough to ever meet it
            if (((stamps[i] - stamps[mainIndex]) % delta + delta) % delta != 0)
            {
                failSlot(i, MultiReader2InputError::InvalidDomain);
                continue;
            }
            const Int last = stamps[i] + static_cast<Int>(views[i].stagedSamples - 1) * delta;
            if (stamps[i] - mainLast > maxDistance || stamps[mainIndex] - last > maxDistance)
                failSlot(i, MultiReader2InputError::SyncFailed);
        }

        bool aligned = true;
        for (SizeT i = 0; i < views.size(); i++)
        {
            if (((haveMask >> i) & 1) == 0 || views[i].failed)
                continue;
            if (stamps[i] != target)
            {
                aligned = false;
                // The grid check makes the distance to the target a whole number of samples in every unit
                discardBefore(views[i], rawStamps[i] + (target - stamps[i]) / delta * views[i].delta);
                settleReadyBit(state, i);
            }
        }
        if (aligned)
        {
            synced = true;
            syncedStart = target;
            nextReadTick = target;
            state.syncing.store(false, std::memory_order_release);
        }
    }

    if (errors.getCount() > 0)
    {
        park(state, MultiReader2StatusType::Event, errors);
        return true;
    }
    return false;
}

ObjectPtr<IMultiReader2Status> MultiReaderDataManager::makeStatus(MultiReader2StatusType type, const DictPtr<IString, IInteger>& errors)
{
    auto descriptors = Dict<IString, IDataDescriptor>();
    for (SizeT i = 0; i < views.size(); i++)
    {
        if (views[i].valueDescriptor.assigned())
            descriptors.set(config.inputIds[i], views[i].valueDescriptor);
    }
    DataDescriptorPtr domain;
    if (mainIndex < views.size())
        domain = views[mainIndex].domainDescriptor;
    return createWithImplementation<IMultiReader2Status, MultiReader2StatusImpl>(domain, type, descriptors, errors, nullptr);
}

void MultiReaderDataManager::park(State& state, MultiReader2StatusType type, const DictPtr<IString, IInteger>& errors)
{
    pendingStatus = makeStatus(type, errors);
    state.parked.store(true, std::memory_order_release);
}

ErrCode MultiReaderDataManager::getAvailableCount(SizeT* count)
{
    OPENDAQ_PARAM_NOT_NULL(count);

    std::scoped_lock lock(consumerMutex);
    *count = 0;

    const auto current = std::atomic_load(&state);
    if (!current)
        return OPENDAQ_SUCCESS;

    // Nothing is readable while an event awaits its commit, is undelivered, or a used input is disconnected
    const auto used = current->usedMask.load(std::memory_order_acquire);
    if (used == 0 || current->parked.load(std::memory_order_acquire))
        return OPENDAQ_SUCCESS;
    if (current->pendingEvents.load(std::memory_order_acquire) > 0)
        return OPENDAQ_SUCCESS;
    if ((current->connectedMask.load(std::memory_order_acquire) & used) != used)
        return OPENDAQ_SUCCESS;

    if (!synced)
        return OPENDAQ_SUCCESS;

    drainSlots(*current);

    SizeT available = std::numeric_limits<SizeT>::max();
    bool any = false;
    for (SizeT i = 0; i < views.size(); i++)
    {
        if (((used >> i) & 1) == 0 || views[i].failed)
            continue;
        available = std::min(available, views[i].stagedSamples);
        any = true;
    }
    *count = any ? available : 0;
    return OPENDAQ_SUCCESS;
}

template <typename TSrc, typename TDst>
static void copySamples(void* dst, SizeT dstOffset, const void* src, SizeT srcOffset, SizeT count)
{
    const auto in = static_cast<const TSrc*>(src) + srcOffset;
    const auto out = static_cast<TDst*>(dst) + dstOffset;
    for (SizeT i = 0; i < count; i++)
        out[i] = static_cast<TDst>(in[i]);
}

template <typename TDst>
static void copyFrom(void* dst, SizeT dstOffset, const void* src, SampleType srcType, SizeT srcOffset, SizeT count)
{
    switch (srcType)
    {
        case SampleType::Float32:
            return copySamples<float, TDst>(dst, dstOffset, src, srcOffset, count);
        case SampleType::Float64:
            return copySamples<double, TDst>(dst, dstOffset, src, srcOffset, count);
        case SampleType::Int8:
            return copySamples<int8_t, TDst>(dst, dstOffset, src, srcOffset, count);
        case SampleType::Int16:
            return copySamples<int16_t, TDst>(dst, dstOffset, src, srcOffset, count);
        case SampleType::Int32:
            return copySamples<int32_t, TDst>(dst, dstOffset, src, srcOffset, count);
        case SampleType::Int64:
            return copySamples<int64_t, TDst>(dst, dstOffset, src, srcOffset, count);
        case SampleType::UInt8:
            return copySamples<uint8_t, TDst>(dst, dstOffset, src, srcOffset, count);
        case SampleType::UInt16:
            return copySamples<uint16_t, TDst>(dst, dstOffset, src, srcOffset, count);
        case SampleType::UInt32:
            return copySamples<uint32_t, TDst>(dst, dstOffset, src, srcOffset, count);
        case SampleType::UInt64:
            return copySamples<uint64_t, TDst>(dst, dstOffset, src, srcOffset, count);
        default:
            break;  // unreachable: value descriptors are validated before sync completes
    }
}

void MultiReaderDataManager::copyConvert(void* dst, SizeT dstOffset, const void* src, SampleType srcType, SizeT srcOffset, SizeT count) const
{
    switch (config.valueReadType)
    {
        case SampleType::Float32:
            return copyFrom<float>(dst, dstOffset, src, srcType, srcOffset, count);
        case SampleType::Float64:
            return copyFrom<double>(dst, dstOffset, src, srcType, srcOffset, count);
        case SampleType::Int8:
            return copyFrom<int8_t>(dst, dstOffset, src, srcType, srcOffset, count);
        case SampleType::Int16:
            return copyFrom<int16_t>(dst, dstOffset, src, srcType, srcOffset, count);
        case SampleType::Int32:
            return copyFrom<int32_t>(dst, dstOffset, src, srcType, srcOffset, count);
        case SampleType::Int64:
            return copyFrom<int64_t>(dst, dstOffset, src, srcType, srcOffset, count);
        case SampleType::UInt8:
            return copyFrom<uint8_t>(dst, dstOffset, src, srcType, srcOffset, count);
        case SampleType::UInt16:
            return copyFrom<uint16_t>(dst, dstOffset, src, srcType, srcOffset, count);
        case SampleType::UInt32:
            return copyFrom<uint32_t>(dst, dstOffset, src, srcType, srcOffset, count);
        case SampleType::UInt64:
            return copyFrom<uint64_t>(dst, dstOffset, src, srcType, srcOffset, count);
        default:
            break;  // the params object rejects unsupported read types
    }
}

// Copies `count` converted samples out of the staged prefix and advances past them
void MultiReaderDataManager::copySlot(SlotView& view, void* buffer, SizeT count)
{
    SizeT written = 0;
    while (written < count)
    {
        const auto dataPacket = view.staged.front().asPtrOrNull<IDataPacket>(true);
        const SizeT packetCount = dataPacket.getSampleCount();
        const SizeT take = std::min(count - written, packetCount - view.frontOffset);
        copyConvert(buffer, written, dataPacket.getData(), dataPacket.getDataDescriptor().getSampleType(), view.frontOffset, take);
        written += take;
        view.frontOffset += take;
        if (view.frontOffset == packetCount)
        {
            view.frontOffset = 0;
            view.staged.pop_front();
        }
    }
    view.stagedSamples -= count;
}

ErrCode MultiReaderDataManager::doRead(IMultiReader2Status** status, void** data, SizeT* count, SizeT* packetOffset, bool withDomain)
{
    std::scoped_lock lock(consumerMutex);

    const auto current = std::atomic_load(&state);
    if (!current)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDSTATE, "The reader is not configured");

    const SizeT requested = *count;
    if (requested != 0 && requested < config.minReadCount)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "The requested count is below MinReadCount");

    *count = 0;
    if (packetOffset != nullptr)
        *packetOffset = 0;

    // A pending event is re-reported until committed
    if (current->parked.load(std::memory_order_acquire))
    {
        *status = pendingStatus.addRefAndReturn();
        return OPENDAQ_SUCCESS;
    }

    drainSlots(*current);

    uint64_t deliveredMask = 0;
    const bool delivered = deliverEvents(*current, deliveredMask);
    const uint64_t gapMask = deliverGaps(*current);
    if (delivered || gapMask != 0)
        resetPendingEvents(*current);

    // A fresh disconnect of a used input parks with a synthesized event; committed ones stay silent
    const auto used = current->usedMask.load(std::memory_order_acquire);
    const auto disconnected = used & ~current->connectedMask.load(std::memory_order_acquire);
    if ((disconnected & ~reportedDisconnectMask) != 0)
    {
        discardAllData(*current);
        auto errors = Dict<IString, IInteger>();
        for (SizeT i = 0; i < views.size(); i++)
        {
            if ((disconnected >> i) & 1)
                errors.set(config.inputIds[i], Integer(static_cast<Int>(MultiReader2InputError::Disconnected)));
        }
        park(*current, MultiReader2StatusType::Event, errors);
        *status = pendingStatus.addRefAndReturn();
        return OPENDAQ_SUCCESS;
    }

    if (delivered || gapMask != 0)
    {
        auto errors = Dict<IString, IInteger>();
        // An event during synchronization fails the affected inputs; they rejoin after the commit
        if (syncStarted && !synced)
        {
            for (SizeT i = 0; i < views.size(); i++)
            {
                if (((deliveredMask & used) >> i) & 1 && i != mainIndex)
                    errors.set(config.inputIds[i], Integer(static_cast<Int>(MultiReader2InputError::SyncFailed)));
            }
        }
        for (SizeT i = 0; i < views.size(); i++)
        {
            if ((gapMask >> i) & 1)
                errors.set(config.inputIds[i], Integer(static_cast<Int>(MultiReader2InputError::Gap)));
        }
        park(*current, MultiReader2StatusType::Event, errors);
        *status = pendingStatus.addRefAndReturn();
        return OPENDAQ_SUCCESS;
    }

    // While waiting for a reconnect nothing synchronizes or reads
    if (disconnected == 0)
    {
        if (!synced && runSync(*current))
        {
            *status = pendingStatus.addRefAndReturn();
            return OPENDAQ_SUCCESS;
        }

        if (synced && requested > 0)
        {
            // Plan: the smallest readable run across the participants, gated by the minimum
            SizeT plan = requested;
            const SizeT base = withDomain ? 1 : 0;
            for (SizeT i = 0; i < views.size(); i++)
            {
                if (((used >> i) & 1) == 0 || views[i].failed)
                    continue;
                plan = std::min(plan, views[i].stagedSamples);
                if (data[base + i] == nullptr)
                    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "A used input was given no buffer");
            }
            if (plan < config.minReadCount)
                plan = 0;

            if (plan > 0)
            {
                if (withDomain)
                {
                    OPENDAQ_PARAM_NOT_NULL(data[0]);
                    const auto timestamps = static_cast<int64_t*>(data[0]);
                    const Int delta = views[mainIndex].delta;
                    for (SizeT k = 0; k < plan; k++)
                        timestamps[k] = nextReadTick + static_cast<Int>(k) * delta;
                }
                for (SizeT i = 0; i < views.size(); i++)
                {
                    if (((used >> i) & 1) == 0 || views[i].failed)
                        continue;
                    copySlot(views[i], data[base + i], plan);
                }
                if (packetOffset != nullptr)
                    *packetOffset = static_cast<SizeT>(nextReadTick);
                nextReadTick += static_cast<Int>(plan) * views[mainIndex].delta;
                *count = plan;
            }
        }
    }

    *status = makeStatus(MultiReader2StatusType::Data, nullptr).detach();
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReaderDataManager::read(IMultiReader2Status** status, void** data, SizeT* count, SizeT* packetOffset)
{
    OPENDAQ_PARAM_NOT_NULL(status);
    OPENDAQ_PARAM_NOT_NULL(data);
    OPENDAQ_PARAM_NOT_NULL(count);
    OPENDAQ_PARAM_NOT_NULL(packetOffset);

    return doRead(status, data, count, packetOffset, false);
}

ErrCode MultiReaderDataManager::readWithDomain(IMultiReader2Status** status, void** data, SizeT* count)
{
    OPENDAQ_PARAM_NOT_NULL(status);
    OPENDAQ_PARAM_NOT_NULL(data);
    OPENDAQ_PARAM_NOT_NULL(count);

    return doRead(status, data, count, nullptr, true);
}

ErrCode MultiReaderDataManager::commitEvent()
{
    std::scoped_lock lock(consumerMutex);

    const auto current = std::atomic_load(&state);
    if (!current || !current->parked.load(std::memory_order_acquire))
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDSTATE, "No event to commit");

    pendingStatus = nullptr;
    // Disconnects reported by this event stay silent until the input reconnects
    reportedDisconnectMask = current->usedMask.load(std::memory_order_acquire) & ~current->connectedMask.load(std::memory_order_acquire);
    // Every committed event restarts synchronization with a fresh timeout window
    synced = false;
    syncStarted = false;
    syncDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    current->syncing.store(true, std::memory_order_release);
    current->syncDeadlineTicks.store(syncDeadline.time_since_epoch().count(), std::memory_order_release);
    current->parked.store(false, std::memory_order_release);
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReaderDataManager::setActive(Bool active)
{
    std::scoped_lock lock(consumerMutex);

    const auto current = std::atomic_load(&state);
    if (!current || !current->parked.load(std::memory_order_acquire))
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDSTATE, "setActive is only valid between an event read and commitEvent");

    current->active.store(active, std::memory_order_release);
    // Data staged before the deactivation would only go stale
    if (!active)
        discardAllData(*current);
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReaderDataManager::setUsed(IString* inputId, Bool used)
{
    OPENDAQ_PARAM_NOT_NULL(inputId);

    std::scoped_lock lock(consumerMutex);

    const auto current = std::atomic_load(&state);
    if (!current || !current->parked.load(std::memory_order_acquire))
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDSTATE, "setUsed is only valid between an event read and commitEvent");

    const auto id = StringPtr::Borrow(inputId);
    for (SizeT i = 0; i < config.inputIds.size(); i++)
    {
        if (config.inputIds[i] == id)
        {
            const auto bit = uint64_t(1) << i;
            if (used)
            {
                current->usedMask.fetch_or(bit, std::memory_order_acq_rel);
                views[i].failed = false;
            }
            else
            {
                current->usedMask.fetch_and(~bit, std::memory_order_acq_rel);
                // Data staged for a now-unused input would only go stale
                discardSlotData(*current, i);
            }
            return OPENDAQ_SUCCESS;
        }
    }

    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTFOUND, R"(Input "%s" was not added to the reader)", id.getCharPtr());
}

void MultiReaderDataManager::connected(SizeT slotIndex)
{
    const auto current = std::atomic_load(&state);
    if (!current || slotIndex >= current->slots.size())
        return;

    current->connectedMask.fetch_or(uint64_t(1) << slotIndex, std::memory_order_acq_rel);
}

void MultiReaderDataManager::disconnected(SizeT slotIndex)
{
    const auto current = std::atomic_load(&state);
    if (!current || slotIndex >= current->slots.size())
        return;

    current->connectedMask.fetch_and(~(uint64_t(1) << slotIndex), std::memory_order_acq_rel);
}

Bool MultiReaderDataManager::addPacket(SizeT slotIndex, const PacketPtr& packet)
{
    const auto current = std::atomic_load(&state);
    if (!current || slotIndex >= current->slots.size() || !packet.assigned())
        return False;

    const auto bit = uint64_t(1) << slotIndex;
    auto& cell = *current->slots[slotIndex];

    if (packet.getType() == PacketType::Event)
    {
        // Descriptor and gap events survive every dropping rule; other event kinds are ignored
        const auto eventPacket = packet.asPtrOrNull<IEventPacket>(true);
        if (!eventPacket.assigned())
            return False;

        const auto eventId = eventPacket.getEventId();
        if (eventId == event_packet_id::IMPLICIT_DOMAIN_GAP_DETECTED)
        {
            // A gap is a boundary: the next read reports it and the commit that follows realigns
            cell.gapCount.fetch_add(1, std::memory_order_release);
            current->pendingEvents.fetch_add(1, std::memory_order_acq_rel);
        }
        else if (eventId == event_packet_id::DATA_DESCRIPTOR_CHANGED)
        {
            // Change packets carry deltas; merge into full state so newest-wins caching loses nothing
            const auto [valueChanged, domainChanged, value, domain] = parseDataDescriptorEventPacket(eventPacket);
            if (valueChanged)
                cell.producerValueDescriptor = value;
            if (domainChanged)
                cell.producerDomainDescriptor = domain;
            EventPacketPtr merged = DataDescriptorChangedEventPacket(descriptorToEventPacketParam(cell.producerValueDescriptor),
                                                                     descriptorToEventPacketParam(cell.producerDomainDescriptor));
            if (IPacket* old = cell.lastEventPacket.exchange(merged.detach(), std::memory_order_acq_rel))
                old->releaseRef();
            cell.eventVersion.fetch_add(1, std::memory_order_release);
            current->pendingEvents.fetch_add(1, std::memory_order_acq_rel);
        }
        else
        {
            return False;
        }
    }
    else
    {
        // While any used input is disconnected all data is dropped; unused inputs and inactive readers drop too
        const auto used = current->usedMask.load(std::memory_order_acquire);
        if ((current->connectedMask.load(std::memory_order_acquire) & used) != used)
            return False;
        if (!current->active.load(std::memory_order_acquire) || (used & bit) == 0)
            return False;

        cell.queue.push(packet);

        // Shared words are touched only on empty-to-non-empty transitions, never per steady-state packet
        if (cell.dataPacketCount.fetch_add(1, std::memory_order_acq_rel) == 0)
            current->readyMask.fetch_or(bit, std::memory_order_release);
    }

    if (!deliverable(*current))
    {
        // During synchronization a flowing input surfaces the timeout for its silent peers
        if (!current->syncing.load(std::memory_order_acquire) || current->parked.load(std::memory_order_acquire))
            return False;
        const auto usedNow = current->usedMask.load(std::memory_order_acquire);
        if ((current->connectedMask.load(std::memory_order_acquire) & usedNow) != usedNow)
            return False;
        const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
        if (now < current->syncDeadlineTicks.load(std::memory_order_acquire))
            return False;
    }

    // Wake election: exactly one producer wins the pass, everyone else sees disarmed
    return current->armed.exchange(false, std::memory_order_acq_rel) ? True : False;
}

Bool MultiReaderDataManager::armDataAvailable()
{
    const auto current = std::atomic_load(&state);
    if (!current)
        return False;

    // Stay disarmed while more is deliverable: the caller owes another pass
    if (deliverable(*current))
        return True;

    current->armed.store(true, std::memory_order_release);

    // Reclaim a wake published between the check and the arm, or it is lost
    if (deliverable(*current) && current->armed.exchange(false, std::memory_order_acq_rel))
        return True;

    return False;
}

END_NAMESPACE_OPENDAQ
