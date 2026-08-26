#include <coretypes/validation.h>
#include <opendaq/multi_reader_data_manager.h>

#include <cassert>

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
    usedMask.store(slotCount >= 64 ? ~uint64_t(0) : (uint64_t(1) << slotCount) - 1, std::memory_order_relaxed);
}

bool MultiReaderDataManager::deliverable(const State& state)
{
    if (state.parked.load(std::memory_order_acquire))
        return false;

    if (state.queuedEventPackets.load(std::memory_order_acquire) > 0)
        return true;

    // An inactive reader delivers only events
    if (!state.active.load(std::memory_order_acquire))
        return false;

    const auto used = state.usedMask.load(std::memory_order_acquire);
    return used != 0 && (state.readyMask.load(std::memory_order_acquire) & used) == used;
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
    std::atomic_store(&state, std::move(newState));
}

void MultiReaderDataManager::clear()
{
    std::scoped_lock lock(consumerMutex);

    const auto current = std::atomic_load(&state);
    if (!current)
        return;

    // Fresh state block drops the queues; the used mask and active flag survive a clear
    auto fresh = std::make_shared<State>(current->slots.size());
    fresh->usedMask.store(current->usedMask.load(std::memory_order_acquire), std::memory_order_relaxed);
    fresh->active.store(current->active.load(std::memory_order_acquire), std::memory_order_relaxed);
    std::atomic_store(&state, std::move(fresh));
}

ErrCode MultiReaderDataManager::getAvailableCount(SizeT* count)
{
    OPENDAQ_PARAM_NOT_NULL(count);

    // Shell: sample accounting arrives with the read path
    *count = 0;
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReaderDataManager::read(IMultiReader2Status** status, void** data, SizeT* count, SizeT* packetOffset)
{
    OPENDAQ_PARAM_NOT_NULL(status);
    OPENDAQ_PARAM_NOT_NULL(data);
    OPENDAQ_PARAM_NOT_NULL(count);
    OPENDAQ_PARAM_NOT_NULL(packetOffset);

    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTIMPLEMENTED, "MultiReaderDataManager::read is not implemented yet");
}

ErrCode MultiReaderDataManager::commitEvent()
{
    std::scoped_lock lock(consumerMutex);

    const auto current = std::atomic_load(&state);
    if (!current || !current->parked.load(std::memory_order_acquire))
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDSTATE, "No event to commit");

    // Event popping arrives with the read path; only the gate is released here
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
                current->usedMask.fetch_or(bit, std::memory_order_acq_rel);
            else
                current->usedMask.fetch_and(~bit, std::memory_order_acq_rel);
            return OPENDAQ_SUCCESS;
        }
    }

    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTFOUND, R"(Input "%s" was not added to the reader)", id.getCharPtr());
}

void MultiReaderDataManager::connected(const StringPtr& /*inputId*/)
{
}

void MultiReaderDataManager::disconnected(const StringPtr& /*inputId*/)
{
}

Bool MultiReaderDataManager::addPacket(SizeT slotIndex, const PacketPtr& packet)
{
    const auto current = std::atomic_load(&state);
    if (!current || slotIndex >= current->slots.size() || !packet.assigned())
        return False;

    const bool isEvent = packet.getType() == PacketType::Event;
    const auto bit = uint64_t(1) << slotIndex;

    // Unused inputs and inactive readers drop data; events always flow
    if (!isEvent)
    {
        if (!current->active.load(std::memory_order_acquire))
            return False;
        if ((current->usedMask.load(std::memory_order_acquire) & bit) == 0)
            return False;
    }

    auto& cell = *current->slots[slotIndex];
    cell.queue.push(packet);

    // Shared words are touched only on transitions and event packets, never per steady-state packet
    if (isEvent)
        current->queuedEventPackets.fetch_add(1, std::memory_order_acq_rel);
    else if (cell.dataPacketCount.fetch_add(1, std::memory_order_acq_rel) == 0)
        current->readyMask.fetch_or(bit, std::memory_order_release);

    if (!deliverable(*current))
        return False;

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
