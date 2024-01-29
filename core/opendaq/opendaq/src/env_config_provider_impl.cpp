#include <opendaq/env_config_provider_impl.h>
#include <cstdlib>
BEGIN_NAMESPACE_OPENDAQ

EnvConfigProviderImpl::EnvConfigProviderImpl()
{
}

DictPtr<IString, IString> EnvConfigProviderImpl::GetEnvValuesStartingWith(const std::string& prefix)
{
    if (prefix.empty())
        return {};

    auto result = Dict<IString, IString>();
    for (char** env = _environ; *env != 0; env++) {
        std::string envVar = *env;

        if (envVar.substr(0, prefix.length()) == prefix)
        {
            size_t pos = envVar.find('=');
            if (pos != std::string::npos)
            {
                std::string key = envVar.substr(0, pos);
                std::string value = envVar.substr(pos + 1);
                result.set(key, value);
            }
        }
    }
    return result;
}

ErrCode INTERFACE_FUNC EnvConfigProviderImpl::populateModuleOptions(IDict* options)
{
    std::string envPrefix = "OPENDAQ_CONFIG_";
    auto modules = GetEnvValuesStartingWith(envPrefix);

    return OPENDAQ_SUCCESS;
}

ListPtr<IString> splitEnvKey(const std::string& envKey, const std::string& prefix, char delimiter = '_')
{
    auto result = List<IString>();

    size_t start = prefix.length();
    size_t end = envKey.find(delimiter, start);

    while (end != std::string::npos) {
        result.pushBack(envKey.substr(start, end - start));
        start = end + 1;
        end = envKey.find(delimiter, start);
    }

    result.pushBack(envKey.substr(start, end));
    return result;
}

std::string toUpperCase(const std::string &input) {
    std::string result = input;
    for (char &c : result)
        c = std::toupper(static_cast<unsigned char>(c));

    return result;
}

ErrCode INTERFACE_FUNC EnvConfigProviderImpl::populateOptions(IDict* options) 
{
    auto optionsPtr = DictPtr<IString, IBaseObject>::Borrow(options);

    std::string envPrefix = "OPENDAQ_CONFIG_";
    auto envDict = GetEnvValuesStartingWith(envPrefix);

    for (const auto& [envKey, envValue] : envDict)
    {
        auto splitedKey = splitEnvKey(envKey, envPrefix);
        if (splitedKey.empty())
            break;

        DictPtr<IString, IBaseObject> optionsValue;

        auto envKeyUpper = toUpperCase(splitedKey[0]);
        if (envKeyUpper == "MODULEMANAGER")
            optionsValue = optionsPtr.get("ModuleManager");
        else if (envKeyUpper == "SCHEDULER")
            optionsValue = optionsPtr.get("Scheduler");
        else if (envKeyUpper == "LOGGING")
            optionsValue = optionsPtr.get("Logging");
        else if (envKeyUpper == "MODULES")
            optionsValue = optionsPtr.get("Modules");
        
        assert(optionsValue.assigned());

        SizeT deep = 0;
        for (const auto & token : splitedKey)
        {
            if (deep == 0)
            {
                deep++;
                continue;
            }

            if (deep == splitedKey.getCount() - 1)
            {
                optionsValue.set(token, envValue);
            } 
            else 
            {
                optionsValue = optionsValue.get(token);
            }

            deep++;
        }
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
