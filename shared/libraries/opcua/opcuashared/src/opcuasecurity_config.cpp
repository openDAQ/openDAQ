#include "opcuashared/opcua.h"
#include "opcuashared/opcuasecurity_config.h"

#include "opcuashared/opcuasecuritycommon.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA

OpcUaSecurityConfig::OpcUaSecurityConfig()
    : certificate(UA_BYTESTRING_NULL)
    , privateKey(UA_BYTESTRING_NULL)
{
}

OpcUaSecurityConfig::OpcUaSecurityConfig(const OpcUaSecurityConfig& config)
{
    operator=(config);
}

OpcUaSecurityConfig& OpcUaSecurityConfig::operator=(const OpcUaSecurityConfig& config)
{
    if (this == &config)
        return *this;

    appUri = config.appUri;
    securityMode = config.securityMode;
    certificate = config.certificate;
    privateKey = config.privateKey;
    trustList = config.trustList;
    revocationList = config.revocationList;
    trustAll = config.trustAll;

    return *this;
}

void OpcUaSecurityConfig::validate() const
{
    if (securityMode == UA_MESSAGESECURITYMODE_INVALID)
        throw OpcUaException(UA_STATUSCODE_BADSECURITYCHECKSFAILED, "Invalid security mode.");

    if (securityMode == UA_MESSAGESECURITYMODE_SIGN || securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT)
    {
        if (!hasCertificate())
            throw OpcUaException(UA_STATUSCODE_BADSECURITYCHECKSFAILED, "Certificate is not set.");

        if (!hasPrivateKey())
            throw OpcUaException(UA_STATUSCODE_BADSECURITYCHECKSFAILED, "Private key not set.");
    }
}

bool OpcUaSecurityConfig::hasCertificate() const
{
    return certificate.getValue().data != NULL;
}

bool OpcUaSecurityConfig::hasPrivateKey() const
{
    return privateKey.getValue().data != NULL;
}

std::optional<std::string> OpcUaSecurityConfig::getAppUriOrParseFromCertificate() const
{
    std::optional<std::string> appUri;
    if (appUri.has_value())
        appUri = appUri.value();
    else if (this->hasCertificate())
        appUri = OpcUaSecurityCommon::parseCertificateUri(certificate.getValue());
    return appUri;
}

OpcUaServerSecurityConfig::OpcUaServerSecurityConfig()
    : OpcUaSecurityConfig()
{
    authenticateUser = [](bool isAnonymous, std::string username, std::string password)
    {
        return UA_STATUSCODE_GOOD;
    };
}

bool OpcUaClientSecurityConfig::isAnonymous() const
{
    return !(username.has_value() && password.has_value());
}

END_NAMESPACE_OPENDAQ_OPCUA
