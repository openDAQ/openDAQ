#include <licensing_example/module_authenticator_impl_win.h>

#include <iostream>

#include <windows.h>
#include <Softpub.h>
#include <wincrypt.h>
#include <wintrust.h>
#include <mscat.h>

BEGIN_NAMESPACE_OPENDAQ

ModuleAuthenticatorImplExample::ModuleAuthenticatorImplExample(const StringPtr& certPath)
    : logger(nullptr)
    , loggerComponent(nullptr)
{
    std::string pathStr = certPath.toStdString();
    certsPath = std::filesystem::path(pathStr);
}

Bool ModuleAuthenticatorImplExample::onAuthenticateModuleBinary(StringPtr& vendorKey, const StringPtr& binaryPath)
{
    LOG_I("Authenticating module binary: \"{}\"", binaryPath);

    vendorKey = String("");

    std::string pathStr = binaryPath.toStdString();
    std::filesystem::path path(pathStr);

    if (!path.has_filename())
    {
        LOG_E("Path \"{}\" is not a file path!", binaryPath.toString());

        return false; 
    }

    HCATADMIN hCatAdmin = NULL;
    const GUID DriverGuid = DRIVER_ACTION_VERIFY;
    if (!CryptCATAdminAcquireContext(&hCatAdmin, &DriverGuid, 0))
    {
        const auto win32ErrCode = GetLastError();
        LOG_E("Failed to initialize the Windows Crypto API! (Win32 error code: {})!", win32ErrCode);

        return false; 
    }

    HANDLE hFile = CreateFileW(path.c_str(),
                               FILE_READ_ATTRIBUTES | FILE_READ_DATA,
                               FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                               NULL,
                               OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL);
    if (INVALID_HANDLE_VALUE == hFile)
    {
        const auto win32ErrCode = GetLastError();
        LOG_E("Failed to open the license module file! (Win32 error code: {})", win32ErrCode);

        return false;
    }

    WINTRUST_DATA wd = {};
    wd.cbStruct = sizeof(WINTRUST_DATA);
    wd.dwUIChoice = WTD_UI_NONE;
    wd.dwStateAction = WTD_STATEACTION_VERIFY;
    wd.fdwRevocationChecks = WTD_REVOKE_NONE;
    wd.dwProvFlags = WTD_CACHE_ONLY_URL_RETRIEVAL;
    wd.dwUnionChoice = WTD_CHOICE_FILE;

    WINTRUST_FILE_INFO wfi = {};
    wd.pFile = &wfi;

    wfi.cbStruct = sizeof(WINTRUST_FILE_INFO);
    wfi.pcwszFilePath = NULL;
    wfi.hFile = hFile;
    wfi.pgKnownSubject = NULL;

    GUID VerifyGuid = WINTRUST_ACTION_GENERIC_VERIFY_V2;
    const auto trustResultCode = WinVerifyTrust((HWND) INVALID_HANDLE_VALUE, &VerifyGuid, &wd);
    
    // Make sure to close the file handle as it's no longer needed!
    CloseHandle(hFile);
    hFile = NULL;

    if (trustResultCode != ERROR_SUCCESS && trustResultCode != CERT_E_UNTRUSTEDROOT) // We allow an untrusted root here for the integration tests..
    {
        const auto win32ErrCode = GetLastError();
        LOG_E("The user is most likely trying to a use a compromised license dll ! (Win Crypto Return code: {})", win32ErrCode);

        return false;
    }

    // Now verify the signature of the license module
    CRYPT_PROVIDER_DATA* pProvData = WTHelperProvDataFromStateData(wd.hWVTStateData);
    if (pProvData == NULL)
    {
        const auto win32ErrCode = GetLastError();
        LOG_E("Failed to access the certificate state data! (Win32 error code: {})", win32ErrCode);

        return false;
    }

    const int idxSigner = 0;
    CRYPT_PROVIDER_SGNR* pCPSigner = WTHelperGetProvSignerFromChain(pProvData, idxSigner, FALSE, 0);
    if (pCPSigner == NULL)
    {
        const auto win32ErrCode = GetLastError();
        LOG_E("Failed to access the certificate signer data! (Win32 error code: {})", win32ErrCode);

        return false;
    }

    PCCERT_CONTEXT pCertificate = CertDuplicateCertificateContext(pCPSigner->pasCertChain->pCert);
    if (pCertificate == NULL)
    {
        const auto win32ErrCode = GetLastError();
        LOG_E("Failed to access the certificate context! (Win32 error code: {})", win32ErrCode);

        return false;
    }

    DWORD size = 0;
    CertGetCertificateContextProperty(pCertificate, CERT_HASH_PROP_ID, NULL, &size);
   
    std::vector<uint8_t> current_license_hashBuffer(size);
    if (!CertGetCertificateContextProperty(pCertificate, CERT_HASH_PROP_ID, &current_license_hashBuffer.front(), &size))
    {
        const auto win32ErrCode = GetLastError();
        LOG_E("Failed to access the certificate hash property! (Win32 error code: {})", win32ErrCode);

        return false;
    }

    std::string certificate_name;
    size = CertGetNameStringA(pCertificate, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, NULL, NULL, NULL);
    if (size)
    {
        std::vector<char> buff(size);
        CertGetNameStringA(pCertificate, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, NULL, &buff.front(), size);
        certificate_name = std::string(buff.cbegin(), buff.cend());

        LOG_I("Module binary \"{}\" signed with certificate {}", path.filename().string(), certificate_name);
    }
    else
    {
        LOG_W("Module binary \"{}\" signed with unknown certificate", path.filename().string());
    }

    // Get a printable version of the (SHA1) of the initial (signing) certificate returned by >> signtool verify /pa /v ${MODULE_PATH}
    auto getPrintableHash = [](const std::vector<uint8_t>& buff) -> std::string
    {
        const size_t MAX_HASH_STRING_SIZE = 100;
        char pszMemberTag[MAX_HASH_STRING_SIZE] = {0};

        const size_t printSize = std::min(MAX_HASH_STRING_SIZE / 2, buff.size());

        for (size_t i = 0; i < printSize; ++i)
            sprintf(&pszMemberTag[i * 2], "%02X", buff[i]);

        return std::string(pszMemberTag);
    };

    const auto current_license_hash = getPrintableHash(current_license_hashBuffer);

    vendorKey = String(current_license_hash);

    return true;
}

Bool ModuleAuthenticatorImplExample::onSetLogger(const LoggerPtr& logger)
{
    if (!logger.assigned())
        return false;

    this->logger = logger;
    this->loggerComponent = logger.addComponent("ModuleAuthenticator");

    return true;
}


END_NAMESPACE_OPENDAQ
