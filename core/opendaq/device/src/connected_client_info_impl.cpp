#include <opendaq/connected_client_info_impl.h>

BEGIN_NAMESPACE_OPENDAQ

const char* ClientAddress = "Address";
const char* ClientProtocolTypeName = "ProtocolType";
const char* ClientProtocolName = "ProtocolName";
const char* ClientTypeName = "ClientType";
const char* ClientHostName = "HostName";

StringPtr ConnectedClientInfoImpl::ProtocolTypeToString(ProtocolType type)
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

ProtocolType ConnectedClientInfoImpl::StringToProtocolType(const StringPtr& type)
{
    if (type == "ConfigurationAndStreaming")
        return ProtocolType::ConfigurationAndStreaming;
    if (type == "Configuration")
        return ProtocolType::Configuration;
    if (type == "Streaming")
        return ProtocolType::Streaming;
    return ProtocolType::Unknown;
}

template <typename T>
typename InterfaceToSmartPtr<T>::SmartPtr ConnectedClientInfoImpl::getTypedProperty(const StringPtr& name)
{
    return objPtr.getPropertyValue(name).template asPtr<T>();
}

ConnectedClientInfoImpl::ConnectedClientInfoImpl()
    : Super()
{
    Super::addProperty(StringProperty(ClientAddress, ""));
    Super::addProperty(StringProperty(ClientProtocolTypeName, ""));
    Super::addProperty(StringProperty(ClientProtocolName, ""));
    Super::addProperty(StringProperty(ClientTypeName, ""));
    Super::addProperty(StringProperty(ClientHostName, ""));
}

ConnectedClientInfoImpl::ConnectedClientInfoImpl(const StringPtr& address,
                                                 ProtocolType protocolType,
                                                 const StringPtr& protocolName,
                                                 const StringPtr& clientType,
                                                 const StringPtr& hostName)
    : ConnectedClientInfoImpl()
{
    Super::setPropertyValue(String(ClientAddress), address);
    Super::setPropertyValue(String(ClientProtocolTypeName), ProtocolTypeToString(protocolType));
    Super::setPropertyValue(String(ClientProtocolName), protocolName);
    Super::setPropertyValue(String(ClientTypeName), clientType);
    Super::setPropertyValue(String(ClientHostName), hostName);
}

ErrCode ConnectedClientInfoImpl::getAddress(IString** address)
{
    OPENDAQ_PARAM_NOT_NULL(address);

    return daqTry([&]()
    {
        *address = getTypedProperty<IString>(ClientAddress).detach();
        return OPENDAQ_SUCCESS;
    });
}

ErrCode ConnectedClientInfoImpl::getProtocolType(ProtocolType* type)
{
    return daqTry([&]
    {
        *type = StringToProtocolType(getTypedProperty<IString>(ClientProtocolTypeName));
        return OPENDAQ_SUCCESS;
    });
}

ErrCode ConnectedClientInfoImpl::getProtocolName(IString** protocolName)
{
    return daqTry([&]
    {
        *protocolName = getTypedProperty<IString>(ClientProtocolName).detach();
        return OPENDAQ_SUCCESS;
    });
}

ErrCode ConnectedClientInfoImpl::getClientTypeName(IString** type)
{
    return daqTry([&]
    {
        *type = getTypedProperty<IString>(ClientTypeName).detach();
        return OPENDAQ_SUCCESS;
    });
}

ErrCode ConnectedClientInfoImpl::getHostName(IString** hostName)
{
    OPENDAQ_PARAM_NOT_NULL(hostName);

    return daqTry([&]()
    {
        *hostName = getTypedProperty<IString>(ClientHostName).detach();
        return OPENDAQ_SUCCESS;
    });
}

ErrCode ConnectedClientInfoImpl::getInterfaceIds(SizeT* idCount, IntfID** ids)
{
    if (idCount == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

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

ErrCode ConnectedClientInfoImpl::getSerializeId(ConstCharPtr* id) const
{
    *id = SerializeId();

    return OPENDAQ_SUCCESS;
}

ConstCharPtr ConnectedClientInfoImpl::SerializeId()
{
    return "ConnectedClientInfo";
}

ErrCode ConnectedClientInfoImpl::Deserialize(ISerializedObject* serialized,
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
                       [](const SerializedObjectPtr& /*serialized*/, const BaseObjectPtr& /*context*/, const StringPtr& /*className*/)
                       {
                           const auto clientInfo =
                               createWithImplementation<IConnectedClientInfo, ConnectedClientInfoImpl>();
                           return clientInfo;
                       }).detach();
        });
}

ErrCode ConnectedClientInfoImpl::clone(IPropertyObject** cloned)
{
    OPENDAQ_PARAM_NOT_NULL(cloned);

    auto obj = createWithImplementation<IConnectedClientInfo, ConnectedClientInfoImpl>();

    return daqTry([this, &obj, &cloned]()
    {
        auto implPtr = static_cast<ConnectedClientInfoImpl*>(obj.getObject());
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

#if !defined(BUILDING_STATIC_LIBRARY)

extern "C"
    ErrCode PUBLIC_EXPORT createConnectedClientInfo(IConnectedClientInfo** objTmp)
{
    return daq::createObject<IConnectedClientInfo, ConnectedClientInfoImpl>(objTmp);
}

extern "C"
    ErrCode PUBLIC_EXPORT createConnectedClientInfoWithParams(IConnectedClientInfo** objTmp,
                                                              IString* address,
                                                              ProtocolType protocolType,
                                                              IString* protocolName,
                                                              IString* clientType,
                                                              IString* hostName)
{
    return daq::createObject<IConnectedClientInfo, ConnectedClientInfoImpl>(objTmp, address, protocolType, protocolName, clientType, hostName);
}

#endif

END_NAMESPACE_OPENDAQ
