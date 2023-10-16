#include <scheduler/version.h>
#include <scheduler/scheduler_config.h>

extern "C" 
void PUBLIC_EXPORT daqSchedulerGetVersion(unsigned int* major, unsigned int* minor, unsigned int* revision)
{
    *major = OPENDAQ_SCHEDULER_MAJOR_VERSION;
    *minor = OPENDAQ_SCHEDULER_MINOR_VERSION;
    *revision = OPENDAQ_SCHEDULER_PATCH_VERSION;
}
