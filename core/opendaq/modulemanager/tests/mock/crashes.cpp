#include <opendaq/module.h>
#include <coretypes/common.h>

extern "C"
daq::ErrCode PUBLIC_EXPORT createModule(daq::IModule**)
{
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_GENERALERROR);
}
