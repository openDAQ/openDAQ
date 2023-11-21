#include <opendaq/scaling_builder_impl.h>
#include <opendaq/signal_errors.h>
#include <coretypes/validation.h>
#include <opendaq/scaling_factory.h>
#include <opendaq/rule_private_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

ScalingBuilderImpl::ScalingBuilderImpl()
    : ScalingBuilderImpl(SampleType::Float64,
                         ScaledSampleType::Float64,
                         ScalingType::Other,
                         Dict<IString, IBaseObject>())
{
}

ScalingBuilderImpl::ScalingBuilderImpl(const ScalingPtr& scalingToCopy)
    : ScalingBuilderImpl(
        scalingToCopy.getInputSampleType(),
        scalingToCopy.getOutputSampleType(),
        scalingToCopy.getType(),
        Dict<IString, IBaseObject>()
        )
{
    auto paramsToCopy = scalingToCopy.getParameters();
    if (paramsToCopy.assigned())
    {
        for (const auto& [k, v] : paramsToCopy)
        {
            params.set(k, v);
        }
    }
}

ScalingBuilderImpl::ScalingBuilderImpl(SampleType inputType,
                                       ScaledSampleType outputType,
                                       ScalingType ruleType,
                                       DictPtr<IString, IBaseObject> params)
    : outputDataType(outputType)
    , inputDataType(inputType)
    , ruleType(ruleType)
    , params(std::move(params))
{
}

ScalingBuilderImpl::ScalingBuilderImpl(NumberPtr scale, NumberPtr offset, SampleType inputType, ScaledSampleType outputType)
    : ScalingBuilderImpl(inputType,
                         outputType,
                         ScalingType::Linear,
                         Dict<IString, IBaseObject>({
                             {"scale", scale},
                             {"offset", offset}
                         }))
{
}

ErrCode ScalingBuilderImpl::setInputDataType(SampleType type)
{
    inputDataType = type;
    return OPENDAQ_SUCCESS;
}

ErrCode ScalingBuilderImpl::setOutputDataType(ScaledSampleType type)
{
    outputDataType = type;
    return OPENDAQ_SUCCESS;
}

ErrCode ScalingBuilderImpl::setScalingType(ScalingType type)
{
    ruleType = type;
    return OPENDAQ_SUCCESS;
}

ErrCode ScalingBuilderImpl::setParameters(IDict* parameters)
{
    OPENDAQ_PARAM_NOT_NULL(parameters);

    params = parameters;
    return OPENDAQ_SUCCESS;
}

ErrCode ScalingBuilderImpl::getInputDataType(SampleType* type)
{
    OPENDAQ_PARAM_NOT_NULL(type);
    
    *type = this->inputDataType;
    return OPENDAQ_SUCCESS;
}

ErrCode ScalingBuilderImpl::getOutputDataType(ScaledSampleType* type)
{
    OPENDAQ_PARAM_NOT_NULL(type);
    
    *type = this->outputDataType;
    return OPENDAQ_SUCCESS;
}

ErrCode ScalingBuilderImpl::getScalingType(ScalingType* type)
{
    OPENDAQ_PARAM_NOT_NULL(type);
    
    *type = this->ruleType;
    return OPENDAQ_SUCCESS;
}

ErrCode ScalingBuilderImpl::getParameters(IDict** parameters)
{
    OPENDAQ_PARAM_NOT_NULL(parameters);

    *parameters = this->params;
    return OPENDAQ_SUCCESS;
}

ErrCode ScalingBuilderImpl::addParameter(IString* name, IBaseObject* parameter)
{
    OPENDAQ_PARAM_NOT_NULL(name);

    return this->params->set(name, parameter);
}

ErrCode ScalingBuilderImpl::removeParameter(IString* name)
{
    OPENDAQ_PARAM_NOT_NULL(name);

    return this->params->deleteItem(name);
}

ErrCode ScalingBuilderImpl::build(IScaling** scaling)
{
    OPENDAQ_PARAM_NOT_NULL(scaling);

    return daqTry([&]()
    {
        DictPtr<IString, IBaseObject> paramsCopy = Dict<IString, IBaseObject>();
        for (const auto& [k, v] : params)
            paramsCopy.set(k, v);
        
        auto scalingObj = Scaling(inputDataType, outputDataType, ruleType, paramsCopy);
        scalingObj.asPtr<IRulePrivate>().verifyParameters();
        *scaling = scalingObj.detach();
        return OPENDAQ_SUCCESS;
    });
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, ScalingBuilder, IScalingBuilder)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, ScalingBuilder, IScalingBuilder, createScalingBuilderFromExisting, IScaling*, scaling)

END_NAMESPACE_OPENDAQ
