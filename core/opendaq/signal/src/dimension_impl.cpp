#include <opendaq/dimension_impl.h>
#include <opendaq/signal_errors.h>
#include <opendaq/range_ptr.h>
#include <opendaq/dimension_ptr.h>
#include <opendaq/dimension_rule_ptr.h>
#include <opendaq/dimension_rule_factory.h>
#include <opendaq/rule_private_ptr.h>
#include <coreobjects/unit_factory.h>
#include <coretypes/validation.h>
#include <opendaq/dimension_factory.h>
#include <opendaq/signal_exceptions.h>

BEGIN_NAMESPACE_OPENDAQ

namespace detail
{
    static const StructTypePtr dimensionStructType = DimensionStructType();
}

DictPtr<IString, IBaseObject> DimensionImpl::PackBuilder(IDimensionBuilder* dimensionBuilder)
{
    const auto builderPtr = DimensionBuilderPtr::Borrow(dimensionBuilder);
    auto params = Dict<IString, IBaseObject>();
    params.set("Name", builderPtr.getName());
    params.set("Unit", builderPtr.getUnit());
    params.set("Rule", builderPtr.getRule());

    return params;
}

DimensionImpl::DimensionImpl(const DimensionRulePtr& rule, const UnitPtr& unit, const StringPtr& name)
    : GenericStructImpl<IDimension, IStruct>(detail::dimensionStructType,
          Dict<IString, IBaseObject>({
              {"Name", name},
              {"Unit", unit},
              {"Rule", rule},
          }))
    , name(name)
    , unit(unit)
    , rule(rule)
{
    if (!rule.assigned())
        DAQ_THROW_EXCEPTION(ConfigurationIncompleteException, "Dimension rule is not assigned.");
}

DimensionImpl::DimensionImpl(IDimensionBuilder* dimensionBuilder)
    : GenericStructImpl<IDimension, IStruct>(detail::dimensionStructType, PackBuilder(dimensionBuilder))
{
    const auto builderPtr = DimensionBuilderPtr::Borrow(dimensionBuilder);
    this->name = builderPtr.getName();
    this->unit = builderPtr.getUnit();
    this->rule = builderPtr.getRule();

    if (!rule.assigned())
        DAQ_THROW_EXCEPTION(ConfigurationIncompleteException, "Dimension rule is not assigned.");
}


ErrCode DimensionImpl::getName(IString** name)
{
    OPENDAQ_PARAM_NOT_NULL(name);

    *name = this->name.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode DimensionImpl::getSize(SizeT* size)
{
    OPENDAQ_PARAM_NOT_NULL(size);

    if (!rule.assigned())
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_CONFIGURATION_INCOMPLETE, "Dimension rule is not assigned.");

    if (rule.getType() == DimensionRuleType::Other)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_UNKNOWN_RULE_TYPE, R"(Rule type is set to "other" and cannot be parse by openDAQ)");

    ErrCode err = rule.asPtr<IRulePrivate>()->verifyParameters();
    OPENDAQ_RETURN_IF_FAILED(err);

    if (rule.getType() == DimensionRuleType::Linear || rule.getType() == DimensionRuleType::Logarithmic)
    {
        *size = rule.getParameters().get("size");
    }
    else if (rule.getType() == DimensionRuleType::List)
    {
        *size = rule.getParameters().get("List").asPtr<IList>().getCount();
    }
    else
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_UNKNOWN_RULE_TYPE);
    }
    return OPENDAQ_SUCCESS;
}

ErrCode DimensionImpl::getUnit(IUnit** unit)
{
    OPENDAQ_PARAM_NOT_NULL(unit);
    
    *unit = this->unit.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode DimensionImpl::getRule(IDimensionRule** rule)
{
    OPENDAQ_PARAM_NOT_NULL(rule);

    *rule = this->rule.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode DimensionImpl::getLabels(IList** labels)
{
    OPENDAQ_PARAM_NOT_NULL(labels);

    if (!rule.assigned())
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_CONFIGURATION_INCOMPLETE, "Dimension rule is not assigned.");

    if (rule.getType() == DimensionRuleType::Other)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_UNKNOWN_RULE_TYPE, R"(Rule type is set to "other" and cannot be parsed by openDAQ)");

    ErrCode err = rule.asPtr<IRulePrivate>()->verifyParameters();
    OPENDAQ_RETURN_IF_FAILED(err);

    err = daqTry([&]()
    {
        if (rule.getType() == DimensionRuleType::List)
            *labels = getListLabels().addRefAndReturn();
        else if (rule.getType() == DimensionRuleType::Linear)
            *labels = getLinearLabels().addRefAndReturn();
        else if (rule.getType() == DimensionRuleType::Logarithmic)
            *labels = getLogLabels().addRefAndReturn();
    });
    OPENDAQ_RETURN_IF_FAILED(err, "Failed to get dimension labels");
    return err;
}

ErrCode DimensionImpl::equals(IBaseObject* other, Bool* equals) const
{
    if (equals == nullptr)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ARGUMENT_NULL, "Equals out-parameter must not be null");

    *equals = false;
    if (!other)
        return OPENDAQ_SUCCESS;

    DimensionPtr dimensionOther = BaseObjectPtr::Borrow(other).asPtrOrNull<IDimension>();
    if (dimensionOther == nullptr)
        return OPENDAQ_SUCCESS;

    if (name != dimensionOther.getName())
        return OPENDAQ_SUCCESS;
    if (!BaseObjectPtr::Equals(unit, dimensionOther.getUnit()))
        return OPENDAQ_SUCCESS;
    if (!BaseObjectPtr::Equals(rule, dimensionOther.getRule()))
        return OPENDAQ_SUCCESS;

    *equals = true;
    return OPENDAQ_SUCCESS;
}

// TODO: Allow ranges in rule
ListPtr<IBaseObject> DimensionImpl::getLinearLabels() const
{
    const SizeT size = rule.getParameters().get("size");
    const Float delta = rule.getParameters().get("delta");
    const Float start = rule.getParameters().get("start");

    auto list = List<IBaseObject>();
    for (SizeT i = 0; i < size; ++i)
        list.pushBack(start +  static_cast<Float>(i) * delta);

    return list;
}

// TODO: Allow ranges in rule
ListPtr<IBaseObject> DimensionImpl::getLogLabels() const
{
    const SizeT size = rule.getParameters().get("size");
    const Float delta = rule.getParameters().get("delta");
    const Float start = rule.getParameters().get("start");
    const Float base = rule.getParameters().get("base");

    auto list = List<IBaseObject>();
    for (SizeT i = 0; i < size; ++i)
        list.pushBack(std::pow(base, start + static_cast<Float>(i) * delta));

    return list;
}

ListPtr<IBaseObject> DimensionImpl::getListLabels() const
{
    return rule.getParameters().get("List");
}

ErrCode DimensionImpl::serialize(ISerializer* serializer)
{
    OPENDAQ_PARAM_NOT_NULL(serializer);

    serializer->startTaggedObject(this);
    {
        serializer->key("rule");
        rule.serialize(serializer);

        if (unit.assigned())
        {
            serializer->key("unit");
            unit.serialize(serializer);
        }

        serializer->key("name");
        serializer->writeString(name.getCharPtr(), name.getLength());
    }
    serializer->endObject();

    return OPENDAQ_SUCCESS;
}

ErrCode DimensionImpl::getSerializeId(ConstCharPtr* id) const
{
    OPENDAQ_PARAM_NOT_NULL(id);

    *id = SerializeId();

    return OPENDAQ_SUCCESS;
}

ConstCharPtr DimensionImpl::SerializeId()
{
    return "Dimension";
}

ErrCode DimensionImpl::Deserialize(ISerializedObject* serialized, IBaseObject*, IFunction*, IBaseObject** obj)
{
    SerializedObjectPtr serializedObj = SerializedObjectPtr::Borrow(serialized);
    auto rule = serializedObj.readObject("rule").asPtr<IDimensionRule>();
    UnitPtr unit;
    if (serializedObj.hasKey("unit"))
        unit = serializedObj.readObject("unit").asPtr<IUnit>();
    auto name = serializedObj.readString("name");

    return createObject<IDimension, DimensionImpl>(reinterpret_cast<IDimension**>(obj), rule, unit, name);
}

#if !defined(BUILDING_STATIC_LIBRARY)

extern "C"
daq::ErrCode PUBLIC_EXPORT createDimension(IDimension** objTmp, IDimensionRule* rule, IUnit* unit, IString* name)
{
    return daq::createObject<IDimension, DimensionImpl>(objTmp, rule, unit, name);
}
extern "C"
daq::ErrCode PUBLIC_EXPORT createDimensionFromBuilder(IDimension** objTmp, IDimensionBuilder* builder)
{
    return daq::createObject<IDimension, DimensionImpl>(objTmp, builder);
}

#endif

END_NAMESPACE_OPENDAQ
