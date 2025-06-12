#include <opendaq/server_capability_impl.h>

BEGIN_NAMESPACE_OPENDAQ

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
