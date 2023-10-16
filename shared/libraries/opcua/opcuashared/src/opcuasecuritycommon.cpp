#include "opcuashared/opcuasecuritycommon.h"
#include "opcuashared/opcuacommon.h"

#ifdef OPCUA_ENABLE_ENCRYPTION
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>
#include <openssl/x509v3.h>
#endif

using namespace std::chrono;

BEGIN_NAMESPACE_OPENDAQ_OPCUA

std::optional<std::string> OpcUaSecurityCommon::parseCertificateUri(const UA_ByteString& certificate)
{
    std::optional<std::string> subjectUri;

#ifndef OPCUA_ENABLE_ENCRYPTION
    throw OpcUaException(UA_STATUSCODE_BADINTERNALERROR, "Encryption was not enabled when building the project.");
#else
    if (certificate.data == NULL)
        return subjectUri;

    const unsigned char* pData = certificate.data;
    X509* certificateX509 = d2i_X509(NULL, &pData, (long) certificate.length);
    if (certificateX509 == NULL)
        return subjectUri;

    GENERAL_NAMES* names = (GENERAL_NAMES*) X509_get_ext_d2i(certificateX509, NID_subject_alt_name, NULL, NULL);
    if (names == NULL)
    {
        X509_free(certificateX509);
        return subjectUri;
    }

    int namesCount = sk_GENERAL_NAME_num(names);
    for (int i = 0; i < namesCount; i++)
    {
        GENERAL_NAME* name = sk_GENERAL_NAME_value(names, i);
        if (name->type == GEN_URI)
        {
            size_t len = name->d.ia5->length;
            void* data = name->d.ia5->data;
            if (data != NULL)
                subjectUri = std::string((const char*) data, len);
            break;
        }
    }

    X509_free(certificateX509);
    sk_GENERAL_NAME_pop_free(names, GENERAL_NAME_free);
#endif

    return subjectUri;
}

UA_StatusCode OpcUaSecurityCommon::verifyCertificateRejectAll(void* verificationContext, const UA_ByteString* certificate)
{
    return UA_STATUSCODE_BADCERTIFICATEUNTRUSTED;
}

END_NAMESPACE_OPENDAQ_OPCUA
