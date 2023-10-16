#include "opcuashared/opcuaendpoint.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA

OpcUaEndpoint::OpcUaEndpoint(const std::string& name, const std::string& url)
    : name(name)
    , url(url)
{
}

OpcUaEndpoint::OpcUaEndpoint()
{
}

void OpcUaEndpoint::setName(const std::string& name)
{
    this->name = name;
}

const std::string& OpcUaEndpoint::getName() const
{
    return name;
}

void OpcUaEndpoint::setUrl(const std::string& url)
{
    this->url = url;
}

const std::string& OpcUaEndpoint::getUrl() const
{
    return url;
}

void OpcUaEndpoint::setSecurityConfig(OpcUaClientSecurityConfig* securityConfig)
{
    if (securityConfig == NULL)
        this->securityConfig.reset();
    else
        this->securityConfig = *securityConfig;
}

const OpcUaClientSecurityConfig* OpcUaEndpoint::getSecurityConfig() const
{
    return this->securityConfig.has_value() ? &this->securityConfig.value() : NULL;
}

const UA_DataTypeArray* OpcUaEndpoint::getCustomDataTypes()const
{
    return customDataTypeList.getCustomDataTypes();
}

void OpcUaEndpoint::registerCustomTypes(const size_t typesSize, const UA_DataType* types)
{
    customDataTypeList.add(typesSize, types);
    
}

void OpcUaEndpoint::setLogLevel(const UA_LogLevel logLevel)
{
    this->logLevel = logLevel;
}

UA_LogLevel OpcUaEndpoint::getLogLevel() const
{
    return this->logLevel;
}

END_NAMESPACE_OPENDAQ_OPCUA
