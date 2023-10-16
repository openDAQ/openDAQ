#include "opcuashared/opcuaversion.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA

#pragma push_macro("major")
#pragma push_macro("minor")

#undef major
#undef minor

OpcUaVersion::OpcUaVersion(const char* version)
{
    std::sscanf(version, "%d.%d.%d", &major, &minor, &patch);
}

std::string OpcUaVersion::toString() const
{
    char buffer[36];
    snprintf(buffer, sizeof(buffer), "%d.%d.%d", major, minor, patch);
    return buffer;
}

bool OpcUaVersion::Compatible(const OpcUaVersion& serverVersion, const OpcUaVersion& systemVersion)
{
    return serverVersion.major == systemVersion.major && serverVersion.minor <= systemVersion.minor;
}

bool OpcUaVersion::HasFeature(const OpcUaVersion& serverVersion, const OpcUaVersion& featureVersion)
{
    if (serverVersion.major > featureVersion.major)
        return true;

    return serverVersion.major == featureVersion.major && featureVersion.minor <= serverVersion.minor;
}

#pragma pop_macro("minor")
#pragma pop_macro("major")

END_NAMESPACE_OPENDAQ_OPCUA
