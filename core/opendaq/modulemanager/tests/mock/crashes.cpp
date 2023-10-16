#include <opendaq/module.h>
#include <coretypes/common.h>

extern "C"
daq::ErrCode PUBLIC_EXPORT createModule(daq::IModule**)
{
    return OPENDAQ_ERR_GENERALERROR;
}
