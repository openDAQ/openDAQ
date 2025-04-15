#include <opendaq/module.h>
#include <coretypes/common.h>
#include <coretypes/stringobject.h>

extern "C"
daq::ErrCode PUBLIC_EXPORT createModule(daq::IModule**)
{
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_GENERALERROR);
}


extern "C"
daq::ErrCode PUBLIC_EXPORT checkDependencies(daq::IString** message)
{
    createString(message, "Mock failure");
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_GENERALERROR);
}
