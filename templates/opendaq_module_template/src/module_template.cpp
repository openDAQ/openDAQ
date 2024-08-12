#include <opendaq_module_template/module_template.h>
#include <coreobjects/property_factory.h>
#include <opendaq/device_info_factory.h>
#include <opendaq/device_type_factory.h>

BEGIN_NAMESPACE_OPENDAQ

ListPtr<IDeviceInfo> ModuleTemplateHooks::onGetAvailableDevices()
{
    std::scoped_lock lock(module_->sync);
    auto deviceInfo = List<IDeviceInfo>();
    const auto options = context.getModuleOptions(id);

    const auto availableDevTypes = module_->getAvailableDeviceTypes(options);
    std::map<std::string, DeviceTypeParams> devTypesMap;
    for (const auto& devTypeInfo : availableDevTypes)
        devTypesMap[devTypeInfo.id] = devTypeInfo;
    
    const auto availableDevInfo = module_->getAvailableDeviceInfo(options);
    for (const auto& devInfoParams : availableDevInfo)
    {
        if (devTypesMap.find(devInfoParams.typeId) == devTypesMap.end())
            deviceInfo.pushBack(createDeviceInfo(devInfoParams, {}));
        deviceInfo.pushBack(createDeviceInfo(devInfoParams, devTypesMap.at(devInfoParams.typeId)));
    }

    return deviceInfo.detach();
}

DictPtr<IString, IDeviceType> ModuleTemplateHooks::onGetAvailableDeviceTypes()
{
    std::scoped_lock lock(module_->sync);
    const auto options = context.getModuleOptions(id);
    const auto deviceTypes = module_->getAvailableDeviceTypes(options);

    auto typesDict = Dict<IString, IDeviceType>();
    for (const auto& typeInfo : deviceTypes)
    {
        auto type = DeviceTypeBuilder().setId(typeInfo.id)
                                       .setConnectionStringPrefix(typeInfo.connectionStringPrefix)
                                       .setDescription(typeInfo.description)
                                       .setDefaultConfig(typeInfo.defaultConfiguration)
                                       .build();
        typesDict.set(typeInfo.id, type);
    }

    return typesDict.detach();
}

DevicePtr ModuleTemplateHooks::onCreateDevice(const StringPtr& connectionString, const ComponentPtr& parent, const PropertyObjectPtr& config)
{
    if (!connectionString.assigned() || connectionString == "")
        throw InvalidParameterException("Connection string must not be empty");

    std::scoped_lock lock(module_->sync);
    std::string s = connectionString;
    const std::string delimiter = "://";
    const std::string prefix = s.substr(0, s.find(delimiter));
    const std::string address = s.substr(s.find(delimiter) + delimiter.length(), s.length());

    DeviceTypeParams typeInfo;
    const auto options = context.getModuleOptions(id);

    bool found = false;
    for (const auto& availableDeviceType : module_->getAvailableDeviceTypes(options))
    {
        if (availableDeviceType.connectionStringPrefix == prefix)
        {
            typeInfo = availableDeviceType;
            found = true;
            break;
        }
    }

    if (!found)
        throw InvalidParameterException("Device with given connection string prefix was not found");

    DeviceInfoParams deviceInfo;
    
    found = false;
    for (const auto& availableDeviceInfo : module_->getAvailableDeviceInfo(options))
    {
        if (availableDeviceInfo.address == address)
        {
            deviceInfo = availableDeviceInfo;
            found = true;
            break;
        }
    }
    
    if (!found)
        throw InvalidParameterException("Device with address {} was not found", address);

    DeviceInfoPtr info = createDeviceInfo(deviceInfo, typeInfo);

    DeviceParams params;
    params.info = info;
    params.parent = parent;
    params.context = context;

    params.typeId = typeInfo.id;
    params.address = address;

    params.config = config.assigned() ? config : typeInfo.defaultConfiguration;
    params.config = params.config.assigned() ? params.config : PropertyObject();
    params.options = options.assigned() ? options : Dict<IString, IBaseObject>();

    params.logName = typeInfo.name;
    params.localId = deviceInfo.manufacturer + "_" + deviceInfo.serialNumber;

    if (module_->devices.count(params.localId))
        throw AlreadyExistsException{"Device with local ID \"{}\" already exist", params.localId};

    auto device = module_->createDevice(params);
    if (!device.assigned())
        throw InvalidParameterException("Device creation failed");
    module_->devices.insert(device.getLocalId());

    if (parent.assigned())
    {
        parent.getOnComponentCoreEvent() +=
            [this](const ComponentPtr& /*comp*/, const CoreEventArgsPtr& args)
            {
                if (args.getEventId() == static_cast<Int>(CoreEventId::ComponentRemoved))
                {
                    const auto id = args.getParameters().get("Id");
                    if (module_->devices.count(id))
                    {
                        module_->deviceRemoved(id);
                        module_->devices.erase(id);
                    }
                }
            };
    }

    return device.detach();
}

DeviceInfoPtr ModuleTemplateHooks::createDeviceInfo(const DeviceInfoParams& infoParams, const DeviceTypeParams& typeParams)
{
    if (infoParams.serialNumber.empty())
        throw ArgumentNullException("Serial number must not be empty");
    if (infoParams.manufacturer.empty())
        throw ArgumentNullException("Manufacturer must not be empty");
    
    auto deviceInfo = DeviceInfo(typeParams.connectionStringPrefix + "://" + infoParams.address);

    if (!typeParams.id.empty() && !typeParams.connectionStringPrefix.empty())
    {
        auto deviceType = DeviceTypeBuilder().setId(typeParams.id)
                                             .setConnectionStringPrefix(typeParams.connectionStringPrefix)
                                             .setDescription(typeParams.description)
                                             .setDefaultConfig(typeParams.defaultConfiguration)
                                             .build();

        deviceInfo.setDeviceType(deviceType.detach());
    }

    deviceInfo.setName(infoParams.name);
    deviceInfo.setManufacturer(infoParams.manufacturer);
    deviceInfo.setManufacturerUri(infoParams.manufacturerUri);
    deviceInfo.setModel(infoParams.model);
    deviceInfo.setProductCode(infoParams.productCode);
    deviceInfo.setDeviceRevision(infoParams.deviceRevision);
    deviceInfo.setHardwareRevision(infoParams.hardwareRevision);
    deviceInfo.setSoftwareRevision(infoParams.softwareRevision);
    deviceInfo.setDeviceManual(infoParams.deviceManual);
    deviceInfo.setDeviceClass(infoParams.deviceClass);
    deviceInfo.setSerialNumber(infoParams.serialNumber);
    deviceInfo.setProductInstanceUri(infoParams.productInstanceUri);
    deviceInfo.setRevisionCounter(infoParams.revisionCounter);
    deviceInfo.setAssetId(infoParams.assetId);
    deviceInfo.setMacAddress(infoParams.macAddress);
    deviceInfo.setParentMacAddress(infoParams.parentMacAddress);
    deviceInfo.setPlatform(infoParams.platform);
    deviceInfo.setPosition(infoParams.position);
    deviceInfo.setSystemType(infoParams.systemType);
    deviceInfo.setSystemUuid(infoParams.systemUuid);
    deviceInfo.setLocation(infoParams.location);

    for (const auto& [key, value] : infoParams.other)
        deviceInfo.addProperty(StringProperty(key, value));

    return deviceInfo.detach();
}

std::vector<DeviceTypeParams> ModuleTemplate::getAvailableDeviceTypes(const DictPtr<IString, IBaseObject>& /*options*/)
{
    return {};
}

std::vector<DeviceInfoParams> ModuleTemplate::getAvailableDeviceInfo(const DictPtr<IString, IBaseObject>& /*options*/)
{
	return {};
}

DevicePtr ModuleTemplate::createDevice(const DeviceParams& /*params*/)
{
    return nullptr;
}

void ModuleTemplate::deviceRemoved(const std::string& /*deviceLocalId*/)
{
}

ModuleParams ModuleTemplate::buildModuleParams()
{
	return {};
}

END_NAMESPACE_OPENDAQ
