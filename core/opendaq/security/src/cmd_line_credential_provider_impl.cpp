#include <opendaq/cmd_line_credential_provider_impl.h>
#include <coreobjects/exceptions.h>
#include <fmt/format.h>
#include <iostream>
#include <fstream>

#ifdef _WIN32
#include <conio.h>
#include <codecvt>
#include <locale>
#else
#include <termios.h>
#include <unistd.h>
#endif

BEGIN_NAMESPACE_OPENDAQ

static const std::string CmdLineCredentialProviderDescription = "Prompts for credentials interactively via the command line.";

CmdLineCredentialProviderImpl::CmdLineCredentialProviderImpl()
{
}

ErrCode CmdLineCredentialProviderImpl::getDescription(IString** description)
{
    OPENDAQ_PARAM_NOT_NULL(description);

    *description = String(CmdLineCredentialProviderDescription).detach();
    return OPENDAQ_SUCCESS;
}

ErrCode CmdLineCredentialProviderImpl::requestCredentials(ICredentialRequest* request, IDict** credentials)
{
    OPENDAQ_PARAM_NOT_NULL(credentials);
    OPENDAQ_PARAM_NOT_NULL(request);

    const auto requestPtr = CredentialRequestPtr::Borrow(request);
    const auto authenticationMethod = requestPtr.getAuthenticationMethod();
    if (!authenticationMethod.assigned())
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "Credential request has no authentication method set");

    *credentials = readCredential(requestPtr, authenticationMethod.getFields()).detach();
    return OPENDAQ_SUCCESS;
}

ErrCode CmdLineCredentialProviderImpl::cacheCredentials(ICredentialRequest* request, IDict* credentials)
{
    OPENDAQ_PARAM_NOT_NULL(request);
    OPENDAQ_PARAM_NOT_NULL(credentials);

    const auto requestPtr = CredentialRequestPtr::Borrow(request);
    const auto authenticationMethod = requestPtr.getAuthenticationMethod();
    if (!authenticationMethod.assigned())
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "Credential request has no authentication method set");

    // Every field of `credentials` is cached, whatever its kind - folded into the same per-(device, method) cache entry.
    const DictPtr<IString, IString> credentialObj = credentials;
    auto& cached = credentialCache[MakeCacheKey(requestPtr)];
    if (!cached.assigned())
        cached = Dict<IString, IString>();

    for (const auto& [name, value] : credentialObj)
        cached.set(name, value);

    return OPENDAQ_SUCCESS;
}

DictPtr<IString, IString> CmdLineCredentialProviderImpl::readCredential(const CredentialRequestPtr& request, const DictPtr<IString, ICredentialField>& fields)
{
    auto credential = Dict<IString, IString>();
    bool printedDetails = false;

    auto& cached = credentialCache[MakeCacheKey(request)];
    if (!cached.assigned())
        cached = Dict<IString, IString>();

    for (const auto& [name, field] : fields)
    {
        const bool isFilePath = field.getKind() == CredentialFieldKind::FilePath;

        StringPtr value;
        if (cached.hasKey(name))
            value = cached.get(name);

        if (!value.assigned())
        {
            if (!printedDetails)
            {
                printRequestDetails(request);
                printedDetails = true;
            }

            // Read exactly once, like every other field kind - no retry - but a `FilePath` is still validated
            // locally right after it's freshly read (not on a cache hit): the module reading it back expects a
            // real, readable file.
            value = String(readLine(fmt::format("{}: ", name.toStdString()), field.getKind() == CredentialFieldKind::Secret));

            if (isFilePath && !isFileAccessible(value.toStdString()))
                DAQ_THROW_EXCEPTION(AuthenticationFailedException, "File \"{}\" does not exist or is not accessible", value);

            cached.set(name, value);
        }

        credential.set(name, value);
    }

    return credential;
}

CmdLineCredentialProviderImpl::CacheKey CmdLineCredentialProviderImpl::MakeCacheKey(const CredentialRequestPtr& request)
{
    const StringPtr manufacturer = request.getManufacturer();
    const StringPtr serialNumber = request.getSerialNumber();
    const StringPtr model = request.getModel();
    const bool hasManufacturer = manufacturer.assigned() && manufacturer.getLength() > 0;
    const bool hasSerialNumber = serialNumber.assigned() && serialNumber.getLength() > 0;
    const bool hasModel = model.assigned() && model.getLength() > 0;
    const StringPtr methodId = request.getAuthenticationMethod().getId();

    if (!hasManufacturer && !hasSerialNumber)
    {
        // Neither is available to identify the connection by - e.g. a streaming connection, which (unlike
        // a device's `daq://manufacturer_serial` smart string) isn't resolved through manufacturer/serial
        // discovery. Fall back to the connection string itself - the module that formed this request is
        // expected to have already canonicalized it (routing prefix trimmed, every parameter made explicit;
        // see `Module::onGetCanonicalConnectionString`), so it stays a stable identifier regardless of how
        // much of it the caller originally left implicit.
        const StringPtr connectionString = request.getConnectionString();
        return std::make_tuple(connectionString.assigned() ? connectionString.toStdString() : std::string(),
                               std::string(),
                               hasModel ? model.toStdString() : std::string(),
                               methodId.toStdString());
    }

    return std::make_tuple(hasManufacturer ? manufacturer.toStdString() : std::string(),
                           hasSerialNumber ? serialNumber.toStdString() : std::string(),
                           hasModel ? model.toStdString() : std::string(),
                           methodId.toStdString());
}

bool CmdLineCredentialProviderImpl::isFileAccessible(const std::string& path)
{
    return std::ifstream(path).good();
}

void CmdLineCredentialProviderImpl::printRequestDetails(const CredentialRequestPtr& request)
{
    std::cout << '\n';
    std::cout << "============================================================\n";
    std::cout << "Authentication required\n";
    std::cout << "============================================================\n\n";

    std::cout << "Component type : " << request.getComponentType().getName() << '\n';
    std::cout << "Connection string : " << request.getConnectionString() << '\n';

    if (const StringPtr displayName = request.getDisplayName(); displayName.assigned() && displayName.getLength() > 0)
        std::cout << "Device : " << displayName << '\n';

    std::cout << '\n';
}

std::string CmdLineCredentialProviderImpl::readLine(const std::string& prompt, bool hide)
{
    std::cout << prompt;
    std::cout.flush();

    if (!hide)
    {
        std::string value;
        if (!std::getline(std::cin, value))
            throw std::runtime_error("Input cancelled");

        return value;
    }

#ifdef _WIN32

    std::wstring value;

    while (true)
    {
        const wchar_t ch = _getwch();

        switch (ch)
        {
            case L'\r': // Enter
            {
                std::wcout << std::endl;
#if defined(__clang__)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
                std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> converter;
#if defined(__clang__)
    #pragma clang diagnostic pop
#endif
                return converter.to_bytes(value);
            }

            case 3: // Ctrl+C
                throw std::runtime_error("Input cancelled.");

            case L'\b': // Backspace
                if (!value.empty())
                    value.pop_back();
                break;

            case 0:
            case 0xE0:
                _getwch(); // Consume extended key code.
                break;

            default:
                value.push_back(ch);
                break;
        }
    }

#else

    termios oldAttr{};
    if (tcgetattr(STDIN_FILENO, &oldAttr) != 0)
        throw std::runtime_error("Failed to access terminal.");

    struct EchoGuard
    {
        explicit EchoGuard(const termios& attr)
            : oldAttr(attr)
        {
        }

        ~EchoGuard()
        {
            tcsetattr(STDIN_FILENO, TCSANOW, &oldAttr);
        }

        termios oldAttr;
    } guard(oldAttr);

    termios newAttr = oldAttr;
    newAttr.c_lflag &= ~ECHO;

    if (tcsetattr(STDIN_FILENO, TCSANOW, &newAttr) != 0)
        throw std::runtime_error("Failed to disable terminal echo.");

    std::string value;

    if (!std::getline(std::cin, value))
        throw std::runtime_error("Input cancelled.");

    std::cout << std::endl;

    return value;

#endif
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, CmdLineCredentialProvider, ICredentialProvider)

END_NAMESPACE_OPENDAQ
