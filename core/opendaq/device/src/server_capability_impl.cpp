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
    static auto enumeration = EnumerationType(EnumerationName, List<IString>("Both", "Streaming", "Structure", "Unknown"));

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
                                            const StringPtr& connectionType
                                            )
    : Super()
    , protocolType(Enumeration(EnumerationName, protocolType, getTypeManager()))
{
    Super::addProperty(StringPropertyBuilder("connectionString", connectionString).setReadOnly(true).build());
    Super::addProperty(StringPropertyBuilder("protocolName", protocolName).setReadOnly(true).build());
    Super::addProperty(StringPropertyBuilder("connectionType", connectionType).setReadOnly(true).build());
}

StringPtr ServerCapabilityImpl::getStringProperty(const StringPtr& name)
{
    const auto obj = this->template borrowPtr<PropertyObjectPtr>();
    return obj.getPropertyValue(name).template asPtr<IString>();
}

ErrCode ServerCapabilityImpl::getConnectionString(IString** connectionString)
{
    return daqTry([&]() {
        *connectionString = getStringProperty("connectionString").detach();
        return OPENDAQ_SUCCESS;
    });
}

ErrCode ServerCapabilityImpl::getProtocolName(IString** protocolName)
{
    return daqTry([&]() {
        *protocolName = getStringProperty("protocolName").detach();
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
        *type = getStringProperty("connectionType").detach();
        return OPENDAQ_SUCCESS;
    });
}

extern "C" ErrCode PUBLIC_EXPORT createServerCapability(IServerCapability** objTmp, 
                                            IString* connectionString,
                                            IString* protocolName,
                                            IString* protocolType, 
                                            IString* connectionType)
{
    return daq::createObject<IServerCapability, ServerCapabilityImpl>(objTmp, connectionString, protocolName, protocolType, connectionType);
}

END_NAMESPACE_OPENDAQ
