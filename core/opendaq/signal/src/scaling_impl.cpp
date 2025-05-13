#include <coretypes/validation.h>
#include <opendaq/scaling_factory.h>
#include <opendaq/scaling_impl.h>
#include <opendaq/signal_errors.h>

BEGIN_NAMESPACE_OPENDAQ

namespace detail
{
    static const StructTypePtr scalingStructType = ScalingStructType();
}

DictPtr<IString, IBaseObject> ScalingImpl::PackBuilder(IScalingBuilder* scalingBuilder)
{
    const auto builderPtr = ScalingBuilderPtr::Borrow(scalingBuilder);
    auto params = Dict<IString, IBaseObject>();
    params.set("InputDataType", static_cast<Int>(builderPtr.getInputDataType()));
    params.set("OutputDataType", static_cast<Int>(builderPtr.getOutputDataType()));
    params.set("RuleType", static_cast<Int>(builderPtr.getScalingType()));
    params.set("Parameters", builderPtr.getParameters());
    return params;
}

ScalingImpl::ScalingImpl(SampleType inputType, ScaledSampleType outputType, ScalingType ruleType, DictPtr<IString, IBaseObject> params)
    : GenericStructImpl<IScaling, IStruct, IRulePrivate>(detail::scalingStructType,
                                                         Dict<IString, IBaseObject>({
                                                             {"OutputDataType", static_cast<Int>(outputType)},
                                                             {"InputDataType", static_cast<Int>(inputType)},
                                                             {"RuleType", static_cast<Int>(ruleType)},
                                                             {"Parameters", params},
                                                         }))
    , outputDataType(outputType)
    , inputDataType(inputType)
    , ruleType(ruleType)
    , params(std::move(params))
{
    DAQ_CHECK_ERROR_INFO(verifyParametersInternal());

    if (params.supportsInterface<IFreezable>())
        params.freeze();
}

ScalingImpl::ScalingImpl(NumberPtr scale, NumberPtr offset, SampleType inputType, ScaledSampleType outputType)
    : ScalingImpl(inputType, outputType, ScalingType::Linear, Dict<IString, IBaseObject>({{"scale", scale}, {"offset", offset}}))
{
}

ScalingImpl::ScalingImpl(IScalingBuilder* scalingBuilder)
    : GenericStructImpl<IScaling, IStruct, IRulePrivate>(detail::scalingStructType, PackBuilder(scalingBuilder))

{
    const auto builderPtr = ScalingBuilderPtr::Borrow(scalingBuilder);
    this->inputDataType = builderPtr.getInputDataType();
    this->outputDataType = builderPtr.getOutputDataType();
    this->ruleType = builderPtr.getScalingType();
    this->params = builderPtr.getParameters();

    DAQ_CHECK_ERROR_INFO(verifyParametersInternal());

    if (params.supportsInterface<IFreezable>())
        params.freeze();
}

ErrCode ScalingImpl::getInputSampleType(SampleType* type)
{
    OPENDAQ_PARAM_NOT_NULL(type);

    *type = inputDataType;
    return OPENDAQ_SUCCESS;
}

ErrCode ScalingImpl::getOutputSampleType(ScaledSampleType* type)
{
    OPENDAQ_PARAM_NOT_NULL(type);

    *type = outputDataType;
    return OPENDAQ_SUCCESS;
}

ErrCode ScalingImpl::getType(ScalingType* type)
{
    OPENDAQ_PARAM_NOT_NULL(type);

    *type = ruleType;
    return OPENDAQ_SUCCESS;
}

ErrCode ScalingImpl::getParameters(IDict** parameters)
{
    OPENDAQ_PARAM_NOT_NULL(parameters);

    *parameters = params.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode ScalingImpl::verifyParameters()
{
    return verifyParametersInternal();
}

ErrCode INTERFACE_FUNC ScalingImpl::equals(IBaseObject* other, Bool* equals) const
{
    if (equals == nullptr)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ARGUMENT_NULL, "Equals out-parameter must not be null");

    *equals = false;
    if (other == nullptr)
        return OPENDAQ_SUCCESS;

    ScalingPtr scalingOther = BaseObjectPtr::Borrow(other).asPtrOrNull<IScaling>();
    if (scalingOther == nullptr)
        return OPENDAQ_SUCCESS;

    if (inputDataType != scalingOther.getInputSampleType())
        return OPENDAQ_SUCCESS;
    if (outputDataType != scalingOther.getOutputSampleType())
        return OPENDAQ_SUCCESS;
    if (ruleType != scalingOther.getType())
        return OPENDAQ_SUCCESS;
    if (!BaseObjectPtr::Equals(params, scalingOther.getParameters()))
        return OPENDAQ_SUCCESS;

    *equals = true;
    return OPENDAQ_SUCCESS;
}

ErrCode ScalingImpl::serialize(ISerializer* serializer)
{
    OPENDAQ_PARAM_NOT_NULL(serializer);

    serializer->startTaggedObject(this);
    {
        serializer->key("outputDataType");
        serializer->writeInt(static_cast<Int>(outputDataType));

        serializer->key("inputDataType");
        serializer->writeInt(static_cast<Int>(inputDataType));

        serializer->key("ruleType");
        serializer->writeInt(static_cast<Int>(ruleType));

        serializer->key("params");
        params.serialize(serializer);
    }
    serializer->endObject();

    return OPENDAQ_SUCCESS;
}

ErrCode ScalingImpl::getSerializeId(ConstCharPtr* id) const
{
    OPENDAQ_PARAM_NOT_NULL(id);

    *id = SerializeId();

    return OPENDAQ_SUCCESS;
}

ConstCharPtr ScalingImpl::SerializeId()
{
    return "Scaling";
}

ErrCode ScalingImpl::Deserialize(ISerializedObject* serialized, IBaseObject*, IFunction* /*factoryCallback*/, IBaseObject** obj)
{
    SerializedObjectPtr serializedObj = SerializedObjectPtr::Borrow(serialized);
    auto outputDataType = static_cast<ScaledSampleType>(serializedObj.readInt("outputDataType"));
    auto inputDataType = static_cast<SampleType>(serializedObj.readInt("inputDataType"));
    auto ruleType = static_cast<ScalingType>(serializedObj.readInt("ruleType"));
    DictPtr<IString, IBaseObject> params = serializedObj.readObject("params");

    return createObject<IScaling, ScalingImpl>(reinterpret_cast<IScaling**>(obj), inputDataType, outputDataType, ruleType, params);
}

// TODO: Disallow negative numbers where they are invalid
ErrCode ScalingImpl::verifyParametersInternal() const
{
    if (!params.assigned())
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_CONFIGURATION_INCOMPLETE, "Scaling parameters are not set.");

    if (inputDataType > SampleType::Int64)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALID_SAMPLE_TYPE, "Scaling input data can consist only of real numbers.");

    if (ruleType == ScalingType::Linear)
    {
        if (params.getCount() != 2)
        {
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALID_PARAMETERS,
                                 R"(Linear Scaling has an invalid number of parameters. Required parameters are "scale" and "offset".)");
        }

        if (!params.hasKey("scale") || !params.hasKey("offset"))
        {
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALID_PARAMETERS,
                                 R"(Linear scaling has invalid parameters. Required parameters are "scale" and "offset".)");
        }

        if (!params.get("scale").supportsInterface<INumber>() || !params.get("offset").supportsInterface<INumber>())
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALID_PARAMETERS, "Linear scaling parameters must be numbers.");
    }

    return OPENDAQ_SUCCESS;
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY,
                                            Scaling,
                                            IScaling,
                                            SampleType,
                                            inputDataType,
                                            ScaledSampleType,
                                            outputDataType,
                                            ScalingType,
                                            scalingType,
                                            IDict*,
                                            parameters)

#if !defined(BUILDING_STATIC_LIBRARY)

// Specializations

extern "C" daq::ErrCode PUBLIC_EXPORT
createLinearScaling(IScaling** objTmp, INumber* scale, INumber* offset, SampleType inputType, ScaledSampleType outputType)
{
    return daq::createObject<IScaling, ScalingImpl>(objTmp, scale, offset, inputType, outputType);
}

extern "C" daq::ErrCode PUBLIC_EXPORT createScalingFromBuilder(IScaling** objTmp, IScalingBuilder* builder)
{
    return daq::createObject<IScaling, ScalingImpl>(objTmp, builder);
}

#endif

END_NAMESPACE_OPENDAQ
