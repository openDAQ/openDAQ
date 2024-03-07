#include <opendaq/device_capability_impl.h>


BEGIN_NAMESPACE_OPENDAQ


DeviceCapabilityImpl::DeviceCapabilityImpl( const StringPtr& connectionString,
                                            const StringPtr& protocolName,
                                            ProtocolType protocolType,
                                            ConnectionType connectionType
                                            )
    : connectionString(connectionString)
    , protocolName(protocolName)
    , protocolType(protocolType)
    , connectionType(connectionType)
{
}

ErrCode DeviceCapabilityImpl::getConnectionString(IString** connectionString)
{
    OPENDAQ_PARAM_NOT_NULL(connectionString);
    *connectionString = this->connectionString.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode DeviceCapabilityImpl::getProtocolName(IString** protocolName)
{
    OPENDAQ_PARAM_NOT_NULL(protocolName);
    *protocolName = this->protocolName.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode DeviceCapabilityImpl::getProtocolType(ProtocolType* type)
{
    OPENDAQ_PARAM_NOT_NULL(type);
    *type = protocolType;
    return OPENDAQ_SUCCESS;
}

ErrCode DeviceCapabilityImpl::getConnectionType(ConnectionType* type)
{
    OPENDAQ_PARAM_NOT_NULL(type);
    *type = connectionType;
    return OPENDAQ_SUCCESS;
}

extern "C" ErrCode PUBLIC_EXPORT createDeviceCapability(IDeviceCapability** objTmp, 
                                            IString* connectionString,
                                            IString* protocolName,
                                            ProtocolType protocolType, 
                                            ConnectionType connectionType)
{
    return daq::createObject<IDeviceCapability, DeviceCapabilityImpl>(objTmp, connectionString, protocolName, protocolType, connectionType);
}

END_NAMESPACE_OPENDAQ
