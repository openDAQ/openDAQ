#include <opendaq/data_rule_impl.h>
#include <opendaq/signal_errors.h>
#include <coretypes/cloneable.h>
#include <coretypes/impl.h>
#include <coretypes/validation.h>
#include <opendaq/data_rule_factory.h>
#include <coretypes/dictobject_factory.h>

BEGIN_NAMESPACE_OPENDAQ

namespace detail
{
    static const StructTypePtr dataRuleStructType = DataRuleStructType();

    static DictPtr<IString, IBaseObject> checkTypeAndBuildParams(DataRuleType ruleType, const NumberPtr& param1, const NumberPtr& param2)
    {
        if (ruleType == DataRuleType::Explicit)
            return Dict<IString, IBaseObject>({{"minExpectedDelta", param1}, {"maxExpectedDelta", param2}});
        if (ruleType == DataRuleType::Linear)
            return Dict<IString, IBaseObject>({{"delta", param1}, {"start", param2}});

        throw InvalidParameterException{"Invalid type of data rule. Rules with 2 number parameters can only be explicit or linear."};
    }
}

DataRuleImpl::DataRuleImpl(DataRuleType ruleType, const DictPtr<IString, IBaseObject>& params)
    : GenericStructImpl<IDataRule, IStruct, IRulePrivate>(
          detail::dataRuleStructType, Dict<IString, IBaseObject>({{"type", static_cast<Int>(ruleType)}, {"parameters", params}}))
    , ruleType(ruleType)
    , params(params)
{
    checkErrorInfo(verifyParametersInternal());
}

DataRuleImpl::DataRuleImpl(DataRuleType ruleType, const NumberPtr& param1, const NumberPtr& param2)
    : DataRuleImpl(ruleType, detail::checkTypeAndBuildParams(ruleType, param1, param2))
{
}

DataRuleImpl::DataRuleImpl(const NumberPtr& constant)
    : DataRuleImpl(DataRuleType::Constant, Dict<IString, IBaseObject>({{"constant", constant}}))
{
}

DataRuleImpl::DataRuleImpl()
    : DataRuleImpl(DataRuleType::Explicit, Dict<IString, IBaseObject>())
{
}

DataRuleImpl::DataRuleImpl(IDataRuleBuilder * dataRuleBuilder)
    : DataRuleImpl(DataRuleBuilderPtr(dataRuleBuilder).getType(), DataRuleBuilderPtr(dataRuleBuilder).getParameters())
{
}

ErrCode DataRuleImpl::getType(DataRuleType* type)
{
    if (!type)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *type = ruleType;
    return OPENDAQ_SUCCESS;
}

ErrCode DataRuleImpl::getParameters(IDict** parameters)
{
    if (!parameters)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *parameters = params.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode DataRuleImpl::verifyParameters()
{
    return verifyParametersInternal();
}

ErrCode INTERFACE_FUNC DataRuleImpl::equals(IBaseObject* other, Bool* equals) const
{
    if (equals == nullptr)
        return this->makeErrorInfo(OPENDAQ_ERR_ARGUMENT_NULL, "Equals out-parameter must not be null.");

    *equals = false;
    if (other == nullptr)
        return OPENDAQ_SUCCESS;

    DataRulePtr ruleOther = BaseObjectPtr::Borrow(other).asPtrOrNull<IDataRule>();
    if (ruleOther == nullptr)
        return OPENDAQ_SUCCESS;

    if (ruleType != ruleOther.getType())
        return OPENDAQ_SUCCESS;
    if (!BaseObjectPtr::Equals(params, ruleOther.getParameters()))
        return OPENDAQ_SUCCESS;

    *equals = true;
    return OPENDAQ_SUCCESS;
}

ErrCode DataRuleImpl::serialize(ISerializer* serializer)
{
    OPENDAQ_PARAM_NOT_NULL(serializer);

    serializer->startTaggedObject(this);
    {
        serializer->key("ruleType");
        serializer->writeInt(static_cast<Int>(ruleType));

        serializer->key("params");
        params.serialize(serializer);
    }
    serializer->endObject();

    return OPENDAQ_SUCCESS;
}

ErrCode DataRuleImpl::getSerializeId(ConstCharPtr* id) const
{
    OPENDAQ_PARAM_NOT_NULL(id);

    *id = SerializeId();

    return OPENDAQ_SUCCESS;
}

ConstCharPtr DataRuleImpl::SerializeId()
{
    return "DataRule";
}

ErrCode DataRuleImpl::Deserialize(ISerializedObject* serialized, IBaseObject*, IFunction*, IBaseObject** obj)
{
    SerializedObjectPtr serializedObj = SerializedObjectPtr::Borrow(serialized);
    auto ruleType = static_cast<DataRuleType>(serializedObj.readInt("ruleType"));
    DictPtr<IString, IBaseObject> params = serializedObj.readObject("params");

    return createObject<IDataRule, DataRuleImpl>(reinterpret_cast<IDataRule**>(obj), ruleType, params);
}

// TODO: Disallow negative numbers where they are invalid
ErrCode DataRuleImpl::verifyParametersInternal()
{
    if (!params.assigned() && ruleType != DataRuleType::Explicit)
        return makeErrorInfo(OPENDAQ_ERR_CONFIGURATION_INCOMPLETE, "Data rule parameters are not set");

    if (ruleType == DataRuleType::Linear)
    {
        if (params.getCount() != 2)
        {
            return makeErrorInfo(OPENDAQ_ERR_INVALID_PARAMETERS,
                                 R"(Linear rule has an invalid number of parameters. Required parameters are "delta" and "start")");
        }

        if (!params.hasKey("delta") || !params.hasKey("start"))
        {
            return makeErrorInfo(OPENDAQ_ERR_INVALID_PARAMETERS,
                                 R"(Linear rule has invalid parameters. Required parameters are "delta" and "start")");
        }

        if (!params.get("delta").asPtrOrNull<INumber>().assigned() || !params.get("start").asPtrOrNull<INumber>().assigned())
        {
            return makeErrorInfo(OPENDAQ_ERR_INVALID_PARAMETERS, "Linear scaling parameters must be numbers.");
        }
    }

    if (ruleType == DataRuleType::Constant)
    {
        if (params.getCount() != 1)
        {
            return makeErrorInfo(OPENDAQ_ERR_INVALID_PARAMETERS,
                                 R"(Constant rule has an invalid number of parameters. The "constant" parameter is required.)");
        }

        if (!params.hasKey("constant"))
        {
            return makeErrorInfo(OPENDAQ_ERR_INVALID_PARAMETERS,
                                 R"(Constant rule has invalid parameters. The "constant" parameter is required.)");
        }

        if (!params.get("constant").asPtrOrNull<INumber>().assigned())
            return makeErrorInfo(OPENDAQ_ERR_INVALID_PARAMETERS, R"(The "constant" parameter must be number.)");
    }

    try
    {
        if (params.assigned() && !params.isFrozen())
            params.freeze();
    }
    catch (const DaqException& e)
    {
        return errorFromException(e);
    }

    return OPENDAQ_SUCCESS;
}

#if !defined(BUILDING_STATIC_LIBRARY)

/////////////////////
////
//// FACTORIES
////
////////////////////

// Specializations

extern "C"
daq::ErrCode PUBLIC_EXPORT createLinearDataRule(IDataRule** objTmp, INumber* delta, INumber* start)
{
    return daq::createObject<IDataRule, DataRuleImpl>(objTmp, DataRuleType::Linear, delta, start);
}

extern "C"
daq::ErrCode PUBLIC_EXPORT createConstantDataRule(IDataRule** objTmp, INumber* constant)
{
    return daq::createObject<IDataRule, DataRuleImpl, NumberPtr>(objTmp, constant);
}

extern "C"
daq::ErrCode PUBLIC_EXPORT createExplicitDataRule(IDataRule** objTmp)
{
    return daq::createObject<IDataRule, DataRuleImpl>(objTmp);
}

extern "C"
daq::ErrCode PUBLIC_EXPORT createExplicitDomainDataRule(IDataRule** objTmp, INumber* minExpectedDelta, INumber* maxExpectedDelta)
{
    return daq::createObject<IDataRule, DataRuleImpl>(objTmp, DataRuleType::Explicit, minExpectedDelta, maxExpectedDelta);
}

extern "C" daq::ErrCode PUBLIC_EXPORT createDataRule(IDataRule** objTmp, DataRuleType ruleType, IDict* parameters)
{
    return daq::createObject<IDataRule, DataRuleImpl>(objTmp, ruleType, parameters);
}

extern "C" daq::ErrCode PUBLIC_EXPORT createDataRuleFromBuilder(IDataRule** objTmp, IDataRuleBuilder* builder)
{
    return daq::createObject<IDataRule, DataRuleImpl>(objTmp, builder);
}

#endif

END_NAMESPACE_OPENDAQ
