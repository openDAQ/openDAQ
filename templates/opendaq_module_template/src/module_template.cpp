#include <opendaq_module_template/module_template.h>
#include <coreobjects/property_factory.h>
#include <opendaq/device_info_factory.h>
#include <opendaq/device_type_factory.h>

BEGIN_NAMESPACE_OPENDAQ

ModuleTemplate::ModuleTemplate(const ModuleTemplateParams& params)
    : ModuleTemplateParamsValidation(params)
    , Module(params.name, params.version, params.context, params.id)
{
    loggerComponent = this->context.getLogger().getOrAddComponent(params.logName);
}

ListPtr<IDeviceInfo> ModuleTemplate::onGetAvailableDevices()
{
    auto deviceInfo = List<IDeviceInfo>();
    const auto options = context.getModuleOptions(id);

    const auto types = getDeviceTypes();
    for (const auto& type : types)
    {
        const auto id = type.getId();
        for (const auto& fields: getDeviceInfoFields(id, options))
            deviceInfo.pushBack(createDeviceInfo(fields, type));
    }

    return deviceInfo.detach();
}

DictPtr<IString, IDeviceType> ModuleTemplate::onGetAvailableDeviceTypes()
{
    const ListPtr<IDeviceType> deviceTypes = getDeviceTypes();

    DictPtr<IString, IDeviceType> typesDict = Dict<IString, IDeviceType>();
    for (const auto& deviceType : deviceTypes)
        typesDict.set(String(deviceType.getId()), deviceType);

    return typesDict.detach();
}

DevicePtr ModuleTemplate::onCreateDevice(const StringPtr& connectionString, const ComponentPtr& parent, const PropertyObjectPtr& config)
{
    if (!connectionString.assigned() || connectionString == "")
        throw InvalidParameterException("Connection string must not be empty");

    std::scoped_lock lock(sync);
    std::string s = connectionString;
    const std::string delimiter = "://";
    const std::string prefix = s.substr(0, s.find(delimiter));
    const std::string address = s.substr(s.find(delimiter) + delimiter.length(), s.length());

    DeviceTypePtr type;
    DeviceInfoPtr info;

    for (const auto& deviceType : getDeviceTypes())
    {
        if (deviceType.getConnectionStringPrefix() == prefix)
        {
            type = deviceType;
            break;
        }
    }

    if (!type.assigned())
        throw InvalidParameterException("Device with given connection string prefix was not found");

    for (const auto& infoFields : getDeviceInfoFields(type.getId(), context.getModuleOptions(id)))
    {
        if (infoFields.address == address)
        {
            info = createDeviceInfo(infoFields, type);
            break;
        }
    }
    
    if (!info.assigned())
        throw NotFoundException("Device with given connection string was not found.");

    const auto options = context.getModuleOptions(id);

    GetDeviceParams params;
    params.typeId = type.getId().toStdString();
    params.address = address;
    params.info = info;
    params.parent = parent;
    params.config = config;
    params.options = options.assigned() ? options : Dict<IString, IBaseObject>();

    return getDevice(params);
}

DeviceInfoPtr ModuleTemplate::createDeviceInfo(const DeviceInfoFields& fields, const DeviceTypePtr& type)
{
    if (fields.serialNumber.empty())
        throw ArgumentNullException("Serial number must not be empty");
    if (fields.manufacturer.empty())
        throw ArgumentNullException("Manufacturer must not be empty");
    if (type.getConnectionStringPrefix() == "")
        throw ArgumentNullException("Connection string prefix must not be empty");
    if (type.getId() == "")
        throw ArgumentNullException("Device type ID must not be empty");

    auto deviceInfo = DeviceInfo(type.getConnectionStringPrefix() + "://" + fields.address);
    deviceInfo.setDeviceType(type);
    deviceInfo.setName(fields.name);
    deviceInfo.setManufacturer(fields.manufacturer);
    deviceInfo.setManufacturerUri(fields.manufacturerUri);
    deviceInfo.setModel(fields.model);
    deviceInfo.setProductCode(fields.productCode);
    deviceInfo.setDeviceRevision(fields.deviceRevision);
    deviceInfo.setHardwareRevision(fields.hardwareRevision);
    deviceInfo.setSoftwareRevision(fields.softwareRevision);
    deviceInfo.setDeviceManual(fields.deviceManual);
    deviceInfo.setDeviceClass(fields.deviceClass);
    deviceInfo.setSerialNumber(fields.serialNumber);
    deviceInfo.setProductInstanceUri(fields.productInstanceUri);
    deviceInfo.setRevisionCounter(fields.revisionCounter);
    deviceInfo.setAssetId(fields.assetId);
    deviceInfo.setMacAddress(fields.macAddress);
    deviceInfo.setParentMacAddress(fields.parentMacAddress);
    deviceInfo.setPlatform(fields.platform);
    deviceInfo.setPosition(fields.position);
    deviceInfo.setSystemType(fields.systemType);
    deviceInfo.setSystemUuid(fields.systemUuid);
    deviceInfo.setLocation(fields.location);

    for (const auto& [key, value] : fields.other)
        deviceInfo.addProperty(StringProperty(key, value));

    return deviceInfo.detach();
}

std::vector<DeviceInfoFields> ModuleTemplate::getDeviceInfoFields(const std::string& /*typeId*/, const DictPtr<IString, IBaseObject>& /*options*/)
{
    return {};
}

std::vector<DeviceTypePtr> ModuleTemplate::getDeviceTypes()
{
    return {};
}

DevicePtr ModuleTemplate::getDevice(const GetDeviceParams& /*params*/)
{
    return nullptr;
}

void ModuleTemplate::deviceRemoved(const std::string& /*deviceLocalId*/)
{
}

END_NAMESPACE_OPENDAQ
