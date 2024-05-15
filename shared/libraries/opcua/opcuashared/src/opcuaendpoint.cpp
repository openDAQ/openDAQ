#include "opcuashared/opcuaendpoint.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA

OpcUaEndpoint::OpcUaEndpoint(const std::string& url)
    : url(url)
{
}

OpcUaEndpoint::OpcUaEndpoint(const std::string& url, const std::string& username, const std::string& password)
    : url(url)
    , username(username)
    , password(password)
{
}

void OpcUaEndpoint::setName(const std::string& name)
{
    this->name = name;
}

const std::string OpcUaEndpoint::getName() const
{
    return name;
}

void OpcUaEndpoint::setUrl(const std::string& url)
{
    this->url = url;
}

const std::string OpcUaEndpoint::getUrl() const
{
    return url;
}

void OpcUaEndpoint::setUsername(const std::string& username)
{
    this->username = username;
}

const std::string OpcUaEndpoint::getUsername() const
{
    return username;
}

void OpcUaEndpoint::setPassword(const std::string& password)
{
    this->password = password;
}

const std::string OpcUaEndpoint::getPassword() const
{
    return password;
}

const UA_DataTypeArray* OpcUaEndpoint::getCustomDataTypes()const
{
    return customDataTypeList.getCustomDataTypes();
}

bool OpcUaEndpoint::isAnonymous()
{
    return username.empty();
}

void OpcUaEndpoint::registerCustomTypes(const size_t typesSize, const UA_DataType* types)
{
    customDataTypeList.add(typesSize, types);
}

END_NAMESPACE_OPENDAQ_OPCUA
