#include <coretypes/version_info_factory.h>
#include <coretypes/string_ptr.h>
#include <opendaq/custom_log.h>
#include <ref_device_module/ref_device_impl.h>
#include <ref_device_module/ref_device_module_impl.h>
#include <ref_device_module/version.h>
#include <opendaq/device_type_factory.h>

BEGIN_NAMESPACE_REF_DEVICE_MODULE

static constexpr size_t DEFAULT_MAX_REFERENCE_DEVICE_COUNT = 2;
static constexpr char DEVICE_TYPE_ID[] = "daqref";
static constexpr char CONNECTION_STRING_PREFIX[] = "daqref";

RefDeviceModule::RefDeviceModule(const ContextPtr& context)
    : ModuleTemplate("ReferenceDeviceModule",
                     VersionInfo(REF_DEVICE_MODULE_MAJOR_VERSION, REF_DEVICE_MODULE_MINOR_VERSION, REF_DEVICE_MODULE_PATCH_VERSION),
                     context,
                     REF_MODULE_NAME)
      , maxNumberOfDevices(DEFAULT_MAX_REFERENCE_DEVICE_COUNT)
{
    const auto options = this->context.getModuleOptions(REF_MODULE_NAME);
    if (options.assigned() && options.hasKey("MaxNumberOfDevices"))
        maxNumberOfDevices = options.get("MaxNumberOfDevices");
}

std::vector<DeviceInfoFields> RefDeviceModule::getDeviceInfoFields(const std::string& typeId, const DictPtr<IString, IBaseObject>& options)
{
    StringPtr customName;
    StringPtr customSerial;

    if (options.hasKey("SerialNumber"))
    {
        customSerial = options.get("SerialNumber");
        if (maxNumberOfDevices > 1)
            LOG_W("Max number of reference devices cannot be greater than 1 if a custom serial number is provided through module options.")
        maxNumberOfDevices = 1;
    }

    if (options.hasKey("Name"))
        customName = options.get("Name");

    std::vector<DeviceInfoFields> fields;
    if (typeId == DEVICE_TYPE_ID)
    {
        for (size_t i = 0; i < maxNumberOfDevices; ++i)
        {
            DeviceInfoFields info;
            info.connectionAddress = fmt::format("device{}", i);
            info.name = customName.assigned() ? customName.toStdString() : fmt::format("Reference device {}", i);
            info.manufacturer = "openDAQ";
            info.manufacturerUri = "https://www.opendaq.com/";
            info.model = "Reference device";
            info.productCode = "REF_DEV";
            info.deviceRevision = "1.0";
            info.hardwareRevision = "1.0";
            info.softwareRevision = "1.0";
            info.serialNumber = customSerial.assigned() ? customSerial.toStdString() : fmt::format("ref_dev_{}", i);
            fields.push_back(info);
        }
    }

    return fields;
}

std::vector<DeviceTypePtr> RefDeviceModule::getDeviceTypes()
{
    return {DeviceTypeBuilder()
                .setId(DEVICE_TYPE_ID)
                .setDescription("Reference device")
                .setName("Reference device")
                .setConnectionStringPrefix(CONNECTION_STRING_PREFIX)
                .build()};
}

DevicePtr RefDeviceModule::getDevice(const std::string& typeId,
                                     const std::string& connectionAddress,
                                     const DeviceInfoPtr& info,
                                     const FolderPtr& parent,
                                     const PropertyObjectPtr& config,
                                     const DictPtr<IString, IBaseObject>& options)
{
    if (devices.count(connectionAddress) && devices[connectionAddress].assigned() && devices[connectionAddress].getRef().assigned())
    {
        LOG_W("Device with id \"{}\" already exist", id)
        throw AlreadyExistsException();
    }

    StringPtr localId;
    if (options.hasKey("LocalId"))
    {
        localId = options.get("LocalId");
        if (maxNumberOfDevices > 1)
            LOG_W("Max number of reference devices cannot be greater than 1 if a custom Local ID is provided through module options.")
        maxNumberOfDevices = 1;
    }
    else
        localId = fmt::format("ref_dev{}", getIdFromAddress(connectionAddress));

    const auto devicePtr = createWithImplementation<IDevice, RefDeviceImpl>(localId, info, config, context, parent);
    devices[connectionAddress] = devicePtr;
    return devicePtr;
}

size_t RefDeviceModule::getIdFromAddress(const std::string& address)
{
    const std::string prefixWithDeviceStr = "device";
    const auto idStr = address.substr(prefixWithDeviceStr.size(), std::string::npos);
    return std::stoi(idStr);
}

END_NAMESPACE_REF_DEVICE_MODULE
