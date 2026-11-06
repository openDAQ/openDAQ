#include <coretypes/ctutils.h>
#include <coretypes/event_args_factory.h>
#include <coretypes/validation.h>
#include <opendaq/connection_internal.h>
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

// The listener on one of the reader's ports: it forwards the callbacks to the core with the port's intake, and holds
// nothing else. The port holds it weakly and the reader strongly
class MultiReader2Impl::PortListener final : public ImplementationOfWeak<IInputPortNotifications, IMultiReader2PortListener>
{
public:
    PortListener(std::shared_ptr<Core> core, std::weak_ptr<Intake> intake)
        : core(std::move(core))
        , intake(std::move(intake))
    {
    }

    ErrCode INTERFACE_FUNC acceptsSignal(IInputPort* /*port*/, ISignal* /*signal*/, Bool* accept) override
    {
        OPENDAQ_PARAM_NOT_NULL(accept);
        *accept = True;
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC connected(IInputPort* /*port*/) override
    {
        // A failure here would fail the user's connect; the reader never refuses one
        const ErrCode errCode = daqTry(
            [&]
            {
                if (const auto own = intake.lock())
                    core->applyConnect(*own);
            });
        if (OPENDAQ_FAILED(errCode))
            daqClearErrorInfo();
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC disconnected(IInputPort* /*port*/) override
    {
        return daqTry(
            [&]
            {
                if (const auto own = intake.lock())
                    core->applyDisconnect(*own);
            });
    }

    // On the sending thread: marks the port, without a reader lock
    ErrCode INTERFACE_FUNC packetReceived(IInputPort* /*port*/) override
    {
        return daqTry(
            [&]
            {
                if (const auto own = intake.lock())
                    core->notify(*own);
            });
    }

private:
    std::shared_ptr<Core> core;
    std::weak_ptr<Intake> intake;
};

MultiReader2Impl::MultiReader2Impl(IMultiReader2Params* params)
    : core(std::make_shared<Core>())
{
    // The wake takes a weak reference to this object, so it must be ref-guarded during construction
    this->internalAddRef();
    try
    {
        // One reusable call; it holds the reader weakly, and strongly while the handler runs. It lets go of the reader
        // before it asks for the next call, so no next call finds a reader its user released during this one
        core->wakeWork = Work([this, readerRef = this->getWeakRefInternal<IMultiReader2>(), weakCore = std::weak_ptr<Core>(core)]
        {
            const auto c = weakCore.lock();
            if (!c)
                return;
            bool failed;
            {
                const auto reader = readerRef.getRef();
                if (!reader.assigned())
                    return;
                failed = runWake();
            }
            if (c->dataManager.callFinished(failed))
                c->scheduleWake();
        });

        // One reusable ingest run; it holds the core weakly, and strongly while it runs
        core->ingestWork = Work([weakCore = std::weak_ptr<Core>(core)]
        {
            const auto c = weakCore.lock();
            if (c && c->ingestTask.run([&] { c->drainMarked(); }))
                c->scheduleIngest();
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
    std::atomic_store(&core->wiring, std::shared_ptr<const Wiring>());

    // An ingest run still in flight finds no slot, and leaves the port alone
    for (auto& slot : slots)
    {
        std::scoped_lock lock(slot.intake->mutex);
        slot.intake->slot.reset();
    }
    for (auto& slot : slots)
        releaseSlot(slot);
}

// ================================================================ configuration

// Calls from inside the owner's acceptsDescriptor would re-enter the data manager under its own mutex
#define MULTI_READER2_NOT_FROM_CALLBACK()                                                                                       \
    do                                                                                                                          \
    {                                                                                                                           \
        if (core->dataManager.inCallback())                                                                                     \
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDSTATE, "The reader cannot be called from acceptsDescriptor");        \
    } while (0)

ErrCode MultiReader2Impl::configure(IMultiReader2Params* params, IMultiReader2Status** status)
{
    OPENDAQ_PARAM_NOT_NULL(status);
    return configureInternal(params, status);
}

// The params object stores what it is given; every check of the params as a whole is here
ErrCode MultiReader2Impl::resolveParams(IMultiReader2Params* params, MultiReaderDataManager::Config& config)
{
    ListPtr<IComponent> inputs;
    OPENDAQ_RETURN_IF_FAILED(params->getInputs(&inputs));
    if (!inputs.assigned())
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "The input list is not assigned");

    ComponentPtr mainInput;
    OPENDAQ_RETURN_IF_FAILED(params->getMainInput(&mainInput));
    OPENDAQ_RETURN_IF_FAILED(params->getAcceptsDescriptor(&config.acceptsDescriptor));
    OPENDAQ_RETURN_IF_FAILED(params->getValueReadType(&config.readType));
    OPENDAQ_RETURN_IF_FAILED(params->getMinReadCount(&config.minReadCount));
    OPENDAQ_RETURN_IF_FAILED(params->getErrorPolicy(&config.errorPolicy));

    switch (config.readType)
    {
        case SampleType::Undefined:
        case SampleType::Float32:
        case SampleType::Float64:
        case SampleType::UInt8:
        case SampleType::Int8:
        case SampleType::UInt16:
        case SampleType::Int16:
        case SampleType::UInt32:
        case SampleType::Int32:
        case SampleType::UInt64:
        case SampleType::Int64:
            break;
        default:
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "The value read type must be numeric or Undefined");
    }
    if (config.minReadCount == 0)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "The minimum read count must be at least 1");

    // Inputs are told apart by object: a component re-created under the same global ID is another input
    std::unordered_set<IComponent*> seen;
    bool signals = false;
    for (SizeT i = 0; i < inputs.getCount(); i++)
    {
        const ComponentPtr component = inputs[i];
        if (!component.assigned())
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "An input is not assigned");

        const bool isSignal = component.asPtrOrNull<ISignal>(true).assigned();
        if (!isSignal && !component.asPtrOrNull<IInputPortConfig>(true).assigned())
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "An input is neither a signal nor an input port");
        if (i > 0 && isSignal != signals)
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "Inputs must all be signals or all input ports");
        signals = isSignal;
        if (!seen.insert(component.getObject()).second)
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_DUPLICATEITEM, R"(Input "%s" appears more than once)", component.getGlobalId().getCharPtr());

        if (mainInput.assigned() && mainInput.getObject() == component.getObject())
            config.mainSlot = i;
        config.inputs.push_back(component);
    }

    if (mainInput.assigned() && !config.mainSlot.has_value())
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, R"(Main input "%s" is not in the input list)", mainInput.getGlobalId().getCharPtr());
    return OPENDAQ_SUCCESS;
}

std::vector<MultiReader2Impl::Slot>::const_iterator MultiReader2Impl::findSlot(const ComponentPtr& input) const
{
    return std::find_if(slots.begin(), slots.end(), [&](const Slot& slot) { return slot.input.getObject() == input.getObject(); });
}

// The checks that need the reader's ports, before any port is touched: every input has a scheduler, and no new port is
// held by another reader. retained tells which inputs already have a slot
ErrCode MultiReader2Impl::checkInputs(const MultiReaderDataManager::Config& config, std::vector<bool>& retained) const
{
    retained.assign(config.inputs.size(), false);
    for (SizeT i = 0; i < config.inputs.size(); ++i)
    {
        const ComponentPtr& input = config.inputs[i];

        // The ingest task and the handler run on the context's scheduler
        const auto inputContext = input.getContext();
        if (!inputContext.assigned() || !inputContext.getScheduler().assigned())
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDSTATE, R"(The context of input "%s" has no scheduler)", input.getGlobalId().getCharPtr());

        retained[i] = findSlot(input) != slots.end();
        if (retained[i])
            continue;

        // Held by a reader: another MultiReader2's port listener, or IReader of the older readers. Any other listener is
        // taken over; the port holds it weakly, so a destroyed reader frees the port
        if (const auto port = input.asPtrOrNull<IInputPortConfig>(true); port.assigned())
        {
            const auto portListener = port.getListener();
            if (portListener.assigned() &&
                (portListener.supportsInterface<IMultiReader2PortListener>() || portListener.supportsInterface<IReader>()))
                return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ALREADYEXISTS, R"(Input port "%s" is held by another reader)", input.getGlobalId().getCharPtr());
        }
    }
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2Impl::configureInternal(IMultiReader2Params* params, IMultiReader2Status** status)
{
    OPENDAQ_PARAM_NOT_NULL(params);
    MULTI_READER2_NOT_FROM_CALLBACK();

    // A rejected object leaves the reader untouched. Inside the error boundary: a list holding something other than a
    // component throws when it is read
    MultiReaderDataManager::Config config;
    OPENDAQ_RETURN_IF_FAILED(daqTry([&] { return resolveParams(params, config); }));

    std::scoped_lock lock(mutex);
    return daqTry([&]() -> ErrCode
    {
        std::vector<bool> retained;
        OPENDAQ_RETURN_IF_FAILED(checkInputs(config, retained));
        const SizeT count = config.inputs.size();

        // The first input's context: from the takeover on, the ingest task and the handler run on its scheduler
        const auto context = config.inputs.empty() ? nullptr : config.inputs.front().getContext();
        const SchedulerPtr scheduler = context.assigned() ? context.getScheduler() : nullptr;
        const auto logger = context.assigned() ? context.getLogger() : nullptr;
        config.loggerComponent = logger.assigned() ? logger.getOrAddComponent("MultiReader2") : nullptr;

        // A failed takeover releases the new ports and leaves the old slots and wiring as they were
        const auto oldWiring = core->currentWiring();
        std::vector<Slot> newSlots;
        const auto abandon = [&]
        {
            for (SizeT i = 0; i < newSlots.size(); ++i)
            {
                if (!retained[i])
                    releaseSlot(newSlots[i]);
            }
            std::atomic_store(&core->wiring, oldWiring);
        };

        // No intake lock yet: the new ports and their signals' descriptors, which must not be read under one
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
            if (std::none_of(config.inputs.begin(), config.inputs.end(),
                             [&](const ComponentPtr& input) { return input.getObject() == slot.input.getObject(); }))
                leaving.push_back(slot);
        }

        // Every intake, so that no callback reaches the data manager with an old slot index. The wiring lists the new
        // ports before they are taken over: a packet after the takeover marks its port and waits for the intake
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
        publishWiring(interim, scheduler, config.loggerComponent);

        // A new input starts from its port's connection after the takeover; a retained one keeps what the data manager
        // saw. A port connected elsewhere since its descriptors were read starts without them, and its connect is applied
        // after this configure
        config.connected.assign(count, false);
        config.valueDescriptors.assign(count, nullptr);
        config.domainDescriptors.assign(count, nullptr);
        std::vector<std::shared_ptr<Intake>> reconnected;
        try
        {
            for (SizeT i = 0; i < count; ++i)
            {
                if (retained[i])
                    continue;
                takeOver(newSlots[i]);

                const ConnectionPtr connection = newSlots[i].port.getConnection();
                newSlots[i].intake->connection = connection.assigned() ? connection.asPtr<IConnectionInternal>() : nullptr;
                if (connection.getObject() == states[i].connection.getObject())
                {
                    config.connected[i] = states[i].connected;
                    config.valueDescriptors[i] = states[i].value;
                    config.domainDescriptors[i] = states[i].domain;
                }
                else if (connection.assigned())
                {
                    config.connected[i] = true;
                    reconnected.push_back(newSlots[i].intake);
                }
            }
        }
        catch (...)
        {
            abandon();
            throw;
        }

        // Schedules no onDataAvailable
        slots = std::move(newSlots);
        const LoggerComponentPtr loggerComponent = config.loggerComponent;
        const ErrCode reconfigured = core->dataManager.reconfigure(std::move(config), status);

        for (SizeT i = 0; i < slots.size(); ++i)
            slots[i].intake->slot = i;
        for (const auto& slot : leaving)
            slot.intake->slot.reset();
        publishWiring(slots, scheduler, loggerComponent);
        intakeLocks.clear();

        for (auto& slot : leaving)
            releaseSlot(slot);
        for (const auto& intake : reconnected)
            core->applyConnect(*intake);
        // The ports' queued packets, a connect's descriptor event first, go in now rather than ahead of the first data
        for (const auto& slot : slots)
            slot.intake->marked.store(true);
        core->drainMarked();
        return reconfigured;
    });
}

// A new input's port, not yet taken over. For a signal input the reader creates the port and connects it while the port
// has no listener and notifies nothing, so no connect callback reaches the reader
MultiReader2Impl::Slot MultiReader2Impl::makeSlot(const ComponentPtr& input)
{
    Slot slot;
    slot.input = input;
    if (const auto signal = input.asPtrOrNull<ISignal>(true); signal.assigned())
    {
        slot.port = InputPort(input.getContext(), nullptr, "multi_reader2_" + signal.getLocalId().toStdString());
        slot.port.connect(signal);
        slot.ownsPort = true;
    }
    else
    {
        slot.port = input.asPtr<IInputPortConfig>(true);
    }
    slot.intake = std::make_shared<Intake>(slot.port);
    slot.listener = createWithImplementation<IInputPortNotifications, PortListener>(core, std::weak_ptr<Intake>(slot.intake));
    return slot;
}

// Under the port's intake lock: port calls only. The listener first, so no notification finds the port without one; the
// port notifies on the sending thread, which only marks it for the ingest task
void MultiReader2Impl::takeOver(const Slot& slot)
{
    slot.port.setListener(slot.listener);
    slot.port.setNotificationMethod(PacketReadyNotification::SameThread);
}

// Never throws: it runs in the destructor and after a failed takeover. A port handed to the reader comes out without a
// listener, also from a configure that failed before taking it over
void MultiReader2Impl::releaseSlot(Slot& slot)
{
    if (!slot.port.assigned())
        return;

    const ErrCode errCode = daqTry([&]
    {
        // An own port keeps its listener, which holds the intake weakly and goes quiet with the slot: a port does not guard
        // a listener replaced while its signal sends
        if (slot.ownsPort)
        {
            slot.port.disconnect();
            return;
        }
        // The method first, so no notification finds the port without a listener. No previous listener is restored
        slot.port.setNotificationMethod(PacketReadyNotification::None);
        slot.port.setListener(nullptr);
    });
    if (OPENDAQ_FAILED(errCode))
        daqClearErrorInfo();
}

// Calls the signal, so never under an intake lock
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

void MultiReader2Impl::publishWiring(const std::vector<Slot>& wired, const SchedulerPtr& scheduler, const LoggerComponentPtr& loggerComponent)
{
    auto wiring = std::make_shared<Wiring>();
    for (const auto& slot : wired)
        wiring->intakes.push_back(slot.intake);
    wiring->scheduler = scheduler;
    wiring->loggerComponent = loggerComponent;
    std::atomic_store(&core->wiring, std::shared_ptr<const Wiring>(std::move(wiring)));
}

// ================================================================ consumer side

ErrCode MultiReader2Impl::getAvailableCount(SizeT* count)
{
    OPENDAQ_PARAM_NOT_NULL(count);
    MULTI_READER2_NOT_FROM_CALLBACK();
    *count = core->dataManager.getAvailableCount();
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2Impl::read(void* data, void* domain, SizeT* count, SizeT timeoutMs, SizeT* packetOffset, IMultiReader2Status** status)
{
    MULTI_READER2_NOT_FROM_CALLBACK();
    return core->dataManager.read(static_cast<void**>(data), static_cast<std::int64_t*>(domain), count, timeoutMs, packetOffset, status);
}

ErrCode MultiReader2Impl::skipSamples(SizeT* count, SizeT timeoutMs, SizeT* packetOffset, IMultiReader2Status** status)
{
    MULTI_READER2_NOT_FROM_CALLBACK();
    return core->dataManager.skipSamples(count, timeoutMs, packetOffset, status);
}

ErrCode MultiReader2Impl::getOnDataAvailable(IEvent** event)
{
    OPENDAQ_PARAM_NOT_NULL(event);
    *event = core->onDataAvailable.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2Impl::reevaluate(IMultiReader2Status** status)
{
    MULTI_READER2_NOT_FROM_CALLBACK();
    std::scoped_lock lock(mutex);
    return core->dataManager.reevaluate(status);
}

// ================================================================ wake

// Schedules the call the data manager claimed; one that cannot be scheduled, as on a stopped scheduler, releases the
// claim, or no call would ever be scheduled again
void MultiReader2Impl::Core::scheduleWake()
{
    // Nobody to call, as when the user reads from a loop: the claim goes, and no call is scheduled for nothing
    if (onDataAvailable.getListenerCount() == 0)
    {
        dataManager.callAborted();
        return;
    }

    const auto wire = currentWiring();
    const ErrCode errCode = wire && wire->scheduler.assigned() ? wire->scheduler->scheduleWork(wakeWork) : OPENDAQ_ERR_INVALIDSTATE;
    if (OPENDAQ_FAILED(errCode))
    {
        daqClearErrorInfo();
        dataManager.callAborted();
    }
}

// One call of the handler on the scheduler thread, with no reader lock held. One handler is assumed: one that throws
// stops the handlers after it. The exception is logged and the handler kept. Returns whether it failed
bool MultiReader2Impl::runWake()
{
    core->dataManager.callStarted();

    std::string failure;
    try
    {
        BaseObjectPtr sender;
        core->onDataAvailable(sender, EventArgs(0, "DataAvailable"));
        return false;
    }
    catch (const std::exception& e)
    {
        failure = e.what();
    }
    catch (...)
    {
        failure = "unknown exception";
    }

    daqClearErrorInfo();
    const auto wire = core->currentWiring();
    const LoggerComponentPtr loggerComponent = wire ? wire->loggerComponent : nullptr;
    if (loggerComponent.assigned())
        LOG_E("The onDataAvailable handler failed: {}", failure);
    return true;
}

// ================================================================ port callbacks and ingest

std::shared_ptr<const MultiReader2Impl::Wiring> MultiReader2Impl::Core::currentWiring() const
{
    return std::atomic_load(&wiring);
}

// The connect callback, also run by configure for a port connected elsewhere during its takeover
void MultiReader2Impl::Core::applyConnect(Intake& intake)
{
    const PortState state = readPortState(intake.port);

    bool wake;
    {
        std::scoped_lock lock(intake.mutex);
        const ConnectionPtr connection = intake.port.getConnection();
        intake.connection = connection.assigned() ? connection.asPtr<IConnectionInternal>() : nullptr;
        // A connect or disconnect since the state was read has its own callback, applied after this one
        if (!intake.slot.has_value() || connection.getObject() != state.connection.getObject())
            return;
        wake = state.connected ? dataManager.connected(*intake.slot, state.value, state.domain) : dataManager.disconnected(*intake.slot);
    }
    if (wake)
        scheduleWake();
}

void MultiReader2Impl::Core::applyDisconnect(Intake& intake)
{
    bool wake;
    {
        std::scoped_lock lock(intake.mutex);
        const ConnectionPtr connection = intake.port.getConnection();
        intake.connection = connection.assigned() ? connection.asPtr<IConnectionInternal>() : nullptr;
        if (!intake.slot.has_value())
            return;
        wake = dataManager.disconnected(*intake.slot);
    }
    if (wake)
        scheduleWake();
}

// On the sending thread, which never waits. With no ingest run scheduled or running it takes its packets in itself; a held
// lock, or a run in progress, which takes them anyway, leaves them to the ingest task
void MultiReader2Impl::Core::notify(Intake& intake)
{
    if (ingestTask.idle() && takeIn(intake))
        return;
    markForTask(intake);
}

void MultiReader2Impl::Core::markForTask(Intake& intake)
{
    intake.marked.store(true);
    if (ingestTask.notify())
        scheduleIngest();
}

// One batch of the port's packets, and the handler decision with it, under the port's and the engine's locks, both only
// tried: false when one is held. A port with more than a batch is left to the ingest task
bool MultiReader2Impl::Core::takeIn(Intake& intake)
{
    bool wake = false;
    bool more = false;
    bool dropped = false;
    {
        std::unique_lock portLock(intake.mutex, std::try_to_lock);
        if (!portLock.owns_lock())
            return false;
        const auto engineLock = dataManager.lockIngest(false);
        if (!engineLock.owns_lock())
            return false;
        if (!intake.slot.has_value())
            return true;  // the port left the reader, or has not joined it yet: configure applies its connection

        try
        {
            MultiReaderDataManager::Ingested ingested;
            more = transferBatch(intake, engineLock, ingested) == batch;
            wake = dataManager.claimWake(engineLock, ingested);
            dropped = ingested.dropped;
        }
        catch (const std::exception& e)
        {
            // Never to the sender: its packets are not the sender's failure
            daqClearErrorInfo();
            const auto wire = currentWiring();
            const LoggerComponentPtr loggerComponent = wire ? wire->loggerComponent : nullptr;
            if (loggerComponent.assigned())
                LOG_W("{}: the queued packets failed to ingest: {}", intake.port.getLocalId().toStdString(), e.what());
        }
        catch (...)
        {
            daqClearErrorInfo();
            const auto wire = currentWiring();
            const LoggerComponentPtr loggerComponent = wire ? wire->loggerComponent : nullptr;
            if (loggerComponent.assigned())
                LOG_W("{}: the queued packets failed to ingest: unknown exception", intake.port.getLocalId().toStdString());
        }
    }
    if (more)
        markForTask(intake);
    else if (dropped && ingestTask.notify())
        scheduleIngest();  // the queues the critical limit dropped are freed by the task, not by the sender
    if (wake)
        scheduleWake();
    return true;
}

// A run that cannot be scheduled, as on a stopped scheduler, leaves the task idle: the next notification schedules again
void MultiReader2Impl::Core::scheduleIngest()
{
    const auto wire = currentWiring();
    const ErrCode errCode = wire && wire->scheduler.assigned() ? wire->scheduler->scheduleWork(ingestWork) : OPENDAQ_ERR_INVALIDSTATE;
    if (OPENDAQ_FAILED(errCode))
    {
        daqClearErrorInfo();
        ingestTask.cancel();
    }
}

// The ingest run: every marked port is drained. A port that fails is logged and drained again on its next packet. The
// handler is asked for once per run, so a burst over many ports costs one call
void MultiReader2Impl::Core::drainMarked()
{
    const auto wire = currentWiring();
    if (!wire)
        return;

    MultiReaderDataManager::Ingested ingested;
    for (const auto& intake : wire->intakes)
    {
        // A plain load first: most ports of a large reader are not marked, and a load does not take the cache line
        if (!intake->marked.load(std::memory_order_relaxed) || !intake->marked.exchange(false))
            continue;
        try
        {
            ingested |= ingest(*intake);
        }
        catch (const std::exception& e)
        {
            daqClearErrorInfo();
            const LoggerComponentPtr loggerComponent = wire->loggerComponent;
            if (loggerComponent.assigned())
                LOG_W("{}: the queued packets failed to ingest: {}", intake->port.getLocalId().toStdString(), e.what());
        }
        catch (...)
        {
            daqClearErrorInfo();
            const LoggerComponentPtr loggerComponent = wire->loggerComponent;
            if (loggerComponent.assigned())
                LOG_W("{}: the queued packets failed to ingest: unknown exception", intake->port.getLocalId().toStdString());
        }
    }
    dataManager.releaseDropped();
    if (!ingested.trigger && !ingested.data)
        return;
    bool wake;
    {
        const auto engineLock = dataManager.lockIngest(true);
        wake = dataManager.claimWake(engineLock, ingested);
    }
    if (wake)
        scheduleWake();
}

// The ingest task's share of a port: its queued packets in batches, the engine's lock taken per batch, so a read is not held
// up behind a backlog. The connection is a single-consumer queue: the intake lock makes this its only consumer while configure
// may move the port. A port that keeps receiving does not hold up the others or the handler: after maxBatches it is marked
// again, and the run repeats
MultiReaderDataManager::Ingested MultiReader2Impl::Core::ingest(Intake& intake)
{
    std::scoped_lock portLock(intake.mutex);
    MultiReaderDataManager::Ingested ingested;
    if (!intake.slot.has_value())
        return ingested;

    for (SizeT batches = 0; batches < maxBatches; ++batches)
    {
        const auto engineLock = dataManager.lockIngest(true);
        if (transferBatch(intake, engineLock, ingested) < batch)
            return ingested;
    }
    intake.marked.store(true);
    ingestTask.notify();  // inside the run: it runs once more
    return ingested;
}

// Up to one batch of the port's packets to the engine, under the port's and the engine's locks; returns how many
SizeT MultiReader2Impl::Core::transferBatch(Intake& intake,
                                             const std::unique_lock<std::mutex>& engineLock,
                                             MultiReaderDataManager::Ingested& ingested)
{
    if (!intake.connection.assigned())
        return 0;

    IPacket* raw[batch];
    PacketPtr packets[batch];
    SizeT count = batch;
    checkErrorInfo(intake.connection->dequeueUpTo(raw, &count));
    // Adopted at once, so none leaks if a later step throws
    for (SizeT i = 0; i < count; ++i)
        packets[i] = PacketPtr::Adopt(raw[i]);
    if (count > 0)
        ingested |= dataManager.addPackets(engineLock, *intake.slot, packets, count);
    return count;
}

END_NAMESPACE_OPENDAQ
