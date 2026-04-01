#include <opendaq/module.h>
#include <coretypes/impl.h>

#include "mock_module.h"
#include <opendaq/opendaq_config.h>
#include <opendaq/version.h>

extern "C"
daq::ErrCode PUBLIC_EXPORT createModule(daq::IModule** module)
{
    OPENDAQ_PARAM_NOT_NULL(module);

    return daq::createObject<daq::IModule, MockModuleImpl>(module, "mock_dep");
}

extern "C"
daq::ErrCode PUBLIC_EXPORT checkDependencies(daq::IString** logMessage)
{
    return OPENDAQ_SUCCESS;
}

extern "C"
daq::ErrCode PUBLIC_EXPORT getCoreVersionMetadata(EnumerateMetadataFieldFunc enumerateFieldFunc, void* userData)
{
    enumerateFieldFunc("major", OPENDAQ_OPENDAQ_MAJOR_VERSION_STR, userData);
    enumerateFieldFunc("minor", OPENDAQ_OPENDAQ_MINOR_VERSION_STR, userData);
    enumerateFieldFunc("patch", OPENDAQ_OPENDAQ_PATCH_VERSION_STR, userData);
    enumerateFieldFunc("branch", OPENDAQ_OPENDAQ_BRANCH_NAME, userData);
    enumerateFieldFunc("sha", OPENDAQ_OPENDAQ_REVISION_HASH, userData);
    return OPENDAQ_SUCCESS;
}
