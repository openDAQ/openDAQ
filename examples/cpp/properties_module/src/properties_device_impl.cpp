#include <fmt/format.h>
#include <opendaq/context_ptr.h>
#include <opendaq/thread_name.h>
#include <properties_module/common.h>
#include <properties_module/properties_device_impl.h>

#include <iomanip>

#include "opendaq/device_type_factory.h"

BEGIN_NAMESPACE_PROPERTIES_MODULE

static StringPtr ToIso8601(const std::chrono::system_clock::time_point& timePoint);

PropertiesDeviceImpl::PropertiesDeviceImpl(const daq::ContextPtr& ctx,
                                           const daq::ComponentPtr& parent,
                                           const StringPtr& localId,
                                           const StringPtr& name)
    : GenericDevice<>(ctx, parent, localId, nullptr, name)
{
}

DeviceInfoPtr PropertiesDeviceImpl::CreateDeviceInfo(size_t id, const StringPtr& serialNumber)
{
    auto devInfo = DeviceInfoWithChanegableFields({"userName", "location"});
    devInfo.setName(fmt::format("Device {}", id));
    devInfo.setConnectionString(fmt::format("daqref://device{}", id));
    devInfo.setManufacturer("openDAQ");
    devInfo.setModel("Properties device");
    devInfo.setDeviceType(CreateType());
    std::string currentTime = ToIso8601(std::chrono::system_clock::now());
    devInfo.addProperty(StringProperty("SetupDate", currentTime));
    return devInfo;
}

DeviceTypePtr PropertiesDeviceImpl::CreateType()
{
    const auto defaultConfig = PropertyObject();
    return DeviceType("properties", "Properties device", "Properties device", "properties", defaultConfig);
}

bool PropertiesDeviceImpl::allowAddDevicesFromModules()
{
    return true;
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
