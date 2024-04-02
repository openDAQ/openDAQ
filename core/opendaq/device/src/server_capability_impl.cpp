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
const char* PrimaryAddress = "address";

StringPtr ServerCapabilityConfigImpl::ProtocolTypeToString(ProtocolType type)
{
    switch (type)
    {
        case(ProtocolType::StructureAndStreaming):
            return "Structure&Streaming";
        case(ProtocolType::Structure):
            return "Structure";
        case(ProtocolType::Streaming):
            return "Streaming";
        default:
            return "Unknown";
    }
}

ProtocolType ServerCapabilityConfigImpl::StringToProtocolType(const StringPtr& type)
{
    if (type == "Structure&Streaming")
        return ProtocolType::StructureAndStreaming;
    if (type == "Structure")
        return ProtocolType::Structure; 
    if (type == "Streaming")
        return ProtocolType::Streaming;
    return ProtocolType::StructureAndStreaming; 
}

ServerCapabilityConfigImpl::ServerCapabilityConfigImpl(const StringPtr& protocolName, ProtocolType protocolType)
    : Super()
{
    Super::addProperty(StringProperty(PrimaryConnectionString, ""));
    Super::addProperty(ListProperty(ConnectionStrings, List<IString>()));
    Super::addProperty(StringProperty(ProtocolName, protocolName));
    Super::addProperty(StringProperty(ProtocolTypeName, ProtocolTypeToString(protocolType)));
    Super::addProperty(StringProperty(ConnectionType, "Unknwown"));
    Super::addProperty(BoolProperty(CoreEventsEnabled, false));
    Super::addProperty(StringProperty(ProtocolId, ""));
    Super::addProperty(StringProperty(PrimaryAddress, ""));
}

template <typename T>
typename InterfaceToSmartPtr<T>::SmartPtr ServerCapabilityConfigImpl::getTypedProperty(const StringPtr& name)
{
    const auto obj = this->template borrowPtr<PropertyObjectPtr>();
    return obj.getPropertyValue(name).template asPtr<T>();
}

ErrCode ServerCapabilityConfigImpl::getPrimaryConnectionString(IString** connectionString)
{
    return daqTry([&]() {
        *connectionString = getTypedProperty<IString>(PrimaryConnectionString).detach();
        return OPENDAQ_SUCCESS;
    });
}

ErrCode ServerCapabilityConfigImpl::setPrimaryConnectionString(IString* connectionString)
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
        {
            checkErrorInfo(Super::setPropertyValue(String(PrimaryConnectionString), connectionString));
        }
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

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, ServerCapabilityConfig,
    IServerCapabilityConfig, createServerCapability,
    IString*, protocolName,
    ProtocolType, protocolType
)

END_NAMESPACE_OPENDAQ
