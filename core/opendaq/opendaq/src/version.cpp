#include <opendaq/opendaq_config.h>
#include <opendaq/version.h>
#include <coretypes/stringobject_factory.h>

extern "C"
void PUBLIC_EXPORT daqOpenDaqGetVersion(unsigned int* major, unsigned int* minor, unsigned int* revision)
{
    *major = OPENDAQ_OPENDAQ_MAJOR_VERSION;
    *minor = OPENDAQ_OPENDAQ_MINOR_VERSION;
    *revision = OPENDAQ_OPENDAQ_PATCH_VERSION;
}

extern "C"
daq::ErrCode PUBLIC_EXPORT getSdkCoreVersionMetadata(unsigned int* major, unsigned int* minor, unsigned int* patch, daq::IString** branch, daq::IString** sha, daq::IString** fork)
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
