#include <coretypes/cloneable.h>
#include <coretypes/impl.h>
#include <coretypes/validation.h>
#include <opendaq/data_rule_builder_impl.h>
#include <opendaq/data_rule_factory.h>
#include <opendaq/rule_private_ptr.h>
#include <opendaq/signal_errors.h>

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

ErrCode DataRuleBuilderImpl::build(IDataRule** dataRule)
{
    OPENDAQ_PARAM_NOT_NULL(dataRule);

    const auto builderPtr = this->borrowPtr<DataRuleBuilderPtr>();

    return daqTry(
        [&]()
        {
            *dataRule = DataRuleFromBuilder(builderPtr).detach();
            return OPENDAQ_SUCCESS;
        });
}

ErrCode DataRuleBuilderImpl::setType(DataRuleType type)
{
    ruleType = type;
    return OPENDAQ_SUCCESS;
}

ErrCode DataRuleBuilderImpl::getType(DataRuleType* type)
{
    OPENDAQ_PARAM_NOT_NULL(type);
    *type = this->ruleType;
    return OPENDAQ_SUCCESS;
}

ErrCode DataRuleBuilderImpl::setParameters(IDict* parameters)
{
    params = parameters;
    return OPENDAQ_SUCCESS;
}

ErrCode DataRuleBuilderImpl::getParameters(IDict** parameters)
{
    OPENDAQ_PARAM_NOT_NULL(parameters);
    *parameters = this->params.addRefAndReturn();
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

#if !defined(BUILDING_STATIC_LIBRARY)

/////////////////////
////
//// FACTORIES
////
////////////////////

extern "C" daq::ErrCode PUBLIC_EXPORT createDataRuleBuilder(IDataRuleBuilder** objTmp)
{
    return daq::createObject<IDataRuleBuilder, DataRuleBuilderImpl>(objTmp);
}

extern "C" daq::ErrCode PUBLIC_EXPORT createDataRuleBuilderFromExisting(IDataRuleBuilder** objTmp, IDataRule* ruleToCopy)
{
    return daq::createObject<IDataRuleBuilder, DataRuleBuilderImpl>(objTmp, DataRulePtr(ruleToCopy));
}

#endif

END_NAMESPACE_OPENDAQ
