#include <opendaq/base_config_provider_impl.h>
#include <cstdlib>
#include <string>
#include <coretypes/coretypes.h>

BEGIN_NAMESPACE_OPENDAQ

DictPtr<IString, IString> BaseConfigProviderImpl::GetValuesStartingWith(const ListPtr<IString>& cmdLineArgs, const std::string& prefix)
{
    auto result = Dict<IString, IString>();
    if (!cmdLineArgs.assigned() || cmdLineArgs.empty())
        return result;

    for (const std::string & rawArg : cmdLineArgs)
    {
        if (rawArg.substr(0, prefix.length()) == prefix)
        {
            size_t pos = rawArg.find('=', prefix.length());
            if (pos != std::string::npos)
            {
                std::string key = rawArg.substr(0, pos);
                std::string value = rawArg.substr(pos + 1);
                result.set(key, value);
            }
        }
    }
    return result;
}

std::string BaseConfigProviderImpl::ToLowerCase(const std::string &input) 
{
    std::string result = input;
    for (char &c : result)
        c = std::tolower(static_cast<unsigned char>(c));

    return result;
}

ListPtr<IString> BaseConfigProviderImpl::SplitKey(const std::string& envKey, const std::string& prefix, char delimiter)
{
    auto result = List<IString>();

    size_t start = prefix.length();
    size_t end = envKey.find(delimiter, start);

    while (end != std::string::npos) 
    {
        result.pushBack(ToLowerCase(envKey.substr(start, end - start)));
        start = end + 1;
        end = envKey.find(delimiter, start);
    }

    result.pushBack(ToLowerCase(envKey.substr(start, end)));
    return result;
}


BaseObjectPtr BaseConfigProviderImpl::TryConvertToBoolean(const std::string& value)
{
    auto upEnvValue = ToLowerCase(value);
    if (upEnvValue == "true" || upEnvValue == "false")
        return Boolean(upEnvValue == "true");
    return {};
}

BaseObjectPtr BaseConfigProviderImpl::TryConvertToFloat(const std::string& value)
{
    if (value.find('.') == std::string::npos)
        return {};

    try
    {
        Float number = std::stod(value);
        return Floating(number);
    }
    catch (...)
    {
    }
    return {};
}

BaseObjectPtr BaseConfigProviderImpl::TryConvertToInteger(const std::string& value)
{
    try
    {
        Int number = std::stoll(value);
        return Integer(number);
    }
    catch(...)
    {

    }
    return {};
}

BaseObjectPtr BaseConfigProviderImpl::HandleUnderfineValue(const std::string& value)
{
    if (value.length() > 1 && value[0] == '"' && value[value.length() - 1] == '"')
        return String(value.substr(1, value.length() - 2));

    if (auto val = TryConvertToBoolean(value); val.assigned())
        return val;

    if (auto val = TryConvertToFloat(value); val.assigned())
        return val;

    if (auto val = TryConvertToInteger(value); val.assigned())
        return val;

    return String(value);
}

bool BaseConfigProviderImpl::HandleOptionLeaf(DictPtr<IString, IBaseObject> optionsValue, StringPtr envKey, StringPtr envValue)
{
    std::string value = envValue;
    if (value.length() > 1 && value[0] == '"' && value[value.length() - 1] == '"')
        envValue = value.substr(1, value.length() - 2);

    auto child = optionsValue.get(envKey);
    CoreType childType = child.assigned() ? child.getCoreType() : CoreType::ctString;
    switch (childType)
    {
        case CoreType::ctBool:
        {
            if (auto val = TryConvertToBoolean(envValue); val.assigned())
            {
                optionsValue.set(envKey, val);
                return true;
            }
            return false;
        }
        case CoreType::ctInt:
        {
            if (auto val = TryConvertToInteger(envValue); val.assigned())
            {
                optionsValue.set(envKey, val);
                return true;
            }
            return false;
        }
        case CoreType::ctFloat:
        {
            if (auto val = TryConvertToFloat(envValue); val.assigned())
            {
                optionsValue.set(envKey, val);
                return true;
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
bool BaseConfigProviderImpl::WriteValue(DictPtr<IString,IBaseObject> options, const ListPtr<IString>& tokens, const std::string& value)
{
    for (SizeT depth = 0; depth < tokens.getCount(); depth++)
    {
        if (!options.assigned())
            return false;

        const auto & token = tokens[depth];

        if (depth == tokens.getCount() - 1)
        {
            if (options.hasKey(token))
                HandleOptionLeaf(options, token, value);
            else
                options.set(token, HandleUnderfineValue(value));
        } 
        else 
        {
            if (!options.hasKey(token))
                options.set(token, Dict<IString, IBaseObject>());

            auto child = options.get(token);
            if (child.assigned() && child.getCoreType() != CoreType::ctDict)
                // type mistamatach
                return false;
            options = child;
        }
    }
    return true;
}

END_NAMESPACE_OPENDAQ
