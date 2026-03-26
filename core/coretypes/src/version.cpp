#include <coretypes/version.h>
#include <coretypes/coretypes_config.h>

extern "C"
void PUBLIC_EXPORT daqCoreTypesGetVersion(unsigned int* major, unsigned int* minor, unsigned int* revision)
{
    *major = OPENDAQ_CORETYPES_MAJOR_VERSION;
    *minor = OPENDAQ_CORETYPES_MINOR_VERSION;
    *revision = OPENDAQ_CORETYPES_PATCH_VERSION;
}
