#include <coretypes/ctutils.h>
#include <coretypes/event_args_factory.h>
#include <coretypes/validation.h>
#include <opendaq/custom_log.h>
#include <opendaq/input_port_factory.h>
#include <opendaq/multi_reader2_impl.h>
#include <opendaq/multi_reader2_status_ptr.h>
#include <opendaq/reader.h>
#include <opendaq/signal_ptr.h>
#include <opendaq/work_factory.h>

#include <algorithm>
#include <unordered_set>

BEGIN_NAMESPACE_OPENDAQ

MultiReader2Impl::MultiReader2Impl(IMultiReader2Params* params)
{
    // configure registers this object as a port listener, so it must be ref-guarded during construction
    this->internalAddRef();
    try
    {
        // One reusable call; it holds the reader weakly, and strongly while it runs (W4, W5)
        wakeWork = Work([this, readerRef = this->getWeakRefInternal<IMultiReader2>()]
        {
            const auto reader = readerRef.getRef();
            if (reader.assigned())
                runWake();
        });
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

// A new input's port, not yet taken over. For a signal input the reader creates the port and connects it while the
// port has no listener and notifies nothing, so no connect callback reaches the reader
MultiReader2Impl::Slot MultiReader2Impl::makeSlot(const ComponentPtr& input)
{
    if (const auto signal = input.asPtrOrNull<ISignal>(true); signal.assigned())
    {
        InputPortConfigPtr port = InputPort(input.getContext(), nullptr, "multi_reader2_" + signal.getLocalId().toStdString());
        port.connect(signal);
        return {input, port, true, std::make_shared<Intake>()};
    }
    return {input, input.asPtr<IInputPortConfig>(true), false, std::make_shared<Intake>()};
}

// Under the port's intake lock: port calls only
void MultiReader2Impl::takeOver(const Slot& slot)
{
    // The listener first: a Scheduler port without a listener aborts the process when a packet arrives
    slot.port.setListener(this->thisPtr<InputPortNotificationsPtr>());
    slot.port.setNotificationMethod(PacketReadyNotification::Scheduler);
}

// Never throws: it runs in the destructor and after a failed takeover. A port handed to the reader comes out without
// a listener, also from a configure that failed before taking it over
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

// Calls the signal, so never under an intake lock (C18)
MultiReader2Impl::PortState MultiReader2Impl::readPortState(const InputPortPtr& port)
{
    PortState state;
    state.connection = port.getConnection();
    if (!state.connection.assigned())
        return state;

    const SignalPtr signal = state.connection.getSignal();
    if (!signal.assigned())
        return state;

    state.connected = true;
    state.value = signal.getDescriptor();
    if (const auto domainSignal = signal.getDomainSignal(); domainSignal.assigned())
        state.domain = domainSignal.getDescriptor();
    return state;
}

void MultiReader2Impl::publishWiring(const std::vector<Slot>& wired)
{
    auto newWiring = std::make_shared<Wiring>();
    for (const auto& slot : wired)
        newWiring->intakeOf[slot.port.asPtr<IInputPort>(true)] = slot.intake;
    newWiring->scheduler = scheduler;
    newWiring->loggerComponent = loggerComponent;
    std::atomic_store(&wiring, std::shared_ptr<const Wiring>(std::move(newWiring)));
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

        // A failed takeover releases the new ports and leaves the old slots and wiring as they were
        const auto oldWiring = std::atomic_load(&wiring);
        std::vector<Slot> newSlots;
        const auto abandon = [&]
        {
            for (SizeT i = 0; i < newSlots.size(); ++i)
            {
                if (!retained[i])
                    releaseSlot(newSlots[i]);
            }
            std::atomic_store(&wiring, oldWiring);
        };

        // No intake lock yet: the new ports and their signals' descriptors, which must not be read under one (C18)
        std::vector<PortState> states(count);
        newSlots.reserve(count);
        try
        {
            for (SizeT i = 0; i < count; ++i)
            {
                newSlots.push_back(retained[i] ? *findSlot(config.inputs[i]) : makeSlot(config.inputs[i]));
                if (!retained[i])
                    states[i] = readPortState(newSlots[i].port);
            }
        }
        catch (...)
        {
            abandon();
            throw;
        }

        std::vector<Slot> leaving;
        for (const auto& slot : slots)
        {
            const auto stays = std::any_of(config.inputs.begin(), config.inputs.end(),
                                           [&](const ComponentPtr& input) { return input.getGlobalId() == slot.input.getGlobalId(); });
            if (!stays)
                leaving.push_back(slot);
        }

        // Every intake, so that no callback reaches the data manager with an old slot index. The wiring lists the new
        // ports before they are taken over: a callback after the takeover finds the intake and waits for it
        std::vector<std::unique_lock<std::mutex>> intakeLocks;
        intakeLocks.reserve(newSlots.size() + leaving.size());
        for (const auto& slot : newSlots)
            intakeLocks.emplace_back(slot.intake->mutex);
        for (const auto& slot : leaving)
            intakeLocks.emplace_back(slot.intake->mutex);

        std::vector<Slot> interim(slots);
        for (SizeT i = 0; i < count; ++i)
        {
            if (!retained[i])
                interim.push_back(newSlots[i]);
        }
        publishWiring(interim);

        // A new input starts from its port's connection after the takeover; a retained one keeps what the data
        // manager saw. A port connected elsewhere since its descriptors were read starts without them, and its
        // connect is applied after this configure
        config.connected.assign(count, false);
        config.valueDescriptors.assign(count, nullptr);
        config.domainDescriptors.assign(count, nullptr);
        std::vector<IInputPort*> reconnected;
        try
        {
            for (SizeT i = 0; i < count; ++i)
            {
                if (retained[i])
                    continue;
                takeOver(newSlots[i]);

                const ConnectionPtr connection = newSlots[i].port.getConnection();
                if (connection.getObject() == states[i].connection.getObject())
                {
                    config.connected[i] = states[i].connected;
                    config.valueDescriptors[i] = states[i].value;
                    config.domainDescriptors[i] = states[i].domain;
                }
                else if (connection.assigned())
                {
                    config.connected[i] = true;
                    reconnected.push_back(newSlots[i].port.asPtr<IInputPort>(true));
                }
            }
        }
        catch (...)
        {
            abandon();
            throw;
        }

        slots = std::move(newSlots);
        context = slots.empty() ? nullptr : slots.front().input.getContext();
        scheduler = context.assigned() ? context.getScheduler() : nullptr;
        const auto logger = context.assigned() ? context.getLogger() : nullptr;
        loggerComponent = logger.assigned() ? logger.getOrAddComponent("MultiReader2") : nullptr;

        // Schedules no onDataAvailable
        const ErrCode reconfigured = dataManager.reconfigure(std::move(config), status);

        for (SizeT i = 0; i < slots.size(); ++i)
            slots[i].intake->slot = i;
        for (const auto& slot : leaving)
            slot.intake->slot.reset();
        publishWiring(slots);
        intakeLocks.clear();

        for (auto& slot : leaving)
            releaseSlot(slot);
        for (const auto port : reconnected)
            applyConnect(port);
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

ErrCode MultiReader2Impl::read(void* data, void* domain, SizeT* count, SizeT timeoutMs, SizeT* packetOffset, IMultiReader2Status** status)
{
    MULTI_READER2_NOT_FROM_CALLBACK();
    return dataManager.read(static_cast<void**>(data), static_cast<std::int64_t*>(domain), count, timeoutMs, packetOffset, status);
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

ErrCode MultiReader2Impl::reevaluate(IMultiReader2Status** status)
{
    MULTI_READER2_NOT_FROM_CALLBACK();
    std::scoped_lock lock(mutex);
    return dataManager.reevaluate(status);
}

// ---------------------------------------------------------------- wake

// Schedules the call the data manager claimed (W4); one that cannot be scheduled, as on a stopped scheduler, releases
// the claim, or no call would ever be scheduled again
void MultiReader2Impl::scheduleWake()
{
    const auto wire = std::atomic_load(&wiring);
    const ErrCode errCode = wire && wire->scheduler.assigned() ? wire->scheduler->scheduleWork(wakeWork) : OPENDAQ_ERR_INVALIDSTATE;
    if (OPENDAQ_FAILED(errCode))
    {
        daqClearErrorInfo();
        dataManager.callAborted();
    }
}

// One call of the handler on the scheduler thread, with no reader lock held (W5). One handler is assumed: one that
// throws stops the handlers after it. The exception is logged and the handler kept (B4)
void MultiReader2Impl::runWake()
{
    dataManager.callStarted();

    bool failed = false;
    std::string failure;
    try
    {
        BaseObjectPtr sender;
        onDataAvailable(sender, EventArgs(0, "DataAvailable"));
    }
    catch (const std::exception& e)
    {
        failed = true;
        failure = e.what();
    }
    catch (...)
    {
        failed = true;
        failure = "unknown exception";
    }

    if (failed)
    {
        daqClearErrorInfo();
        const auto wire = std::atomic_load(&wiring);
        const LoggerComponentPtr loggerComponent = wire ? wire->loggerComponent : nullptr;
        if (loggerComponent.assigned())
            LOG_E("The onDataAvailable handler failed: {}", failure);
    }

    if (dataManager.callFinished(failed))
        scheduleWake();
}

// ---------------------------------------------------------------- port callbacks

std::shared_ptr<MultiReader2Impl::Intake> MultiReader2Impl::intakeOf(IInputPort* port) const
{
    const auto wire = std::atomic_load(&wiring);
    if (!wire)
        return nullptr;
    const auto key = InputPortPtr::Borrow(port).asPtrOrNull<IInputPort>(true);
    const auto it = wire->intakeOf.find(key);
    if (it == wire->intakeOf.end())
        return nullptr;
    return it->second;
}

// The connect callback, also run by configure for a port connected elsewhere during its takeover
void MultiReader2Impl::applyConnect(IInputPort* port)
{
    const auto portPtr = InputPortPtr::Borrow(port);
    const PortState state = readPortState(portPtr);
    const auto intake = intakeOf(port);
    if (!intake)
        return;

    bool wake;
    {
        std::scoped_lock lock(intake->mutex);
        // A connect or disconnect since the state was read has its own callback, applied after this one
        if (!intake->slot || portPtr.getConnection().getObject() != state.connection.getObject())
            return;
        wake = state.connected ? dataManager.connected(*intake->slot, state.value, state.domain)
                               : dataManager.disconnected(*intake->slot);
    }
    if (wake)
        scheduleWake();
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

    // A failure here would fail the user's connect; the reader never refuses one
    const ErrCode errCode = daqTry([&] { applyConnect(port); });
    if (OPENDAQ_FAILED(errCode))
        daqClearErrorInfo();
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2Impl::disconnected(IInputPort* port)
{
    OPENDAQ_PARAM_NOT_NULL(port);

    return daqTry([&]
    {
        const auto intake = intakeOf(port);
        if (!intake)
            return;

        bool wake;
        {
            std::scoped_lock lock(intake->mutex);
            if (!intake->slot)
                return;
            wake = dataManager.disconnected(*intake->slot);
        }
        if (wake)
            scheduleWake();
    });
}

ErrCode MultiReader2Impl::packetReceived(IInputPort* port)
{
    OPENDAQ_PARAM_NOT_NULL(port);

    return daqTry([&]
    {
        const auto intake = intakeOf(port);
        if (!intake)
            return;

        bool wake = false;
        {
            // The connection is a single-consumer queue, and Scheduler ports notify on several threads: the intake
            // lock makes this call its only consumer and keeps the batches in order
            std::scoped_lock lock(intake->mutex);
            if (!intake->slot)
                return;
            const auto connection = InputPortPtr::Borrow(port).getConnection();
            if (!connection.assigned())
                return;
            for (const auto& packet : connection.dequeueAll())
                wake = dataManager.addPacket(*intake->slot, packet) || wake;
        }
        if (wake)
            scheduleWake();
    });
}

END_NAMESPACE_OPENDAQ
