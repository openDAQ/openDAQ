#include <coreobjects/version.h>
#include <coreobjects/coreobjects_config.h>

extern "C"
void PUBLIC_EXPORT daqCoreObjectsGetVersion(unsigned int* major, unsigned int* minor, unsigned int* revision)
{
    *major = 0u;
    *minor = 0u;
    *revision = 0u;
}
