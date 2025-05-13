#include <coretypes/version_info_factory.h>
#include <properties_module/properties_device_impl.h>
#include <properties_module/properties_module_impl.h>
#include <properties_module/version.h>

BEGIN_NAMESPACE_PROPERTIES_MODULE

PropertiesModuleImpl::PropertiesModuleImpl(ContextPtr ctx)
    : Module("PropertiesModule",
             daq::VersionInfo(PROPERTIES_MODULE_MAJOR_VERSION, PROPERTIES_MODULE_MINOR_VERSION, PROPERTIES_MODULE_PATCH_VERSION),
             std::move(ctx),
             "PropertiesModule")
    , deviceAdded(false)
{
}

ListPtr<IDeviceInfo> PropertiesModuleImpl::onGetAvailableDevices()
{
    auto availableDevices = List<IDeviceInfo>();

    availableDevices.pushBack(PropertiesDeviceImpl::CreateDeviceInfo());

    return availableDevices;
}

DictPtr<IString, IDeviceType> PropertiesModuleImpl::onGetAvailableDeviceTypes()
{
    auto result = Dict<IString, IDeviceType>();

    auto deviceType = PropertiesDeviceImpl::CreateType();
    result.set(deviceType.getId(), deviceType);

    return result;
}

DevicePtr PropertiesModuleImpl::onCreateDevice(const StringPtr& connectionString,
                                               const ComponentPtr& parent,
                                               const PropertyObjectPtr& /*config*/)
{
    std::scoped_lock lock(sync);

    std::string connStr = connectionString;
    if (connStr.find("properties://") != 0)
        throw std::runtime_error("Invalid connection string prefix");

    if (deviceAdded)
        throw std::runtime_error("Only one device can be created");

    auto device = createWithImplementation<IDevice, PropertiesDeviceImpl>(context, parent, "random");



    deviceAdded = true;
    return device;
}

END_NAMESPACE_PROPERTIES_MODULE
