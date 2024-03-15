#include <opendaq/server_capability_impl.h>
#include <coretypes/type_manager_factory.h>
#include <coretypes/enumeration_type_factory.h>
#include <coretypes/enumeration_factory.h>


BEGIN_NAMESPACE_OPENDAQ

const char* EnumerationName = "ProtocolType_v1";

TypeManagerPtr ServerCapabilityImpl::getTypeManager()
{
    static auto typeManager = TypeManager();
    // NOTE: values are set based on priority
    // NOTE: please update version of EnumerationName if you are changing list of values in EnumerationType
    static auto enumeration = EnumerationType(EnumerationName, List<IString>("Structure&Streaming", "Structure", "Streaming", "ServerStreaming", "Unknown"));

    static bool enumerationAdded = false;
    if (!enumerationAdded)
    {
        typeManager.addType(enumeration);
        enumerationAdded = true;
    }
    return typeManager;
}

ServerCapabilityImpl::ServerCapabilityImpl( const StringPtr& connectionString,
                                            const StringPtr& protocolName,
                                            const StringPtr& protocolType,
                                            const StringPtr& connectionType,
                                            ClientUpdateMethod updateMethod
                                            )
    : Super()
    , protocolType(Enumeration(EnumerationName, protocolType, getTypeManager()))
{
    Super::addProperty(StringPropertyBuilder("connectionString", connectionString).setReadOnly(true).build());
    Super::addProperty(StringPropertyBuilder("protocolName", protocolName).setReadOnly(true).build());
    Super::addProperty(StringPropertyBuilder("connectionType", connectionType).setReadOnly(true).build());
    Super::addProperty(IntPropertyBuilder("updateMethod", (Int)updateMethod).setReadOnly(true).build());
}

ServerCapabilityImpl::ServerCapabilityImpl(const StringPtr& protocolId)
    : Super()
    , protocolType(Enumeration(EnumerationName, "ServerStreaming", getTypeManager()))
{
    Super::addProperty(StringPropertyBuilder("protocolId", protocolId).setReadOnly(true).build());
    Super::addProperty(StringProperty("address", ""));
}

template <typename T>
typename InterfaceToSmartPtr<T>::SmartPtr ServerCapabilityImpl::getTypedProperty(const StringPtr& name)
{
    const auto obj = this->template borrowPtr<PropertyObjectPtr>();
    return obj.getPropertyValue(name).template asPtr<T>();
}

ErrCode ServerCapabilityImpl::getConnectionString(IString** connectionString)
{
    return daqTry([&]() {
        *connectionString = getTypedProperty<IString>("connectionString").detach();
        return OPENDAQ_SUCCESS;
    });
}

ErrCode ServerCapabilityImpl::getProtocolName(IString** protocolName)
{
    return daqTry([&]() {
        *protocolName = getTypedProperty<IString>("protocolName").detach();
        return OPENDAQ_SUCCESS;
    });
}

ErrCode ServerCapabilityImpl::getProtocolType(IEnumeration** type)
{
    OPENDAQ_PARAM_NOT_NULL(type);
    *type = protocolType.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode ServerCapabilityImpl::getConnectionType(IString** type)
{
    return daqTry([&]() {
        *type = getTypedProperty<IString>("connectionType").detach();
        return OPENDAQ_SUCCESS;
    });
}

ErrCode ServerCapabilityImpl::getUpdateMethod(ClientUpdateMethod* method)
{
    return daqTry([&]() {
        *method = ClientUpdateMethod(getTypedProperty<IInteger>("updateMethod"));
        return OPENDAQ_SUCCESS;
    });
}

extern "C" ErrCode PUBLIC_EXPORT createServerCapability(IServerCapability** objTmp, 
                                            IString* connectionString,
                                            IString* protocolName,
                                            IString* protocolType, 
                                            IString* connectionType,
                                            ClientUpdateMethod updateMethod)
{
    return daq::createObject<IServerCapability, ServerCapabilityImpl>(objTmp, connectionString, protocolName, protocolType, connectionType, updateMethod);
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, ServerCapability,
    IServerCapability, createServerStreamingCapability,
    IString*, protocolId
)

END_NAMESPACE_OPENDAQ
