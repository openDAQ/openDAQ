#include <opendaq_module_template/module_template.h>
#include <coreobjects/property_factory.h>
#include <opendaq/device_info_factory.h>
#include <opendaq/device_type_factory.h>

BEGIN_NAMESPACE_OPENDAQ_TEMPLATES

ListPtr<IDeviceInfo> ModuleTemplateHooks::onGetAvailableDevices()
{
    std::scoped_lock lock(module_->sync);
    auto deviceInfo = List<IDeviceInfo>();
    const auto options = context.getModuleOptions(moduleInfo.getId());

    const auto availableDevTypes = module_->getAvailableDeviceTypes(options);
    std::map<std::string, DeviceTypeParams> devTypesMap;
    for (const auto& devTypeInfo : availableDevTypes)
        devTypesMap[devTypeInfo.id] = devTypeInfo;
    
    const auto availableDevInfo = module_->getAvailableDeviceInfo(options);
    for (const auto& devInfoParams : availableDevInfo)
    {
        if (devTypesMap.find(devInfoParams.typeId.value) == devTypesMap.end())
            deviceInfo.pushBack(createDeviceInfo(devInfoParams, {}));
        deviceInfo.pushBack(createDeviceInfo(devInfoParams, devTypesMap.at(devInfoParams.typeId.value)));
    }

    return deviceInfo.detach();
}

DictPtr<IString, IDeviceType> ModuleTemplateHooks::onGetAvailableDeviceTypes()
{
    std::scoped_lock lock(module_->sync);
    const auto options = context.getModuleOptions(moduleInfo.getId());
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
        if (availableDeviceInfo.address.value == address)
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

    params.logName = typeInfo.name;
    params.localId = deviceInfo.manufacturer.value + "_" + deviceInfo.serialNumber.value;

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
    if (infoParams.serialNumber.value.empty())
        throw ArgumentNullException("Serial number must not be empty");
    if (infoParams.manufacturer.value.empty())
        throw ArgumentNullException("Manufacturer must not be empty");
    
    auto deviceInfo = DeviceInfo(typeParams.connectionStringPrefix + "://" + infoParams.address.value);

    if (!typeParams.id.empty() && !typeParams.connectionStringPrefix.empty())
    {
        auto deviceType = DeviceTypeBuilder().setId(typeParams.id)
                                             .setConnectionStringPrefix(typeParams.connectionStringPrefix)
                                             .setDescription(typeParams.description)
                                             .setDefaultConfig(typeParams.defaultConfiguration)
                                             .build();

        deviceInfo.setDeviceType(deviceType.detach());
    }

    deviceInfo.setName(infoParams.name.value);
    deviceInfo.setManufacturer(infoParams.manufacturer.value);
    deviceInfo.setManufacturerUri(infoParams.manufacturerUri.value);
    deviceInfo.setModel(infoParams.model.value);
    deviceInfo.setProductCode(infoParams.productCode.value);
    deviceInfo.setDeviceRevision(infoParams.deviceRevision.value);
    deviceInfo.setHardwareRevision(infoParams.hardwareRevision.value);
    deviceInfo.setSoftwareRevision(infoParams.softwareRevision.value);
    deviceInfo.setDeviceManual(infoParams.deviceManual.value);
    deviceInfo.setDeviceClass(infoParams.deviceClass.value);
    deviceInfo.setSerialNumber(infoParams.serialNumber.value);
    deviceInfo.setProductInstanceUri(infoParams.productInstanceUri.value);
    deviceInfo.setRevisionCounter(infoParams.revisionCounter.value);
    deviceInfo.setAssetId(infoParams.assetId.value);
    deviceInfo.setMacAddress(infoParams.macAddress.value);
    deviceInfo.setParentMacAddress(infoParams.parentMacAddress.value);
    deviceInfo.setPlatform(infoParams.platform.value);
    deviceInfo.setPosition(infoParams.position.value);
    deviceInfo.setSystemType(infoParams.systemType.value);
    deviceInfo.setSystemUuid(infoParams.systemUuid.value);
    deviceInfo.setLocation(infoParams.location.value);

    for (const auto& [key, value] : infoParams.other)
        deviceInfo.addProperty(StringProperty(key, value.value));

    return deviceInfo.detach();
}

void ModuleTemplateHooks::populateModuleOptions(const DictPtr<IString, IBaseObject>& userOptions, const DictPtr<IString, IBaseObject>& defaultOptions)
{
    for (const auto& [key, value] : defaultOptions)
    {
        if (userOptions.hasKey(key))
        {
            auto userOption = userOptions.get(key);

            if (value.getCoreType() != userOption.getCoreType())
                continue;

            if (userOption.getCoreType() == ctDict)
                populateModuleOptions(value, userOption);
            else
                checkErrorInfo(defaultOptions->set(key, userOptions.get(key)));
        }
        else
        {
            checkErrorInfo(defaultOptions->set(key, value));
        }
    }
}

DictPtr<IString, IBaseObject> ModuleTemplateHooks::mergeModuleOptions(const PropertyObjectPtr& userOptions,
                                                                      const PropertyObjectPtr& defaultOptions) const
{
    try
    {
        if (!userOptions.assigned())
            return defaultOptions;

        BaseObjectPtr outOptions;
        defaultOptions.asPtr<ICloneable>()->clone(&outOptions);

        populateModuleOptions(userOptions, outOptions);
        return outOptions.detach();
    }
    catch (const DaqException& e)
    {
        LOG_W("Failed to merge module options: {}", e.what())
        return userOptions;
    }
    catch (const std::exception& e)
    {
        LOG_W("Failed to merge module options: {}", e.what())
        return userOptions;
    }
}

void ModuleTemplateHooks::populateDefaultConfig(const PropertyObjectPtr& userConfig, const PropertyObjectPtr& defaultConfig)
{

    for (const auto& defaultProp : defaultConfig.getAllProperties())
    {
        const auto propName = defaultProp.getName();

        if (userConfig.hasProperty(propName))
        {
            const auto userProp = userConfig.getProperty(propName);

            if (userProp.getValueType() != defaultProp.getValueType())
                continue;

            if (userProp.getValueType() == ctObject)
                populateDefaultConfig(defaultProp.getValue(), userProp.getValue());
            else
                defaultConfig.setPropertyValue(propName, userProp.getValue());
        }
    }
}

PropertyObjectPtr ModuleTemplateHooks::mergeConfig(const PropertyObjectPtr& userConfig, const PropertyObjectPtr& defaultConfig) const
{
    try
    {
        if (!userConfig.assigned())
            return defaultConfig;

        PropertyObjectPtr outObj = defaultConfig.asPtr<IPropertyObjectInternal>().clone();
        populateDefaultConfig(userConfig, outObj);
        return outObj;
    }
    catch (const DaqException& e)
    {
        LOG_W("Failed to merge configuration: {}", e.what())
        return userConfig;
    }
    catch (const std::exception& e)
    {
        LOG_W("Failed to merge configuration: {}", e.what())
        return userConfig;
    }
}

std::vector<DeviceTypeParams> ModuleTemplate::getAvailableDeviceTypes(const DictPtr<IString, IBaseObject>& /*options*/)
{
    return {};
}

std::vector<DeviceInfoParams> ModuleTemplate::getAvailableDeviceInfo(const DictPtr<IString, IBaseObject>& /*options*/)
{
	return {};
}

PropertyObjectPtr ModuleTemplate::createDefaultConfiguration(const StringPtr& /*typeId*/)
{
	return PropertyObject();
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

END_NAMESPACE_OPENDAQ_TEMPLATES
