#include <opendaq/cmd_line_args_config_provider_impl.h>
#include <string>

BEGIN_NAMESPACE_OPENDAQ

CmdLineArgsConfigProviderImpl::CmdLineArgsConfigProviderImpl(const ListPtr<IString>& cmdLineArgs)
    : cmdLineArgs(cmdLineArgs)
{
    if (!cmdLineArgs.assigned())
        DAQ_THROW_EXCEPTION(ArgumentNullException("List of command line args is not assigned"));
}

ErrCode INTERFACE_FUNC CmdLineArgsConfigProviderImpl::populateOptions(IDict* options) 
{
    OPENDAQ_PARAM_NOT_NULL(options);

    auto optionsPtr = DictPtr<IString, IBaseObject>::Borrow(options);

    std::string prefix = "-C";
    for (const auto& [argKey, argVal]: GetValuesStartingWith(cmdLineArgs, prefix))
    {
        WriteValue(optionsPtr, SplitKey(argKey, prefix, '_'), argVal);
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
