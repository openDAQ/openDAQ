#include <opendaq/module.h>
#include <coretypes/impl.h>

#include "mock_module.h"

#include <opendaq/module_check_dependencies.h>
#include <opendaq/opendaq_config.h>

extern "C"
daq::ErrCode PUBLIC_EXPORT createModule(daq::IModule** module)
{
    OPENDAQ_PARAM_NOT_NULL(module);

    return daq::createObject<daq::IModule, MockModuleImpl>(module, "mock_ver_match");
}

extern "C"
daq::ErrCode PUBLIC_EXPORT getCoreVersionMetadata(daq::EnumerateMetadataFieldFunc enumerateFieldFunc, void* userData)
{
    enumerateFieldFunc("major", OPENDAQ_OPENDAQ_MAJOR_VERSION_STR, userData);
    enumerateFieldFunc("minor", OPENDAQ_OPENDAQ_MINOR_VERSION_STR, userData);
    enumerateFieldFunc("patch", "", userData);
    return OPENDAQ_SUCCESS;
}
