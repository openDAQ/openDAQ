#include <opendaq/env_config_provider_impl.h>
#include <cstdlib>
#include <string>
#include <coretypes/coretypes.h>

#ifdef _environ
    #define ENVIRON _environ
#else
    extern char **environ;
    #define ENVIRON environ
#endif

BEGIN_NAMESPACE_OPENDAQ

ListPtr<IString> EnvConfigProviderImpl::GetEnvValues()
{
    auto result = List<IString>();
    for (char** env = ENVIRON; *env != 0; env++) 
        result.pushBack(*env);

    return result;
}

ErrCode EnvConfigProviderImpl::populateOptions(IDict* options) 
{
    if (options == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    auto optionsPtr = DictPtr<IString, IBaseObject>::Borrow(options);

    std::string envPrefix = "OPENDAQ_CONFIG_";
    auto envDict = GetValuesStartingWith(GetEnvValues(), envPrefix);

    for (const auto& [envKey, envValue] : envDict)
    {
        WriteValue(optionsPtr, SplitKey(envKey, envPrefix, '_'), envValue);
    }
    return OPENDAQ_SUCCESS;
}

/////////////////////
////
//// FACTORIES
////
////////////////////

extern "C" ErrCode PUBLIC_EXPORT createEnvConfigProvider(IConfigProvider** objTmp)
{
    return daq::createObject<IConfigProvider, EnvConfigProviderImpl>(objTmp);
}

END_NAMESPACE_OPENDAQ
