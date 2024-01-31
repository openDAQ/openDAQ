#include <opendaq/cmd_line_args_config_provider_impl.h>
#include <string>

BEGIN_NAMESPACE_OPENDAQ

CmdLineArgsConfigProviderImpl::CmdLineArgsConfigProviderImpl(const ListPtr<IString>& cmdLineArgs)
    : cmdLineArgs(cmdLineArgs)
{
}

ErrCode INTERFACE_FUNC CmdLineArgsConfigProviderImpl::populateOptions(IDict* options) 
{
    if (options == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    auto optionsPtr = DictPtr<IString, IBaseObject>::Borrow(options);

    std::string prefix = "--config-";
    for (const auto& [argKey, argVal]: GetValuesStartingWith(cmdLineArgs, prefix))
    {
        auto splitedKey = SplitByDelimiter(argKey, prefix);

        if (splitedKey.empty())
            break;

        DictPtr<IString, IBaseObject> optionsValue;

        auto argKeyUpper = ToUpperCase(splitedKey[0]);
        if (argKeyUpper == "MODULEMANAGER")
            optionsValue = optionsPtr.get("ModuleManager");
        else if (argKeyUpper == "SCHEDULER")
            optionsValue = optionsPtr.get("Scheduler");
        else if (argKeyUpper == "LOGGING")
            optionsValue = optionsPtr.get("Logging");
        else if (argKeyUpper == "MODULES")
            optionsValue = optionsPtr.get("Modules");
        else if (argKeyUpper == "ROOTDEVICE")
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
                    handleOptionLeaf(optionsValue, token, argVal);
                else
                    optionsValue.set(token, argVal);
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

bool CmdLineArgsConfigProviderImpl::handleOptionLeaf(DictPtr<IString, IBaseObject> optionsValue, StringPtr argKey, StringPtr argValue)
{
    auto child = optionsValue.get(argKey);
    CoreType childType = child.assigned() ? child.getCoreType() : CoreType::ctString;
    switch (childType)
    {
        case CoreType::ctBool:
        {
            auto upEnvValue = ToUpperCase(argValue);
            bool booleanVal = upEnvValue == "TRUE" || upEnvValue == "1"; 
            optionsValue.set(argKey, booleanVal);
            return true;
        }
        case CoreType::ctInt:
        {
            try 
            {
                Int number = std::stoll(argValue.toStdString());
                optionsValue.set(argKey, number);
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
                Float number = std::stod(argValue.toStdString());
                optionsValue.set(argKey, number);
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
            optionsValue.set(argKey, argValue);
            return true;
        }
        default:
        {
            // not supported type or we are not in leaf
            return false;
        }
    }
}

std::string CmdLineArgsConfigProviderImpl::ToUpperCase(const std::string &input) 
{
    std::string result = input;
    for (char &c : result)
        c = std::toupper(static_cast<unsigned char>(c));

    return result;
}

DictPtr<IString, IString> CmdLineArgsConfigProviderImpl::GetValuesStartingWith(const ListPtr<IString>& cmdLineArgs, const std::string& prefix)
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

ListPtr<IString> CmdLineArgsConfigProviderImpl::SplitByDelimiter(const std::string& argKey, const std::string& prefix, char delimiter)
{
    auto result = List<IString>();

    size_t start = prefix.length();
    size_t end = argKey.find(delimiter, start);

    while (end != std::string::npos) 
    {
        result.pushBack(argKey.substr(start, end - start));
        start = end + 1;
        end = argKey.find(delimiter, start);
    }

    result.pushBack(argKey.substr(start, end));
    return result;
}

/////////////////////
////
//// FACTORIES
////
////////////////////

extern "C" ErrCode PUBLIC_EXPORT createCmdLineArgsConfigProvider(IConfigProvider** objTmp, IList* cmdLineArgs)
{
    return daq::createObject<IConfigProvider, CmdLineArgsConfigProviderImpl>(objTmp, cmdLineArgs);
}

END_NAMESPACE_OPENDAQ
