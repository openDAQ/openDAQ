#include <coreobjects/version.h>
#include <coreobjects/coreobjects_config.h>

extern "C"
void PUBLIC_EXPORT daqCoreObjectsGetVersion(unsigned int* major, unsigned int* minor, unsigned int* revision)
{
    *major = OPENDAQ_COREOBJECTS_MAJOR_VERSION;
    *minor = OPENDAQ_COREOBJECTS_MINOR_VERSION;
    *revision = OPENDAQ_COREOBJECTS_PATCH_VERSION;
}
