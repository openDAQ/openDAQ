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
<<<<<<< HEAD
    vendorKey = StringPtr("Null");

    const std::string data_file = binaryPath.toStdString(); 
    const std::string sig_file = data_file + ".asc"; 
    // Setting the locale is important here for gpgme, is reset at the end of this function
=======
    vendorKey = StringPtr("");

    const std::string data_file = binaryPath.toStdString(); 
    const std::string sig_file = data_file + ".asc"; 
    // Setting the locale is important here for gpgme; this is reset on return
>>>>>>> a14bcd7d (cleaned up linux licensing example)
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
<<<<<<< HEAD
        std::cerr << "[GPGME] Failed to create context: " << gpgme_strerror(err) << "\n";
=======
        LOG_W("[GPGME] Failed to create context: {}", gpgme_strerror(err));
>>>>>>> a14bcd7d (cleaned up linux licensing example)
        setlocale(LC_CTYPE, original_locale.c_str());

        return false;
    }

    // Open files
<<<<<<< HEAD
    if (gpgme_data_new_from_file(&sig, sig_file.c_str(), 1) != GPG_ERR_NO_ERROR ||
        gpgme_data_new_from_file(&data, data_file.c_str(), 1) != GPG_ERR_NO_ERROR) {
        std::cerr << "[GPGME] Failed to open input files.\n";
=======
    if (gpgme_data_new_from_file(&sig, sig_file.c_str(), 1) != GPG_ERR_NO_ERROR) {
        LOG_W("[GPGME] Failed to open signature. \'.asc\' file missing or invalid.");
>>>>>>> a14bcd7d (cleaned up linux licensing example)
        gpgme_release(ctx);
        setlocale(LC_CTYPE, original_locale.c_str());
        return false;
    }

<<<<<<< HEAD
    // Verify detached signature
    err = gpgme_op_verify(ctx, sig, data, nullptr);
    if (err) {
        std::cerr << "[GPGME] Verification failed: " << gpgme_strerror(err) << "\n";
=======
    // Open files
    if (gpgme_data_new_from_file(&data, data_file.c_str(), 1) != GPG_ERR_NO_ERROR) {
        LOG_W("[GPGME] Failed to open binary. \'.so\' file missing or invalid.");
        gpgme_release(ctx);
        setlocale(LC_CTYPE, original_locale.c_str());
        return false;
    }
    
    // Verify detached signature
    err = gpgme_op_verify(ctx, sig, data, nullptr);
    if (err) {
        LOG_W("[GPGME] Verification failed: {}", gpgme_strerror(err));
>>>>>>> a14bcd7d (cleaned up linux licensing example)
        gpgme_release(ctx);
        setlocale(LC_CTYPE, original_locale.c_str());
        return false;
    }

    // Get result
    gpgme_verify_result_t result = gpgme_op_verify_result(ctx);
    if (!result || !result->signatures) {
<<<<<<< HEAD
        std::cerr << "[GPGME] No signature found in result.\n";
=======
        LOG_W("[GPGME] No signature found in result.\n");
>>>>>>> a14bcd7d (cleaned up linux licensing example)
        gpgme_release(ctx);
        setlocale(LC_CTYPE, original_locale.c_str());
        return false;
    }

    // Inspect signature
    gpgme_signature_t sig_info = result->signatures;
    if (sig_info->status != GPG_ERR_NO_ERROR) {
<<<<<<< HEAD
        std::cerr << "[GPGME] Invalid signature: " << gpgme_strerror(sig_info->status) << "\n";
=======
        LOG_W("[GPGME] Invalid signature: {}", gpgme_strerror(sig_info->status));
>>>>>>> a14bcd7d (cleaned up linux licensing example)
        gpgme_release(ctx);
        setlocale(LC_CTYPE, original_locale.c_str());
        return false;
    }

    // Show info about the signer
<<<<<<< HEAD
    std::cout << "[OK] Signature is valid.\n";
    
    vendorKey = StringPtr(sig_info->fpr);
    
    std::cout << "    - Fingerprint: " << vendorKey << "\n";
=======
    LOG_I("[GPGME] Signature is valid.");
    
    vendorKey = StringPtr(sig_info->fpr);
    
>>>>>>> a14bcd7d (cleaned up linux licensing example)
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
