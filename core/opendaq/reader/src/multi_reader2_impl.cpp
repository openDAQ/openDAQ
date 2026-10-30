#include <coretypes/event_args_factory.h>
#include <coretypes/validation.h>
#include <opendaq/connection_internal.h>
#include <opendaq/custom_log.h>
#include <opendaq/input_port_factory.h>
#include <opendaq/multi_reader2_convert.h>
#include <opendaq/multi_reader2_impl.h>
#include <opendaq/multi_reader2_params_ptr.h>
#include <opendaq/multi_reader2_status_ptr.h>
#include <opendaq/reader.h>
#include <opendaq/signal_ptr.h>
#include <opendaq/work_factory.h>

BEGIN_NAMESPACE_OPENDAQ

ErrCode MultiReader2ParamsImpl::getInputs(IList** inputs)
{
    OPENDAQ_PARAM_NOT_NULL(inputs);
    std::scoped_lock lock(mutex);
    *inputs = this->inputs.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2ParamsImpl::setInputs(IList* inputs)
{
    std::scoped_lock lock(mutex);
    this->inputs = inputs;
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2ParamsImpl::getMainInput(IComponent** input)
{
    OPENDAQ_PARAM_NOT_NULL(input);
    std::scoped_lock lock(mutex);
    *input = mainInput.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2ParamsImpl::setMainInput(IComponent* input)
{
    std::scoped_lock lock(mutex);
    mainInput = input;
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2ParamsImpl::getAcceptsDescriptor(IFunction** acceptsDescriptor)
{
    OPENDAQ_PARAM_NOT_NULL(acceptsDescriptor);
    std::scoped_lock lock(mutex);
    *acceptsDescriptor = this->acceptsDescriptor.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2ParamsImpl::setAcceptsDescriptor(IFunction* acceptsDescriptor)
{
    std::scoped_lock lock(mutex);
    this->acceptsDescriptor = acceptsDescriptor;
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2ParamsImpl::getValueReadType(SampleType* valueReadType)
{
    OPENDAQ_PARAM_NOT_NULL(valueReadType);
    std::scoped_lock lock(mutex);
    *valueReadType = this->valueReadType;
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2ParamsImpl::setValueReadType(SampleType valueReadType)
{
    std::scoped_lock lock(mutex);
    this->valueReadType = valueReadType;
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2ParamsImpl::getMinReadCount(SizeT* count)
{
    OPENDAQ_PARAM_NOT_NULL(count);
    std::scoped_lock lock(mutex);
    *count = minReadCount;
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2ParamsImpl::setMinReadCount(SizeT count)
{
    std::scoped_lock lock(mutex);
    minReadCount = count;
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2ParamsImpl::getErrorPolicy(MultiReader2ErrorPolicy* policy)
{
    OPENDAQ_PARAM_NOT_NULL(policy);
    std::scoped_lock lock(mutex);
    *policy = errorPolicy;
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2ParamsImpl::setErrorPolicy(MultiReader2ErrorPolicy policy)
{
    std::scoped_lock lock(mutex);
    errorPolicy = policy;
    return OPENDAQ_SUCCESS;
}

namespace
{

multi_reader2::Announced announcedOf(const InputPortPtr& port)
{
    multi_reader2::Announced announced;
    const auto signal = port.getSignal();
    if (!signal.assigned())
        return announced;
    announced.connected = true;
    announced.value = signal.getDescriptor();
    if (const auto domainSignal = signal.getDomainSignal(); domainSignal.assigned())
        announced.domain = domainSignal.getDescriptor();
    return announced;
}

// A port another reader listens to: a MultiReader2 or one of the older readers
bool heldByOtherReader(const InputPortConfigPtr& port, const InputPortNotificationsPtr& own)
{
    const auto listener = port.getListener();
    if (!listener.assigned() || listener.getObject() == own.getObject())
        return false;
    return listener.supportsInterface<IMultiReader2Intake>() || listener.supportsInterface<IReader>();
}

}

class MultiReader2Impl::Intake : public ImplementationOfWeak<IInputPortNotifications, IMultiReader2Intake>
{
public:
    explicit Intake(std::shared_ptr<Core> core)
        : core(std::move(core))
    {
    }

    ErrCode INTERFACE_FUNC acceptsSignal(IInputPort*, ISignal*, Bool* accept) override
    {
        OPENDAQ_PARAM_NOT_NULL(accept);
        *accept = True;
        return OPENDAQ_SUCCESS;
    }
    ErrCode INTERFACE_FUNC connected(IInputPort* port) override
    {
        return daqTry([&] { core->connected(port); });
    }
    ErrCode INTERFACE_FUNC disconnected(IInputPort* port) override
    {
        return daqTry([&] { core->disconnected(port); });
    }
    ErrCode INTERFACE_FUNC packetReceived(IInputPort* port) override
    {
        return daqTry(
            [&]
            {
                if (const auto p = core->portOf(port))
                    core->ingest(*p);
            });
    }

private:
    const std::shared_ptr<Core> core;
};

MultiReader2Impl::MultiReader2Impl(IMultiReader2Params* params)
    : intake(createWithImplementation<IInputPortNotifications, Intake>(core))
{
    this->internalAddRef();
    try
    {
        core->work = Work(
            [this, weak = this->template getWeakRefInternal<IMultiReader2>(), shared = std::weak_ptr<Core>(core)]
            {
                // A reader released before the call runs is gone, and the call is skipped. The call lets go of the
                // reader before it clears its pending flag, so no next call, scheduled by ingest or by this one, can
                // find a reader its user released during this call
                const auto c = shared.lock();
                if (!c)
                    return;
                const auto triggersAtStart = c->dataManager.triggerCount();
                bool failed;
                {
                    const auto self = weak.getRef();
                    if (!self.assigned())
                        return;
                    failed = !runHandler();
                }
                if (c->dataManager.callFinished(failed, triggersAtStart, c->subscribed()))
                    c->schedule();
            });
        if (params == nullptr)
            DAQ_THROW_EXCEPTION(ArgumentNullException, "The params are null");
        apply(params, nullptr);
    }
    catch (...)
    {
        this->releaseWeakRefOnException();
        throw;
    }
}

MultiReader2Impl::~MultiReader2Impl()
{
    for (const auto& port : *core->wiring)
        release(*port);
}

// Validates the params, then takes over and releases ports and has the data manager apply them. Construction passes no status
void MultiReader2Impl::apply(IMultiReader2Params* paramsRaw, IMultiReader2Status** status)
{
    const auto params = MultiReader2ParamsPtr::Borrow(paramsRaw);
    const auto inputs = params.getInputs();
    const ComponentPtr main = params.getMainInput();
    MultiReader2DataManager::Config config;
    config.acceptsDescriptor = params.getAcceptsDescriptor();
    config.readType = params.getValueReadType();
    config.minReadCount = params.getMinReadCount();
    config.errorPolicy = params.getErrorPolicy();
    if (!inputs.assigned())
        DAQ_THROW_EXCEPTION(InvalidParameterException, "The input list is not assigned");
    if (config.minReadCount == 0)
        DAQ_THROW_EXCEPTION(InvalidParameterException, "MinReadCount must be at least 1");
    if (!multi_reader2::isReadType(config.readType))
        DAQ_THROW_EXCEPTION(InvalidParameterException, "The value read type must be Undefined or a real or integer type");
    if (config.errorPolicy != MultiReader2ErrorPolicy::Invalidate && config.errorPolicy != MultiReader2ErrorPolicy::Exclude)
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Unknown error policy");

    bool signals = false;
    for (SizeT i = 0; i < inputs.getCount(); ++i)
    {
        const ComponentPtr input = inputs[i];
        if (!input.assigned())
            DAQ_THROW_EXCEPTION(InvalidParameterException, "An input is not assigned");
        const bool isSignal = input.supportsInterface<ISignal>();
        if (!isSignal && !input.supportsInterface<IInputPortConfig>())
            DAQ_THROW_EXCEPTION(InvalidParameterException, "An input is neither a signal nor an input port");
        if (i > 0 && isSignal != signals)
            DAQ_THROW_EXCEPTION(InvalidParameterException, "The inputs must be all signals or all input ports");
        signals = isSignal;
        for (const auto& previous : config.inputs)
            if (previous.getObject() == input.getObject())
                DAQ_THROW_EXCEPTION(DuplicateItemException, "An input is listed twice");
        if (main.assigned() && main.getObject() == input.getObject())
            config.mainSlot = i;
        config.inputs.push_back(input);
    }
    if (main.assigned() && !config.mainSlot)
        DAQ_THROW_EXCEPTION(InvalidParameterException, "The main input is not in the input list");

    std::scoped_lock configureLock(configureMutex);
    const auto old = core->wiring;
    auto next = std::make_shared<Wiring>();
    std::vector<std::shared_ptr<Port>> fresh;
    for (const auto& input : config.inputs)
    {
        const auto it = std::find_if(old->begin(), old->end(), [&](const auto& p) { return p->input.getObject() == input.getObject(); });
        if (it != old->end())
        {
            next->push_back(*it);
            continue;
        }
        auto port = std::make_shared<Port>();
        port->input = input;
        if (!input.supportsInterface<ISignal>())
        {
            port->port = input.asPtr<IInputPortConfig>(true);
            if (heldByOtherReader(port->port, intake))
                DAQ_THROW_EXCEPTION(AlreadyExistsException, "An input port is held by another reader");
        }
        next->push_back(port);
        fresh.push_back(port);
    }
    SchedulerPtr scheduler;
    if (!config.inputs.empty())
    {
        const auto context = config.inputs.front().getContext();
        scheduler = context.assigned() ? context.getScheduler() : nullptr;
        if (!scheduler.assigned())
            DAQ_THROW_EXCEPTION(InvalidStateException, "The reader needs a context with a scheduler");
        // Set once, before any handler call: the handler reads it without a lock
        if (!loggerComponent.assigned())
            loggerComponent = context.getLogger().getOrAddComponent("MultiReader2");
    }

    // New ports are taken over with their intake locks held, so their packets wait until the data manager knows them
    std::vector<std::unique_lock<std::mutex>> held;
    SizeT taken = 0;
    try
    {
        for (const auto& port : fresh)
        {
            held.emplace_back(port->intake);
            if (!port->port.assigned())
            {
                const auto signal = port->input.asPtr<ISignal>(true);
                port->port = InputPort(signal.getContext(), nullptr, "MultiReader2_" + signal.getLocalId().toStdString());
                port->port.connect(signal);
                port->owned = true;
            }
            port->key = port->port.asPtr<IInputPort>(true).getObject();
            port->port.setListener(intake);
            port->port.setNotificationMethod(PacketReadyNotification::Scheduler);
            ++taken;
        }

        // Published before the descriptors are read: a later connect or disconnect then waits on the intake lock, not dropped
        {
            std::scoped_lock lock(core->wiringMutex);
            core->wiring = next;
            if (scheduler.assigned())
                core->scheduler = scheduler;
        }
        for (const auto& port : *next)
        {
            config.keys.push_back(port.get());
            config.current.push_back(announcedOf(port->port));
        }
    }
    catch (...)
    {
        // A failed take-over leaves no port with the reader: those taken over are let go, a connected own port disconnected
        {
            std::scoped_lock lock(core->wiringMutex);
            core->wiring = old;
        }
        for (SizeT i = 0; i < fresh.size(); ++i)
            if (i < taken || fresh[i]->owned)
                release(*fresh[i]);
        throw;
    }
    config.loggerComponent = loggerComponent;
    if (status)
        core->dataManager.configure(std::move(config), status);
    else
        core->dataManager.construct(std::move(config));
    held.clear();

    for (const auto& port : *old)
        if (std::find(next->begin(), next->end(), port) == next->end())
            release(*port);
    // The configuration is in effect: a port that fails to drain here is drained again on its next packet
    for (const auto& port : fresh)
    {
        try
        {
            core->ingest(*port);
        }
        catch (const std::exception& e)
        {
            LOG_W("A taken over port failed to drain: {}", e.what());
        }
    }
}

void MultiReader2Impl::release(Port& entry)
{
    // None first: a Scheduler port without a listener aborts on a packet; called from destruction, so errors are dropped
    entry.port->setNotificationMethod(PacketReadyNotification::None);
    entry.port->setListener(nullptr);
    if (entry.owned)
        entry.port->disconnect();
    daqClearErrorInfo();
}

ErrCode MultiReader2Impl::notFromJudgement() const
{
    if (core->dataManager.inCallback())
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDSTATE, "The reader cannot be called from acceptsDescriptor");
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2Impl::configure(IMultiReader2Params* params, IMultiReader2Status** status)
{
    OPENDAQ_PARAM_NOT_NULL(params);
    OPENDAQ_PARAM_NOT_NULL(status);
    OPENDAQ_RETURN_IF_FAILED(notFromJudgement());
    return daqTry([&] { apply(params, status); });
}

ErrCode MultiReader2Impl::getAvailableCount(SizeT* count)
{
    OPENDAQ_PARAM_NOT_NULL(count);
    OPENDAQ_RETURN_IF_FAILED(notFromJudgement());
    *count = core->dataManager.getAvailableCount();
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2Impl::read(void* data, SizeT* count, SizeT timeoutMs, SizeT* packetOffset, IMultiReader2Status** status)
{
    OPENDAQ_RETURN_IF_FAILED(notFromJudgement());
    return daqTry([&] { return core->dataManager.read(static_cast<void**>(data), count, timeoutMs, packetOffset, status); });
}

ErrCode MultiReader2Impl::readWithDomain(void* data, SizeT* count, SizeT timeoutMs, IMultiReader2Status** status)
{
    OPENDAQ_RETURN_IF_FAILED(notFromJudgement());
    return daqTry([&] { return core->dataManager.readWithDomain(static_cast<void**>(data), count, timeoutMs, status); });
}

ErrCode MultiReader2Impl::skipSamples(SizeT* count, SizeT timeoutMs, SizeT* packetOffset, IMultiReader2Status** status)
{
    OPENDAQ_RETURN_IF_FAILED(notFromJudgement());
    return daqTry([&] { return core->dataManager.skipSamples(count, timeoutMs, packetOffset, status); });
}

ErrCode MultiReader2Impl::getOnDataAvailable(IEvent** event)
{
    OPENDAQ_PARAM_NOT_NULL(event);
    *event = core->onDataAvailable.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

// Calls the handler; what it throws is logged, and the call counts as failed
bool MultiReader2Impl::runHandler()
{
    try
    {
        BaseObjectPtr sender;
        EventArgsPtr<> args = EventArgs(0, "DataAvailable");
        core->onDataAvailable(sender, args);
        return true;
    }
    catch (const std::exception& e)
    {
        LOG_E("The onDataAvailable handler failed: {}", e.what());
    }
    catch (...)
    {
        LOG_E("The onDataAvailable handler failed");
    }
    return false;
}

std::shared_ptr<MultiReader2Impl::Port> MultiReader2Impl::Core::portOf(IInputPort* port) const
{
    std::scoped_lock lock(wiringMutex);
    for (const auto& p : *wiring)
        if (p->key == port)
            return p;
    return nullptr;
}

void MultiReader2Impl::Core::connected(IInputPort* port)
{
    const auto p = portOf(port);
    if (!p)
        return;
    // The descriptors are read before the intake lock: the signal may be sending
    const auto announced = announcedOf(p->port);
    std::scoped_lock lock(p->intake);
    if (dataManager.connected(p.get(), announced, subscribed()))
        schedule();
}

void MultiReader2Impl::Core::disconnected(IInputPort* port)
{
    const auto p = portOf(port);
    if (!p)
        return;
    std::scoped_lock lock(p->intake);
    if (dataManager.disconnected(p.get(), subscribed()))
        schedule();
}

void MultiReader2Impl::Core::ingest(Port& port)
{
    std::scoped_lock lock(port.intake);
    const auto connection = port.port.getConnection();
    if (!connection.assigned())
        return;
    const auto internal = connection.asPtr<IConnectionInternal>(true);
    const bool listened = subscribed();
    bool wake = false;
    constexpr SizeT batch = 64;
    IPacket* packets[batch];
    PacketPtr adopted[batch];
    SizeT count;
    do
    {
        count = batch;
        checkErrorInfo(internal->dequeueUpTo(packets, &count));
        // Adopted before any is ingested, so a packet that fails to ingest leaks none behind it
        for (SizeT i = 0; i < count; ++i)
            adopted[i] = PacketPtr::Adopt(packets[i]);
        for (SizeT i = 0; i < count; ++i)
            wake = dataManager.addPacket(&port, adopted[i], listened) || wake;
    } while (count == batch);
    if (wake)
        schedule();
}

bool MultiReader2Impl::Core::subscribed() const
{
    return onDataAvailable.getListenerCount() > 0;
}

void MultiReader2Impl::Core::schedule()
{
    const auto current = [this]
    {
        std::scoped_lock lock(wiringMutex);
        return scheduler;
    }();
    // A stopped scheduler takes no more work: the reader is being torn down with it
    if (OPENDAQ_FAILED(current->scheduleWork(work)))
        daqClearErrorInfo();
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, MultiReader2Params, IMultiReader2Params)
OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, MultiReader2, IMultiReader2, IMultiReader2Params*, params)

END_NAMESPACE_OPENDAQ
