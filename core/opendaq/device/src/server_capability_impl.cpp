#include <opendaq/server_capability_impl.h>
#include <coretypes/type_manager_factory.h>
#include <coretypes/enumeration_type_factory.h>
#include <coretypes/enumeration_factory.h>


BEGIN_NAMESPACE_OPENDAQ

const char* EnumerationName = "ProtocolType_v1";

const char* PrimaryConnectionString = "PrimaryConnectionString";
const char* ConnectionStrings = "ConnectionStrings";
const char* ProtocolName = "ProtocolName";
const char* ProtocolTypeName = "ProtocolType";
const char* ConnectionType = "ConnectionType";
const char* CoreEventsEnabled = "CoreEventsEnabled"; 
const char* ProtocolId = "protocolId";
const char* Prefix = "prefix";
const char* Addresses = "Addresses";
const char* Port = "Port";

StringPtr ServerCapabilityConfigImpl::ProtocolTypeToString(ProtocolType type)
{
    switch (type)
    {
        case(ProtocolType::Configuration):
            return "Configuration";
        case(ProtocolType::Streaming):
            return "Streaming";
        case(ProtocolType::ConfigurationAndStreaming):
            return "ConfigurationAndStreaming";
        case ProtocolType::Unknown:
            return "Unknown";
    }
    return "Unknown";
}

ProtocolType ServerCapabilityConfigImpl::StringToProtocolType(const StringPtr& type)
{
    if (type == "ConfigurationAndStreaming")
        return ProtocolType::ConfigurationAndStreaming;
    if (type == "Configuration")
        return ProtocolType::Configuration; 
    if (type == "Streaming")
        return ProtocolType::Streaming;
    return ProtocolType::Unknown; 
}

ServerCapabilityConfigImpl::ServerCapabilityConfigImpl(const StringPtr& protocolId, const StringPtr& protocolName, ProtocolType protocolType)
    : Super()
{
    Super::addProperty(StringProperty(PrimaryConnectionString, ""));
    Super::addProperty(ListProperty(ConnectionStrings, List<IString>()));
    Super::addProperty(StringProperty(ProtocolName, ""));
    Super::addProperty(StringProperty(ProtocolId, ""));
    Super::addProperty(StringProperty(ProtocolTypeName, ProtocolTypeToString(ProtocolType::Unknown)));
    Super::addProperty(StringProperty(ConnectionType, "Unknown"));
    Super::addProperty(BoolProperty(CoreEventsEnabled, false));
    Super::addProperty(StringProperty(Prefix, ""));
    Super::addProperty(ListProperty(Addresses, List<IString>()));
    Super::addProperty(IntProperty(Port, -1));

    Super::setPropertyValue(String(ProtocolId), protocolId);
    Super::setPropertyValue(String(ProtocolName), protocolName);
    Super::setPropertyValue(String(ProtocolTypeName), ProtocolTypeToString(protocolType));
}

template <typename T>
typename InterfaceToSmartPtr<T>::SmartPtr ServerCapabilityConfigImpl::getTypedProperty(const StringPtr& name)
{
    return objPtr.getPropertyValue(name).template asPtr<T>();
}

ErrCode ServerCapabilityConfigImpl::getConnectionString(IString** connectionString)
{
    return daqTry([&]() {
        *connectionString = getTypedProperty<IString>(PrimaryConnectionString).detach();
        return OPENDAQ_SUCCESS;
    });
}

ErrCode ServerCapabilityConfigImpl::setConnectionString(IString* connectionString)
{
    OPENDAQ_PARAM_NOT_NULL(connectionString);
    return daqTry([&]() {
        checkErrorInfo(Super::setPropertyValue(String(PrimaryConnectionString), connectionString));
        checkErrorInfo(addConnectionString(connectionString));
        return OPENDAQ_SUCCESS;
    });
}

ErrCode ServerCapabilityConfigImpl::getConnectionStrings(IList** connectionStrings)
{
    return daqTry([&]() {
        *connectionStrings = getTypedProperty<IList>(ConnectionStrings).detach();
        return OPENDAQ_SUCCESS;
    });
}

ErrCode ServerCapabilityConfigImpl::addConnectionString(IString* connectionString)
{
    OPENDAQ_PARAM_NOT_NULL(connectionString);
    return daqTry([&]() {
        ListPtr<IString> connectionStrings = getTypedProperty<IList>(ConnectionStrings);
        connectionStrings.pushBack(connectionString);
        checkErrorInfo(Super::setPropertyValue(String(ConnectionStrings), connectionStrings));

        if (connectionStrings.getCount() == 1)
            checkErrorInfo(Super::setPropertyValue(String(PrimaryConnectionString), connectionString));

        return OPENDAQ_SUCCESS;
    });
}

ErrCode ServerCapabilityConfigImpl::getProtocolName(IString** protocolName)
{
    return daqTry([&]() {
        *protocolName = getTypedProperty<IString>(ProtocolName).detach();
        return OPENDAQ_SUCCESS;
    });
}

ErrCode ServerCapabilityConfigImpl::setProtocolName(IString* protocolName)
{
    OPENDAQ_PARAM_NOT_NULL(protocolName);
    return Super::setPropertyValue(String(ProtocolName), protocolName);
}

ErrCode ServerCapabilityConfigImpl::getProtocolId(IString** protocolId)
{
    return daqTry([&]() {
        *protocolId = getTypedProperty<IString>(ProtocolId).detach();
        return OPENDAQ_SUCCESS;
    });
}

ErrCode ServerCapabilityConfigImpl::setProtocolId(IString* protocolId)
{
    OPENDAQ_PARAM_NOT_NULL(protocolId);
    return Super::setPropertyValue(String(ProtocolId), protocolId);
}

ErrCode ServerCapabilityConfigImpl::getProtocolType(ProtocolType* type)
{
    return daqTry([&]() {
        *type = StringToProtocolType(getTypedProperty<IString>(ProtocolTypeName));
        return OPENDAQ_SUCCESS;
    });
}

ErrCode ServerCapabilityConfigImpl::setProtocolType(ProtocolType type)
{
    return Super::setPropertyValue(String(ProtocolTypeName), ProtocolTypeToString(type));
}

ErrCode ServerCapabilityConfigImpl::getPrefix(IString** prefix)
{
    return daqTry([&]() {
        *prefix = getTypedProperty<IString>(Prefix).detach();
        return OPENDAQ_SUCCESS;
    });
}

ErrCode ServerCapabilityConfigImpl::setPrefix(IString* prefix)
{
    return Super::setPropertyValue(String(Prefix), prefix);
}

ErrCode ServerCapabilityConfigImpl::getConnectionType(IString** type)
{
    return daqTry([&]() {
        *type = getTypedProperty<IString>(ConnectionType).detach();
        return OPENDAQ_SUCCESS;
    });
}
ErrCode ServerCapabilityConfigImpl::setConnectionType(IString* type)
{
    OPENDAQ_PARAM_NOT_NULL(type);
    return Super::setPropertyValue(String(ConnectionType), type);
}

ErrCode ServerCapabilityConfigImpl::getCoreEventsEnabled(Bool* enabled)
{
    return daqTry([&]() {
        *enabled = getTypedProperty<IBoolean>(CoreEventsEnabled);
        return OPENDAQ_SUCCESS;
    });
}

ErrCode ServerCapabilityConfigImpl::setCoreEventsEnabled(Bool enabled)
{
    return Super::setPropertyValue(String(CoreEventsEnabled), BooleanPtr(enabled));
}

ErrCode ServerCapabilityConfigImpl::getSerializeId(ConstCharPtr* id) const
{
    *id = SerializeId();

    return OPENDAQ_SUCCESS;
}

ErrCode ServerCapabilityConfigImpl::getAddresses(IList** addresses)
{
    return daqTry([&]() {
        *addresses = getTypedProperty<IList>(Addresses).detach();
        return OPENDAQ_SUCCESS;
    });
}

ErrCode ServerCapabilityConfigImpl::addAddress(IString* address)
{
    OPENDAQ_PARAM_NOT_NULL(address);
    return daqTry([&]() {
        ListPtr<IString> addresses = getTypedProperty<IList>(Addresses);
        addresses.pushBack(address);
        checkErrorInfo(Super::setPropertyValue(String(Addresses), addresses));
        return OPENDAQ_SUCCESS;
    });
}

ErrCode ServerCapabilityConfigImpl::getInterfaceIds(SizeT* idCount, IntfID** ids)
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

ConstCharPtr ServerCapabilityConfigImpl::SerializeId()
{
    return "ServerCapability";
}

ErrCode ServerCapabilityConfigImpl::Deserialize(ISerializedObject* serialized,
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
                           const auto cap = createWithImplementation<IServerCapability, ServerCapabilityConfigImpl>("", "", ProtocolType::Unknown);
                           return cap;
                       }).detach();
        });
}

PropertyObjectPtr ServerCapabilityConfigImpl::createCloneBase()
{
    const auto obj = createWithImplementation<IServerCapability, ServerCapabilityConfigImpl>("", "", ProtocolType::Unknown);
    return obj;
}

ErrCode ServerCapabilityConfigImpl::getPort(IInteger** port)
{
    OPENDAQ_PARAM_NOT_NULL(port);
    
    return daqTry([&]() {
        *port = getTypedProperty<IInteger>(Port).detach();
        return OPENDAQ_SUCCESS;
    });
}

ErrCode ServerCapabilityConfigImpl::setPort(IInteger* port)
{
    return Super::setPropertyValue(String(Port), port);
}

#if !defined(BUILDING_STATIC_LIBRARY)

extern "C"
ErrCode PUBLIC_EXPORT createServerCapability(IServerCapabilityConfig** objTmp,
                                             IString* protocolId,
                                             IString* protocolName,
                                             ProtocolType protocolType)
{
    return daq::createObject<IServerCapabilityConfig, ServerCapabilityConfigImpl>(objTmp, protocolId, protocolName, protocolType);
}

#endif

END_NAMESPACE_OPENDAQ
