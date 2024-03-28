#include <opendaq/device_info_impl.h>
#include <coretypes/validation.h>
#include "coretypes/impl.h"
#include <coreobjects/property_object_factory.h>

BEGIN_NAMESPACE_OPENDAQ

template <typename TInterface, typename... Interfaces>
DeviceInfoConfigImpl<TInterface, Interfaces...>::DeviceInfoConfigImpl(const StringPtr& name, const StringPtr& connectionString, const StringPtr& customSdkVersion)
    : Super()
    , serverCapabilities(List<IServerCapability>())
{
    createAndSetDefaultStringProperty("name", name);
    createAndSetDefaultStringProperty("manufacturer", "");
    createAndSetDefaultStringProperty("manufacturerUri", "");
    createAndSetDefaultStringProperty("model", "");
    createAndSetDefaultStringProperty("productCode", "");
    createAndSetDefaultStringProperty("deviceRevision", "");
    createAndSetDefaultStringProperty("hardwareRevision", "");
    createAndSetDefaultStringProperty("softwareRevision", "");
    createAndSetDefaultStringProperty("deviceManual", "");
    createAndSetDefaultStringProperty("deviceClass", "");
    createAndSetDefaultStringProperty("serialNumber", "");
    createAndSetDefaultStringProperty("productInstanceUri", "");
    createAndSetDefaultIntProperty("revisionCounter", 0);
    createAndSetDefaultStringProperty("assetId", "");
    createAndSetDefaultStringProperty("macAddress", "");
    createAndSetDefaultStringProperty("parentMacAddress", "");
    createAndSetDefaultStringProperty("platform", "");
    createAndSetDefaultIntProperty("position", 0);
    createAndSetDefaultStringProperty("systemType", "");
    createAndSetDefaultStringProperty("systemUuid", "");
    createAndSetDefaultStringProperty("connectionString", connectionString);
    createAndSetDefaultStringProperty("sdkVersion", "");

    if (customSdkVersion.assigned())
        Super::setProtectedPropertyValue(String("sdkVersion"), customSdkVersion);
    else
        Super::setProtectedPropertyValue(String("sdkVersion"), String(OPENDAQ_PACKAGE_VERSION));
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::setName(IString* name)
{
    return Super::setProtectedPropertyValue(String("name"), name);
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getName(IString** name)
{
    return daqTry([&]() {
        *name = getStringProperty("name").detach();
        return OPENDAQ_SUCCESS;
    });
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::setConnectionString(IString* connectionString)
{
    return Super::setProtectedPropertyValue(String("connectionString"), connectionString);
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getConnectionString(IString** connectionString)
{
    return daqTry([&]() {
        *connectionString = getStringProperty("connectionString").detach();
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
    return Super::setProtectedPropertyValue(String("manufacturer"), manufacturer);
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getManufacturer(IString** manufacturer)
{
    return daqTry([&]() {
        *manufacturer = getStringProperty("manufacturer").detach();
        return OPENDAQ_SUCCESS;
    });
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::setManufacturerUri(IString* manufacturerUri)
{
    return Super::setProtectedPropertyValue(String("manufacturerUri"), manufacturerUri);
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getManufacturerUri(IString** manufacturerUri)
{
    return daqTry([&]() {
        *manufacturerUri = getStringProperty("manufacturerUri").detach();
        return OPENDAQ_SUCCESS;
    });
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::setModel(IString* model)
{
    return Super::setProtectedPropertyValue(String("model"), model);
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getModel(IString** model)
{
    return daqTry([&]() {
        *model = getStringProperty("model").detach();
        return OPENDAQ_SUCCESS;
    });
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::setProductCode(IString* productCode)
{
    return Super::setProtectedPropertyValue(String("productCode"), productCode);
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getProductCode(IString** productCode)
{
    return daqTry([&]() {
        *productCode = getStringProperty("productCode").detach();
        return OPENDAQ_SUCCESS;
    });
}

template <typename TInterface, typename ... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getDeviceRevision(IString** deviceRevision)
{
    return daqTry([&]() {
        *deviceRevision = getStringProperty("deviceRevision").detach();
        return OPENDAQ_SUCCESS;
    });
}

template <typename TInterface, typename ... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::setDeviceRevision(IString* deviceRevision)
{
    return Super::setProtectedPropertyValue(String("deviceRevision"), deviceRevision);
}

template <typename TInterface, typename ... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getAssetId(IString** id)
{
    return daqTry([&]() {
        *id = getStringProperty("assetId").detach();
        return OPENDAQ_SUCCESS;
    });
}

template <typename TInterface, typename ... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::setAssetId(IString* id)
{
    return Super::setProtectedPropertyValue(String("assetId"), id);
}

template <typename TInterface, typename ... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getMacAddress(IString** macAddress)
{
    return daqTry([&]() {
        *macAddress = getStringProperty("macAddress").detach();
        return OPENDAQ_SUCCESS;
    });
}

template <typename TInterface, typename ... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::setMacAddress(IString* macAddress)
{
    return Super::setProtectedPropertyValue(String("macAddress"), macAddress);
}

template <typename TInterface, typename ... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getParentMacAddress(IString** macAddress)
{
    return daqTry([&]() {
        *macAddress = getStringProperty("parentMacAddress").detach();
        return OPENDAQ_SUCCESS;
    });
}

template <typename TInterface, typename ... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::setParentMacAddress(IString* macAddress)
{
    return Super::setProtectedPropertyValue(String("parentMacAddress"), macAddress);
}

template <typename TInterface, typename ... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getPlatform(IString** platform)
{
    return daqTry([&]() {
        *platform = getStringProperty("platform").detach();
        return OPENDAQ_SUCCESS;
    });
}

template <typename TInterface, typename ... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::setPlatform(IString* platform)
{
    return Super::setProtectedPropertyValue(String("platform"), platform);
}

template <typename TInterface, typename ... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getPosition(Int* position)
{
    return daqTry([&]() {
        *position = getIntProperty("position");
        return OPENDAQ_SUCCESS;
    });
}

template <typename TInterface, typename ... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::setPosition(Int position)
{
    return Super::setProtectedPropertyValue(String("position"), Integer(position));
}

template <typename TInterface, typename ... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getSystemType(IString** type)
{
    return daqTry([&]() {
        *type = getStringProperty("systemType").detach();
        return OPENDAQ_SUCCESS;
    });
}

template <typename TInterface, typename ... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::setSystemType(IString* type)
{
    return Super::setProtectedPropertyValue(String("systemType"), type);
}

template <typename TInterface, typename ... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getSystemUuid(IString** uuid)
{
    return daqTry([&]() {
        *uuid = getStringProperty("systemUuid").detach();
        return OPENDAQ_SUCCESS;
    });
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::setSystemUuid(IString* uuid)
{
    return Super::setProtectedPropertyValue(String("systemUuid"), uuid);
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
        *version = getStringProperty("sdkVersion").detach();
        return OPENDAQ_SUCCESS;
    });
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::setHardwareRevision(IString* hardwareRevision)
{
    return Super::setProtectedPropertyValue(String("hardwareRevision"), hardwareRevision);
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getHardwareRevision(IString** hardwareRevision)
{
    return daqTry([&]() {
        *hardwareRevision = getStringProperty("hardwareRevision").detach();
        return OPENDAQ_SUCCESS;
    });
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::setSoftwareRevision(IString* softwareRevision)
{
    return Super::setProtectedPropertyValue(String("softwareRevision"), softwareRevision);
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getSoftwareRevision(IString** softwareRevision)
{
    return daqTry([&]() {
        *softwareRevision = getStringProperty("softwareRevision").detach();
        return OPENDAQ_SUCCESS;
    });
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::setDeviceManual(IString* deviceManual)
{
    return Super::setProtectedPropertyValue(String("deviceManual"), deviceManual);
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getDeviceManual(IString** deviceManual)
{
    return daqTry([&]() {
        *deviceManual = getStringProperty("deviceManual").detach();
        return OPENDAQ_SUCCESS;
    });
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::setDeviceClass(IString* deviceClass)
{
    return Super::setProtectedPropertyValue(String("deviceClass"), deviceClass);
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getDeviceClass(IString** deviceClass)
{
    return daqTry([&]() {
        *deviceClass = getStringProperty("deviceClass").detach();
        return OPENDAQ_SUCCESS;
    });
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::setSerialNumber(IString* serialNumber)
{
    return Super::setProtectedPropertyValue(String("serialNumber"), serialNumber);
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getSerialNumber(IString** serialNumber)
{
    return daqTry([&]() {
        *serialNumber = getStringProperty("serialNumber").detach();
        return OPENDAQ_SUCCESS;
    });
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::setProductInstanceUri(IString* productInstanceUri)
{
    return Super::setProtectedPropertyValue(String("productInstanceUri"), productInstanceUri);
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getProductInstanceUri(IString** productInstanceUri)
{
    return daqTry([&]() {
        *productInstanceUri = getStringProperty("productInstanceUri").detach();
        return OPENDAQ_SUCCESS;
    });
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::setRevisionCounter(Int revisionCounter)
{
    return Super::setProtectedPropertyValue(String("revisionCounter"), Integer(revisionCounter));
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getRevisionCounter(Int* revisionCounter)
{
    return daqTry([&]() {
        *revisionCounter = getIntProperty("revisionCounter");
        return OPENDAQ_SUCCESS;
    });
}

template <typename TInterface, typename ... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::addProperty(IProperty* property)
{
    CoreType type;
    property->getValueType(&type);
    if (static_cast<int>(type) > 3)
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
    
    auto serverCapabilityPtr = ServerCapabilityPtr::Borrow(serverCapability);
    bool isServerStreaming = serverCapabilityPtr.getProtocolType().getValue() == "ServerStreaming";

    if (isServerStreaming)
    {
        for (const auto& capability : serverCapabilities)
        {
            if (capability.getProtocolType().getValue() == "ServerStreaming" && capability.getPropertyValue("protocolId") == serverCapabilityPtr.getPropertyValue("protocolId"))
                return OPENDAQ_ERR_DUPLICATEITEM;
        }
    }
    
    serverCapabilities.pushBack(serverCapability);
    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::removeServerCapability(IString* protocolId)
{
    OPENDAQ_PARAM_NOT_NULL(protocolId);
    
    size_t i = 0;
    while(i < serverCapabilities.getCount())
    {
        auto serverCapability = serverCapabilities[i];
        if (serverCapability.getProtocolType().getValue() == "ServerStreaming" && serverCapability.getPropertyValue("protocolId") == StringPtr::Borrow(protocolId))
            serverCapabilities.removeAt(i);
        else
            i++;
    }
    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::clearServerStreamingCapabilities()
{
    size_t i = 0;
    while(i < serverCapabilities.getCount())
    {
        ServerCapabilityPtr serverCapability = serverCapabilities[i];
        if (serverCapability.getProtocolType().getValue() == "ServerStreaming")
            serverCapabilities.removeAt(i);
        else
            i++;
    }
    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename ... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getServerCapabilities(IList** serverCapabilities)
{
    OPENDAQ_PARAM_NOT_NULL(serverCapabilities);

    *serverCapabilities = this->serverCapabilities.addRefAndReturn();
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
