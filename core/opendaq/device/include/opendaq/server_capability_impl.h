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
#include <opendaq/server_capability_config.h>
#include <coretypes/validation.h>
#include <coretypes/intfs.h>
#include <coretypes/string_ptr.h>
#include <coretypes/type_manager_ptr.h>
#include <coretypes/enumeration_type_ptr.h>
#include <coretypes/enumeration_ptr.h>
#include <coreobjects/property_object_impl.h>
#include <opendaq/context_ptr.h>
#include <opendaq/address_info_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

class ServerCapabilityConfigImpl : public GenericPropertyObjectImpl<IServerCapabilityConfig>
{
public:
    using Super = GenericPropertyObjectImpl<IServerCapabilityConfig>;

    explicit ServerCapabilityConfigImpl(const StringPtr& protocolId, const StringPtr& protocolName, ProtocolType protocolType);

    ErrCode INTERFACE_FUNC getConnectionString(IString** connectionString) override;
    ErrCode INTERFACE_FUNC setConnectionString(IString* connectionString) override;

    ErrCode INTERFACE_FUNC getConnectionStrings(IList** connectionStrings) override;
    ErrCode INTERFACE_FUNC addConnectionString(IString* connectionString) override;
        
    ErrCode INTERFACE_FUNC getProtocolName(IString** protocolName) override;
    ErrCode INTERFACE_FUNC setProtocolName(IString* protocolName) override;

    ErrCode INTERFACE_FUNC getProtocolId(IString** protocolId) override;
    ErrCode INTERFACE_FUNC setProtocolId(IString* protocolId) override;
    
    ErrCode INTERFACE_FUNC getProtocolType(ProtocolType* type) override;
    ErrCode INTERFACE_FUNC setProtocolType(ProtocolType type) override;

    ErrCode INTERFACE_FUNC getProtocolVersion(IString** version) override;
    ErrCode INTERFACE_FUNC setProtocolVersion(IString* version) override;

    ErrCode INTERFACE_FUNC getPrefix(IString** prefix) override;
    ErrCode INTERFACE_FUNC setPrefix(IString* prefix) override;
    
    ErrCode INTERFACE_FUNC getConnectionType(IString** type) override;
    ErrCode INTERFACE_FUNC setConnectionType(IString* type) override;
    
    ErrCode INTERFACE_FUNC getCoreEventsEnabled(Bool* enabled) override;
    ErrCode INTERFACE_FUNC setCoreEventsEnabled(Bool enabled) override;  

    ErrCode INTERFACE_FUNC getAddresses(IList** addresses) override;
    ErrCode INTERFACE_FUNC addAddress(IString* address) override;

    ErrCode INTERFACE_FUNC getPort(IInteger** port) override;
    ErrCode INTERFACE_FUNC setPort(IInteger* port) override;

    ErrCode INTERFACE_FUNC getAddressInfo(IList** addressesInfo) override;
    ErrCode INTERFACE_FUNC addAddressInfo(IAddressInfo* addressInfo) override;

    ErrCode INTERFACE_FUNC getInterfaceIds(SizeT* idCount, IntfID** ids) override;

    ErrCode INTERFACE_FUNC clone(IPropertyObject** cloned) override;

    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;

private:
    const char* PrimaryConnectionString = "PrimaryConnectionString";
    const char* ConnectionStrings = "ConnectionStrings";
    const char* ProtocolName = "ProtocolName";
    const char* ProtocolTypeName = "ProtocolType";
    const char* ProtocolVersonName = "ProtocolVersion";
    const char* ConnectionType = "ConnectionType";
    const char* CoreEventsEnabled = "CoreEventsEnabled"; 
    const char* ProtocolId = "protocolId";
    const char* Prefix = "Prefix";
    const char* Addresses = "Addresses";
    const char* AddressInfo = "AddressInfo";
    const char* Port = "Port";


    template <typename T>
    typename InterfaceToSmartPtr<T>::SmartPtr getTypedProperty(const StringPtr& name);

    static StringPtr ProtocolTypeToString(ProtocolType type);
    static ProtocolType StringToProtocolType(const StringPtr& type);
};


inline StringPtr ServerCapabilityConfigImpl::ProtocolTypeToString(ProtocolType type)
{
    switch (type)
    {
        case(ProtocolType::Configuration):
            return "Configuration";
        case(ProtocolType::Streaming):
            return "Streaming";
        case(ProtocolType::ConfigurationAndStreaming):
            return "ConfigurationAndStreaming";
        default:
            return "Unknown";
    }
}

inline ProtocolType ServerCapabilityConfigImpl::StringToProtocolType(const StringPtr& type)
{
    if (type == "ConfigurationAndStreaming")
        return ProtocolType::ConfigurationAndStreaming;
    if (type == "Configuration")
        return ProtocolType::Configuration; 
    if (type == "Streaming")
        return ProtocolType::Streaming;
    return ProtocolType::Unknown; 
}

inline ServerCapabilityConfigImpl::ServerCapabilityConfigImpl(const StringPtr& protocolId, const StringPtr& protocolName, ProtocolType protocolType)
    : Super()
{
    Super::addProperty(StringProperty(PrimaryConnectionString, ""));
    Super::addProperty(ListProperty(ConnectionStrings, List<IString>()));
    Super::addProperty(StringProperty(ProtocolName, ""));
    Super::addProperty(StringProperty(ProtocolId, ""));
    Super::addProperty(StringProperty(ProtocolTypeName, ProtocolTypeToString(ProtocolType::Unknown)));
    Super::addProperty(StringProperty(ProtocolVersonName, ""));
    Super::addProperty(StringProperty(ConnectionType, "Unknown"));
    Super::addProperty(BoolProperty(CoreEventsEnabled, false));
    Super::addProperty(StringProperty(Prefix, ""));
    Super::addProperty(ListProperty(Addresses, List<IString>()));
    Super::addProperty(IntProperty(Port, -1));
    Super::addProperty(ObjectProperty(AddressInfo, PropertyObject()));

    Super::setPropertyValue(String(ProtocolId), protocolId);
    Super::setPropertyValue(String(ProtocolName), protocolName);
    Super::setPropertyValue(String(ProtocolTypeName), ProtocolTypeToString(protocolType));
}

template <typename T>
typename InterfaceToSmartPtr<T>::SmartPtr ServerCapabilityConfigImpl::getTypedProperty(const StringPtr& name)
{
    return objPtr.getPropertyValue(name).template asPtr<T>();
}

inline ErrCode ServerCapabilityConfigImpl::getConnectionString(IString** connectionString)
{
    return daqTry([&]
    {
        *connectionString = getTypedProperty<IString>(PrimaryConnectionString).detach();
        return OPENDAQ_SUCCESS;
    });
}

inline ErrCode ServerCapabilityConfigImpl::setConnectionString(IString* connectionString)
{
    OPENDAQ_PARAM_NOT_NULL(connectionString);
    return daqTry([&]
    {
        checkErrorInfo(Super::setPropertyValue(String(PrimaryConnectionString), connectionString));
        checkErrorInfo(addConnectionString(connectionString));
        return OPENDAQ_SUCCESS;
    });
}

inline ErrCode ServerCapabilityConfigImpl::getConnectionStrings(IList** connectionStrings)
{
    return daqTry([&]
    {
        *connectionStrings = getTypedProperty<IList>(ConnectionStrings).detach();
        return OPENDAQ_SUCCESS;
    });
}

inline ErrCode ServerCapabilityConfigImpl::addConnectionString(IString* connectionString)
{
    OPENDAQ_PARAM_NOT_NULL(connectionString);
    return daqTry([&]
    {
        ListPtr<IString> connectionStrings = getTypedProperty<IList>(ConnectionStrings);
        connectionStrings.pushBack(connectionString);
        checkErrorInfo(Super::setPropertyValue(String(ConnectionStrings), connectionStrings));

        if (connectionStrings.getCount() == 1)
            checkErrorInfo(Super::setPropertyValue(String(PrimaryConnectionString), connectionString));

        return OPENDAQ_SUCCESS;
    });
}

inline ErrCode ServerCapabilityConfigImpl::getProtocolName(IString** protocolName)
{
    return daqTry([&]
    {
        *protocolName = getTypedProperty<IString>(ProtocolName).detach();
        return OPENDAQ_SUCCESS;
    });
}

inline ErrCode ServerCapabilityConfigImpl::setProtocolName(IString* protocolName)
{
    OPENDAQ_PARAM_NOT_NULL(protocolName);
    return Super::setPropertyValue(String(ProtocolName), protocolName);
}

inline ErrCode ServerCapabilityConfigImpl::getProtocolId(IString** protocolId)
{
    return daqTry([&]
    {
        *protocolId = getTypedProperty<IString>(ProtocolId).detach();
        return OPENDAQ_SUCCESS;
    });
}

inline ErrCode ServerCapabilityConfigImpl::setProtocolId(IString* protocolId)
{
    OPENDAQ_PARAM_NOT_NULL(protocolId);
    return Super::setPropertyValue(String(ProtocolId), protocolId);
}

inline ErrCode ServerCapabilityConfigImpl::getProtocolType(ProtocolType* type)
{
    return daqTry([&]
    {
        *type = StringToProtocolType(getTypedProperty<IString>(ProtocolTypeName));
        return OPENDAQ_SUCCESS;
    });
}

inline ErrCode ServerCapabilityConfigImpl::setProtocolType(ProtocolType type)
{
    return Super::setPropertyValue(String(ProtocolTypeName), ProtocolTypeToString(type));
}

inline ErrCode ServerCapabilityConfigImpl::getProtocolVersion(IString** version)
{
    return daqTry([&]
    {
        *version = getTypedProperty<IString>(ProtocolVersonName).detach();
        return OPENDAQ_SUCCESS;
    });
}

inline ErrCode ServerCapabilityConfigImpl::setProtocolVersion(IString* version)
{
    return Super::setPropertyValue(String(ProtocolVersonName), version);
}

inline ErrCode ServerCapabilityConfigImpl::getPrefix(IString** prefix)
{
    return daqTry([&]
    {
        *prefix = getTypedProperty<IString>(Prefix).detach();
        return OPENDAQ_SUCCESS;
    });
}

inline ErrCode ServerCapabilityConfigImpl::setPrefix(IString* prefix)
{
    return Super::setPropertyValue(String(Prefix), prefix);
}

inline ErrCode ServerCapabilityConfigImpl::getConnectionType(IString** type)
{
    return daqTry([&]
    {
        *type = getTypedProperty<IString>(ConnectionType).detach();
        return OPENDAQ_SUCCESS;
    });
}
inline ErrCode ServerCapabilityConfigImpl::setConnectionType(IString* type)
{
    OPENDAQ_PARAM_NOT_NULL(type);
    return Super::setPropertyValue(String(ConnectionType), type);
}

inline ErrCode ServerCapabilityConfigImpl::getCoreEventsEnabled(Bool* enabled)
{
    return daqTry([&]
    {
        *enabled = getTypedProperty<IBoolean>(CoreEventsEnabled);
        return OPENDAQ_SUCCESS;
    });
}

inline ErrCode ServerCapabilityConfigImpl::setCoreEventsEnabled(Bool enabled)
{
    return Super::setPropertyValue(String(CoreEventsEnabled), BooleanPtr(enabled));
}

inline ErrCode ServerCapabilityConfigImpl::getSerializeId(ConstCharPtr* id) const
{
    *id = SerializeId();

    return OPENDAQ_SUCCESS;
}

inline ErrCode ServerCapabilityConfigImpl::getAddresses(IList** addresses)
{
    return daqTry([&]
    {
        *addresses = getTypedProperty<IList>(Addresses).detach();
        return OPENDAQ_SUCCESS;
    });
}

inline ErrCode ServerCapabilityConfigImpl::addAddress(IString* address)
{
    OPENDAQ_PARAM_NOT_NULL(address);
    return daqTry([&]
    {
        ListPtr<IString> addresses = getTypedProperty<IList>(Addresses);
        addresses.pushBack(address);
        checkErrorInfo(Super::setPropertyValue(String(Addresses), addresses));
        return OPENDAQ_SUCCESS;
    });
}

inline ErrCode ServerCapabilityConfigImpl::getInterfaceIds(SizeT* idCount, IntfID** ids)
{
    OPENDAQ_PARAM_NOT_NULL(idCount);

    *idCount = InterfaceIds::Count() + 1;
    if (ids == nullptr)
    {
        return OPENDAQ_SUCCESS;
    }

    **ids = IPropertyObject::Id;
    (*ids)++;

    InterfaceIds::AddInterfaceIds(*ids);
    return OPENDAQ_SUCCESS;
}

inline ConstCharPtr ServerCapabilityConfigImpl::SerializeId()
{
    return "ServerCapability";
}

inline ErrCode ServerCapabilityConfigImpl::Deserialize(ISerializedObject* serialized,
                                                IBaseObject* context,
                                                IFunction* factoryCallback,
                                                IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(obj);

    return daqTry([&obj, &serialized, &context, &factoryCallback]
    {
        *obj = Super::DeserializePropertyObject(
                serialized,
                context,
                factoryCallback,
                    [](const SerializedObjectPtr& /*serialized*/, const BaseObjectPtr& /*context*/, const StringPtr& /*className*/)
                    {
                        const auto cap = createWithImplementation<IServerCapability, ServerCapabilityConfigImpl>("", "", ProtocolType::Unknown);
                        return cap;
                    }).detach();
    });
}

inline ErrCode ServerCapabilityConfigImpl::getPort(IInteger** port)
{
    OPENDAQ_PARAM_NOT_NULL(port);
    
    return daqTry([&]
    {
        *port = getTypedProperty<IInteger>(Port).detach();
        return OPENDAQ_SUCCESS;
    });
}

inline ErrCode ServerCapabilityConfigImpl::setPort(IInteger* port)
{
    return Super::setPropertyValue(String(Port), port);
}

inline ErrCode ServerCapabilityConfigImpl::getAddressInfo(IList** addressesInfo)
{
    OPENDAQ_PARAM_NOT_NULL(addressesInfo);
    ListPtr<IAddressInfo> infos = List<IAddressInfo>();

    BaseObjectPtr obj;
    StringPtr str = "AddressInfo";
    ErrCode err = this->getPropertyValue(str, &obj);
    OPENDAQ_RETURN_IF_FAILED(err);

    const auto addressInfoPtr = obj.asPtr<IPropertyObject>();
    for (const auto& prop : addressInfoPtr.getAllProperties())
    {
        if (prop.getValueType() == ctObject)
        {
            BaseObjectPtr cap;
            err = addressInfoPtr->getPropertyValue(prop.getName(), &cap);
            OPENDAQ_RETURN_IF_FAILED(err);

            infos.pushBack(cap.detach());
        }
    }

    *addressesInfo = infos.detach();
    return OPENDAQ_SUCCESS;
}

inline ErrCode ServerCapabilityConfigImpl::addAddressInfo(IAddressInfo* addressInfo)
{
    OPENDAQ_PARAM_NOT_NULL(addressInfo);
    
    StringPtr address;
    ErrCode err = addressInfo->getAddress(&address);
    OPENDAQ_RETURN_IF_FAILED(err);

    BaseObjectPtr obj;
    StringPtr str = "AddressInfo";
    err = this->getPropertyValue(str, &obj);
    OPENDAQ_RETURN_IF_FAILED(err);

    const auto addressInfoPtr = obj.asPtr<IPropertyObject>();
    for (const auto& prop : addressInfoPtr.getAllProperties())
    {
        if (prop.getValueType() != ctObject)
            continue;

        const AddressInfoPtr addr = addressInfoPtr.getPropertyValue(prop.getName());
        if (addr.getAddress() == address)
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_DUPLICATEITEM);
    }
    std::string addressStr = address;
    addressStr.erase(std::remove_if(
            addressStr.begin(),
            addressStr.end(),
            [](char c) { return c == '/' || c == '.' || c == '[' || c == ']'; }),
            addressStr.end());
    addressInfoPtr.addProperty(ObjectProperty(addressStr, addressInfo));
    return OPENDAQ_SUCCESS;
}

inline ErrCode ServerCapabilityConfigImpl::clone(IPropertyObject** cloned)
{
    OPENDAQ_PARAM_NOT_NULL(cloned);

    auto obj = createWithImplementation<IServerCapability, ServerCapabilityConfigImpl>("", "", ProtocolType::Unknown);

    return daqTry([this, &obj, &cloned]
    {
        auto implPtr = static_cast<ServerCapabilityConfigImpl*>(obj.getObject());
        implPtr->configureClonedMembers(valueWriteEvents,
                                        valueReadEvents,
                                        endUpdateEvent,
                                        triggerCoreEvent,
                                        localProperties,
                                        propValues,
                                        customOrder,
                                        permissionManager);

        *cloned = obj.detach();
        return OPENDAQ_SUCCESS;
    });
}

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(ServerCapabilityConfigImpl)

END_NAMESPACE_OPENDAQ
