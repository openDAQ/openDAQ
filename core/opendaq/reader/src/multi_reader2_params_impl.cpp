#include <coretypes/validation.h>
#include <opendaq/input_port_config_ptr.h>
#include <opendaq/multi_reader2_params_impl.h>
#include <opendaq/signal_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

MultiReader2ParamsImpl::MultiReader2ParamsImpl()
    : inputs(List<IComponent>())
{
}

bool MultiReader2ParamsImpl::contains(const std::string& globalId) const
{
    for (const auto& component : inputs)
    {
        if (component.getGlobalId().toStdString() == globalId)
            return true;
    }
    return false;
}

ErrCode MultiReader2ParamsImpl::getInputs(IList** inputs)
{
    OPENDAQ_PARAM_NOT_NULL(inputs);

    std::scoped_lock lock(mutex);
    *inputs = this->inputs.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2ParamsImpl::setInputs(IList* inputs)
{
    OPENDAQ_PARAM_NOT_NULL(inputs);

    const auto list = ListPtr<IComponent>::Borrow(inputs);
    if (list.getCount() == 0)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "The input list must not be empty");

    bool expectSignals = false;
    std::unordered_set<std::string> ids;
    for (SizeT i = 0; i < list.getCount(); i++)
    {
        const auto component = list[i];
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
    }

    std::scoped_lock lock(mutex);
    // Used flags of inputs that stay in the list are kept; the others are forgotten
    for (auto it = unusedIds.begin(); it != unusedIds.end();)
    {
        if (ids.count(*it) == 0)
            it = unusedIds.erase(it);
        else
            ++it;
    }
    this->inputs = ListPtr<IComponent>(inputs);
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
    mainInput = ComponentPtr(input);
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2ParamsImpl::getInputUsed(IComponent* input, Bool* used)
{
    OPENDAQ_PARAM_NOT_NULL(input);
    OPENDAQ_PARAM_NOT_NULL(used);

    std::scoped_lock lock(mutex);
    const auto id = ComponentPtr::Borrow(input).getGlobalId().toStdString();
    if (!contains(id))
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, R"(Input "%s" is not in the input list)", id.c_str());

    *used = unusedIds.count(id) == 0;
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2ParamsImpl::setInputUsed(IComponent* input, Bool used)
{
    OPENDAQ_PARAM_NOT_NULL(input);

    std::scoped_lock lock(mutex);
    const auto id = ComponentPtr::Borrow(input).getGlobalId().toStdString();
    if (!contains(id))
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, R"(Input "%s" is not in the input list)", id.c_str());
    if (!used && mainInput.assigned() && mainInput.getGlobalId().toStdString() == id)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "The pinned main input cannot be unused");

    if (used)
        unusedIds.erase(id);
    else
        unusedIds.insert(id);
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2ParamsImpl::getUsed(Bool* used)
{
    OPENDAQ_PARAM_NOT_NULL(used);

    std::scoped_lock lock(mutex);
    *used = this->used;
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2ParamsImpl::setUsed(Bool used)
{
    std::scoped_lock lock(mutex);
    this->used = used;
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
    switch (valueReadType)
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
    if (count == 0)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "The minimum read count must be at least 1");

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
    if (policy != MultiReader2ErrorPolicy::Invalidate && policy != MultiReader2ErrorPolicy::Exclude)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "Unknown error policy");

    std::scoped_lock lock(mutex);
    errorPolicy = policy;
    return OPENDAQ_SUCCESS;
}

END_NAMESPACE_OPENDAQ
