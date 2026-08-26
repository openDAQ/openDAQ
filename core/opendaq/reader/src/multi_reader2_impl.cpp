#include <coreobjects/ownable_ptr.h>
#include <coretypes/event_args_factory.h>
#include <coreobjects/property_object_factory.h>
#include <coretypes/validation.h>
#include <opendaq/input_port_factory.h>
#include <opendaq/multi_reader2_impl.h>
#include <opendaq/tags_private_ptr.h>

#include <algorithm>
#include <unordered_set>

BEGIN_NAMESPACE_OPENDAQ

MultiReader2Impl::MultiReader2Impl(IMultiReader2Params* params)
{
    // Ref-guard pattern from MultiReaderImpl: configure registers this as a port listener
    this->internalAddRef();
    try
    {
        checkErrorInfo(configure(params));
    }
    catch (...)
    {
        this->releaseWeakRefOnException();
        throw;
    }
}

std::vector<MultiReader2Impl::Slot>::iterator MultiReader2Impl::findSlot(const StringPtr& inputId)
{
    return std::find_if(slots.begin(), slots.end(), [&inputId](const Slot& slot) { return slot.inputId == inputId; });
}

// Maps a notifying port to its slot id; falls back to the port id for unknown ports
StringPtr MultiReader2Impl::findSlotId(IInputPort* port)
{
    std::scoped_lock lock(mutex);
    for (const auto& slot : slots)
    {
        if (slot.port == port)
            return slot.inputId;
    }
    return InputPortPtr::Borrow(port).getGlobalId();
}

// Expects the reader lock to be held
ErrCode MultiReader2Impl::addInputComponent(const ComponentPtr& component)
{
    if (!context.assigned())
        context = component.getContext();

    if (findSlot(component.getGlobalId()) != slots.end())
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_DUPLICATEITEM, R"(Input "%s" was already added)", component.getGlobalId().getCharPtr());

    const auto listener = this->thisPtr<InputPortNotificationsPtr>();

    if (auto signal = component.asPtrOrNull<ISignal>(); signal.assigned())
    {
        auto port = InputPort(context, nullptr, fmt::format("multi_reader_signal_{}", signal.getLocalId()));
        port.getTags().asPtr<ITagsPrivate>().add("MultiReaderInternalPort");
        port.setListener(listener);
        port.setNotificationMethod(PacketReadyNotification::SameThread);
        port.connect(signal);

        slots.push_back({port, signal.getGlobalId(), true});
    }
    else if (auto port = component.asPtrOrNull<IInputPortConfig>(); port.assigned())
    {
        if (!portBinder.assigned())
            portBinder = PropertyObject();
        port.asPtr<IOwnable>().setOwner(portBinder);

        port.setListener(listener);
        port.setNotificationMethod(PacketReadyNotification::SameThread);

        slots.push_back({port, port.getGlobalId(), false});
    }
    else
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "Input is neither a signal nor an input port");
    }

    return OPENDAQ_SUCCESS;
}

// Internal ports are torn down; adopted external ports only stop notifying us
void MultiReader2Impl::detachSlot(Slot& slot)
{
    if (slot.ownsPort)
        slot.port.disconnect();
    else
        slot.port.setListener(nullptr);
}

ErrCode MultiReader2Impl::configure(IMultiReader2Params* params)
{
    OPENDAQ_PARAM_NOT_NULL(params);

    ListPtr<IComponent> inputs;
    ErrCode errCode = params->getInputs(&inputs);
    OPENDAQ_RETURN_IF_FAILED(errCode);

    if (!inputs.assigned() || inputs.getCount() == 0)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "Params input list must not be empty");

    ComponentPtr mainInput;
    errCode = params->getMainInput(&mainInput);
    OPENDAQ_RETURN_IF_FAILED(errCode);

    SizeT newMinReadCount;
    errCode = params->getMinReadCount(&newMinReadCount);
    OPENDAQ_RETURN_IF_FAILED(errCode);

    Bool newRequireSameRates;
    errCode = params->getRequireSameRates(&newRequireSameRates);
    OPENDAQ_RETURN_IF_FAILED(errCode);

    std::unordered_set<std::string> newIds;
    for (const auto& component : inputs)
    {
        if (!newIds.insert(component.getGlobalId().toStdString()).second)
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_DUPLICATEITEM, R"(Input "%s" appears more than once)", component.getGlobalId().getCharPtr());
    }

    // The main input must be one of the params inputs; the first input is the default
    if (mainInput.assigned() && newIds.count(mainInput.getGlobalId().toStdString()) == 0)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, R"(Main input "%s" is not in the input list)", mainInput.getGlobalId().getCharPtr());

    std::scoped_lock lock(mutex);

    // Drop inputs missing from the new list
    for (auto it = slots.begin(); it != slots.end();)
    {
        if (newIds.count(it->inputId.toStdString()) == 0)
        {
            detachSlot(*it);
            it = slots.erase(it);
        }
        else
        {
            ++it;
        }
    }

    // Add inputs not present yet
    for (const auto& component : inputs)
    {
        if (findSlot(component.getGlobalId()) == slots.end())
        {
            errCode = addInputComponent(component);
            OPENDAQ_RETURN_IF_FAILED(errCode);
        }
    }

    // Re-order slots to follow the params list
    std::vector<Slot> reordered;
    reordered.reserve(slots.size());
    for (const auto& component : inputs)
    {
        const auto it = findSlot(component.getGlobalId());
        reordered.push_back(std::move(*it));
        slots.erase(it);
    }
    slots = std::move(reordered);
    mainInputId = mainInput.assigned() ? mainInput.getGlobalId() : slots.front().inputId;

    // Cached only; nothing consumes these until the read path exists
    minReadCount = newMinReadCount;
    requireSameRates = newRequireSameRates;

    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2Impl::getMainInput(IString** inputId)
{
    OPENDAQ_PARAM_NOT_NULL(inputId);

    std::scoped_lock lock(mutex);
    *inputId = mainInputId.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2Impl::getAvailableCount(SizeT* count)
{
    OPENDAQ_PARAM_NOT_NULL(count);

    // Shell: no packet queues exist yet
    *count = 0;
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2Impl::read(IMultiReader2Status** status, void** data, SizeT* count, SizeT* packetOffset)
{
    OPENDAQ_PARAM_NOT_NULL(status);
    OPENDAQ_PARAM_NOT_NULL(data);
    OPENDAQ_PARAM_NOT_NULL(count);
    OPENDAQ_PARAM_NOT_NULL(packetOffset);

    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTIMPLEMENTED, "MultiReader2::read is not implemented yet");
}

ErrCode MultiReader2Impl::readWithDomain(IMultiReader2Status** status, void** data, SizeT* count)
{
    OPENDAQ_PARAM_NOT_NULL(status);
    OPENDAQ_PARAM_NOT_NULL(data);
    OPENDAQ_PARAM_NOT_NULL(count);

    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTIMPLEMENTED, "MultiReader2::readWithDomain is not implemented yet");
}

ErrCode MultiReader2Impl::commitEvent()
{
    std::scoped_lock lock(mutex);

    if (!eventPending)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDSTATE, "No event to commit");

    // Shell: the event queue does not exist yet, only the gate is tracked
    eventPending = false;
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2Impl::setUsed(IString* inputId, Bool used)
{
    OPENDAQ_PARAM_NOT_NULL(inputId);

    const auto id = StringPtr::Borrow(inputId);

    std::scoped_lock lock(mutex);

    if (!eventPending)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDSTATE, "setUsed is only valid between an event read and commitEvent");

    const auto it = findSlot(id);
    if (it == slots.end())
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTFOUND, R"(Input "%s" was not added to the reader)", id.getCharPtr());

    it->used = used;
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2Impl::setActive(Bool active)
{
    std::scoped_lock lock(mutex);

    if (!eventPending)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDSTATE, "setActive is only valid between an event read and commitEvent");

    this->active = active;
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2Impl::getOnConnected(IEvent** event)
{
    OPENDAQ_PARAM_NOT_NULL(event);

    *event = onConnected.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2Impl::getOnDisconnected(IEvent** event)
{
    OPENDAQ_PARAM_NOT_NULL(event);

    *event = onDisconnected.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2Impl::getOnDataAvailable(IEvent** event)
{
    OPENDAQ_PARAM_NOT_NULL(event);

    *event = onDataAvailable.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2Impl::acceptsSignal(IInputPort* /*port*/, ISignal* /*signal*/, Bool* accept)
{
    OPENDAQ_PARAM_NOT_NULL(accept);

    *accept = True;
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2Impl::connected(IInputPort* port)
{
    OPENDAQ_PARAM_NOT_NULL(port);

    auto portPtr = InputPortPtr::Borrow(port);
    auto args = EventArgs(0, findSlotId(port));
    onConnected(portPtr, args);
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2Impl::disconnected(IInputPort* port)
{
    OPENDAQ_PARAM_NOT_NULL(port);

    auto portPtr = InputPortPtr::Borrow(port);
    auto args = EventArgs(0, findSlotId(port));
    onDisconnected(portPtr, args);
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2Impl::packetReceived(IInputPort* /*port*/)
{
    return OPENDAQ_SUCCESS;
}

END_NAMESPACE_OPENDAQ
