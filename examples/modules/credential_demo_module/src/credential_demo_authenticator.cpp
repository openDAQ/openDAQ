#include <credential_demo_module/credential_demo_authenticator.h>
#include <credential_demo_module/common.h>

#include <opendaq/authentication_method_factory.h>
#include <coreobjects/exceptions.h>
#include <vector>
#include <memory>

#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rand.h>

BEGIN_NAMESPACE_CREDENTIAL_DEMO_MODULE

namespace crypto
{
    using EvpPkeyPtr = std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)>;
    using EvpMdCtxPtr = std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)>;
    using BioPtr = std::unique_ptr<BIO, decltype(&BIO_free)>;

    EvpPkeyPtr ReadPemKeyFile(const std::string& path, bool isPrivateKey)
    {
        BioPtr bio(BIO_new_file(path.c_str(), "r"), &BIO_free);
        if (!bio)
            return EvpPkeyPtr(nullptr, &EVP_PKEY_free);

        EVP_PKEY* key = isPrivateKey
                            ? PEM_read_bio_PrivateKey(bio.get(), nullptr, nullptr, nullptr)
                            : PEM_read_bio_PUBKEY(bio.get(), nullptr, nullptr, nullptr);

        return EvpPkeyPtr(key, &EVP_PKEY_free);
    }

    // Proves that whoever supplied `privateKey` holds the private key matching the module's known
    // public key: a random challenge is signed with the claimed private key, then the signature is
    // checked against the known public key.
    bool VerifyPrivateKeyChallenge(const EvpPkeyPtr& privateKey, const std::string& publicKeyPath)
    {
        if (!privateKey)
            return false;

        auto publicKey = ReadPemKeyFile(publicKeyPath, /*isPrivateKey*/ false);
        if (!publicKey)
            return false;

        unsigned char challenge[32];
        if (RAND_bytes(challenge, sizeof(challenge)) != 1)
            return false;

        EvpMdCtxPtr signCtx(EVP_MD_CTX_new(), &EVP_MD_CTX_free);
        if (!signCtx || EVP_DigestSignInit(signCtx.get(), nullptr, EVP_sha256(), nullptr, privateKey.get()) != 1)
            return false;

        size_t signatureLength = 0;
        if (EVP_DigestSign(signCtx.get(), nullptr, &signatureLength, challenge, sizeof(challenge)) != 1)
            return false;

        std::vector<unsigned char> signature(signatureLength);
        if (EVP_DigestSign(signCtx.get(), signature.data(), &signatureLength, challenge, sizeof(challenge)) != 1)
            return false;

        EvpMdCtxPtr verifyCtx(EVP_MD_CTX_new(), &EVP_MD_CTX_free);
        if (!verifyCtx || EVP_DigestVerifyInit(verifyCtx.get(), nullptr, EVP_sha256(), nullptr, publicKey.get()) != 1)
            return false;

        return EVP_DigestVerify(verifyCtx.get(), signature.data(), signatureLength, challenge, sizeof(challenge)) == 1;
    }
}

namespace authentication
{

void Authenticate(const ContextPtr& ctx, const DictPtr<IString, IString>& credentials, const StringPtr& authenticationMethodId)
{
    const std::string authenticationMethodIdStr = authenticationMethodId.toStdString();

    // Anonymous requires no credentials at all - connects the same way the plain, non-authenticated path
    // does, so there's nothing to check.
    if (authenticationMethodIdStr == StandardAnonymousId)
    {
        return;
    }

    if (authenticationMethodIdStr == StandardPinId)
    {
        const StringPtr pin = credentials.get("Pin");
        if (pin != "1234")
        {
            DAQ_THROW_EXCEPTION(AuthenticationFailedException, "Failed to authenticate - wrong pin-code");
        }
    }
    else if (authenticationMethodIdStr == StandardPrivateKeyFileId)
    {
        const StringPtr privateKeyPath = credentials.get("PrivateKeyFilePath");

        crypto::EvpPkeyPtr privateKey = crypto::ReadPemKeyFile(privateKeyPath.toStdString(), /*isPrivateKey*/ true);

        if (!privateKey)
        {
            DAQ_THROW_EXCEPTION(AuthenticationFailedException, "Failed to authenticate - could not parse the supplied private key");
        }

        const auto moduleOptions = ctx.getModuleOptions(CREDENTIAL_DEMO_MODULE_ID);
        const StringPtr publicKeyPath = moduleOptions.hasKey("PublicKeyPath") ? moduleOptions.get("PublicKeyPath").asPtr<IString>() : nullptr;
        if (!publicKeyPath.assigned() || publicKeyPath.getLength() == 0)
        {
            DAQ_THROW_EXCEPTION(AuthenticationFailedException,
                                 "Failed to authenticate - module has no public key configured (set the \"PublicKeyPath\" module option)");
        }

        if (!crypto::VerifyPrivateKeyChallenge(privateKey, publicKeyPath.toStdString()))
        {
            DAQ_THROW_EXCEPTION(AuthenticationFailedException, "Failed to authenticate - private key challenge verification failed");
        }
    }
    else if (authenticationMethodIdStr == StandardUserNamePasswordId)
    {
        const StringPtr userName = credentials.get("UserName");
        const StringPtr password = credentials.get("Password");
        if (userName != "user" || password != "pass")
        {
            DAQ_THROW_EXCEPTION(AuthenticationFailedException, "Failed to authenticate - wrong username or password");
        }
    }
    else
    {
        DAQ_THROW_EXCEPTION(AuthenticationFailedException, "Failed to authenticate - unknown authentication method id \"{}\"", authenticationMethodIdStr);
    }
}

}

END_NAMESPACE_CREDENTIAL_DEMO_MODULE
