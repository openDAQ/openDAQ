#include <opendaq/dimension_rule_impl.h>
#include <opendaq/signal_errors.h>
#include <coretypes/impl.h>
#include <opendaq/range_ptr.h>
#include <opendaq/dimension_rule_ptr.h>
#include <coretypes/validation.h>
#include <opendaq/dimension_rule_factory.h>

BEGIN_NAMESPACE_OPENDAQ

namespace detail
{
    static const StructTypePtr dimensionRuleStructType = DimensionRuleStructType();
}

DimensionRuleImpl::DimensionRuleImpl(DimensionRuleType ruleType, const DictPtr<IString, IBaseObject>& params)
    : GenericStructImpl<IDimensionRule, IStruct, IRulePrivate>(
          detail::dimensionRuleStructType,
          Dict<IString, IBaseObject>({
              {"ruleType", static_cast<Int>(ruleType)},
              {"parameters", params},
          }))
      , ruleType(ruleType)
      , params(this->fields.get("parameters"))
{
    checkErrorInfo(verifyParametersInternal());
    if (params.assigned() && params.asPtrOrNull<IFreezable>().assigned())
        params.freeze();
}

DimensionRuleImpl::DimensionRuleImpl(const ListPtr<INumber>& list)
    : DimensionRuleImpl(DimensionRuleType::List, Dict<IString, IBaseObject>({{"list", list}}))
{
}

DimensionRuleImpl::DimensionRuleImpl(const NumberPtr& delta, const NumberPtr& start, const SizeT& size)
    : DimensionRuleImpl(
        DimensionRuleType::Linear,
        Dict<IString, IBaseObject>({
            {"delta", delta},
            {"start", start},
            {"size", size},
        }))
{
}

DimensionRuleImpl::DimensionRuleImpl(const NumberPtr& delta, const NumberPtr& start, const NumberPtr& base, const SizeT& size)
    : DimensionRuleImpl(DimensionRuleType::Logarithmic,
                        Dict<IString, IBaseObject>({
                            {"delta", delta},
                            {"start", start},
                            {"base", base},
                            {"size", size},
                        }))
{
}

DimensionRuleImpl::DimensionRuleImpl(IDimensionRuleBuilder* dimensionRuleBuilder)
    :DimensionRuleImpl(
        DimensionRuleBuilderPtr::Borrow(dimensionRuleBuilder).getType(),
        DimensionRuleBuilderPtr::Borrow(dimensionRuleBuilder).getParameters())
{
}

ErrCode DimensionRuleImpl::getType(DimensionRuleType* type)
{
    if (!type)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *type = ruleType;
    return OPENDAQ_SUCCESS;
}

ErrCode DimensionRuleImpl::getParameters(IDict** parameters)
{
    if (!parameters)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *parameters = params.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode DimensionRuleImpl::verifyParameters()
{
    return verifyParametersInternal();
}

ErrCode DimensionRuleImpl::equals(IBaseObject* other, Bool* equals) const
{
    if (equals == nullptr)
        return this->makeErrorInfo(OPENDAQ_ERR_ARGUMENT_NULL, "Equals out-parameter must not be null");

    *equals = false;
    if (!other)
        return OPENDAQ_SUCCESS;

    DimensionRulePtr dimensionRuleOther = BaseObjectPtr::Borrow(other).asPtrOrNull<IDimensionRule>();
    if (dimensionRuleOther == nullptr)
        return OPENDAQ_SUCCESS;

    if (ruleType != dimensionRuleOther.getType())
        return OPENDAQ_SUCCESS;
    if (!BaseObjectPtr::Equals(params, dimensionRuleOther.getParameters()))
        return OPENDAQ_SUCCESS;

    *equals = true;
    return OPENDAQ_SUCCESS;
}

ErrCode DimensionRuleImpl::verifyParametersInternal() const
{
    if (!params.assigned())
        return makeErrorInfo(OPENDAQ_ERR_CONFIGURATION_INCOMPLETE, "Dimension rule parameters are not set");

    ErrCode err = OPENDAQ_SUCCESS;
    if (ruleType == DimensionRuleType::Linear)
        err = checkLinearRuleValidity();
    else if (ruleType == DimensionRuleType::Logarithmic)
        err = checkLogRuleValidity();
    else if (ruleType == DimensionRuleType::List)
        err = checkListRuleValidity();
    return err;
}

ErrCode DimensionRuleImpl::checkLinearRuleValidity() const
{
    if (params.getCount() != 3)
    {
        return makeErrorInfo(OPENDAQ_ERR_INVALID_PARAMETERS,
                             R"(Linear rule has an invalid number of parameters. Required parameters are "delta", "size" and "start")");
    }

    if (!params.hasKey("delta") || !params.hasKey("start") || !params.hasKey("size"))
    {
        return makeErrorInfo(OPENDAQ_ERR_INVALID_PARAMETERS,
                             R"(Linear rule has invalid parameters. Required parameters are "delta", "size" and "start")");
    }

    if (!params.get("delta").asPtrOrNull<INumber>().assigned() || !params.get("start").asPtrOrNull<INumber>().assigned() ||
        !params.get("size").asPtrOrNull<INumber>().assigned())
    {
        return makeErrorInfo(OPENDAQ_ERR_INVALID_PARAMETERS, "Linear scaling parameters must be numbers.");
    }
    return OPENDAQ_SUCCESS;
}

ErrCode DimensionRuleImpl::checkListRuleValidity() const
{
    if (!params.hasKey("list"))
    {
        return makeErrorInfo(OPENDAQ_ERR_INVALID_PARAMETERS, R"(Linear rule has invalid parameters. The "list" parameter is required.)");
    }

    if (!params.get("list").asPtrOrNull<IList>().assigned())
        return makeErrorInfo(OPENDAQ_ERR_INVALID_PARAMETERS, R"(The "list" parameter must be a list object.)");

    if (!listLabelsValid(params.get("list")))
        return makeErrorInfo(OPENDAQ_ERR_INVALID_DIMENSION_LABEL_TYPES,
                             R"(The "list" elements must be either strings, numbers, or ranges. All elements must be of the same kind.)");

    return OPENDAQ_SUCCESS;
}

ErrCode DimensionRuleImpl::checkLogRuleValidity() const
{
    if (params.getCount() != 4)
    {
        return makeErrorInfo(
            OPENDAQ_ERR_INVALID_PARAMETERS,
            R"(Linear rule has an invalid number of parameters. Required parameters are "delta", "size", "base" and "start")");
    }

    if (!params.hasKey("delta") || !params.hasKey("start") || !params.hasKey("size") || !params.hasKey("base"))
    {
        return makeErrorInfo(OPENDAQ_ERR_INVALID_PARAMETERS,
                             R"(Linear rule has invalid parameters. Required parameters are "delta", "size", "base" and "start")");
    }

    if (!params.get("delta").asPtrOrNull<INumber>().assigned() || !params.get("start").asPtrOrNull<INumber>().assigned() ||
        !params.get("size").asPtrOrNull<INumber>().assigned() || !params.get("base").asPtrOrNull<INumber>().assigned())
    {
        return makeErrorInfo(OPENDAQ_ERR_INVALID_PARAMETERS, "Linear scaling parameters must be numbers.");
    }
    return OPENDAQ_SUCCESS;
}

DimensionRuleImpl::LabelType DimensionRuleImpl::getLabelType(const BaseObjectPtr& label)
{
    if (label.asPtrOrNull<IString>(true).assigned())
        return LabelType::String;
    if (label.asPtrOrNull<INumber>(true).assigned())
        return LabelType::Number;
    if (label.asPtrOrNull<IRange>(true).assigned())
        return LabelType::Range;
    return LabelType::None;
}

bool DimensionRuleImpl::listLabelsValid(const ListPtr<IBaseObject>& list)
{
    if (list.getCount() == 0)
        return true;

    const LabelType type = getLabelType(list[0]);
    if (type == LabelType::None)
        return false;

    for (const auto& label : list)
    {
        if (type != getLabelType(label))
            return false;
    }
    return true;
}

ErrCode DimensionRuleImpl::serialize(ISerializer* serializer)
{
    OPENDAQ_PARAM_NOT_NULL(serializer);

    serializer->startTaggedObject(this);
    {
        serializer->key("rule_type");
        serializer->writeInt(static_cast<Int>(ruleType));

        serializer->key("params");
        params.serialize(serializer);
    }
    serializer->endObject();

    return OPENDAQ_SUCCESS;
}

ErrCode DimensionRuleImpl::getSerializeId(ConstCharPtr* id) const
{
    OPENDAQ_PARAM_NOT_NULL(id);

    *id = SerializeId();

    return OPENDAQ_SUCCESS;
}

ConstCharPtr DimensionRuleImpl::SerializeId()
{
    return "DimensionRule";
}

ErrCode DimensionRuleImpl::Deserialize(ISerializedObject* serialized, IBaseObject*, IFunction*, IBaseObject** obj)
{
    SerializedObjectPtr serializedObj = SerializedObjectPtr::Borrow(serialized);
    auto ruleType = static_cast<DimensionRuleType>(serializedObj.readInt("rule_type"));
    DictPtr<IString, IBaseObject> params = serializedObj.readObject("params");

    return createObject<IDimensionRule, DimensionRuleImpl>(reinterpret_cast<IDimensionRule**>(obj), ruleType, params);
}

#if !defined(BUILDING_STATIC_LIBRARY)

/////////////////////
////
//// FACTORIES
////
////////////////////

// Specializations

extern "C"
daq::ErrCode PUBLIC_EXPORT createLinearDimensionRule(IDimensionRule** objTmp, INumber* delta, INumber* start, SizeT size)
{
    return daq::createObject<IDimensionRule, DimensionRuleImpl>(objTmp, delta, start, size);
}

extern "C"
daq::ErrCode PUBLIC_EXPORT createListDimensionRule(IDimensionRule** objTmp, IList* list)
{
    return daq::createObject<IDimensionRule, DimensionRuleImpl>(objTmp, ListPtr<INumber>(list));
}

extern "C"
daq::ErrCode PUBLIC_EXPORT createLogarithmicDimensionRule(IDimensionRule** objTmp,
                                                         INumber* delta,
                                                         INumber* start,
                                                         INumber* base,
                                                         SizeT size)
{
    return daq::createObject<IDimensionRule, DimensionRuleImpl>(objTmp, delta, start, base, size);
}

extern "C" daq::ErrCode PUBLIC_EXPORT createDimensionRule(IDimensionRule** objTmp, DimensionRuleType type, IDict* parameters)
{
    return daq::createObject<IDimensionRule, DimensionRuleImpl>(objTmp, type, parameters);
}

extern "C" daq::ErrCode PUBLIC_EXPORT createDimensionRuleFromBuilder(IDimensionRule** objTmp, IDimensionRuleBuilder* builder)
{
    return daq::createObject<IDimensionRule, DimensionRuleImpl>(objTmp, builder);
}

#endif

END_NAMESPACE_OPENDAQ
