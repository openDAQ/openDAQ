#include <coretypes/validation.h>
#include <opendaq/multi_reader2_params_impl.h>

BEGIN_NAMESPACE_OPENDAQ

MultiReader2ParamsImpl::MultiReader2ParamsImpl()
    : inputs(List<IComponent>())
{
}

ErrCode MultiReader2ParamsImpl::getInputs(IList** inputs)
{
    OPENDAQ_PARAM_NOT_NULL(inputs);

    std::scoped_lock lock(mutex);
    *inputs = this->inputs.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

// Stored as given: configure checks the params as a whole, so a rejected value fails there and leaves the reader as it was
ErrCode MultiReader2ParamsImpl::setInputs(IList* inputs)
{
    OPENDAQ_PARAM_NOT_NULL(inputs);

    std::scoped_lock lock(mutex);
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
    this->acceptsDescriptor = FunctionPtr(acceptsDescriptor);
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
    if (policy != MultiReader2ErrorPolicy::Invalidate && policy != MultiReader2ErrorPolicy::Exclude)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "Unknown error policy");

    std::scoped_lock lock(mutex);
    errorPolicy = policy;
    return OPENDAQ_SUCCESS;
}

END_NAMESPACE_OPENDAQ
