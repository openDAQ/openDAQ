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
const char* ProtocolId = "protocolId"; 
const char* PrimaryAddress = "address";

TypeManagerPtr ServerCapabilityConfigImpl::GetTypeManager(const ContextPtr& context)
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

ServerCapabilityConfigImpl::ServerCapabilityConfigImpl( const ContextPtr& context,
                                            const StringPtr& protocolName,
                                            const StringPtr& protocolType
                                            )
    : Super()
    , typeManager(GetTypeManager(context))
    , protocolType(Enumeration(EnumerationName, protocolType, typeManager))
{
    Super::addProperty(StringProperty(ConnectionString, ""));
    Super::addProperty(StringProperty(ProtocolName, protocolName));
    Super::addProperty(StringProperty(ConnectionType, "Unknwown"));
    Super::addProperty(BoolProperty(UpdateMethod, false));
    Super::addProperty(StringProperty(ProtocolId, ""));
}

ServerCapabilityConfigImpl::ServerCapabilityConfigImpl(const ContextPtr& context, const StringPtr& protocolId)
    : Super()
    , typeManager(GetTypeManager(context))
    , protocolType(Enumeration(EnumerationName, "ServerStreaming", typeManager))
{
    Super::addProperty(StringPropertyBuilder(ProtocolId, protocolId).setReadOnly(true).build());
    Super::addProperty(StringProperty(PrimaryAddress, ""));
}

template <typename T>
typename InterfaceToSmartPtr<T>::SmartPtr ServerCapabilityConfigImpl::getTypedProperty(const StringPtr& name)
{
    const auto obj = this->template borrowPtr<PropertyObjectPtr>();
    return obj.getPropertyValue(name).template asPtr<T>();
}

ErrCode ServerCapabilityConfigImpl::getConnectionString(IString** connectionString)
{
    return daqTry([&]() {
        *connectionString = getTypedProperty<IString>(ConnectionString).detach();
        return OPENDAQ_SUCCESS;
    });
}

ErrCode ServerCapabilityConfigImpl::setConnectionString(IString* connectionString)
{
    OPENDAQ_PARAM_NOT_NULL(connectionString);
    return Super::setPropertyValue(String(ConnectionString), connectionString);
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

ErrCode ServerCapabilityConfigImpl::getProtocolType(IEnumeration** type)
{
    OPENDAQ_PARAM_NOT_NULL(type);
    *type = protocolType.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode ServerCapabilityConfigImpl::setProtocolType(IString* type)
{
    OPENDAQ_PARAM_NOT_NULL(type);
    protocolType = Enumeration(EnumerationName, type, typeManager);
    return OPENDAQ_SUCCESS;
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
        *enabled = getTypedProperty<IBoolean>(UpdateMethod);
        return OPENDAQ_SUCCESS;
    });
}

ErrCode ServerCapabilityConfigImpl::setCoreEventsEnabled(Bool enabled)
{
    return Super::setPropertyValue(String(UpdateMethod), BooleanPtr(enabled));
}

ErrCode ServerCapabilityConfigImpl::getSerializeId(ConstCharPtr* id) const
{
    *id = SerializeId();

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
                           const auto cap = createWithImplementation<IServerCapability, ServerCapabilityConfigImpl>();
                           return cap;
                       }).detach();
        });
}

extern "C" ErrCode PUBLIC_EXPORT createServerCapability(IServerCapabilityConfig** objTmp,
                                                        IContext* context,
                                                        IString* protocolName,
                                                        IString* protocolType)
{
    return daq::createObject<IServerCapabilityConfig, ServerCapabilityConfigImpl>(objTmp, context, protocolName, protocolType);
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, ServerCapabilityConfig,
    IServerCapabilityConfig, createServerStreamingCapability,
    IContext*, context,
    IString*, protocolId
)

END_NAMESPACE_OPENDAQ
