#include <opendaq/data_rule_builder_impl.h>
#include <opendaq/signal_errors.h>
#include <coretypes/cloneable.h>
#include <coretypes/impl.h>
#include <coretypes/validation.h>
#include <opendaq/data_rule_factory.h>
#include <opendaq/rule_private_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

DataRuleBuilderImpl::DataRuleBuilderImpl()
    : ruleType(DataRuleType::Other)
    , params(Dict<IString, IBaseObject>())
{
}

DataRuleBuilderImpl::DataRuleBuilderImpl(const DataRulePtr& ruleToCopy)
    : ruleType(ruleToCopy.getType())
    , params(Dict<IString, IBaseObject>())
{
    auto paramsToCopy = ruleToCopy.getParameters();
    if (paramsToCopy.assigned())
    {
        for (const auto& [k, v] : paramsToCopy)
        {
            params.set(k, v);
        }
    }
}

ErrCode DataRuleBuilderImpl::setType(DataRuleType type)
{
    ruleType = type;
    return OPENDAQ_SUCCESS;
}

ErrCode DataRuleBuilderImpl::setParameters(IDict* parameters)
{
    params = parameters;
    return OPENDAQ_SUCCESS;
}

ErrCode DataRuleBuilderImpl::getType(DataRuleType* type)
{
    OPENDAQ_PARAM_NOT_NULL(type);
    *type = this->ruleType;
    return OPENDAQ_SUCCESS;
}

ErrCode DataRuleBuilderImpl::getParameters(IDict** parameters)
{
    OPENDAQ_PARAM_NOT_NULL(parameters);
    *parameters = this->params;
    return OPENDAQ_SUCCESS;
}

ErrCode DataRuleBuilderImpl::addParameter(IString* name, IBaseObject* parameter)
{
    OPENDAQ_PARAM_NOT_NULL(name);

    return this->params->set(name, parameter);
}

ErrCode DataRuleBuilderImpl::removeParameter(IString* name)
{
    OPENDAQ_PARAM_NOT_NULL(name);

    return this->params->deleteItem(name);
}

ErrCode DataRuleBuilderImpl::build(IDataRule** dataRule)
{
    OPENDAQ_PARAM_NOT_NULL(dataRule);

    return daqTry(
        [&]()
        {
            DictPtr<IString, IBaseObject> paramsCopy = Dict<IString, IBaseObject>();
            for (const auto& [k, v] : params)
                paramsCopy.set(k, v);

            auto dataRuleObj = DataRule(ruleType, params);
            dataRuleObj.asPtr<IRulePrivate>().verifyParameters();
            *dataRule = dataRuleObj.detach();
            return OPENDAQ_SUCCESS;
        });
}

#if !defined(BUILDING_STATIC_LIBRARY)

/////////////////////
////
//// FACTORIES
////
////////////////////

extern "C"
daq::ErrCode PUBLIC_EXPORT createDataRuleBuilder(IDataRuleBuilder** objTmp)
{
    return daq::createObject<IDataRuleBuilder, DataRuleBuilderImpl>(objTmp);
}

extern "C"
daq::ErrCode PUBLIC_EXPORT createDataRuleBuilderFromExisting(IDataRuleBuilder** objTmp, IDataRule* ruleToCopy)
{
    return daq::createObject<IDataRuleBuilder, DataRuleBuilderImpl>(objTmp, DataRulePtr(ruleToCopy));
}

#endif

END_NAMESPACE_OPENDAQ
