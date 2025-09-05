#include <coretypes/version_info_factory.h>
#include <opendaq/custom_log.h>
#include <simulator_device_module/simulator_device_impl.h>
#include <simulator_device_module/simulator_device_module_impl.h>
#include <simulator_device_module/version.h>
#include <boost/asio/ip/host_name.hpp>

BEGIN_NAMESPACE_SIMULATOR_DEVICE_MODULE

SimulatorDeviceModule::SimulatorDeviceModule(const ContextPtr& context)
    : Module(SIMULATOR_MODULE_NAME,
             VersionInfo(SIMULATOR_DEVICE_MODULE_MAJOR_VERSION,
                         SIMULATOR_DEVICE_MODULE_MINOR_VERSION,
                         SIMULATOR_DEVICE_MODULE_PATCH_VERSION),
             context,
             SIMULATOR_MODULE_ID)
{
}

ListPtr<IDeviceInfo> SimulatorDeviceModule::onGetAvailableDevices()
{
    const auto options = populateDefaultModuleOptions(this->context.getModuleOptions(SIMULATOR_MODULE_ID));
    return { SimulatorDeviceImpl::CreateDeviceInfo(options) };
}

DictPtr<IString, IDeviceType> SimulatorDeviceModule::onGetAvailableDeviceTypes()
{
    auto deviceType = SimulatorDeviceImpl::CreateType();
    return Dict<IString, IBaseObject>({{deviceType.getId(), deviceType}});
}

DevicePtr SimulatorDeviceModule::onCreateDevice(const StringPtr& connectionString,
                                                const ComponentPtr& parent,
                                                const PropertyObjectPtr& config)
{
    std::scoped_lock lock(sync);
    
    clearRemovedDevice();
    if (device.assigned())
        DAQ_THROW_EXCEPTION(AlreadyExistsException, "Simulator device is already created!");
    
    const auto options = populateDefaultModuleOptions(this->context.getModuleOptions(SIMULATOR_MODULE_ID));
    auto info = SimulatorDeviceImpl::CreateDeviceInfo(options);

    auto devicePtr = createWithImplementation<IDevice, SimulatorDeviceImpl>(config, context, parent, info);
    device = devicePtr;
    return devicePtr.detach();
}

DictPtr<IString, IBaseObject> SimulatorDeviceModule::populateDefaultModuleOptions(const DictPtr<IString, IBaseObject>& inputOptions)
{
    std::string hostname = boost::asio::ip::host_name();
    auto defaultOptions = Dict<IString, IBaseObject>({{"Name", "Simulator"}, {"Manufacturer", "openDAQ"}, {"SerialNumber", hostname}});

    for (const auto& [key, value] : inputOptions)
    {
        if (defaultOptions.hasKey(key))
        {
            defaultOptions.set(key, value);
        }
    }

    return defaultOptions;
}

void SimulatorDeviceModule::clearRemovedDevice()
{
    const bool isNull = !device.assigned() || !device.getRef().assigned();
    if (isNull || device.getRef().isRemoved())
        device = nullptr;
}

END_NAMESPACE_SIMULATOR_DEVICE_MODULE
