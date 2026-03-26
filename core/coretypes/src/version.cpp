#include <coretypes/version.h>
#include <coretypes/coretypes_config.h>

extern "C"
void PUBLIC_EXPORT daqCoreTypesGetVersion(unsigned int* major, unsigned int* minor, unsigned int* revision)
{
    *major = 0u;
    *minor = 0u;
    *revision = 0u;
}
