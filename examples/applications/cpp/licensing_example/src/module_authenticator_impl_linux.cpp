#include <licensing_example/module_authenticator_impl_linux.h>

#include <filesystem>
#include <iostream>
#include <clocale>
#include <string>
#include <gpgme.h>

BEGIN_NAMESPACE_OPENDAQ

ModuleAuthenticatorImpl::ModuleAuthenticatorImpl(const StringPtr& certPath)
    : logger(nullptr)
    , loggerComponent(nullptr)
{
    std::string pathStr = certPath.toStdString();
    certsPath = std::filesystem::path(pathStr);
}

Bool ModuleAuthenticatorImpl::onAuthenticateModuleBinary(StringPtr& vendorKey, const StringPtr& binaryPath)
{
    vendorKey = StringPtr("Null");

    const std::string data_file = binaryPath.toStdString(); 
    const std::string sig_file = data_file + ".asc"; 
    // Setting the locale is important here for gpgme, is reset at the end of this function
    std::string original_locale = setlocale(LC_CTYPE, nullptr);
    setlocale(LC_CTYPE, "");

    gpgme_check_version(nullptr);
    gpgme_set_locale(nullptr, LC_CTYPE, setlocale(LC_CTYPE, ""));

    gpgme_ctx_t ctx;
    gpgme_data_t sig, data;
    gpgme_error_t err;

    // Init GPGME context
    err = gpgme_new(&ctx);
    if (err) {
        std::cerr << "[GPGME] Failed to create context: " << gpgme_strerror(err) << "\n";
        setlocale(LC_CTYPE, original_locale.c_str());

        return false;
    }

    // Open files
    if (gpgme_data_new_from_file(&sig, sig_file.c_str(), 1) != GPG_ERR_NO_ERROR ||
        gpgme_data_new_from_file(&data, data_file.c_str(), 1) != GPG_ERR_NO_ERROR) {
        std::cerr << "[GPGME] Failed to open input files.\n";
        gpgme_release(ctx);
        setlocale(LC_CTYPE, original_locale.c_str());
        return false;
    }

    // Verify detached signature
    err = gpgme_op_verify(ctx, sig, data, nullptr);
    if (err) {
        std::cerr << "[GPGME] Verification failed: " << gpgme_strerror(err) << "\n";
        gpgme_release(ctx);
        setlocale(LC_CTYPE, original_locale.c_str());
        return false;
    }

    // Get result
    gpgme_verify_result_t result = gpgme_op_verify_result(ctx);
    if (!result || !result->signatures) {
        std::cerr << "[GPGME] No signature found in result.\n";
        gpgme_release(ctx);
        setlocale(LC_CTYPE, original_locale.c_str());
        return false;
    }

    // Inspect signature
    gpgme_signature_t sig_info = result->signatures;
    if (sig_info->status != GPG_ERR_NO_ERROR) {
        std::cerr << "[GPGME] Invalid signature: " << gpgme_strerror(sig_info->status) << "\n";
        gpgme_release(ctx);
        setlocale(LC_CTYPE, original_locale.c_str());
        return false;
    }

    // Show info about the signer
    std::cout << "[OK] Signature is valid.\n";
    
    vendorKey = StringPtr(sig_info->fpr);
    
    std::cout << "    - Fingerprint: " << vendorKey << "\n";
    gpgme_release(ctx);

    setlocale(LC_CTYPE, original_locale.c_str());

    return true;
}

Bool ModuleAuthenticatorImpl::onSetLogger(const LoggerPtr& logger)
{
    this->logger = logger;
    this->loggerComponent = logger.addComponent("ModuleAuthenticator");

    return true;
}

END_NAMESPACE_OPENDAQ
