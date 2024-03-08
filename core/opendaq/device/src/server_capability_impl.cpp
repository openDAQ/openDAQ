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
    : connectionString(connectionString)
    , protocolName(protocolName)
    , protocolType(Enumeration(EnumerationName, protocolType, getTypeManager()))
    , connectionType(connectionType)
{
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

extern "C" ErrCode PUBLIC_EXPORT createServerCapability(IServerCapability** objTmp, 
                                            IString* connectionString,
                                            IString* protocolName,
                                            IString* protocolType, 
                                            IString* connectionType)
{
    return daq::createObject<IServerCapability, ServerCapabilityImpl>(objTmp, connectionString, protocolName, protocolType, connectionType);
}

END_NAMESPACE_OPENDAQ
