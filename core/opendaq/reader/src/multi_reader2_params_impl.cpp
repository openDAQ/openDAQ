#include <coretypes/validation.h>
#include <opendaq/input_port_config_ptr.h>
#include <opendaq/multi_reader2_params_impl.h>
#include <opendaq/signal_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

MultiReader2ParamsImpl::MultiReader2ParamsImpl()
    : inputs(List<IComponent>())
    , unusedInputs(List<IComponent>())
{
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

    // All items must be inputs of the same kind: all signals or all input ports
    bool expectSignals = false;
    bool first = true;
    for (const auto& component : list)
    {
        const bool isSignal = component.asPtrOrNull<ISignal>().assigned();
        if (!isSignal && !component.asPtrOrNull<IInputPortConfig>().assigned())
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "Input is neither a signal nor an input port");

        if (first)
        {
            expectSignals = isSignal;
            first = false;
        }
        else if (isSignal != expectSignals)
        {
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "Inputs must all be signals or all input ports");
        }
    }

    std::scoped_lock lock(mutex);
    this->inputs = list;
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
    OPENDAQ_PARAM_NOT_NULL(input);

    const auto component = ComponentPtr::Borrow(input);
    if (!component.asPtrOrNull<ISignal>().assigned() && !component.asPtrOrNull<IInputPortConfig>().assigned())
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "Main input is neither a signal nor an input port");

    std::scoped_lock lock(mutex);
    mainInput = component;
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2ParamsImpl::getUnusedInputs(IList** inputs)
{
    OPENDAQ_PARAM_NOT_NULL(inputs);

    std::scoped_lock lock(mutex);
    *inputs = this->unusedInputs.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2ParamsImpl::setUnusedInputs(IList* inputs)
{
    OPENDAQ_PARAM_NOT_NULL(inputs);

    // Membership in the input list is validated by the reader's configure
    std::scoped_lock lock(mutex);
    this->unusedInputs = ListPtr<IComponent>::Borrow(inputs);
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2ParamsImpl::getValueReadType(SampleType* valueReadType)
{
    OPENDAQ_PARAM_NOT_NULL(valueReadType);

    std::scoped_lock lock(mutex);
    if (!valueReadTypeAssigned)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTASSIGNED, "Value read type is mandatory and was not set");

    *valueReadType = this->valueReadType;
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2ParamsImpl::setValueReadType(SampleType valueReadType)
{
    std::scoped_lock lock(mutex);
    this->valueReadType = valueReadType;
    valueReadTypeAssigned = true;
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
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "Minimum read count must be at least 1");

    std::scoped_lock lock(mutex);
    minReadCount = count;
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2ParamsImpl::getRequireSameRates(Bool* requireSameRates)
{
    OPENDAQ_PARAM_NOT_NULL(requireSameRates);

    std::scoped_lock lock(mutex);
    *requireSameRates = this->requireSameRates;
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2ParamsImpl::setRequireSameRates(Bool requireSameRates)
{
    std::scoped_lock lock(mutex);
    this->requireSameRates = requireSameRates;
    return OPENDAQ_SUCCESS;
}

END_NAMESPACE_OPENDAQ
