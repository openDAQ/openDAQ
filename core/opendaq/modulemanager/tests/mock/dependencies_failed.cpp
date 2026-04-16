#include <opendaq/module.h>
#include <coretypes/common.h>
#include <coretypes/stringobject.h>
#include <opendaq/version.h>

extern "C"
daq::ErrCode PUBLIC_EXPORT createModule(daq::IModule**)
{
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_GENERALERROR);
}

extern "C"
daq::ErrCode PUBLIC_EXPORT checkDependencies(daq::IString** logMessage)
{
    unsigned int major, minor, patch;
    daq::StringPtr branch, sha;

    daq::ErrCode errCode = getSdkCoreVersionMetadata(&major, &minor, &patch, &branch, &sha, nullptr);
    OPENDAQ_RETURN_IF_FAILED(errCode, "Getting SDK version metadata failed");

    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_GENERALERROR, "Mock module checkDependencies failure");
}
