#include <logger/version.h>
#include <logger/logger_config.h>

extern "C" 
void PUBLIC_EXPORT daqLoggerGetVersion(unsigned int* major, unsigned int* minor, unsigned int* revision)
{
    *major = OPENDAQ_LOGGER_MAJOR_VERSION;
    *minor = OPENDAQ_LOGGER_MINOR_VERSION;
    *revision = OPENDAQ_LOGGER_PATCH_VERSION;
}
