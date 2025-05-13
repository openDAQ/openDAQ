#include <fmt/format.h>
#include <opendaq/context_ptr.h>
#include <opendaq/thread_name.h>
#include <properties_module/common.h>
#include <properties_module/properties_device_impl.h>

#include <iomanip>

#include "opendaq/device_type_factory.h"

BEGIN_NAMESPACE_PROPERTIES_MODULE

static StringPtr ToIso8601(const std::chrono::system_clock::time_point& timePoint);

PropertiesDeviceImpl::PropertiesDeviceImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
    : GenericDevice<>(ctx, parent, localId)
{
}

DeviceInfoPtr PropertiesDeviceImpl::CreateDeviceInfo()
{
    auto devInfo = DeviceInfo("properties://device");

    devInfo.setName("PropertiesDevice");
    devInfo.setManufacturer("openDAQ");
    devInfo.setModel("Properties device");
    devInfo.setSerialNumber("ExamplePD1234");
    devInfo.setDeviceType(CreateType());

    return devInfo;
}

DeviceTypePtr PropertiesDeviceImpl::CreateType()
{
    return DeviceType("prop_dev", "Properties device", "Properties device", "properties");
}

DeviceInfoPtr PropertiesDeviceImpl::onGetInfo()
{
    return CreateDeviceInfo();
}

StringPtr ToIso8601(const std::chrono::system_clock::time_point& timePoint)
{
    std::time_t time = std::chrono::system_clock::to_time_t(timePoint);
    std::tm tm = *std::gmtime(&time);  // Use gmtime for UTC

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");  // ISO 8601 format
    return oss.str();
}

END_NAMESPACE_PROPERTIES_MODULE
