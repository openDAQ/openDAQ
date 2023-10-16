#include <opendaq/dimension_rule_builder_impl.h>
#include <opendaq/signal_errors.h>
#include <opendaq/range_ptr.h>
#include <opendaq/dimension_rule_factory.h>
#include <opendaq/rule_private_ptr.h>
#include <coretypes/validation.h>

BEGIN_NAMESPACE_OPENDAQ

DimensionRuleBuilderImpl::DimensionRuleBuilderImpl()
    : ruleType(DimensionRuleType::Other)
    , params(Dict<IString, IBaseObject>())
{
}

DimensionRuleBuilderImpl::DimensionRuleBuilderImpl(const DimensionRulePtr& rule)
    : ruleType(rule.getType())
    , params(Dict<IString, IBaseObject>())
{
    auto paramsToCopy = rule.getParameters();
    if (paramsToCopy.assigned())
    {
        for (const auto& [k, v] : paramsToCopy)
        {
            params.set(k, v);
        }
    }
}

ErrCode DimensionRuleBuilderImpl::setType(DimensionRuleType type)
{
    ruleType = type;
    return OPENDAQ_SUCCESS;
}

ErrCode DimensionRuleBuilderImpl::setParameters(IDict* parameters)
{
    params = parameters;
    return OPENDAQ_SUCCESS;
}


ErrCode DimensionRuleBuilderImpl::addParameter(IString* name, IBaseObject* parameter)
{
    OPENDAQ_PARAM_NOT_NULL(name);

    return this->params->set(name, parameter);
}

ErrCode DimensionRuleBuilderImpl::removeParameter(IString* name)
{
    OPENDAQ_PARAM_NOT_NULL(name);

    return this->params->deleteItem(name);
}

ErrCode DimensionRuleBuilderImpl::build(IDimensionRule** dimensionRule)
{
    OPENDAQ_PARAM_NOT_NULL(dimensionRule);

    return daqTry(
        [&]()
        {
            DictPtr<IString, IBaseObject> paramsCopy = Dict<IString, IBaseObject>();
            for (const auto& [k, v] : params)
                paramsCopy.set(k, v);

            auto dimensionRuleObj = DimensionRule(ruleType, paramsCopy);
            dimensionRuleObj.asPtr<IRulePrivate>().verifyParameters();
            *dimensionRule = dimensionRuleObj.detach();
            return OPENDAQ_SUCCESS;
        });
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, DimensionRuleBuilder, IDimensionRuleBuilder)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, DimensionRuleBuilder, IDimensionRuleBuilder, createDimensionRuleBuilderFromExisting, IDimensionRule*, ruleToCopy)

END_NAMESPACE_OPENDAQ
