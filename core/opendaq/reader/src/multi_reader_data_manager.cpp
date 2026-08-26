#include <coretypes/validation.h>
#include <opendaq/multi_reader_data_manager.h>

BEGIN_NAMESPACE_OPENDAQ

bool MultiReaderDataManager::deliverable() const
{
    if (eventPending)
        return false;

    if (queuedEventPackets > 0)
        return true;

    bool allHaveData = !queues.empty();
    for (SizeT i = 0; i < queues.size(); i++)
    {
        if (usedFlags[i])
            allHaveData &= !queues[i].empty();
    }
    return allHaveData;
}

void MultiReaderDataManager::reconfigure(Config config)
{
    queues.assign(config.inputIds.size(), {});
    usedFlags.assign(config.inputIds.size(), true);
    this->config = std::move(config);
    queuedEventPackets = 0;
    active = True;
    eventPending = false;
    notificationArmed = true;
}

void MultiReaderDataManager::clear()
{
    for (auto& queue : queues)
        queue.clear();
    queuedEventPackets = 0;
}

ErrCode MultiReaderDataManager::getAvailableCount(SizeT* count)
{
    OPENDAQ_PARAM_NOT_NULL(count);

    // Shell: no packet queues exist yet
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
    if (!eventPending)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDSTATE, "No event to commit");

    eventPending = false;
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReaderDataManager::setActive(Bool active)
{
    if (!eventPending)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDSTATE, "setActive is only valid between an event read and commitEvent");

    this->active = active;
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReaderDataManager::setUsed(IString* inputId, Bool used)
{
    OPENDAQ_PARAM_NOT_NULL(inputId);

    if (!eventPending)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDSTATE, "setUsed is only valid between an event read and commitEvent");

    const auto id = StringPtr::Borrow(inputId);
    for (SizeT i = 0; i < config.inputIds.size(); i++)
    {
        if (config.inputIds[i] == id)
        {
            usedFlags[i] = used;
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
    if (slotIndex >= queues.size() || !packet.assigned())
        return False;

    queues[slotIndex].push_back(packet);
    if (packet.getType() == PacketType::Event)
        queuedEventPackets++;

    if (!notificationArmed || !deliverable())
        return False;

    notificationArmed = false;
    return True;
}

Bool MultiReaderDataManager::armDataAvailable()
{
    // Stay disarmed while more is deliverable: the caller owes another pass
    if (deliverable())
        return True;

    notificationArmed = true;
    return False;
}

END_NAMESPACE_OPENDAQ
