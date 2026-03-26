#include <opendaq/opendaq_config.h>
#include <opendaq/version.h>

extern "C"
void PUBLIC_EXPORT daqOpenDaqGetVersion(unsigned int* major, unsigned int* minor, unsigned int* revision)
{
    *major = 0u;
    *minor = 0u;
    *revision = 0u;
}
