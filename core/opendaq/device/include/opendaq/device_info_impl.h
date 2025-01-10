/*
 * Copyright 2022-2025 openDAQ d.o.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once
#include <opendaq/device_info_config.h>
#include <coretypes/freezable.h>
#include <coretypes/intfs.h>
#include <coretypes/string_ptr.h>
#include <coretypes/dictobject_factory.h>
#include <coreobjects/property_object_impl.h>
#include <opendaq/device_type_ptr.h>
#include <opendaq/device_info_internal.h>
#include <opendaq/server_capability_ptr.h>
#include <coretypes/event_ptr.h>
#include <opendaq/component_ptr.h>
#include <opendaq/device_info_factory.h>
#include <coretypes/validation.h>
#include <coreobjects/property_object_factory.h>
#include <opendaq/custom_log.h>
#include <set>

BEGIN_NAMESPACE_OPENDAQ

template <typename TInterface = IDeviceInfoConfig, typename... Interfaces>
class DeviceInfoConfigImpl;

using DeviceInfoConfigBase = DeviceInfoConfigImpl<>;

template <typename TInterface, typename... Interfaces>
class DeviceInfoConfigImpl : public GenericPropertyObjectImpl<TInterface, IDeviceInfoInternal, Interfaces...>
{
public:
    using Super = GenericPropertyObjectImpl<TInterface, IDeviceInfoInternal, Interfaces...>;

    DeviceInfoConfigImpl();

    explicit DeviceInfoConfigImpl(const StringPtr& name, 
                                  const StringPtr& connectionString, 
                                  const StringPtr& customSdkVersion = nullptr,
                                  const ListPtr<IString>& changeableDefaultPropertyNames = nullptr);

    DeviceInfoConfigImpl(const ListPtr<IString>& changeableDefaultPropertyNames);

    ErrCode INTERFACE_FUNC getName(IString** name) override;
    ErrCode INTERFACE_FUNC getConnectionString(IString** connectionString) override;
    ErrCode INTERFACE_FUNC getDeviceType(IDeviceType** deviceType) override;
    ErrCode INTERFACE_FUNC getManufacturer(IString** manufacturer) override;
    ErrCode INTERFACE_FUNC getManufacturerUri(IString** manufacturerUri) override;
    ErrCode INTERFACE_FUNC getModel(IString** model) override;
    ErrCode INTERFACE_FUNC getProductCode(IString** productCode) override;
    ErrCode INTERFACE_FUNC getDeviceRevision(IString** deviceRevision) override;
    ErrCode INTERFACE_FUNC getHardwareRevision(IString** hardwareRevision) override;
    ErrCode INTERFACE_FUNC getSoftwareRevision(IString** softwareRevision) override;
    ErrCode INTERFACE_FUNC getDeviceManual(IString** deviceManual) override;
    ErrCode INTERFACE_FUNC getDeviceClass(IString** deviceClass) override;
    ErrCode INTERFACE_FUNC getSerialNumber(IString** serialNumber) override;
    ErrCode INTERFACE_FUNC getProductInstanceUri(IString** productInstanceUri) override;
    ErrCode INTERFACE_FUNC getRevisionCounter(Int* revisionCounter) override;
    ErrCode INTERFACE_FUNC getAssetId(IString** id) override;
    ErrCode INTERFACE_FUNC getMacAddress(IString** macAddress) override;
    ErrCode INTERFACE_FUNC getParentMacAddress(IString** macAddress) override;
    ErrCode INTERFACE_FUNC getPlatform(IString** platform) override;
    ErrCode INTERFACE_FUNC getPosition(Int* position) override;
    ErrCode INTERFACE_FUNC getSystemType(IString** type) override;
    ErrCode INTERFACE_FUNC getSystemUuid(IString** uuid) override;
    ErrCode INTERFACE_FUNC getCustomInfoPropertyNames(IList** customInfoNames) override;
    ErrCode INTERFACE_FUNC getSdkVersion(IString** version) override;
    ErrCode INTERFACE_FUNC getLocation(IString** location) override;

    ErrCode INTERFACE_FUNC setName(IString* name) override;
    ErrCode INTERFACE_FUNC setConnectionString(IString* connectionString) override;
    ErrCode INTERFACE_FUNC setDeviceType(IDeviceType* deviceType) override;
    ErrCode INTERFACE_FUNC setManufacturer(IString* manufacturer) override;
    ErrCode INTERFACE_FUNC setManufacturerUri(IString* manufacturerUri) override;
    ErrCode INTERFACE_FUNC setModel(IString* model) override;
    ErrCode INTERFACE_FUNC setProductCode(IString* productCode) override;
    ErrCode INTERFACE_FUNC setDeviceRevision(IString* deviceRevision) override;
    ErrCode INTERFACE_FUNC setHardwareRevision(IString* hardwareRevision) override;
    ErrCode INTERFACE_FUNC setSoftwareRevision(IString* softwareRevision) override;
    ErrCode INTERFACE_FUNC setDeviceManual(IString* deviceManual) override;
    ErrCode INTERFACE_FUNC setDeviceClass(IString* deviceClass) override;
    ErrCode INTERFACE_FUNC setSerialNumber(IString* serialNumber) override;
    ErrCode INTERFACE_FUNC setProductInstanceUri(IString* productInstanceUri) override;
    ErrCode INTERFACE_FUNC setRevisionCounter(Int revisionCounter) override;
    ErrCode INTERFACE_FUNC setAssetId(IString* id) override;
    ErrCode INTERFACE_FUNC setMacAddress(IString* macAddress) override;
    ErrCode INTERFACE_FUNC setParentMacAddress(IString* macAddress) override;
    ErrCode INTERFACE_FUNC setPlatform(IString* platform) override;
    ErrCode INTERFACE_FUNC setPosition(Int position) override;
    ErrCode INTERFACE_FUNC setSystemType(IString* type) override;
    ErrCode INTERFACE_FUNC setSystemUuid(IString* uuid) override;
    ErrCode INTERFACE_FUNC setLocation(IString* location) override;

    ErrCode INTERFACE_FUNC addProperty(IProperty* property) override;

    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;

    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

    ErrCode INTERFACE_FUNC addServerCapability(IServerCapability* serverCapability) override;
    ErrCode INTERFACE_FUNC removeServerCapability(IString* protocolId) override;
    ErrCode INTERFACE_FUNC getServerCapabilities(IList** serverCapabilities) override;
    ErrCode INTERFACE_FUNC clearServerStreamingCapabilities() override;
    ErrCode INTERFACE_FUNC hasServerCapability(IString* protocolId, Bool* hasCapability) override;
    ErrCode INTERFACE_FUNC getServerCapability(IString* protocolId, IServerCapability** capability) override;

    ErrCode INTERFACE_FUNC getConfigurationConnectionInfo(IServerCapability** connectionInfo) override;

    // IPropertyObject
    ErrCode INTERFACE_FUNC getPropertyValueNoLock(IString* propertyName, IBaseObject** value) override;
    ErrCode INTERFACE_FUNC setPropertyValueNoLock(IString* propertyName, IBaseObject* value) override;
    ErrCode INTERFACE_FUNC setProtectedPropertyValue(IString* propertyName, IBaseObject* value) override;

    // IOwnable
    virtual ErrCode INTERFACE_FUNC setOwner(IPropertyObject* newOwner) override;


private:
    ErrCode createAndSetStringProperty(const StringPtr& name, const StringPtr& value);
    ErrCode createAndSetIntProperty(const StringPtr& name, const IntegerPtr& value);
    StringPtr getStringProperty(const StringPtr& name);
    Int getIntProperty(const StringPtr& name);

    bool isPropertyChangeable(const StringPtr& propertyName);

    void triggerCoreEventMetod(const CoreEventArgsPtr& args);

    ErrCode setValueInternal(IString* propertyName, IBaseObject* value);

    std::set<std::string> changeableDefaultPropertyNames;
    DeviceTypePtr deviceType;

    EventPtr<const ComponentPtr, const CoreEventArgsPtr> coreEvent;
    bool isLocal;
};

namespace deviceInfoDetails
{
    static const std::unordered_set<std::string> defaultDeviceInfoPropertyNames = 
    {
        "name", 
        "manufacturer", 
        "manufacturerUri", 
        "model", 
        "productCode", 
        "deviceRevision", 
        "hardwareRevision",
        "softwareRevision", 
        "deviceManual", 
        "deviceClass", 
        "serialNumber",
        "productInstanceUri",
        "revisionCounter",
        "assetId",
        "macAddress",
        "parentMacAddress",
        "platform",
        "position",
        "systemType",
        "systemUuid",
        "connectionString",
        "sdkVersion",
        "location",
        "userName",
        "serverCapabilities",
        "configurationConnectionInfo"
    };
}

inline std::string ToLowerCase(const std::string &input) 
{
    std::string result = input;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

template <typename TInterface, typename ... Interfaces>
DeviceInfoConfigImpl<TInterface, Interfaces...>::DeviceInfoConfigImpl()
    : Super()
{
    this->isLocal = false;
    this->path = "DaqDeviceInfo";
    createAndSetStringProperty("name", "");
    createAndSetStringProperty("connectionString", "");
    createAndSetStringProperty("sdkVersion", "");

    Super::addProperty(ObjectPropertyBuilder("serverCapabilities", PropertyObject()).setReadOnly(true).build());
    Super::addProperty(ObjectPropertyBuilder("configurationConnectionInfo", ServerCapability("", "", ProtocolType::Unknown)).setReadOnly(true).build());

    this->objPtr.getOnPropertyValueRead("name") += [&](PropertyObjectPtr&, PropertyValueEventArgsPtr& value)
    {
        const ComponentPtr ownerPtr = this->owner.assigned() ? this->owner.getRef() : nullptr;
        if (ownerPtr.assigned())
            value.setValue(ownerPtr.getName());
    };
}

template <typename TInterface, typename... Interfaces>
DeviceInfoConfigImpl<TInterface, Interfaces...>::DeviceInfoConfigImpl(const StringPtr& name,
                                                                      const StringPtr& connectionString,
                                                                      const StringPtr& customSdkVersion,
                                                                      const ListPtr<IString>& changeableDefaultPropertyNames)
    : DeviceInfoConfigImpl()
{
    this->isLocal = true;
    if (changeableDefaultPropertyNames.assigned())
    {
        for (const auto& propName : changeableDefaultPropertyNames)
            this->changeableDefaultPropertyNames.insert(ToLowerCase(propName));

        this->changeableDefaultPropertyNames.insert("username");
        this->changeableDefaultPropertyNames.insert("location");
    }

    createAndSetStringProperty("manufacturer", "");
    createAndSetStringProperty("manufacturerUri", "");
    createAndSetStringProperty("model", "");
    createAndSetStringProperty("productCode", "");
    createAndSetStringProperty("deviceRevision", "");
    createAndSetStringProperty("hardwareRevision", "");
    createAndSetStringProperty("softwareRevision", "");
    createAndSetStringProperty("deviceManual", "");
    createAndSetStringProperty("deviceClass", "");
    createAndSetStringProperty("serialNumber", "");
    createAndSetStringProperty("productInstanceUri", "");
    createAndSetIntProperty("revisionCounter", 0);
    createAndSetStringProperty("assetId", "");
    createAndSetStringProperty("macAddress", "");
    createAndSetStringProperty("parentMacAddress", "");
    createAndSetStringProperty("platform", "");
    createAndSetIntProperty("position", 0);
    createAndSetStringProperty("systemType", "");
    createAndSetStringProperty("systemUuid", "");
    createAndSetStringProperty("location", "");
    createAndSetStringProperty("userName", "");

    setValueInternal(String("name"), name);
    setValueInternal(String("connectionString"), connectionString);

    if (customSdkVersion.assigned())
        setValueInternal(String("sdkVersion"), customSdkVersion);
    else
        setValueInternal(String("sdkVersion"), String(OPENDAQ_PACKAGE_VERSION));

    this->changeableDefaultPropertyNames.clear();
}

template <typename TInterface, typename ... Interfaces>
DeviceInfoConfigImpl<TInterface, Interfaces...>::DeviceInfoConfigImpl(const ListPtr<IString>& changeableDefaultPropertyNames)
    : DeviceInfoConfigImpl<TInterface, Interfaces...>("", "", nullptr, changeableDefaultPropertyNames)
{
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::setName(IString* name)
{
    return setValueInternal(String("name"), name);
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getName(IString** name)
{
    return daqTry([&]
    {
        *name = getStringProperty("name").detach();
        return OPENDAQ_SUCCESS;
    });
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::setConnectionString(IString* connectionString)
{
    return setValueInternal(String("connectionString"), connectionString);
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getConnectionString(IString** connectionString)
{
    return daqTry([&]
    {
        *connectionString = getStringProperty("connectionString").detach();
        return OPENDAQ_SUCCESS;
    });
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::setDeviceType(IDeviceType* deviceType)
{
    if (this->frozen)
        return OPENDAQ_ERR_FROZEN;

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
    return setValueInternal(String("manufacturer"), manufacturer);
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getManufacturer(IString** manufacturer)
{
    return daqTry([&]
    {
        *manufacturer = getStringProperty("manufacturer").detach();
        return OPENDAQ_SUCCESS;
    });
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::setManufacturerUri(IString* manufacturerUri)
{
    return setValueInternal(String("manufacturerUri"), manufacturerUri);
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getManufacturerUri(IString** manufacturerUri)
{
    return daqTry([&]
    {
        *manufacturerUri = getStringProperty("manufacturerUri").detach();
        return OPENDAQ_SUCCESS;
    });
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::setModel(IString* model)
{
    return setValueInternal(String("model"), model);
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getModel(IString** model)
{
    return daqTry([&]
    {
        *model = getStringProperty("model").detach();
        return OPENDAQ_SUCCESS;
    });
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::setProductCode(IString* productCode)
{
    return setValueInternal(String("productCode"), productCode);
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getProductCode(IString** productCode)
{
    return daqTry([&]
    {
        *productCode = getStringProperty("productCode").detach();
        return OPENDAQ_SUCCESS;
    });
}

template <typename TInterface, typename ... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getDeviceRevision(IString** deviceRevision)
{
    return daqTry([&]
    {
        *deviceRevision = getStringProperty("deviceRevision").detach();
        return OPENDAQ_SUCCESS;
    });
}

template <typename TInterface, typename ... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::setDeviceRevision(IString* deviceRevision)
{
    return setValueInternal(String("deviceRevision"), deviceRevision);
}

template <typename TInterface, typename ... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getAssetId(IString** id)
{
    return daqTry([&]
    {
        *id = getStringProperty("assetId").detach();
        return OPENDAQ_SUCCESS;
    });
}

template <typename TInterface, typename ... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::setAssetId(IString* id)
{
    return setValueInternal(String("assetId"), id);
}

template <typename TInterface, typename ... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getMacAddress(IString** macAddress)
{
    return daqTry([&]
    {
        *macAddress = getStringProperty("macAddress").detach();
        return OPENDAQ_SUCCESS;
    });
}

template <typename TInterface, typename ... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::setMacAddress(IString* macAddress)
{
    return setValueInternal(String("macAddress"), macAddress);
}

template <typename TInterface, typename ... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getParentMacAddress(IString** macAddress)
{
    return daqTry([&]
    {
        *macAddress = getStringProperty("parentMacAddress").detach();
        return OPENDAQ_SUCCESS;
    });
}

template <typename TInterface, typename ... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::setParentMacAddress(IString* macAddress)
{
    return setValueInternal(String("parentMacAddress"), macAddress);
}

template <typename TInterface, typename ... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getPlatform(IString** platform)
{
    return daqTry([&]
    {
        *platform = getStringProperty("platform").detach();
        return OPENDAQ_SUCCESS;
    });
}

template <typename TInterface, typename ... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::setPlatform(IString* platform)
{
    return setValueInternal(String("platform"), platform);
}

template <typename TInterface, typename ... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getPosition(Int* position)
{
    return daqTry([&]
    {
        *position = getIntProperty("position");
        return OPENDAQ_SUCCESS;
    });
}

template <typename TInterface, typename ... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::setPosition(Int position)
{
    return setValueInternal(String("position"), Integer(position));
}

template <typename TInterface, typename ... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getSystemType(IString** type)
{
    return daqTry([&]
    {
        *type = getStringProperty("systemType").detach();
        return OPENDAQ_SUCCESS;
    });
}

template <typename TInterface, typename ... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::setSystemType(IString* type)
{
    return setValueInternal(String("systemType"), type);
}

template <typename TInterface, typename ... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getSystemUuid(IString** uuid)
{
    return daqTry([&]
    {
        *uuid = getStringProperty("systemUuid").detach();
        return OPENDAQ_SUCCESS;
    });
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::setSystemUuid(IString* uuid)
{
    return setValueInternal(String("systemUuid"), uuid);
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
        if (!deviceInfoDetails::defaultDeviceInfoPropertyNames.count(name))
            customPropNameList.pushBack(name);
    }

    *customInfoNames = customPropNameList.detach();
    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename ... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getSdkVersion(IString** version)
{
    OPENDAQ_PARAM_NOT_NULL(version);

    return daqTry([&]
    {
        *version = getStringProperty("sdkVersion").detach();
        return OPENDAQ_SUCCESS;
    });
}

template <typename TInterface, typename ... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::setLocation(IString* location)
{
    return setValueInternal(String("location"), location);
}

template <typename TInterface, typename ... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getLocation(IString** location)
{
    OPENDAQ_PARAM_NOT_NULL(location);

    return daqTry([&]
    {
        *location = getStringProperty("location").detach();
        return OPENDAQ_SUCCESS;
    });
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::setHardwareRevision(IString* hardwareRevision)
{
    return setValueInternal(String("hardwareRevision"), hardwareRevision);
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getHardwareRevision(IString** hardwareRevision)
{
    return daqTry([&]
    {
        *hardwareRevision = getStringProperty("hardwareRevision").detach();
        return OPENDAQ_SUCCESS;
    });
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::setSoftwareRevision(IString* softwareRevision)
{
    return setValueInternal(String("softwareRevision"), softwareRevision);
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getSoftwareRevision(IString** softwareRevision)
{
    return daqTry([&]
    {
        *softwareRevision = getStringProperty("softwareRevision").detach();
        return OPENDAQ_SUCCESS;
    });
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::setDeviceManual(IString* deviceManual)
{
    return setValueInternal(String("deviceManual"), deviceManual);
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getDeviceManual(IString** deviceManual)
{
    return daqTry([&]
    {
        *deviceManual = getStringProperty("deviceManual").detach();
        return OPENDAQ_SUCCESS;
    });
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::setDeviceClass(IString* deviceClass)
{
    return setValueInternal(String("deviceClass"), deviceClass);
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getDeviceClass(IString** deviceClass)
{
    return daqTry([&]
    {
        *deviceClass = getStringProperty("deviceClass").detach();
        return OPENDAQ_SUCCESS;
    });
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::setSerialNumber(IString* serialNumber)
{
    return setValueInternal(String("serialNumber"), serialNumber);
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getSerialNumber(IString** serialNumber)
{
    return daqTry([&]
    {
        *serialNumber = getStringProperty("serialNumber").detach();
        return OPENDAQ_SUCCESS;
    });
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::setProductInstanceUri(IString* productInstanceUri)
{
    return setValueInternal(String("productInstanceUri"), productInstanceUri);
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getProductInstanceUri(IString** productInstanceUri)
{
    return daqTry([&]
    {
        *productInstanceUri = getStringProperty("productInstanceUri").detach();
        return OPENDAQ_SUCCESS;
    });
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::setRevisionCounter(Int revisionCounter)
{
    return setValueInternal(String("revisionCounter"), Integer(revisionCounter));
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getRevisionCounter(Int* revisionCounter)
{
    return daqTry([&]
    {
        *revisionCounter = getIntProperty("revisionCounter");
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
    if (static_cast<int>(type) > 3 && name != "serverCapabilities")
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
                                                                     IFunction* /*factoryCallback*/,
                                                                     IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(obj);

    return daqTry([&obj, &serialized, &context]
    {
        *obj = Super::DeserializePropertyObject(
            serialized,
            context,
            nullptr,
            [](const SerializedObjectPtr& serialized, const BaseObjectPtr& context, const StringPtr& className)
            {
                const auto info = createWithImplementation<IDeviceInfo, DeviceInfoConfigBase>();
                return info;
            }).detach();
    });
}

template <typename TInterface, typename ... Interfaces>
bool DeviceInfoConfigImpl<TInterface, Interfaces...>::isPropertyChangeable(const StringPtr& propertyName)
{
    return changeableDefaultPropertyNames.count(ToLowerCase(propertyName)) == 1;
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::createAndSetStringProperty(const StringPtr& name, const StringPtr& value)
{
    auto info = StringPropertyBuilder(name, value);
    info.setReadOnly(!isPropertyChangeable(name));
    return Super::addProperty(info.build());
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::createAndSetIntProperty(const StringPtr& name, const IntegerPtr& value)
{
    auto info = IntPropertyBuilder(name, value);
    info.setReadOnly(!isPropertyChangeable(name));
    return Super::addProperty(info.build());
}

template <typename TInterface, typename... Interfaces>
StringPtr DeviceInfoConfigImpl<TInterface, Interfaces...>::getStringProperty(const StringPtr& name)
{
    return this->objPtr.getPropertyValue(name).template asPtr<IString>();
}

template <typename TInterface, typename ... Interfaces>
Int DeviceInfoConfigImpl<TInterface, Interfaces...>::getIntProperty(const StringPtr& name)
{
    return this->objPtr.getPropertyValue(name).template asPtr<IInteger>();
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
    StringPtr str = "serverCapabilities";
    err = this->getPropertyValue(str, &serverCapabilities);
    if (OPENDAQ_FAILED(err))
        return err;

    const auto serverCapabilitiesPtr = serverCapabilities.asPtr<IPropertyObject>(true);
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
    StringPtr str = "serverCapabilities";
    const ErrCode err = this->getPropertyValue(str, &serverCapabilities);
    if (OPENDAQ_FAILED(err))
        return err;
    
    const auto serverCapabilitiesPtr = serverCapabilities.asPtr<IPropertyObject>(true);
    if (!serverCapabilitiesPtr.hasProperty(protocolId))
        return OPENDAQ_ERR_NOTFOUND;

    return serverCapabilitiesPtr->removeProperty(protocolId);
}

template <typename TInterface, typename... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::clearServerStreamingCapabilities()
{
    BaseObjectPtr serverCapabilities;
    StringPtr str = "serverCapabilities";
    ErrCode err = this->getPropertyValue(str, &serverCapabilities);
    if (OPENDAQ_FAILED(err))
        return err;
    
    const auto serverCapabilitiesPtr = serverCapabilities.asPtr<IPropertyObject>(true);
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
    StringPtr str = "serverCapabilities";
    ErrCode err = this->getPropertyValue(str, &obj);
    if (OPENDAQ_FAILED(err))
        return err;

    const auto serverCapabilitiesPtr = obj.asPtr<IPropertyObject>(true);
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
    StringPtr str = "serverCapabilities";
    ErrCode err = this->getPropertyValue(str, &obj);
    if (OPENDAQ_FAILED(err))
        return err;

    const auto serverCapabilitiesPtr = obj.asPtr<IPropertyObject>(true);
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
    StringPtr str = "configurationConnectionInfo";
    ErrCode err = this->getPropertyValue(str, &obj);
    if (OPENDAQ_FAILED(err))
        return err;
    *connectionInfo = obj.asPtr<IServerCapability>().detach();
    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename ... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::setValueInternal(IString* propertyName, IBaseObject* value)
{
    if (this->isLocal)
        return Super::setProtectedPropertyValue(propertyName, value);
    else
        return this->objPtr->setPropertyValue(propertyName, value);
}

template <typename TInterface, typename ... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::getPropertyValueNoLock(IString* propertyName, IBaseObject** value)
{
    OPENDAQ_PARAM_NOT_NULL(propertyName);
    auto propertyNamePtr = StringPtr::Borrow(propertyName);
   
    if (propertyNamePtr == "userName" || propertyNamePtr == "location")
    {
        auto owner = Super::getPropertyObjectParent();
        if (owner.assigned())
            return owner->getPropertyValue(propertyName, value);
    }

    return Super::getPropertyValueNoLock(propertyName, value); 
}

template <typename TInterface, typename ... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::setPropertyValueNoLock(IString* propertyName, IBaseObject* value)
{
    OPENDAQ_PARAM_NOT_NULL(propertyName);
    auto propertyNamePtr = StringPtr::Borrow(propertyName);
    if (propertyNamePtr == "userName" || propertyNamePtr == "location")
    {
        auto owner = Super::getPropertyObjectParent();
        if (owner.assigned())
            return owner->setPropertyValue(propertyName, value);
    }
    return Super::setPropertyValueNoLock(propertyName, value);
}

template <typename TInterface, typename ... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::setProtectedPropertyValue(IString* propertyName, IBaseObject* value)
{
    OPENDAQ_PARAM_NOT_NULL(propertyName);
    auto propertyNamePtr = StringPtr::Borrow(propertyName);
    if (propertyNamePtr == "userName" || propertyNamePtr == "location")
    {
        auto owner = Super::getPropertyObjectParent();
        if (owner.assigned())
            return owner.template as<IPropertyObjectProtected>(true)->setProtectedPropertyValue(propertyName, value);
    }
    return Super::setProtectedPropertyValue(propertyName, value);
}

template <typename TInterface, typename ... Interfaces>
ErrCode DeviceInfoConfigImpl<TInterface, Interfaces...>::setOwner(IPropertyObject* newOwner)
{
    ErrCode err = Super::setOwner(newOwner);
    if (!coreEvent.assigned() && newOwner != nullptr)
    {
        ComponentPtr parent = newOwner;
        parent.getContext()->getOnCoreEvent(&this->coreEvent);
        ProcedurePtr procedure = [this](const CoreEventArgsPtr& args) { this->triggerCoreEventMetod(args); };
        this->setCoreEventTrigger(procedure);
        this->coreEventMuted = false;
    }
    return err;
}

template <typename TInterface, typename ... Interfaces>
void DeviceInfoConfigImpl<TInterface, Interfaces...>::triggerCoreEventMetod(const CoreEventArgsPtr& args)
{
    const ComponentPtr parent = this->owner.assigned() ? this->owner.getRef() : nullptr;
    try
    {
        if (parent.assigned())
            this->coreEvent(parent, args);
    }
    catch (...)
    {
        const auto loggerComponent = parent.getContext().getLogger().getOrAddComponent("DeviceInfo");
        LOG_W("Device info failed while triggering core event {}", args.getEventName());
    }
}

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(DeviceInfoConfigBase)

END_NAMESPACE_OPENDAQ
