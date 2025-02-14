#include <coretypes/version_info_factory.h>
#include <coretypes/string_ptr.h>
#include <ref_template_device_module/ref_template_device_impl.h>
#include <ref_template_device_module/ref_template_device_module_impl.h>
#include <ref_template_device_module/version.h>

BEGIN_NAMESPACE_REF_TEMPLATE_DEVICE_MODULE

static constexpr size_t DEFAULT_MAX_REFERENCE_DEVICE_COUNT = 2;
static constexpr char DEVICE_TYPE_ID[] = "TemplateReferenceDevice";
static constexpr char CONNECTION_STRING_PREFIX[] = "daq.template";

RefTemplateDeviceModuleBase::RefTemplateDeviceModuleBase(const ContextPtr& context)
    : ModuleTemplateHooks(std::make_shared<RefTemplateDeviceModule>(context))
{
}

RefTemplateDeviceModule::RefTemplateDeviceModule(const ContextPtr& context)
    : ModuleTemplate(context)
    , maxNumberOfDevices(DEFAULT_MAX_REFERENCE_DEVICE_COUNT)
{
}

templates::ModuleParams RefTemplateDeviceModule::buildModuleParams()
{
    templates::ModuleParams params;
    params.version = VersionInfo(REF_TEMPLATE_DEVICE_MODULE_MAJOR_VERSION, REF_TEMPLATE_DEVICE_MODULE_MINOR_VERSION, REF_TEMPLATE_DEVICE_MODULE_PATCH_VERSION);
    params.name = "TemplateReferenceDeviceModule";
    params.id = REF_TEMPLATE_MODULE_ID;
    params.logName = "TemplateReferenceDeviceModule";
    params.defaultOptions = createDefaultModuleOptions();
    return params;
}

std::vector<templates::DeviceTypeParams> RefTemplateDeviceModule::getAvailableDeviceTypes(const DictPtr<IString, IBaseObject>& options)
{
    templates::DeviceTypeParams params;
    params.id = DEVICE_TYPE_ID;
    params.name = "Template reference device";
    params.description = "Template reference device";
    params.connectionStringPrefix = CONNECTION_STRING_PREFIX;
    params.defaultConfiguration = createDefaultConfig();
    return {params};
}

std::vector<templates::DeviceInfoParams> RefTemplateDeviceModule::getAvailableDeviceInfo(const DictPtr<IString, IBaseObject>& options)
{
    const StringPtr customName = options.get("Name");
    const StringPtr customSerial = options.get("SerialNumber");
    maxNumberOfDevices = customSerial.getLength() ? static_cast<size_t>(1) : static_cast<size_t>(options.get("MaxNumberOfDevices"));

    std::vector<templates::DeviceInfoParams> params;
    for (size_t i = 0; i < maxNumberOfDevices; ++i)
    {
        templates::DeviceInfoParams info;

        info.address = fmt::format("device{}", i);
        info.typeId = std::string(DEVICE_TYPE_ID);
        info.name = customName.getLength() ? customName.toStdString() : fmt::format("Template reference device {}", i);
        info.manufacturer = {"openDAQ"};
        info.manufacturerUri = {"https://www.opendaq.com/"};
        info.model = {"Template reference device"};
        info.productCode = {"REF_TEMPLATE_DEV"};
        info.deviceRevision = {"1.0"};
        info.hardwareRevision = {"1.0"};
        info.softwareRevision = {"1.0"};
        info.serialNumber = customSerial.getLength() ? customSerial.toStdString() : fmt::format("TempRefDev{}", i);

        params.push_back(info);
    }

    return params;
}

DevicePtr RefTemplateDeviceModule::createDevice(const templates::DeviceParams& params)
{
    return createWithImplementation<IDevice, RefDeviceBase>(params);
}

PropertyObjectPtr RefTemplateDeviceModule::createDefaultConfig()
{
    const auto defaultConfig = PropertyObject();

    defaultConfig.addProperty(IntProperty("NumberOfChannels", 2));
    defaultConfig.addProperty(BoolProperty("EnableCANChannel", False));
    defaultConfig.addProperty(BoolProperty("EnableProtectedChannel", False));
    defaultConfig.addProperty(BoolProperty("EnableLogging", False));
    defaultConfig.addProperty(StringProperty("LoggingPath", "ref_template_device_simulator.log"));
    defaultConfig.addProperty(StringProperty("Name", ""));

    return defaultConfig;
}

DictPtr<IString, IBaseObject> RefTemplateDeviceModule::createDefaultModuleOptions()
{
    DictPtr<IString, IBaseObject> options = Dict<IString, IBaseObject>();
    options.set("Name", "");
    options.set("SerialNumber", "");
    options.set("MaxNumberOfDevices", DEFAULT_MAX_REFERENCE_DEVICE_COUNT);
    return options.detach();
}

END_NAMESPACE_REF_TEMPLATE_DEVICE_MODULE
