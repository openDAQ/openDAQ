#include <coreobjects/version.h>
#include <coreobjects/coreobjects_config.h>
#include <coreobjects/exceptions.h>

extern "C"
void PUBLIC_EXPORT daqCoreObjectsGetVersion(unsigned int* /*major*/, unsigned int* /*minor*/, unsigned int* /*revision*/)
{
    throw(daq::NotCompatibleVersionException("The running version of openDAQ \"{}.{}.{}\" does not support obsolete mechanism for checking core dependencies version",
                                             OPENDAQ_COREOBJECTS_MAJOR_VERSION,
                                             OPENDAQ_COREOBJECTS_MINOR_VERSION,
                                             OPENDAQ_COREOBJECTS_PATCH_VERSION));
}
