#include <coretypes/version_info_factory.h>
#include <coretypes/string_ptr.h>
#include <opendaq/custom_log.h>
#include <ref_device_module/ref_device_impl.h>
#include <ref_device_module/ref_device_module_impl.h>
#include <ref_device_module/version.h>

BEGIN_NAMESPACE_REF_DEVICE_MODULE

static constexpr size_t DEFAULT_MAX_REFERENCE_DEVICE_COUNT = 2;
static constexpr char DEVICE_TYPE_ID[] = "daqref";
static constexpr char CONNECTION_STRING_PREFIX[] = "daqref";

RefDeviceModuleBase::RefDeviceModuleBase(const ContextPtr& context)
    : ModuleTemplateHooks(std::make_shared<RefDeviceModule>(context))
{
}

RefDeviceModule::RefDeviceModule(const ContextPtr& context)
    : ModuleTemplate(context)
    , maxNumberOfDevices(DEFAULT_MAX_REFERENCE_DEVICE_COUNT)
{
}

templates::ModuleParams RefDeviceModule::buildModuleParams()
{
    templates::ModuleParams params;
    params.version = VersionInfo(REF_DEVICE_MODULE_MAJOR_VERSION, REF_DEVICE_MODULE_MINOR_VERSION, REF_DEVICE_MODULE_PATCH_VERSION);
    params.name = "ReferenceDeviceModule";
    params.id = REF_MODULE_ID;
    params.logName = "ReferenceDeviceModule";
    return params;
}

std::vector<templates::DeviceTypeParams> RefDeviceModule::getAvailableDeviceTypes(const DictPtr<IString, IBaseObject>& options)
{
    templates::DeviceTypeParams params;
    params.id = DEVICE_TYPE_ID;
    params.name = "Reference device";
    params.description = "Reference device";
    params.connectionStringPrefix = CONNECTION_STRING_PREFIX;
    params.defaultConfiguration = createDefaultConfig();
    return {params};
}

std::vector<templates::DeviceInfoParams> RefDeviceModule::getAvailableDeviceInfo(const DictPtr<IString, IBaseObject>& options)
{
    StringPtr customName;
    StringPtr customSerial;

    if (options.hasKey("MaxNumberOfDevices"))
        maxNumberOfDevices = options.get("MaxNumberOfDevices");

    if (options.hasKey("SerialNumber"))
    {
        customSerial = options.get("SerialNumber");
        maxNumberOfDevices = 1;

        if (maxNumberOfDevices > 1)
        {
            LOG_W("Max number of reference devices cannot be greater than 1 if a custom serial number is provided through module options.")
        }
    }

    if (options.hasKey("Name"))
        customName = options.get("Name");

    std::vector<templates::DeviceInfoParams> params;
    for (size_t i = 0; i < maxNumberOfDevices; ++i)
    {
        templates::DeviceInfoParams info;

        info.address = fmt::format("device{}", i);
        info.typeId = std::string(DEVICE_TYPE_ID);
        info.name = customName.assigned() ? customName.toStdString() : fmt::format("Reference device {}", i);
        info.manufacturer = {"openDAQ"};
        info.manufacturerUri = {"https://www.opendaq.com/"};
        info.model = {"Reference device"};
        info.productCode = {"REF_DEV"};
        info.deviceRevision = {"1.0"};
        info.hardwareRevision = {"1.0"};
        info.softwareRevision = {"1.0"};
        info.serialNumber = customSerial.assigned() ? customSerial.toStdString() : fmt::format("RefDev{}", i);

        params.push_back(info);
    }

    return params;
}

DevicePtr RefDeviceModule::createDevice(const templates::DeviceParams& params)
{
    return createWithImplementation<IDevice, RefDeviceBase>(params);
}

PropertyObjectPtr RefDeviceModule::createDefaultConfig()
{
    const auto defaultConfig = PropertyObject();

    defaultConfig.addProperty(IntProperty("NumberOfChannels", 2));
    defaultConfig.addProperty(BoolProperty("EnableCANChannel", False));
    defaultConfig.addProperty(BoolProperty("EnableProtectedChannel", False));
    defaultConfig.addProperty(StringProperty("SerialNumber", ""));
    defaultConfig.addProperty(BoolProperty("EnableLogging", False));
    defaultConfig.addProperty(StringProperty("LoggingPath", "ref_device_simulator.log"));
    defaultConfig.addProperty(StringProperty("Name", ""));

    return defaultConfig;
}

END_NAMESPACE_REF_DEVICE_MODULE
