#include <coretypes/version_info_factory.h>
#include <opendaq/custom_log.h>
#include <ref_device_module/ref_device_impl.h>
#include <ref_device_module/ref_device_module_impl.h>
#include <ref_device_module/version.h>

BEGIN_NAMESPACE_REF_DEVICE_MODULE

RefDeviceModule::RefDeviceModule(ContextPtr context)
    : Module("ReferenceDeviceModule",
             daq::VersionInfo(REF_DEVICE_MODULE_MAJOR_VERSION, REF_DEVICE_MODULE_MINOR_VERSION, REF_DEVICE_MODULE_PATCH_VERSION),
             std::move(context),
             REF_MODULE_NAME)
{
    auto options = this->context.getModuleOptions(REF_MODULE_NAME);
    maxNumberOfDevices = options.getOrDefault("MaxNumberOfDevices", 2);
    devices.resize(maxNumberOfDevices);
}

ListPtr<IDeviceInfo> RefDeviceModule::onGetAvailableDevices()
{
    StringPtr serialNumber;

    const auto options = this->context.getModuleOptions(REF_MODULE_NAME);

    if (options.assigned())
    {
        serialNumber = options.getOrDefault("SerialNumber");
    }

    auto availableDevices = List<IDeviceInfo>();

    if (serialNumber.assigned())
    {
        auto info = RefDeviceImpl::CreateDeviceInfo(0, serialNumber);
        availableDevices.pushBack(info);
    }
    else
    {
        for (size_t i = 0; i < 2; i++)
        {
            auto info = RefDeviceImpl::CreateDeviceInfo(i);
            availableDevices.pushBack(info);
        }
    }

    return availableDevices;
}

DictPtr<IString, IDeviceType> RefDeviceModule::onGetAvailableDeviceTypes()
{
    auto result = Dict<IString, IDeviceType>();

    auto deviceType = RefDeviceImpl::CreateType();
    result.set(deviceType.getId(), deviceType);

    return result;
}

DevicePtr RefDeviceModule::onCreateDevice(const StringPtr& connectionString,
                                          const ComponentPtr& parent,
                                          const PropertyObjectPtr& config)
{
    const auto id = getIdFromConnectionString(connectionString);

    std::scoped_lock lock(sync);

    if (id >= devices.size())
    {
        LOG_W("Device with id \"{}\" not found", id);
        DAQ_THROW_EXCEPTION(NotFoundException);
    }

    clearRemovedDevices();
    if (devices[id].assigned() && devices[id].getRef().assigned())
    {
        LOG_W("Device with id \"{}\" already exist", id);
        DAQ_THROW_EXCEPTION(AlreadyExistsException);
    }
    
    const auto options = this->context.getModuleOptions(REF_MODULE_NAME);
    StringPtr localId;
    StringPtr name = fmt::format("Device {}", id);

    if (config.assigned())
    {
        if (config.hasProperty("LocalId"))
        {
            StringPtr localIdTemp = config.getPropertyValue("LocalId");
            localId = localIdTemp.getLength() ? localIdTemp : nullptr;
        }
        if (config.hasProperty("Name"))
        {
            StringPtr nameTemp = config.getPropertyValue("Name");
            name = nameTemp.getLength() ? nameTemp : nullptr;
        }
    }

    if (options.assigned())
    {
        if (StringPtr localIdTemp = options.getOrDefault("LocalId"); localIdTemp.assigned())
            localId = localIdTemp.getLength() ? localIdTemp : nullptr;

        if (StringPtr nameTemp = options.getOrDefault("Name"); nameTemp.assigned())
            name = nameTemp.getLength() ? nameTemp : nullptr;
    }

    if (!localId.assigned())
        localId = fmt::format("RefDev{}", id);

    auto devicePtr = createWithImplementation<IDevice, RefDeviceImpl>(id, config, context, parent, localId, name);
    devices[id] = devicePtr;
    return devicePtr;
}

size_t RefDeviceModule::getIdFromConnectionString(const std::string& connectionString) const
{
    std::string prefixWithDeviceStr = "daqref://device";
    auto found = connectionString.find(prefixWithDeviceStr);
    if (found != 0)
    {
        LOG_W("Invalid connection string \"{}\", no prefix", connectionString);
        DAQ_THROW_EXCEPTION(InvalidParameterException);
    }

    auto idStr = connectionString.substr(prefixWithDeviceStr.size(), std::string::npos);
    size_t id;
    try
    {
        id = std::stoi(idStr);
    }
    catch (const std::invalid_argument&)
    {
        LOG_W("Invalid connection string \"{}\", no id", connectionString);
        DAQ_THROW_EXCEPTION(InvalidParameterException);
    }

    return id;
}

void RefDeviceModule::clearRemovedDevices()
{
    for (auto& device : devices)
    {
        const bool isNull = !device.assigned() || !device.getRef().assigned();
        if (isNull || device.getRef().isRemoved())
            device = nullptr;
    }
}

END_NAMESPACE_REF_DEVICE_MODULE
