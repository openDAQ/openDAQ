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
        WriteValue(optionsPtr, SplitKey(argKey, prefix, '-'), argVal);
    }
    return OPENDAQ_SUCCESS;
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
