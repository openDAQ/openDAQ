#include <opendaq/device_info_impl.h>
#include <opendaq/component_ptr.h>
#include <coretypes/validation.h>
#include "coretypes/impl.h"
#include <coreobjects/property_object_factory.h>
#include <opendaq/device_info_factory.h>
#include <boost/algorithm/string.hpp>

BEGIN_NAMESPACE_OPENDAQ

#if !defined(BUILDING_STATIC_LIBRARY)

extern "C"
ErrCode PUBLIC_EXPORT createDeviceInfoConfig(IDeviceInfoConfig** objTmp, IString* name, IString* connectionString)
{
    return createObject<IDeviceInfoConfig, DeviceInfoConfigImpl<>, IString*, IString*>(objTmp, name, connectionString);
}

extern "C"
ErrCode PUBLIC_EXPORT createDeviceInfoConfigWithCustomSdkVersion(IDeviceInfoConfig** objTmp, IString* name, IString* connectionString, IString* sdkVersion)
{
    return createObject<IDeviceInfoConfig, DeviceInfoConfigImpl<>, IString*, IString*, IString*>(objTmp, name, connectionString, sdkVersion);
}

extern "C"
ErrCode PUBLIC_EXPORT createDeviceInfoConfigWithChanegableFields(IDeviceInfoConfig** objTmp, IList* changeableDefaultPropertyNames)
{
    return createObject<IDeviceInfoConfig, DeviceInfoConfigImpl<>, IList*>(objTmp, changeableDefaultPropertyNames);
}

#endif

END_NAMESPACE_OPENDAQ
