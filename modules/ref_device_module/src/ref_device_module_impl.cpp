#include <coretypes/version_info_factory.h>
#include <opendaq/custom_log.h>
#include <ref_device_module/ref_device_impl.h>
#include <ref_device_module/ref_device_module_impl.h>
#include <ref_device_module/version.h>

BEGIN_NAMESPACE_REF_DEVICE_MODULE

RefDeviceModule::RefDeviceModule(ContextPtr context)
    : Module("Reference device module",
             daq::VersionInfo(REF_DEVICE_MODULE_MAJOR_VERSION, REF_DEVICE_MODULE_MINOR_VERSION, REF_DEVICE_MODULE_PATCH_VERSION),
             std::move(context),
             "ReferenceDevice")
    , maxNumberOfDevices(2)
{
    auto options = this->context.getModuleOptions("RefDevice");
    if (options.hasKey("MaxNumberOfDevices"))
        maxNumberOfDevices = options.get("MaxNumberOfDevices");
    devices.resize(maxNumberOfDevices);
}

ListPtr<IDeviceInfo> RefDeviceModule::onGetAvailableDevices()
{
    auto availableDevices = List<IDeviceInfo>();
    for (size_t i = 0; i < 2; i++)
    {
        auto info = RefDeviceImpl::CreateDeviceInfo(i);
        availableDevices.pushBack(info);
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
        throw NotFoundException();
    }

    if (devices[id].assigned() && devices[id].getRef().assigned())
    {
        LOG_W("Device with id \"{}\" already exist", id);
        throw AlreadyExistsException();
    }

    const auto options = context.getOptions();
    StringPtr localId;
    StringPtr name = fmt::format("Device {}", id);

    if (options.assigned() && options.hasKey("ReferenceDevice"))
    {
        const DictPtr<StringPtr, BaseObjectPtr> referenceDevice = options.get("ReferenceDevice");
        if (referenceDevice.hasKey("LocalId"))
            localId = referenceDevice.get("LocalId");
        if (referenceDevice.hasKey("Name"))
            name = referenceDevice.get("Name");
    }

    if (!localId.assigned())
        localId = fmt::format("ref_dev{}", id);

    auto devicePtr = createWithImplementation<IDevice, RefDeviceImpl>(id, config, context, parent, localId, name);
    devices[id] = devicePtr;
    return devicePtr;
}

bool RefDeviceModule::onAcceptsConnectionParameters(const StringPtr& connectionString, const PropertyObjectPtr& /*config*/)
{
    LOG_T("Connection string: {}", connectionString);
    std::string connStr = connectionString;
    auto found = connStr.find("daqref://");
    return (found == 0);
}

size_t RefDeviceModule::getIdFromConnectionString(const std::string& connectionString) const
{
    std::string prefixWithDeviceStr = "daqref://device";
    auto found = connectionString.find(prefixWithDeviceStr);
    if (found != 0)
    {
        LOG_W("Invalid connection string \"{}\", no prefix", connectionString);
        throw InvalidParameterException();
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
        throw InvalidParameterException();
    }

    return id;
}

END_NAMESPACE_REF_DEVICE_MODULE
