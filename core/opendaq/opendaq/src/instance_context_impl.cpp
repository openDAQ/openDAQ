
#include <opendaq/instance_context_impl.h>
#include <coretypes/coretypes.h>
#include <opendaq/config_provider_factory.h>
#include <opendaq/log_level.h>
#include <cstdlib>

BEGIN_NAMESPACE_OPENDAQ

InstanceContextImpl::InstanceContextImpl()
{
}

ErrCode InstanceContextImpl::getOptions(IDict** options)
{
    auto jsonPath = GetEnvironmentVariableValue("OPENDAQ_CONFIG_PATH", "opendaq-config.json");

    auto generatedOptions = GetDefaultOptions();
    try {
        auto jsonProvider = JsonConfigProvider(jsonPath);
        jsonProvider.populateOptions(generatedOptions);
    } catch (...)
    {
    }

    *options = generatedOptions.detach();
    return OPENDAQ_SUCCESS;
}

DictPtr<IString, IBaseObject> InstanceContextImpl::GetDefaultOptions()
{
    return Dict<IString, IBaseObject>({
        {"ModuleManager", Dict<IString, IBaseObject>({
                {"ModulesPath", ""}
            })},
        {"Scheduler", Dict<IString, IBaseObject>({
                {"WorkersNum", 0}
            })},
        {"Logging", Dict<IString, IBaseObject>({
                {"GlobalLogLevel", OPENDAQ_LOG_LEVEL_DEFAULT}
            })},
        {"RootDevice", Dict<IString, IBaseObject>({
                {"DefaultLocalId", ""},
                {"Connection", ""}
            })},   
        {"Modules", Dict<IString, IBaseObject>()}
    });
}

StringPtr InstanceContextImpl::GetEnvironmentVariableValue(StringPtr variableName, StringPtr defaultValue)
{
    if (!variableName.assigned() || variableName.getLength() == 0)
        return defaultValue;
    
    const char* value = std::getenv(variableName.toStdString().c_str());

    if (value)
        return String(value);
    else
        return defaultValue;
}


extern "C" ErrCode PUBLIC_EXPORT createInstanceContext(IInstanceContext** objTmp)
{
    return daq::createObject<IInstanceContext, InstanceContextImpl>(objTmp);
}

END_NAMESPACE_OPENDAQ