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
    std::atomic_store(&state, std::move(newState));
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

    const bool isEvent = packet.getType() == PacketType::Event;
    const auto bit = uint64_t(1) << slotIndex;
    auto& cell = *current->slots[slotIndex];

    // The newest event packet is always cached so descriptors survive any dropping
    if (isEvent)
    {
        IPacket* newPacket = packet;
        newPacket->addRef();
        if (IPacket* old = cell.lastEventPacket.exchange(newPacket, std::memory_order_acq_rel))
            old->releaseRef();
    }

    // While any used input is disconnected everything is dropped from the queues
    const auto used = current->usedMask.load(std::memory_order_acquire);
    if ((current->connectedMask.load(std::memory_order_acquire) & used) != used)
        return False;

    // Unused inputs and inactive readers drop data; events always flow
    if (!isEvent)
    {
        if (!current->active.load(std::memory_order_acquire))
            return False;
        if ((used & bit) == 0)
            return False;
    }

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
