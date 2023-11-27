#include <coretypes/validation.h>
#include <opendaq/dimension_rule_builder_impl.h>
#include <opendaq/dimension_rule_factory.h>
#include <opendaq/range_ptr.h>
#include <opendaq/rule_private_ptr.h>
#include <opendaq/signal_errors.h>

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

ErrCode DimensionRuleBuilderImpl::build(IDimensionRule** dimensionRule)
{
    OPENDAQ_PARAM_NOT_NULL(dimensionRule);

    const auto builderPtr = this->borrowPtr<DimensionRuleBuilderPtr>();

    return daqTry(
        [&]()
        {
            *dimensionRule = DimensionRuleFromBuilder(builderPtr).detach();
            return OPENDAQ_SUCCESS;
        });
}

ErrCode DimensionRuleBuilderImpl::setType(DimensionRuleType type)
{
    ruleType = type;
    return OPENDAQ_SUCCESS;
}

ErrCode DimensionRuleBuilderImpl::getType(DimensionRuleType* type)
{
    OPENDAQ_PARAM_NOT_NULL(type);

    *type = this->ruleType;
    return OPENDAQ_SUCCESS;
}

ErrCode DimensionRuleBuilderImpl::setParameters(IDict* parameters)
{
    params = parameters;
    return OPENDAQ_SUCCESS;
}

ErrCode DimensionRuleBuilderImpl::getParameters(IDict** parameters)
{
    OPENDAQ_PARAM_NOT_NULL(parameters);

    *parameters = this->params.addRefAndReturn();
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

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, DimensionRuleBuilder, IDimensionRuleBuilder)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, DimensionRuleBuilder, 
    IDimensionRuleBuilder, createDimensionRuleBuilderFromExisting, 
    IDimensionRule*, ruleToCopy)

END_NAMESPACE_OPENDAQ
