#include <coreobjects/ownable_ptr.h>
#include <coreobjects/property_object_factory.h>
#include <coretypes/event_args_factory.h>
#include <coretypes/validation.h>
#include <opendaq/connection_internal.h>
#include <opendaq/input_port_factory.h>
#include <opendaq/multi_reader2_impl.h>
#include <opendaq/signal_ptr.h>
#include <opendaq/tags_private_ptr.h>
#include <opendaq/work_factory.h>

#include <unordered_set>

BEGIN_NAMESPACE_OPENDAQ

MultiReader2Impl::MultiReader2Impl(IMultiReader2Params* params)
{
    // configure registers this object as a port listener, so it must be ref-guarded during construction
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

MultiReader2Impl::~MultiReader2Impl()
{
    std::atomic_store(&wiring, std::shared_ptr<const Wiring>());
    for (auto& slot : slots)
        releaseSlot(slot);
    slots.clear();
}

MultiReaderDataManager& MultiReader2Impl::getDataManager()
{
    return dataManager;
}

// ---------------------------------------------------------------- configuration

ErrCode MultiReader2Impl::resolveParams(IMultiReader2Params* params, MultiReaderDataManager::Config& config, ComponentPtr& mainInput)
{
    ListPtr<IComponent> inputs;
    ErrCode errCode = params->getInputs(&inputs);
    OPENDAQ_RETURN_IF_FAILED(errCode);
    if (!inputs.assigned() || inputs.getCount() == 0)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "The input list must not be empty");

    errCode = params->getMainInput(&mainInput);
    OPENDAQ_RETURN_IF_FAILED(errCode);

    Bool used;
    errCode = params->getUsed(&used);
    OPENDAQ_RETURN_IF_FAILED(errCode);

    SampleType readType;
    errCode = params->getValueReadType(&readType);
    OPENDAQ_RETURN_IF_FAILED(errCode);

    SizeT minReadCount;
    errCode = params->getMinReadCount(&minReadCount);
    OPENDAQ_RETURN_IF_FAILED(errCode);

    MultiReader2ErrorPolicy errorPolicy;
    errCode = params->getErrorPolicy(&errorPolicy);
    OPENDAQ_RETURN_IF_FAILED(errCode);

    std::unordered_set<std::string> ids;
    bool expectSignals = false;
    for (SizeT i = 0; i < inputs.getCount(); i++)
    {
        const ComponentPtr component = inputs[i];
        if (!component.assigned())
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "An input is not assigned");

        const bool isSignal = component.asPtrOrNull<ISignal>(true).assigned();
        if (!isSignal && !component.asPtrOrNull<IInputPortConfig>(true).assigned())
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "An input is neither a signal nor an input port");
        if (i == 0)
            expectSignals = isSignal;
        else if (isSignal != expectSignals)
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "Inputs must all be signals or all input ports");
        if (!ids.insert(component.getGlobalId().toStdString()).second)
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_DUPLICATEITEM, R"(Input "%s" appears more than once)", component.getGlobalId().getCharPtr());

        Bool inputUsed;
        errCode = params->getInputUsed(component, &inputUsed);
        OPENDAQ_RETURN_IF_FAILED(errCode);

        config.inputs.push_back(component);
        config.inputUsed.push_back(inputUsed);
        if (mainInput.assigned() && mainInput.getGlobalId() == component.getGlobalId())
        {
            if (!inputUsed)
                return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "The pinned main input cannot be unused");
            config.mainSlot = i;
        }
    }

    if (mainInput.assigned() && !config.mainSlot.has_value())
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, R"(Main input "%s" is not in the input list)", mainInput.getGlobalId().getCharPtr());

    config.connected.assign(config.inputs.size(), false);
    config.used = used;
    config.readType = readType;
    config.minReadCount = minReadCount;
    config.errorPolicy = errorPolicy;
    return OPENDAQ_SUCCESS;
}

MultiReader2Impl::Slot MultiReader2Impl::makeSlot(const ComponentPtr& input)
{
    const auto listener = this->thisPtr<InputPortNotificationsPtr>();

    if (const auto signal = input.asPtrOrNull<ISignal>(true); signal.assigned())
    {
        auto port = InputPort(context, nullptr, "multi_reader2_" + signal.getLocalId().toStdString());
        port.getTags().asPtr<ITagsPrivate>().add("MultiReaderInternalPort");
        port.setListener(listener);
        port.setNotificationMethod(PacketReadyNotification::Scheduler);
        return {input, port, true};
    }

    auto port = input.asPtr<IInputPortConfig>(true);
    if (!port.getParent().assigned())
    {
        if (!portBinder.assigned())
            portBinder = PropertyObject();
        port.asPtr<IOwnable>(true).setOwner(portBinder);
    }
    port.setListener(listener);
    port.setNotificationMethod(PacketReadyNotification::Scheduler);
    return {input, port, false};
}

void MultiReader2Impl::releaseSlot(Slot& slot)
{
    if (!slot.port.assigned())
        return;
    if (slot.ownsPort)
        slot.port.disconnect();
    // A port in Scheduler mode without a listener cannot take packets, so notifications are switched off first
    slot.port.setNotificationMethod(PacketReadyNotification::None);
    slot.port.setListener(nullptr);
    slot.port.release();
}

ErrCode MultiReader2Impl::configure(IMultiReader2Params* params)
{
    OPENDAQ_PARAM_NOT_NULL(params);

    MultiReaderDataManager::Config config;
    ComponentPtr mainInput;
    ErrCode errCode = resolveParams(params, config, mainInput);
    OPENDAQ_RETURN_IF_FAILED(errCode);

    std::unique_lock lock(mutex);

    if (dataManager.isSameConfig(config))
        return OPENDAQ_SUCCESS;

    if (!context.assigned())
    {
        context = config.inputs.front().getContext();
        if (context.assigned())
            scheduler = context.getScheduler();
    }

    // Callbacks turn into no-ops until the new wiring is in place; packets wait in the connections meanwhile
    const auto oldWiring = std::atomic_load(&wiring);
    std::atomic_store(&wiring, std::shared_ptr<const Wiring>());

    std::vector<Slot> newSlots;
    std::vector<bool> fresh;
    newSlots.reserve(config.inputs.size());
    const std::vector<Slot> oldSlots = slots;
    try
    {
        for (const auto& input : config.inputs)
        {
            const auto id = input.getGlobalId();
            auto it = std::find_if(slots.begin(), slots.end(), [&id](const Slot& slot) { return slot.input.getGlobalId() == id; });
            if (it != slots.end())
            {
                newSlots.push_back(std::move(*it));
                slots.erase(it);
                fresh.push_back(false);
            }
            else
            {
                newSlots.push_back(makeSlot(input));
                fresh.push_back(true);
            }
        }
    }
    catch (const DaqException& e)
    {
        // A port that cannot be taken over (owned by another reader) fails the call and leaves every port as it was
        for (SizeT i = 0; i < newSlots.size(); i++)
        {
            if (fresh[i])
                releaseSlot(newSlots[i]);
        }
        slots = oldSlots;
        std::atomic_store(&wiring, oldWiring);
        return errorFromException(e);
    }
    for (auto& slot : slots)
        releaseSlot(slot);
    slots = std::move(newSlots);

    // New slots start from what their signal announces now, so data arriving before the first read is kept
    config.valueDescriptors.assign(slots.size(), nullptr);
    config.domainDescriptors.assign(slots.size(), nullptr);
    for (SizeT i = 0; i < slots.size(); i++)
    {
        const Slot& slot = slots[i];
        SignalPtr signal = slot.ownsPort ? slot.input.asPtr<ISignal>(true) : slot.port.getSignal();
        config.connected[i] = signal.assigned();
        if (fresh[i] && signal.assigned())
        {
            config.valueDescriptors[i] = signal.getDescriptor();
            if (const auto domainSignal = signal.getDomainSignal(); domainSignal.assigned())
                config.domainDescriptors[i] = domainSignal.getDescriptor();
        }
    }

    if (!wakeWork.assigned())
    {
        wakeWork = Work([this, thisRef = this->getWeakRefInternal<IMultiReader2>()]
        {
            const auto self = thisRef.getRef();
            if (self.assigned())
                runWake();
        });
    }

    dataManager.reconfigure(std::move(config));

    auto newWiring = std::make_shared<Wiring>();
    for (SizeT i = 0; i < slots.size(); i++)
        newWiring->slotOf[slots[i].port.asPtrOrNull<IInputPort>(true)] = i;
    std::atomic_store(&wiring, std::shared_ptr<const Wiring>(std::move(newWiring)));

    std::vector<Slot> snapshot = slots;
    lock.unlock();

    // Outside the lock: connecting and draining run the port callbacks, which may schedule the wake
    for (const auto& slot : snapshot)
    {
        if (slot.ownsPort && !slot.port.getConnection().assigned())
            slot.port.connect(slot.input.asPtr<ISignal>(true));
    }
    bool wake = false;
    for (SizeT i = 0; i < snapshot.size(); i++)
    {
        const auto& slot = snapshot[i];
        const auto connection = slot.port.getConnection();
        // A connection edge that fell into the window without wiring is re-derived from the port
        if (dataManager.setConnected(i, connection.assigned()).wake)
            wake = true;
        if (!connection.assigned())
            continue;
        const auto internal = connection.asPtrOrNull<IConnectionInternal>(true);
        if (!internal.assigned())
            continue;
        // The current descriptors are re-announced so the manager sees the state at the end of the ingested data
        if (OPENDAQ_SUCCEEDED(internal->enqueueLastDescriptor()))
            packetReceived(slot.port);
    }

    // The first status after a configure always carries changes
    (void) wake;
    scheduleWake();
    return OPENDAQ_SUCCESS;
}

// ---------------------------------------------------------------- consumer side

ErrCode MultiReader2Impl::getMainInput(IString** inputId)
{
    OPENDAQ_PARAM_NOT_NULL(inputId);
    *inputId = dataManager.getMainInputId().detach();
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2Impl::getAvailableCount(SizeT* count)
{
    OPENDAQ_PARAM_NOT_NULL(count);
    *count = dataManager.getAvailableCount();
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2Impl::read(void* data, SizeT* count, SizeT* packetOffset, IMultiReader2Status** status)
{
    return dataManager.read(static_cast<void**>(data), count, packetOffset, status);
}

ErrCode MultiReader2Impl::readWithDomain(void* data, SizeT* count, IMultiReader2Status** status)
{
    return dataManager.readWithDomain(static_cast<void**>(data), count, status);
}

ErrCode MultiReader2Impl::skipSamples(SizeT* count, IMultiReader2Status** status)
{
    return dataManager.skipSamples(count, status);
}

ErrCode MultiReader2Impl::getOnDataAvailable(IEvent** event)
{
    OPENDAQ_PARAM_NOT_NULL(event);
    *event = onDataAvailable.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

// ---------------------------------------------------------------- wake

void MultiReader2Impl::scheduleWake()
{
    if (wakePending.exchange(true))
        return;

    if (scheduler.assigned())
    {
        if (OPENDAQ_FAILED(scheduler->scheduleWork(wakeWork)))
        {
            daqClearErrorInfo();
            wakePending = false;
        }
        return;
    }

    // No scheduler in the context: the wake degrades to running inline
    runWake();
}

void MultiReader2Impl::runWake()
{
    const auto generation = dataManager.getWakeGeneration();

    if (onDataAvailable.getListenerCount() > 0)
    {
        InputPortPtr sender;
        auto args = EventArgs(0, "DataAvailable");
        onDataAvailable(sender, args);
    }

    // Anything that arrived while the handler ran owes another pass
    if (dataManager.getWakeGeneration() != generation)
    {
        if (scheduler.assigned())
        {
            if (OPENDAQ_FAILED(scheduler->scheduleWork(wakeWork)))
            {
                daqClearErrorInfo();
                wakePending = false;
            }
            return;
        }
        wakePending = false;
        scheduleWake();
        return;
    }

    wakePending = false;
    if (dataManager.getWakeGeneration() != generation)
        scheduleWake();
}

// ---------------------------------------------------------------- port callbacks

std::optional<SizeT> MultiReader2Impl::slotOf(IInputPort* port) const
{
    const auto wire = std::atomic_load(&wiring);
    if (!wire)
        return std::nullopt;
    const auto key = InputPortPtr::Borrow(port).asPtrOrNull<IInputPort>(true);
    const auto it = wire->slotOf.find(key);
    if (it == wire->slotOf.end())
        return std::nullopt;
    return it->second;
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
    const auto slot = slotOf(port);
    if (!slot.has_value())
        return OPENDAQ_SUCCESS;
    if (dataManager.connected(*slot).wake)
        scheduleWake();
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2Impl::disconnected(IInputPort* port)
{
    OPENDAQ_PARAM_NOT_NULL(port);
    const auto slot = slotOf(port);
    if (!slot.has_value())
        return OPENDAQ_SUCCESS;
    if (dataManager.disconnected(*slot).wake)
        scheduleWake();
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2Impl::packetReceived(IInputPort* port)
{
    OPENDAQ_PARAM_NOT_NULL(port);
    const auto slot = slotOf(port);
    if (!slot.has_value())
        return OPENDAQ_SUCCESS;

    const auto portPtr = InputPortPtr::Borrow(port);
    const auto connection = portPtr.getConnection();
    if (!connection.assigned())
        return OPENDAQ_SUCCESS;
    const auto internal = connection.asPtrOrNull<IConnectionInternal>(true);
    if (!internal.assigned())
        return OPENDAQ_SUCCESS;

    constexpr SizeT batchCapacity = 64;
    IPacket* batch[batchCapacity];
    bool wake = false;
    SizeT count;
    do
    {
        count = batchCapacity;
        if (OPENDAQ_FAILED(internal->dequeueUpTo(batch, &count)))
        {
            daqClearErrorInfo();
            break;
        }
        for (SizeT i = 0; i < count; i++)
        {
            const auto packet = PacketPtr::Adopt(batch[i]);
            if (dataManager.addPacket(*slot, packet).wake)
                wake = true;
        }
    } while (count == batchCapacity);

    if (wake)
        scheduleWake();
    return OPENDAQ_SUCCESS;
}

END_NAMESPACE_OPENDAQ
