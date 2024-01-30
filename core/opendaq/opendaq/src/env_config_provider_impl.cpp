#include <opendaq/env_config_provider_impl.h>
#include <cstdlib>
#include <string>

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
        else if (envKeyUpper == "ROOTDEVICE")
            optionsValue = optionsPtr.get("RootDevice");

        SizeT deep = 0;
        for (const auto & token : splitedKey)
        {
            if (deep == 0)
            {
                if (optionsValue.assigned())
                {
                    deep++;
                    continue;
                }
                optionsValue = optionsPtr;
            }

            if (!optionsValue.assigned())
                break;

            if (deep == splitedKey.getCount() - 1)
            {
                if (optionsValue.hasKey(token))
                {
                    auto child = optionsValue.get(token);
                    switch (child.getCoreType())
                    {
                        case CoreType::ctBool:
                        {
                            auto upEnvValue = toUpperCase(envValue);
                            bool booleanVal = upEnvValue == "TRUE" || upEnvValue == "1"; 
                            optionsValue.set(token, booleanVal);
                            break;
                        }
                        case CoreType::ctInt:
                        {
                            try 
                            {
                                Int number = std::stoll(envValue.toStdString());
                                optionsValue.set(token, number);
                            }
                            catch(...)
                            {
                                // can not convert to long long or out of range
                            }
                            break;
                        }
                        case CoreType::ctFloat:
                        {
                            try 
                            {
                                Float number = std::stod(envValue.toStdString());
                                optionsValue.set(token, number);
                            }
                            catch(...)
                            {
                                // can not convert string value to Float
                            }
                            break;
                        }
                        case CoreType::ctString:
                        {
                            optionsValue.set(token, envValue);
                            break;
                        }
                        default:
                        {
                            // not supported type or we are not in leaf
                        }
                    }
                }
                else
                {
                    optionsValue.set(token, envValue);
                }
            } 
            else 
            {
                if (!optionsValue.hasKey(token))
                {
                    optionsValue.set(token, Dict<IString, IBaseObject>());
                }
                auto child = optionsValue.get(token);
                if (child.getCoreType() != CoreType::ctDict)
                {
                    // type mistamatach
                    break;
                }
                optionsValue = child;
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
