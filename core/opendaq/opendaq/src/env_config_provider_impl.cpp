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

EnvConfigProviderImpl::EnvConfigProviderImpl()
{
}

DictPtr<IString, IString> EnvConfigProviderImpl::GetEnvValuesStartingWith(const std::string& prefix)
{
    if (prefix.empty())
        return {};

    auto result = Dict<IString, IString>();
    for (char** env = ENVIRON; *env != 0; env++) 
    {
        std::string envVar = *env;

        if (envVar.substr(0, prefix.length()) == prefix)
        {
            size_t pos = envVar.find('=', prefix.length());
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

ListPtr<IString> EnvConfigProviderImpl::SplitEnvKey(const std::string& envKey, const std::string& prefix, char delimiter)
{
    auto result = List<IString>();

    size_t start = prefix.length();
    size_t end = envKey.find(delimiter, start);

    while (end != std::string::npos) 
    {
        result.pushBack(envKey.substr(start, end - start));
        start = end + 1;
        end = envKey.find(delimiter, start);
    }

    result.pushBack(envKey.substr(start, end));
    return result;
}

std::string EnvConfigProviderImpl::ToUpperCase(const std::string &input) 
{
    std::string result = input;
    for (char &c : result)
        c = std::toupper(static_cast<unsigned char>(c));

    return result;
}

BaseObjectPtr EnvConfigProviderImpl::handleUnderfineValue(const std::string& value)
{
    if (value.length() > 1 && value[0] == '"' && value[value.length() - 1] == '"')
        return String(value.substr(1, value.length() - 2));

    // try parse as boolean
    auto upEnvValue = ToUpperCase(value);
    if (upEnvValue == "TRUE" || upEnvValue == "FALSE")
        return Boolean(upEnvValue == "TRUE");

    // try parse as float
    if (value.find('.') != std::string::npos)
    {
        try
        {
            Float number = std::stod(value);
            return Floating(number);
        }
        catch (...)
        {
        }
    }

    // try paese as integer
    try
    {
        Int number = std::stoll(value);
        return Integer(number);
    }
    catch(...)
    {

    }

    // leave as string
    return String(value);
}

bool EnvConfigProviderImpl::handleOptionLeaf(DictPtr<IString, IBaseObject> optionsValue, StringPtr envKey, StringPtr envValue)
{
    if (envValue.getLength() > 1 && envValue[0] == '"' && envValue[envValue.getLength() - 1] == '"')
       envValue = envValue.toStdString().substr(1, envValue.getLength() - 2);

    auto child = optionsValue.get(envKey);
    CoreType childType = child.assigned() ? child.getCoreType() : CoreType::ctString;
    switch (childType)
    {
        case CoreType::ctBool:
        {
            auto upEnvValue = ToUpperCase(envValue);
            bool booleanVal = upEnvValue == "TRUE" || upEnvValue == "1"; 
            optionsValue.set(envKey, booleanVal);
            return true;
        }
        case CoreType::ctInt:
        {
            try 
            {
                Int number = std::stoll(envValue.toStdString());
                optionsValue.set(envKey, number);
                return true;
            }
            catch(...)
            {
                // can not convert to long long or out of range
            }
            return false;
        }
        case CoreType::ctFloat:
        {
            try 
            {
                Float number = std::stod(envValue.toStdString());
                optionsValue.set(envKey, number);
                return true;
            }
            catch(...)
            {
                // can not convert string value to Float
            }
            return false;
        }
        case CoreType::ctString:
        {
            optionsValue.set(envKey, envValue);
            return true;
        }
        default:
        {
            // not supported type or we are not in leaf
            return false;
        }
    }
}

ErrCode INTERFACE_FUNC EnvConfigProviderImpl::populateOptions(IDict* options) 
{
    if (options == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    auto optionsPtr = DictPtr<IString, IBaseObject>::Borrow(options);

    std::string envPrefix = "OPENDAQ_CONFIG_";
    auto envDict = GetEnvValuesStartingWith(envPrefix);

    for (const auto& [envKey, envValue] : envDict)
    {
        auto splitedKey = SplitEnvKey(envKey, envPrefix);
        if (splitedKey.empty())
            break;

        DictPtr<IString, IBaseObject> optionsValue;

        auto envKeyUpper = ToUpperCase(splitedKey[0]);
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

        for (SizeT depth = 0; depth < splitedKey.getCount(); depth++)
        {
            if (depth == 0)
            {
                if (optionsValue.assigned())
                    continue;
                optionsValue = optionsPtr;
            }

            if (!optionsValue.assigned())
                break;

            const auto & token = splitedKey[depth];

            if (depth == splitedKey.getCount() - 1)
            {
                if (optionsValue.hasKey(token))
                    handleOptionLeaf(optionsValue, token, envValue);
                else
                    optionsValue.set(token, handleUnderfineValue(envValue));
            } 
            else 
            {
                if (!optionsValue.hasKey(token))
                    optionsValue.set(token, Dict<IString, IBaseObject>());

                auto child = optionsValue.get(token);
                if (child.assigned() && child.getCoreType() != CoreType::ctDict)
                    // type mistamatach
                    break;
                optionsValue = child;
            }
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
