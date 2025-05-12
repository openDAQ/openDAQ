#include <coretypes/version_info_factory.h>
#include <properties_module/properties_device_impl.h>
#include <properties_module/properties_module_impl.h>
#include <properties_module/version.h>
#include <utility>

BEGIN_NAMESPACE_PROPERTIES_MODULE
PropertiesModule::PropertiesModule(ContextPtr context)
    : Module("PropertiesModule",
             daq::VersionInfo(PROPERTIES_MODULE_MAJOR_VERSION, PROPERTIES_MODULE_MINOR_VERSION, PROPERTIES_MODULE_PATCH_VERSION),
             std::move(context))
{
}

ListPtr<IDeviceInfo> PropertiesModule::onGetAvailableDevices()
{
    auto availableDevices = List<IDeviceInfo>();

    availableDevices.pushBack(PropertiesDeviceImpl::CreateDeviceInfo(0));

    return availableDevices;
}

END_NAMESPACE_PROPERTIES_MODULE
