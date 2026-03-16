#include <coretypes/version.h>
#include <coretypes/coretypes_config.h>
#include <coretypes/exceptions.h>

extern "C"
void PUBLIC_EXPORT daqCoreTypesGetVersion(unsigned int* /*major*/, unsigned int* /*minor*/, unsigned int* /*revision*/)
{
    throw(daq::NotCompatibleVersionException("The running version of openDAQ \"{}.{}.{}\" does not support obsolete mechanism for checking core dependencies version",
                                             OPENDAQ_CORETYPES_MAJOR_VERSION,
                                             OPENDAQ_CORETYPES_MINOR_VERSION,
                                             OPENDAQ_CORETYPES_PATCH_VERSION));
}
