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
daq::ErrCode PUBLIC_EXPORT getCoreVersionMetadata(unsigned int* major, unsigned int* minor, unsigned int* patch, daq::IString** branch, daq::IString** sha, daq::IString** fork)
{
    if (major != nullptr)
        *major = OPENDAQ_OPENDAQ_MAJOR_VERSION;
    if (minor != nullptr)
        *minor = OPENDAQ_OPENDAQ_MINOR_VERSION;
    if (patch != nullptr)
        *patch = OPENDAQ_OPENDAQ_PATCH_VERSION;
    if (branch != nullptr)
        *branch = daq::String(OPENDAQ_OPENDAQ_BRANCH_NAME).detach();
    if (sha != nullptr)
        *sha = daq::String(OPENDAQ_OPENDAQ_REVISION_HASH).detach();
    return OPENDAQ_SUCCESS;
}
