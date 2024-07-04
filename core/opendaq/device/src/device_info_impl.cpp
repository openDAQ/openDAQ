#include <opendaq/device_info_impl.h>
#include <opendaq/component_ptr.h>
#include <coretypes/validation.h>
#include "coretypes/impl.h"
#include <coreobjects/property_object_factory.h>
#include <opendaq/device_info_factory.h>

BEGIN_NAMESPACE_OPENDAQ

template <typename TInterface, typename... Interfaces>
DeviceInfoConfigImpl<TInterface, Interfaces...>::DeviceInfoConfigImpl(const StringPtr& name, const StringPtr& connectionString, const StringPtr& customSdkVersion)
    : Super()
{
    createAndSetDefaultStringProperty("Name", "");
    createAndSetDefaultStringProperty("Manufacturer", "");
    createAndSetDefaultStringProperty("ManufacturerUri", "");
    createAndSetDefaultStringProperty("Model", "");
    createAndSetDefaultStringProperty("ProductCode", "");
    createAndSetDefaultStringProperty("DeviceRevision", "");
    createAndSetDefaultStringProperty("HardwareRevision", "");
    createAndSetDefaultStringProperty("SoftwareRevision", "");
    createAndSetDefaultStringProperty("DeviceManual", "");
    createAndSetDefaultStringProperty("DeviceClass", "");
    createAndSetDefaultStringProperty("SerialNumber", "");
    createAndSetDefaultStringProperty("ProductInstanceUri", "");
    createAndSetDefaultIntProperty("RevisionCounter", 0);
    createAndSetDefaultStringProperty("AssetId", "");
    createAndSetDefaultStringProperty("MacAddress", "");
    createAndSetDefaultStringProperty("ParentMacAddress", "");
    createAndSetDefaultStringProperty("Platform", "");
    createAndSetDefaultIntProperty("Position", 0);
    createAndSetDefaultStringProperty("SystemType", "");
    createAndSetDefaultStringProperty("SystemUuid", "");
    createAndSetDefaultStringProperty("ConnectionString", "");
    createAndSetDefaultStringProperty("SdkVersion", "");
    createAndSetDefaultStringProperty("Location", "");
    
    Super::setProtectedPropertyValue(String("Name"), name);
    Super::setProtectedPropertyValue(String("ConnectionString"), connectionString);

    Super::addProperty(ObjectProperty("ServerCapabilities", PropertyObject()));
    defaultPropertyNames.insert("ServerCapabilities");

    Super::addProperty(ObjectProperty("ConfigurationConnectionInfo", ServerCapability("", "", ProtocolType::Unknown)));
    defaultPropertyNames.insert("ConfigurationConnectionInfo");

    if (customSdkVersion.assigned())
        Super::setProtectedPropertyValue(String("SdkVersion"), customSdkVersion);
    else
        Super::setProtectedPropertyValue(String("SdkVersion"), String(OPENDAQ_PACKAGE_VERSION));

    this->objPtr.getOnPropertyValueRead("Location") += [&](PropertyObjectPtr&, PropertyValueEventArgsPtr& value)
    {
        const PropertyObjectPtr ownerPtr = this->owner.assigned() ? this->owner.getRef() : nullptr;
        if (ownerPtr.assigned())
        {
            value.setValue(ownerPtr.getPropertyValue("Location"));
        }
    };

    this->objPtr.getOnPropertyValueRead("Name") += [&](PropertyObjectPtr&, PropertyValueEventArgsPtr& value)
    {
        const ComponentPtr ownerPtr = this->owner.assigned() ? this->owner.getRef() : nullptr;
        if (ownerPtr.assigned())
        {
            value.setValue(ownerPtr.getName());
        }
    };
}

template <typename TInterface, typename ... Interfaces>
DeviceInfoConfigImpl<TInterface, Interfaces...>::DeviceInfoConfigImpl()
    : DeviceInfoConfigImpl<TInterface, Interfaces...>("", "")
{
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::setName(IString* name)
{
    return Super::setProtectedPropertyValue(String("Name"), name);
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getName(IString** name)
{
    return daqTry([&]() {
        *name = getStringProperty("Name").detach();
        return OPENDAQ_SUCCESS;
    });
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::setConnectionString(IString* connectionString)
{
    return Super::setProtectedPropertyValue(String("ConnectionString"), connectionString);
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getConnectionString(IString** connectionString)
{
    return daqTry([&]() {
        *connectionString = getStringProperty("ConnectionString").detach();
        return OPENDAQ_SUCCESS;
    });
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::setDeviceType(IDeviceType* deviceType)
{
    if (this->frozen)
    {
        return OPENDAQ_ERR_FROZEN;
    }

    this->deviceType = deviceType;
    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getDeviceType(IDeviceType** deviceType)
{
    OPENDAQ_PARAM_NOT_NULL(deviceType);

    *deviceType = this->deviceType.addRefAndReturn();

    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::setManufacturer(IString* manufacturer)
{
    return Super::setProtectedPropertyValue(String("Manufacturer"), manufacturer);
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getManufacturer(IString** manufacturer)
{
    return daqTry([&]() {
        *manufacturer = getStringProperty("Manufacturer").detach();
        return OPENDAQ_SUCCESS;
    });
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::setManufacturerUri(IString* manufacturerUri)
{
    return Super::setProtectedPropertyValue(String("ManufacturerUri"), manufacturerUri);
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getManufacturerUri(IString** manufacturerUri)
{
    return daqTry([&]() {
        *manufacturerUri = getStringProperty("ManufacturerUri").detach();
        return OPENDAQ_SUCCESS;
    });
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::setModel(IString* model)
{
    return Super::setProtectedPropertyValue(String("Model"), model);
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getModel(IString** model)
{
    return daqTry([&]() {
        *model = getStringProperty("Model").detach();
        return OPENDAQ_SUCCESS;
    });
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::setProductCode(IString* productCode)
{
    return Super::setProtectedPropertyValue(String("ProductCode"), productCode);
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getProductCode(IString** productCode)
{
    return daqTry([&]() {
        *productCode = getStringProperty("ProductCode").detach();
        return OPENDAQ_SUCCESS;
    });
}

template <typename TInterface, typename ... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getDeviceRevision(IString** deviceRevision)
{
    return daqTry([&]() {
        *deviceRevision = getStringProperty("DeviceRevision").detach();
        return OPENDAQ_SUCCESS;
    });
}

template <typename TInterface, typename ... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::setDeviceRevision(IString* deviceRevision)
{
    return Super::setProtectedPropertyValue(String("DeviceRevision"), deviceRevision);
}

template <typename TInterface, typename ... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getAssetId(IString** id)
{
    return daqTry([&]() {
        *id = getStringProperty("AssetId").detach();
        return OPENDAQ_SUCCESS;
    });
}

template <typename TInterface, typename ... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::setAssetId(IString* id)
{
    return Super::setProtectedPropertyValue(String("AssetId"), id);
}

template <typename TInterface, typename ... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getMacAddress(IString** macAddress)
{
    return daqTry([&]() {
        *macAddress = getStringProperty("MacAddress").detach();
        return OPENDAQ_SUCCESS;
    });
}

template <typename TInterface, typename ... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::setMacAddress(IString* macAddress)
{
    return Super::setProtectedPropertyValue(String("MacAddress"), macAddress);
}

template <typename TInterface, typename ... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getParentMacAddress(IString** macAddress)
{
    return daqTry([&]() {
        *macAddress = getStringProperty("ParentMacAddress").detach();
        return OPENDAQ_SUCCESS;
    });
}

template <typename TInterface, typename ... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::setParentMacAddress(IString* macAddress)
{
    return Super::setProtectedPropertyValue(String("ParentMacAddress"), macAddress);
}

template <typename TInterface, typename ... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getPlatform(IString** platform)
{
    return daqTry([&]() {
        *platform = getStringProperty("Platform").detach();
        return OPENDAQ_SUCCESS;
    });
}

template <typename TInterface, typename ... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::setPlatform(IString* platform)
{
    return Super::setProtectedPropertyValue(String("Platform"), platform);
}

template <typename TInterface, typename ... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getPosition(Int* position)
{
    return daqTry([&]() {
        *position = getIntProperty("Position");
        return OPENDAQ_SUCCESS;
    });
}

template <typename TInterface, typename ... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::setPosition(Int position)
{
    return Super::setProtectedPropertyValue(String("Position"), Integer(position));
}

template <typename TInterface, typename ... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getSystemType(IString** type)
{
    return daqTry([&]() {
        *type = getStringProperty("SystemType").detach();
        return OPENDAQ_SUCCESS;
    });
}

template <typename TInterface, typename ... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::setSystemType(IString* type)
{
    return Super::setProtectedPropertyValue(String("SystemType"), type);
}

template <typename TInterface, typename ... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getSystemUuid(IString** uuid)
{
    return daqTry([&]() {
        *uuid = getStringProperty("SystemUuid").detach();
        return OPENDAQ_SUCCESS;
    });
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::setSystemUuid(IString* uuid)
{
    return Super::setProtectedPropertyValue(String("SystemUuid"), uuid);
}

template <typename TInterface, typename ... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getCustomInfoPropertyNames(IList** customInfoNames)
{
    auto propList = List<IProperty>();
    const ErrCode err = Super::getAllProperties(&propList);
    if (OPENDAQ_FAILED(err))
        return err;

    auto customPropNameList = List<IString>();
    for (auto prop : propList)
    {
        auto name = prop.getName();
        if (!defaultPropertyNames.count(name))
            customPropNameList.pushBack(name);
    }

    *customInfoNames = customPropNameList.detach();
    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename ... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getSdkVersion(IString** version)
{
    OPENDAQ_PARAM_NOT_NULL(version);

    return daqTry([&]() {
        *version = getStringProperty("SdkVersion").detach();
        return OPENDAQ_SUCCESS;
    });
}

template <typename TInterface, typename ... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::setLocation(IString* location)
{
    return Super::setProtectedPropertyValue(String("Location"), location);
}

template <typename TInterface, typename ... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getLocation(IString** location)
{
    OPENDAQ_PARAM_NOT_NULL(location);

    return daqTry([&]() {
        *location = getStringProperty("Location").detach();
        return OPENDAQ_SUCCESS;
    });
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::setHardwareRevision(IString* hardwareRevision)
{
    return Super::setProtectedPropertyValue(String("HardwareRevision"), hardwareRevision);
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getHardwareRevision(IString** hardwareRevision)
{
    return daqTry([&]() {
        *hardwareRevision = getStringProperty("HardwareRevision").detach();
        return OPENDAQ_SUCCESS;
    });
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::setSoftwareRevision(IString* softwareRevision)
{
    return Super::setProtectedPropertyValue(String("SoftwareRevision"), softwareRevision);
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getSoftwareRevision(IString** softwareRevision)
{
    return daqTry([&]() {
        *softwareRevision = getStringProperty("SoftwareRevision").detach();
        return OPENDAQ_SUCCESS;
    });
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::setDeviceManual(IString* deviceManual)
{
    return Super::setProtectedPropertyValue(String("DeviceManual"), deviceManual);
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getDeviceManual(IString** deviceManual)
{
    return daqTry([&]() {
        *deviceManual = getStringProperty("DeviceManual").detach();
        return OPENDAQ_SUCCESS;
    });
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::setDeviceClass(IString* deviceClass)
{
    return Super::setProtectedPropertyValue(String("DeviceClass"), deviceClass);
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getDeviceClass(IString** deviceClass)
{
    return daqTry([&]() {
        *deviceClass = getStringProperty("DeviceClass").detach();
        return OPENDAQ_SUCCESS;
    });
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::setSerialNumber(IString* serialNumber)
{
    return Super::setProtectedPropertyValue(String("SerialNumber"), serialNumber);
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getSerialNumber(IString** serialNumber)
{
    return daqTry([&]() {
        *serialNumber = getStringProperty("SerialNumber").detach();
        return OPENDAQ_SUCCESS;
    });
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::setProductInstanceUri(IString* productInstanceUri)
{
    return Super::setProtectedPropertyValue(String("ProductInstanceUri"), productInstanceUri);
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getProductInstanceUri(IString** productInstanceUri)
{
    return daqTry([&]() {
        *productInstanceUri = getStringProperty("ProductInstanceUri").detach();
        return OPENDAQ_SUCCESS;
    });
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::setRevisionCounter(Int revisionCounter)
{
    return Super::setProtectedPropertyValue(String("RevisionCounter"), Integer(revisionCounter));
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getRevisionCounter(Int* revisionCounter)
{
    return daqTry([&]() {
        *revisionCounter = getIntProperty("RevisionCounter");
        return OPENDAQ_SUCCESS;
    });
}

template <typename TInterface, typename ... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::addProperty(IProperty* property)
{
    OPENDAQ_PARAM_NOT_NULL(property);
    StringPtr name;
    property->getName(&name);

    CoreType type;
    property->getValueType(&type);
    if (static_cast<int>(type) > 3 && name != "ServerCapabilities" && name != "serverCapabilities")
        return this->makeErrorInfo(OPENDAQ_ERR_INVALIDPARAMETER, "Only String, Int, Bool, or Float-type properties can be added to Device Info.");

    BaseObjectPtr selValues;
    if (property->getSelectionValues(&selValues); selValues.assigned())
        return this->makeErrorInfo(OPENDAQ_ERR_INVALIDPARAMETER, "Selection-type properties cannot be added to Device Info.");

    return Super::addProperty(property);
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getSerializeId(ConstCharPtr* id) const
{
    *id = SerializeId();

    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... Interfaces>
ConstCharPtr DeviceInfoConfigImpl<TInterface, Interfaces...>::SerializeId()
{
    return "DeviceInfo";
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::Deserialize(ISerializedObject* serialized,
                                                                     IBaseObject* context,
                                                                     IFunction* factoryCallback,
                                                                     IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(obj);

    return daqTry(
        [&obj, &serialized, &context, &factoryCallback]()
        {
            *obj = Super::DeserializePropertyObject(
                    serialized,
                    context,
                    factoryCallback,
                       [](const SerializedObjectPtr& serialized, const BaseObjectPtr& context, const StringPtr& className)
                       {
                           const auto info = createWithImplementation<IDeviceInfo, DeviceInfoConfigBase>();
                           return info;
                       }).detach();
        });
}

template <typename TInterface, typename ... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::createAndSetDefaultStringProperty(const StringPtr& name, const BaseObjectPtr& value)
{
    defaultPropertyNames.insert(name);
    return createAndSetStringProperty(name, value);
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::createAndSetStringProperty(const StringPtr& name, const StringPtr& value)
{
    auto info = StringPropertyBuilder(name, value);
    info.setReadOnly(true);

    return Super::addProperty(info.build());
}

template <typename TInterface, typename ... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::createAndSetDefaultIntProperty(const StringPtr& name, const BaseObjectPtr& value)
{
    defaultPropertyNames.insert(name);
    return createAndSetIntProperty(name, value);
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::createAndSetIntProperty(const StringPtr& name, const IntegerPtr& value)
{
    auto info = IntPropertyBuilder(name, value);
    info.setReadOnly(true);

    return Super::addProperty(info.build());
}

template <typename TInterface, typename... Interfaces>
StringPtr DeviceInfoConfigImpl<TInterface, Interfaces...>::getStringProperty(const StringPtr& name)
{
    const auto obj = this->template borrowPtr<PropertyObjectPtr>();
    return obj.getPropertyValue(name).template asPtr<IString>();
}

template <typename TInterface, typename ... Interfaces>
Int DeviceInfoConfigImpl<TInterface, Interfaces...>::getIntProperty(const StringPtr& name)
{
    const auto obj = this->template borrowPtr<PropertyObjectPtr>();
    return obj.getPropertyValue(name).template asPtr<IInteger>();
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::addServerCapability(IServerCapability* serverCapability)
{
    OPENDAQ_PARAM_NOT_NULL(serverCapability);
    
    StringPtr id;
    ErrCode err = serverCapability->getProtocolId(&id);
    if (OPENDAQ_FAILED(err))
        return err;

    BaseObjectPtr serverCapabilities;
    StringPtr str = "ServerCapabilities";
    err = this->getPropertyValue(str, &serverCapabilities);
    if (OPENDAQ_FAILED(err))
        return err;

    const auto serverCapabilitiesPtr = serverCapabilities.asPtr<IPropertyObject>();
    for (const auto& prop : serverCapabilitiesPtr.getAllProperties())
    {
        if (prop.getValueType() != ctObject)
            continue;

        const ServerCapabilityPtr capability = serverCapabilitiesPtr.getPropertyValue(prop.getName());
        if (capability.getProtocolId() == id)
            return OPENDAQ_ERR_DUPLICATEITEM;
    }

    serverCapabilitiesPtr.addProperty(ObjectProperty(id, serverCapability));
    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::removeServerCapability(IString* protocolId)
{
    OPENDAQ_PARAM_NOT_NULL(protocolId);

    BaseObjectPtr serverCapabilities;
    StringPtr str = "ServerCapabilities";
    const ErrCode err = this->getPropertyValue(str, &serverCapabilities);
    if (OPENDAQ_FAILED(err))
        return err;
    
    const auto serverCapabilitiesPtr = serverCapabilities.asPtr<IPropertyObject>();
    if (!serverCapabilitiesPtr.hasProperty(protocolId))
        return OPENDAQ_ERR_NOTFOUND;

    
    return serverCapabilitiesPtr->removeProperty(protocolId);
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::clearServerStreamingCapabilities()
{
    BaseObjectPtr serverCapabilities;
    StringPtr str = "ServerCapabilities";
    ErrCode err = this->getPropertyValue(str, &serverCapabilities);
    if (OPENDAQ_FAILED(err))
        return err;
    
    const auto serverCapabilitiesPtr = serverCapabilities.asPtr<IPropertyObject>();
    const auto props = serverCapabilitiesPtr.getAllProperties();
    for (const auto& prop : props)
    {
        if (prop.getValueType() == ctObject)
        {
            err = serverCapabilitiesPtr->removeProperty(prop.getName());
            if (OPENDAQ_FAILED(err))
                return err;
        }
    }

    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename ... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::hasServerCapability(IString* protocolId, Bool* hasCapability)
{
    OPENDAQ_PARAM_NOT_NULL(hasCapability);
    OPENDAQ_PARAM_NOT_NULL(protocolId);
    
    BaseObjectPtr obj;
    StringPtr str = "ServerCapabilities";
    ErrCode err = this->getPropertyValue(str, &obj);
    if (OPENDAQ_FAILED(err))
        return err;

    const auto serverCapabilitiesPtr = obj.asPtr<IPropertyObject>();
    serverCapabilitiesPtr->hasProperty(protocolId, hasCapability);
    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename ... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getServerCapability(IString* protocolId, IServerCapability** capability)
{
    OPENDAQ_PARAM_NOT_NULL(protocolId);
    OPENDAQ_PARAM_NOT_NULL(capability);

    Bool hasCap;
    ErrCode err = this->hasServerCapability(protocolId, &hasCap);
    if (OPENDAQ_FAILED(err))
        return err;

    if (!hasCap)
        return OPENDAQ_ERR_NOTFOUND;
    
    BaseObjectPtr obj;
    StringPtr str = "serverCapabilities";
    err = this->getPropertyValue(str, &obj);
    if (OPENDAQ_FAILED(err))
        return err;

    const auto serverCapabilitiesPtr = obj.asPtr<IPropertyObject>();
    *capability = serverCapabilitiesPtr.getPropertyValue(protocolId).asPtr<IServerCapability>().detach();
    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename ... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getServerCapabilities(IList** serverCapabilities)
{
    OPENDAQ_PARAM_NOT_NULL(serverCapabilities);
    ListPtr<IServerCapability> caps = List<IServerCapability>();

    BaseObjectPtr obj;
    StringPtr str = "ServerCapabilities";
    ErrCode err = this->getPropertyValue(str, &obj);
    if (OPENDAQ_FAILED(err))
        return err;

    const auto serverCapabilitiesPtr = obj.asPtr<IPropertyObject>();
    for (const auto& prop : serverCapabilitiesPtr.getAllProperties())
    {
        if (prop.getValueType() == ctObject)
        {
            BaseObjectPtr cap;
            err = serverCapabilitiesPtr->getPropertyValue(prop.getName(), &cap);
            if (OPENDAQ_FAILED(err))
                return err;

            caps.pushBack(cap.detach());
        }
    }

    *serverCapabilities = caps.detach();
    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename ... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getConfigurationConnectionInfo(IServerCapability** connectionInfo)
{
    BaseObjectPtr obj;
    StringPtr str = "ConfigurationConnectionInfo";
    ErrCode err = this->getPropertyValue(str, &obj);
    if (OPENDAQ_FAILED(err))
        return err;
    *connectionInfo = obj.asPtr<IServerCapability>().detach();
    return OPENDAQ_SUCCESS;
}

#if !defined(BUILDING_STATIC_LIBRARY)

extern "C"
ErrCode PUBLIC_EXPORT createDeviceInfoConfig(IDeviceInfoConfig** objTmp, IString* name, IString* connectionString)
{
    return createObject<IDeviceInfoConfig, DeviceInfoConfigImpl<>, IString*, IString*>(objTmp, name, connectionString);
}

extern "C"
ErrCode PUBLIC_EXPORT createDeviceInfoConfigWithCustomSdkVersion(IDeviceInfoConfig** objTmp, IString* name, IString* connectionString, IString* sdkVersion)
{
    return createObject<IDeviceInfoConfig, DeviceInfoConfigImpl<>, IString*, IString*, IString*>(objTmp, name, connectionString, sdkVersion);
}

#endif

END_NAMESPACE_OPENDAQ
