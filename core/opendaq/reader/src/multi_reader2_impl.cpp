#include <coretypes/ctutils.h>
#include <coretypes/validation.h>
#include <opendaq/input_port_factory.h>
#include <opendaq/multi_reader2_impl.h>
#include <opendaq/multi_reader2_status_ptr.h>
#include <opendaq/reader.h>
#include <opendaq/signal_ptr.h>

#include <algorithm>
#include <unordered_set>

BEGIN_NAMESPACE_OPENDAQ

MultiReader2Impl::MultiReader2Impl(IMultiReader2Params* params)
{
    // configure registers this object as a port listener, so it must be ref-guarded during construction
    this->internalAddRef();
    try
    {
        checkErrorInfo(configureInternal(params, nullptr));
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
    if (!inputs.assigned())
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "The input list is not assigned");

    errCode = params->getMainInput(&mainInput);
    OPENDAQ_RETURN_IF_FAILED(errCode);

    FunctionPtr acceptsDescriptor;
    errCode = params->getAcceptsDescriptor(&acceptsDescriptor);
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

        config.inputs.push_back(component);
        if (mainInput.assigned() && mainInput.getGlobalId() == component.getGlobalId())
            config.mainSlot = i;
    }

    if (mainInput.assigned() && !config.mainSlot.has_value())
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, R"(Main input "%s" is not in the input list)", mainInput.getGlobalId().getCharPtr());

    config.acceptsDescriptor = acceptsDescriptor;
    config.readType = readType;
    config.minReadCount = minReadCount;
    config.errorPolicy = errorPolicy;
    return OPENDAQ_SUCCESS;
}

// Takes over a new input. configureInternal has checked that its context has a scheduler and that no reader
// holds its port
MultiReader2Impl::Slot MultiReader2Impl::makeSlot(const ComponentPtr& input)
{
    InputPortConfigPtr port;
    bool ownsPort = false;
    if (const auto signal = input.asPtrOrNull<ISignal>(true); signal.assigned())
    {
        // Connected while the port has no listener and notifies nothing, so no connect callback reaches the reader;
        // the slot starts from the state configureInternal reads off the port
        port = InputPort(input.getContext(), nullptr, "multi_reader2_" + signal.getLocalId().toStdString());
        port.connect(signal);
        ownsPort = true;
    }
    else
    {
        port = input.asPtr<IInputPortConfig>(true);
    }

    // TODO(spec A12): take the port's intake lock here; configureInternal holds it until the wiring is published,
    // so a connect or disconnect in between waits and is applied after it
    // The listener first: a Scheduler port without a listener aborts the process when a packet arrives
    port.setListener(this->thisPtr<InputPortNotificationsPtr>());
    port.setNotificationMethod(PacketReadyNotification::Scheduler);
    return {input, port, ownsPort};
}

// Never throws: it runs in the destructor and after a failed takeover
void MultiReader2Impl::releaseSlot(Slot& slot)
{
    if (!slot.port.assigned())
        return;

    const ErrCode errCode = daqTry([&]
    {
        // The method first: a Scheduler port without a listener aborts the process when a packet arrives. No previous
        // listener is restored
        slot.port.setNotificationMethod(PacketReadyNotification::None);
        slot.port.setListener(nullptr);
        if (slot.ownsPort)
            slot.port.disconnect();
    });
    if (OPENDAQ_FAILED(errCode))
        daqClearErrorInfo();
}

// Calls from inside the owner's acceptsDescriptor would re-enter the data manager under its own mutex
#define MULTI_READER2_NOT_FROM_CALLBACK() \
    do \
    { \
        if (dataManager.inCallback()) \
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDSTATE, "The reader cannot be called from acceptsDescriptor"); \
    } while (0)

ErrCode MultiReader2Impl::configure(IMultiReader2Params* params, IMultiReader2Status** status)
{
    OPENDAQ_PARAM_NOT_NULL(status);
    return configureInternal(params, status);
}

ErrCode MultiReader2Impl::configureInternal(IMultiReader2Params* params, IMultiReader2Status** status)
{
    OPENDAQ_PARAM_NOT_NULL(params);
    MULTI_READER2_NOT_FROM_CALLBACK();

    // A rejected object leaves the reader untouched
    MultiReaderDataManager::Config config;
    ComponentPtr mainInput;
    const ErrCode errCode = resolveParams(params, config, mainInput);
    OPENDAQ_RETURN_IF_FAILED(errCode);

    std::scoped_lock lock(mutex);

    // Never true before the first configure, so the constructor always reconfigures
    if (dataManager.isSameConfig(config))
        return dataManager.rejudge(status);

    return daqTry([&]() -> ErrCode
    {
        const auto findSlot = [this](const ComponentPtr& input)
        {
            return std::find_if(slots.begin(), slots.end(),
                                [&](const Slot& slot) { return slot.input.getGlobalId() == input.getGlobalId(); });
        };

        // Every check before any port is touched
        const SizeT count = config.inputs.size();
        std::vector<bool> retained(count, false);
        for (SizeT i = 0; i < count; ++i)
        {
            const ComponentPtr& input = config.inputs[i];

            // A port in a context without a scheduler silently falls back to SameThread notifications
            const auto inputContext = input.getContext();
            if (!inputContext.assigned() || !inputContext.getScheduler().assigned())
                return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDSTATE, R"(The context of input "%s" has no scheduler)",
                                           input.getGlobalId().getCharPtr());

            retained[i] = findSlot(input) != slots.end();
            if (retained[i])
                continue;

            // Held by a reader: one supporting IMultiReader2, or IReader of the older readers. Any other listener is
            // taken over; the port holds it weakly, so a destroyed reader frees the port
            if (const auto port = input.asPtrOrNull<IInputPortConfig>(true); port.assigned())
            {
                const auto listener = port.getListener();
                if (listener.assigned() && (listener.supportsInterface<IMultiReader2>() || listener.supportsInterface<IReader>()))
                    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ALREADYEXISTS, R"(Input port "%s" is held by another reader)",
                                               input.getGlobalId().getCharPtr());
            }
        }

        // Retained inputs keep their slot; a failed takeover releases the new ports and leaves the old slots as they were
        std::vector<Slot> newSlots;
        newSlots.reserve(count);
        try
        {
            for (SizeT i = 0; i < count; ++i)
                newSlots.push_back(retained[i] ? *findSlot(config.inputs[i]) : makeSlot(config.inputs[i]));
        }
        catch (...)
        {
            for (SizeT i = 0; i < newSlots.size(); ++i)
            {
                if (!retained[i])
                    releaseSlot(newSlots[i]);
            }
            throw;
        }

        // A new input starts from its port's state, read after the takeover; a retained one keeps what the data
        // manager saw. For a signal input the port is always connected
        config.connected.assign(count, false);
        config.valueDescriptors.assign(count, nullptr);
        config.domainDescriptors.assign(count, nullptr);
        for (SizeT i = 0; i < count; ++i)
        {
            if (retained[i])
                continue;
            const SignalPtr signal = newSlots[i].port.getSignal();
            if (!signal.assigned())
                continue;
            config.connected[i] = true;
            config.valueDescriptors[i] = signal.getDescriptor();
            if (const auto domainSignal = signal.getDomainSignal(); domainSignal.assigned())
                config.domainDescriptors[i] = domainSignal.getDescriptor();
        }

        // Inputs that left the list
        for (auto& slot : slots)
        {
            const auto stays = std::any_of(config.inputs.begin(), config.inputs.end(),
                                           [&](const ComponentPtr& input) { return input.getGlobalId() == slot.input.getGlobalId(); });
            if (!stays)
                releaseSlot(slot);
        }

        // The old wiring maps retained ports to their old slot indices
        // TODO(spec A12): until the intake locks exist, a callback from here until the new wiring is published finds
        // no slot and is dropped
        std::atomic_store(&wiring, std::shared_ptr<const Wiring>());
        slots = std::move(newSlots);
        context = slots.empty() ? nullptr : slots.front().input.getContext();
        scheduler = context.assigned() ? context.getScheduler() : nullptr;

        // Schedules no onDataAvailable
        const ErrCode reconfigured = dataManager.reconfigure(std::move(config), status);

        auto newWiring = std::make_shared<Wiring>();
        for (SizeT i = 0; i < slots.size(); ++i)
            newWiring->slotOf[slots[i].port.asPtr<IInputPort>(true)] = i;
        std::atomic_store(&wiring, std::shared_ptr<const Wiring>(std::move(newWiring)));
        return reconfigured;
    });
}

// ---------------------------------------------------------------- consumer side

ErrCode MultiReader2Impl::getAvailableCount(SizeT* count)
{
    OPENDAQ_PARAM_NOT_NULL(count);
    MULTI_READER2_NOT_FROM_CALLBACK();
    *count = dataManager.getAvailableCount();
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2Impl::read(void* data, SizeT* count, SizeT timeoutMs, SizeT* packetOffset, IMultiReader2Status** status)
{
    MULTI_READER2_NOT_FROM_CALLBACK();
    return dataManager.read(static_cast<void**>(data), count, timeoutMs, packetOffset, status);
}

ErrCode MultiReader2Impl::readWithDomain(void* data, SizeT* count, SizeT timeoutMs, IMultiReader2Status** status)
{
    MULTI_READER2_NOT_FROM_CALLBACK();
    return dataManager.readWithDomain(static_cast<void**>(data), count, timeoutMs, status);
}

ErrCode MultiReader2Impl::skipSamples(SizeT* count, SizeT timeoutMs, SizeT* packetOffset, IMultiReader2Status** status)
{
    MULTI_READER2_NOT_FROM_CALLBACK();
    return dataManager.skipSamples(count, timeoutMs, packetOffset, status);
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
    // TODO(spec C14, A11): schedule wakeWork on the scheduler; the pending flag lives in the data manager, so
    // ingest schedules only when no call is pending or running
}

void MultiReader2Impl::runWake()
{
    // TODO(spec C14, B4, A2c): raise onDataAvailable with a null sender; catch and log an exception at error level;
    // then dataManager.callFinished(failed) decides on another call
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
    // TODO(spec A12, B1): read the signal's descriptors, then under the port's intake lock dataManager.connected
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2Impl::disconnected(IInputPort* port)
{
    OPENDAQ_PARAM_NOT_NULL(port);
    // TODO(spec A12): under the port's intake lock dataManager.disconnected
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2Impl::packetReceived(IInputPort* port)
{
    OPENDAQ_PARAM_NOT_NULL(port);
    // TODO(spec A12): dequeue and ingest under the port's own intake lock, not one lock for every port
    return OPENDAQ_SUCCESS;
}

END_NAMESPACE_OPENDAQ
