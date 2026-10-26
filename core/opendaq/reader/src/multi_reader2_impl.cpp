#include <coretypes/validation.h>
#include <opendaq/multi_reader2_impl.h>
#include <opendaq/multi_reader2_status_ptr.h>
#include <opendaq/signal_ptr.h>

#include <unordered_set>

BEGIN_NAMESPACE_OPENDAQ

MultiReader2Impl::MultiReader2Impl(IMultiReader2Params* params)
{
    // configure registers this object as a port listener, so it must be ref-guarded during construction
    this->internalAddRef();
    try
    {
        MultiReader2StatusPtr status;
        checkErrorInfo(configure(params, &status));
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

    config.connected.assign(config.inputs.size(), false);
    config.acceptsDescriptor = acceptsDescriptor;
    config.readType = readType;
    config.minReadCount = minReadCount;
    config.errorPolicy = errorPolicy;
    return OPENDAQ_SUCCESS;
}

MultiReader2Impl::Slot MultiReader2Impl::makeSlot(const ComponentPtr& input)
{
    // TODO(spec A10): a port is held by another reader when its listener supports IMultiReader2 or IReader:
    // fail with ALREADYEXISTS. Any other listener is taken over; the port runs in Scheduler mode. A signal input
    // gets a port of its own (ownsPort)
    return {input, nullptr, false};
}

void MultiReader2Impl::releaseSlot(Slot& /*slot*/)
{
    // TODO(spec A10, C5): set the notification method to None first, then clear the listener; no previous
    // listener is restored. A port the reader created is also disconnected
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
    OPENDAQ_PARAM_NOT_NULL(params);
    OPENDAQ_PARAM_NOT_NULL(status);
    MULTI_READER2_NOT_FROM_CALLBACK();

    // TODO(spec A1, B5, A10, A11):
    // - validate with resolveParams; a rejected object leaves the reader untouched
    // - INVALIDSTATE without a scheduler (once there are inputs to take a context from)
    // - equal params: dataManager.rejudge, return its status
    // - otherwise swap the slots (makeSlot / releaseSlot), rebuild the wiring, dataManager.reconfigure, return its status
    // - schedule no onDataAvailable
    return OPENDAQ_ERR_NOTIMPLEMENTED;
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
