#include <opendaq/opendaq_config.h>
#include <opendaq/version.h>
#include <opendaq/exceptions.h>

extern "C"
void PUBLIC_EXPORT daqOpenDaqGetVersion(unsigned int* /*major*/, unsigned int* /*minor*/, unsigned int* /*revision*/)
{
    throw(daq::NotCompatibleVersionException("The running version of openDAQ \"{}.{}.{}\" does not support obsolete mechanism for checking core dependencies version",
                                             OPENDAQ_OPENDAQ_MAJOR_VERSION,
                                             OPENDAQ_OPENDAQ_MINOR_VERSION,
                                             OPENDAQ_OPENDAQ_PATCH_VERSION));
}
