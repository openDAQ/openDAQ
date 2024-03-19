#include <opendaq/server_capability_impl.h>
#include <coretypes/type_manager_factory.h>
#include <coretypes/enumeration_type_factory.h>
#include <coretypes/enumeration_factory.h>


BEGIN_NAMESPACE_OPENDAQ

const char* EnumerationName = "ProtocolType_v1";

const char* ConnectionString = "ConnectionString";
const char* ProtocolName = "ProtocolName";
const char* ConnectionType = "ConnectionType";
const char* UpdateMethod = "UpdateMethod"; 
const char* ProtocolId = "ProtocolId"; 
const char* PrimaryAddress = "Address";

TypeManagerPtr ServerCapabilityImpl::GetTypeManager(const ContextPtr& context)
{
    auto typeManager = context.getTypeManager();

    if (typeManager == nullptr)
        throw ArgumentNullException("ServerCapability got context with not assigned type manager");

    // NOTE: values are set based on priority
    // NOTE: please update version of EnumerationName if you are changing list of values in EnumerationType
    if (!typeManager.hasType(EnumerationName))
    {
        auto enumeration = EnumerationType(EnumerationName, List<IString>("Structure&Streaming", "Structure", "Streaming", "ServerStreaming", "Unknown"));
        typeManager.addType(enumeration);
    }
    return typeManager;
}

ServerCapabilityImpl::ServerCapabilityImpl( const ContextPtr& context,
                                            const StringPtr& connectionString,
                                            const StringPtr& protocolName,
                                            const StringPtr& protocolType,
                                            const StringPtr& connectionType,
                                            ClientUpdateMethod updateMethod
                                            )
    : Super()
    , typeManager(GetTypeManager(context))
    , protocolType(Enumeration(EnumerationName, protocolType, typeManager))
{
    Super::addProperty(StringPropertyBuilder(ConnectionString, connectionString).setReadOnly(true).build());
    Super::addProperty(StringPropertyBuilder(ProtocolName, protocolName).setReadOnly(true).build());
    Super::addProperty(StringPropertyBuilder(ConnectionType, connectionType).setReadOnly(true).build());
    Super::addProperty(IntPropertyBuilder(UpdateMethod, (Int)updateMethod).setReadOnly(true).build());
}

ServerCapabilityImpl::ServerCapabilityImpl(const ContextPtr& context, const StringPtr& protocolId)
    : Super()
    , typeManager(GetTypeManager(context))
    , protocolType(Enumeration(EnumerationName, "ServerStreaming", typeManager))
{
    Super::addProperty(StringPropertyBuilder(ProtocolId, protocolId).setReadOnly(true).build());
    Super::addProperty(StringProperty(PrimaryAddress, ""));
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
        *connectionString = getTypedProperty<IString>(ConnectionString).detach();
        return OPENDAQ_SUCCESS;
    });
}

ErrCode ServerCapabilityImpl::getProtocolName(IString** protocolName)
{
    return daqTry([&]() {
        *protocolName = getTypedProperty<IString>(ProtocolName).detach();
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
        *type = getTypedProperty<IString>(ConnectionType).detach();
        return OPENDAQ_SUCCESS;
    });
}

ErrCode ServerCapabilityImpl::getUpdateMethod(ClientUpdateMethod* method)
{
    return daqTry([&]() {
        *method = ClientUpdateMethod(getTypedProperty<IInteger>(UpdateMethod));
        return OPENDAQ_SUCCESS;
    });
}

extern "C" ErrCode PUBLIC_EXPORT createServerCapability(IServerCapability** objTmp, 
                                            IContext* context,
                                            IString* connectionString,
                                            IString* protocolName,
                                            IString* protocolType, 
                                            IString* connectionType,
                                            ClientUpdateMethod updateMethod)
{
    return daq::createObject<IServerCapability, ServerCapabilityImpl>(objTmp, context, connectionString, protocolName, protocolType, connectionType, updateMethod);
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, ServerCapability,
    IServerCapability, createServerStreamingCapability,
    IContext*, context,
    IString*, protocolId
)

END_NAMESPACE_OPENDAQ
