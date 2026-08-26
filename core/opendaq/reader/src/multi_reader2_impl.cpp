#include <coreobjects/ownable_ptr.h>
#include <coretypes/event_args_factory.h>
#include <coreobjects/property_object_factory.h>
#include <coretypes/validation.h>
// packet.h must precede connection_internal.h, which uses IPacket without including it
#include <opendaq/packet.h>
#include <opendaq/connection_internal.h>
#include <opendaq/input_port_factory.h>
#include <opendaq/multi_reader2_impl.h>
#include <opendaq/tags_private_ptr.h>
#include <opendaq/work_factory.h>

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

    ListPtr<IComponent> unusedInputs;
    errCode = params->getUnusedInputs(&unusedInputs);
    OPENDAQ_RETURN_IF_FAILED(errCode);

    SampleType newValueReadType;
    errCode = params->getValueReadType(&newValueReadType);
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

    // Unused inputs must be part of the input list; the main input must stay used
    const auto resolvedMainId = mainInput.assigned() ? mainInput.getGlobalId().toStdString() : inputs[0].getGlobalId().toStdString();
    std::unordered_set<std::string> unusedIds;
    if (unusedInputs.assigned())
    {
        for (const auto& component : unusedInputs)
        {
            const auto id = component.getGlobalId().toStdString();
            if (newIds.count(id) == 0)
                return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, R"(Unused input "%s" is not in the input list)", component.getGlobalId().getCharPtr());
            if (id == resolvedMainId)
                return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "The main input cannot be unused");
            unusedIds.insert(id);
        }
    }

    // Cut the notification wire: callbacks turn into no-ops; in-flight ones keep the old snapshot alive
    std::atomic_store(&wiring, std::shared_ptr<const Wiring>());

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

    if (!notificationWork.assigned())
    {
        scheduler = context.assigned() ? context.getScheduler() : nullptr;
        notificationWork = Work([this, thisRef = this->getWeakRefInternal<IMultiReader2>()]
        {
            const auto self = thisRef.getRef();
            if (!self.assigned())
                return;

            InputPortPtr port;
            EventArgsPtr<> args;
            onDataAvailable(port, args);

            // Single-shot for polling consumers; re-schedule on armDataAvailable when callback-driven
            dataManager.armDataAvailable();
        });
    }

    auto newWiring = std::make_shared<Wiring>();
    for (SizeT i = 0; i < slots.size(); i++)
        newWiring->portMap[slots[i].port.asPtr<IInputPort>(true)] = {i, slots[i].inputId};
    std::atomic_store(&wiring, std::shared_ptr<const Wiring>(std::move(newWiring)));

    MultiReaderDataManager::Config managerConfig;
    for (const auto& slot : slots)
    {
        managerConfig.inputIds.push_back(slot.inputId);
        managerConfig.usedFlags.push_back(unusedIds.count(slot.inputId.toStdString()) == 0);
    }
    managerConfig.mainInputId = mainInputId;
    managerConfig.valueReadType = newValueReadType;
    managerConfig.minReadCount = newMinReadCount;
    managerConfig.requireSameRates = newRequireSameRates;
    dataManager.reconfigure(std::move(managerConfig));

    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2Impl::getMainInput(IString** inputId)
{
    OPENDAQ_PARAM_NOT_NULL(inputId);

    std::scoped_lock lock(mutex);
    *inputId = mainInputId.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

void MultiReader2Impl::scheduleNotificationPass()
{
    if (scheduler.assigned() && notificationWork.assigned())
    {
        // A failed schedule (e.g. stopped scheduler) re-arms so a later packet can retry
        if (OPENDAQ_FAILED(scheduler->scheduleWork(notificationWork)))
        {
            daqClearErrorInfo();
            dataManager.armDataAvailable();
        }
        return;
    }

    // No scheduler in the context: degrade to running the pass inline, single-shot
    InputPortPtr port;
    EventArgsPtr<> args;
    onDataAvailable(port, args);
    dataManager.armDataAvailable();
}

ErrCode MultiReader2Impl::getAvailableCount(SizeT* count)
{
    return dataManager.getAvailableCount(count);
}

ErrCode MultiReader2Impl::read(IMultiReader2Status** status, void** data, SizeT* count, SizeT* packetOffset)
{
    return dataManager.read(status, data, count, packetOffset);
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
    return dataManager.commitEvent();
}

ErrCode MultiReader2Impl::setUsed(IString* inputId, Bool used)
{
    return dataManager.setUsed(inputId, used);
}

ErrCode MultiReader2Impl::setActive(Bool active)
{
    return dataManager.setActive(active);
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

    const auto wire = std::atomic_load(&wiring);
    if (!wire)
        return OPENDAQ_SUCCESS;

    // Normalize through queryInterface so the key matches regardless of the caller's interface path
    auto portPtr = InputPortPtr::Borrow(port);
    const auto it = wire->portMap.find(portPtr.asPtrOrNull<IInputPort>(true));
    if (it == wire->portMap.end())
        return OPENDAQ_SUCCESS;

    dataManager.connected(it->second.inputId);

    auto args = EventArgs(0, it->second.inputId);
    onConnected(portPtr, args);
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2Impl::disconnected(IInputPort* port)
{
    OPENDAQ_PARAM_NOT_NULL(port);

    const auto wire = std::atomic_load(&wiring);
    if (!wire)
        return OPENDAQ_SUCCESS;

    auto portPtr = InputPortPtr::Borrow(port);
    const auto it = wire->portMap.find(portPtr.asPtrOrNull<IInputPort>(true));
    if (it == wire->portMap.end())
        return OPENDAQ_SUCCESS;

    dataManager.disconnected(it->second.inputId);

    auto args = EventArgs(0, it->second.inputId);
    onDisconnected(portPtr, args);
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2Impl::packetReceived(IInputPort* port)
{
    OPENDAQ_PARAM_NOT_NULL(port);

    const auto wire = std::atomic_load(&wiring);
    if (!wire)
        return OPENDAQ_SUCCESS;

    auto portPtr = InputPortPtr::Borrow(port);
    const auto it = wire->portMap.find(portPtr.asPtrOrNull<IInputPort>(true));
    if (it == wire->portMap.end())
        return OPENDAQ_SUCCESS;

    const auto connection = portPtr.getConnection();
    if (!connection.assigned())
        return OPENDAQ_SUCCESS;

    const auto internal = connection.asPtrOrNull<IConnectionInternal>(true);
    if (!internal.assigned())
        return OPENDAQ_SUCCESS;

    constexpr SizeT batchCapacity = 64;
    IPacket* batch[batchCapacity];

    bool notify = false;
    SizeT count;
    do
    {
        count = batchCapacity;
        if (OPENDAQ_FAILED(internal->dequeueUpTo(batch, &count)))
            break;

        for (SizeT i = 0; i < count; i++)
        {
            const auto packet = PacketPtr::Adopt(batch[i]);
            if (dataManager.addPacket(it->second.index, packet))
                notify = true;
        }
    } while (count == batchCapacity);

    // The pass runs on a scheduler worker; the producer only enqueues the coalesced task
    if (notify)
        scheduleNotificationPass();

    return OPENDAQ_SUCCESS;
}

END_NAMESPACE_OPENDAQ
