#include <opendaq/module.h>
#include <opendaq/module_check_dependencies.h>
#include <opendaq/opendaq_config.h>

extern "C"
daq::ErrCode PUBLIC_EXPORT createModule(daq::IModule** module)
{
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_GENERALERROR);
}

extern "C"
daq::ErrCode PUBLIC_EXPORT getCoreVersionMetadata(daq::EnumerateMetadataFieldFunc enumerateFieldFunc, void* userData)
{
    enumerateFieldFunc("major", OPENDAQ_OPENDAQ_MAJOR_VERSION_STR, userData);
    enumerateFieldFunc("minor", "", userData);
    return OPENDAQ_SUCCESS;
}
