#include <opendaq/device_capability_impl.h>


BEGIN_NAMESPACE_OPENDAQ


DeviceCapabilityImpl::DeviceCapabilityImpl( ProtocolType protocolType,
                                            ConnectionType connectionType,
                                            const StringPtr& connectionStringPrefix,
                                            const StringPtr& host,
                                            Int port,
                                            const StringPtr& path)
    : protocolType(protocolType)
    , connectionType(connectionType)
    , connectionStringPrefix(connectionStringPrefix)
    , host(host)
    , port(port)
    , path(path)
{
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

ErrCode DeviceCapabilityImpl::getConnectionStringPrefix(IString** connectionStringPrefix)
{
    OPENDAQ_PARAM_NOT_NULL(connectionStringPrefix);
    *connectionStringPrefix = this->connectionStringPrefix.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode DeviceCapabilityImpl::getConnectionString(IString** connectionString)
{
    OPENDAQ_PARAM_NOT_NULL(connectionString);
    StringPtr result = connectionStringPrefix.toStdString() + host.toStdString();
    if (port != -1)
        result = result.toStdString() + ":" + std::to_string(port);
    if (path.assigned())
        result = result.toStdString() + path.toStdString();
    *connectionString = result.detach();
    return OPENDAQ_SUCCESS;
}

ErrCode DeviceCapabilityImpl::getHost(IString** host)
{
    OPENDAQ_PARAM_NOT_NULL(host);
    *host = this->host.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode DeviceCapabilityImpl::getPort(Int* port)
{
    OPENDAQ_PARAM_NOT_NULL(port);
    *port = this->port;
    return OPENDAQ_SUCCESS;
}

ErrCode DeviceCapabilityImpl::getPath(IString** path)
{
    OPENDAQ_PARAM_NOT_NULL(host);
    *path = this->path.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

extern "C" ErrCode PUBLIC_EXPORT createDeviceCapability(IDeviceCapability** objTmp, 
                                            ProtocolType protocolType,
                                            ConnectionType connectionType,
                                            IString* connectionStringPrefix,
                                            IString* host,
                                            Int port,
                                            IString* path)
{
    return daq::createObject<IDeviceCapability, DeviceCapabilityImpl>(objTmp, protocolType, connectionType, connectionStringPrefix, host, port, path);
}

END_NAMESPACE_OPENDAQ
