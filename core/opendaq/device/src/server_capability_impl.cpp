#include <opendaq/server_capability_impl.h>
#include <coretypes/type_manager_factory.h>
#include <coretypes/enumeration_type_factory.h>
#include <coretypes/enumeration_factory.h>


BEGIN_NAMESPACE_OPENDAQ

const char* EnumerationName = "ProtocolType";

TypeManagerPtr ServerCapabilityImpl::getTypeManager()
{
    static auto typeManager = TypeManager();
    return typeManager;
}
EnumerationTypePtr ServerCapabilityImpl::GetProtocolTypeEnumeration()
{
    bool enumerationAdded = false;
    auto typeManager = getTypeManager();
    static auto enumeration = EnumerationType(EnumerationName, List<IString>("Unknown", "Streaming", "Structure", "Both"));
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
    : connectionString(connectionString)
    , protocolName(protocolName)
    , connectionType(connectionType)
{
    GetProtocolTypeEnumeration();
    this->protocolType = Enumeration(EnumerationName, protocolType, getTypeManager());
}

ErrCode ServerCapabilityImpl::getConnectionString(IString** connectionString)
{
    OPENDAQ_PARAM_NOT_NULL(connectionString);
    *connectionString = this->connectionString.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode ServerCapabilityImpl::getProtocolName(IString** protocolName)
{
    OPENDAQ_PARAM_NOT_NULL(protocolName);
    *protocolName = this->protocolName.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode ServerCapabilityImpl::getProtocolType(IEnumeration** type)
{
    OPENDAQ_PARAM_NOT_NULL(type);
    *type = protocolType.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode ServerCapabilityImpl::getConnectionType(IString** type)
{
    OPENDAQ_PARAM_NOT_NULL(type);
    *type = connectionType.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode ServerCapabilityImpl::setSupportedProtocolType(IString* type)
{
    OPENDAQ_PARAM_NOT_NULL(type);
    return OPENDAQ_SUCCESS;
}

ErrCode ServerCapabilityImpl::setProtocolType(IString* type)
{
    OPENDAQ_PARAM_NOT_NULL(type);
    return OPENDAQ_SUCCESS;
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
