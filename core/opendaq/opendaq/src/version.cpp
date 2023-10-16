#include <opendaq/opendaq_config.h>
#include <opendaq/version.h>

extern "C"
void PUBLIC_EXPORT daqOpenDaqGetVersion(unsigned int* major, unsigned int* minor, unsigned int* revision)
{
    *major = OPENDAQ_OPENDAQ_MAJOR_VERSION;
    *minor = OPENDAQ_OPENDAQ_MINOR_VERSION;
    *revision = OPENDAQ_OPENDAQ_PATCH_VERSION;
}
