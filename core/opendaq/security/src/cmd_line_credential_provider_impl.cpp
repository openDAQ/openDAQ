#include <opendaq/cmd_line_credential_provider_impl.h>
#include <opendaq/security_exceptions.h>
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

DictPtr<IString, IString> CmdLineCredentialProviderImpl::readCredential(const CredentialRequestPtr& request, const DictPtr<IString, ICredentialField>& fields)
{
    auto credential = Dict<IString, IString>();
    bool printedDetails = false;

    for (const auto& [name, field] : fields)
    {
        const bool isFilePath = field.getKind() == CredentialFieldKind::FilePath;

        if (!printedDetails)
        {
            printRequestDetails(request);
            printedDetails = true;
        }

        const StringPtr value = String(readLine(fmt::format("{}: ", name.toStdString()), field.getKind() == CredentialFieldKind::Secret));

        if (isFilePath && !isFileAccessible(value.toStdString()))
        {
            DAQ_THROW_EXCEPTION(NotFoundException, "File \"{}\" does not exist or is not accessible", value);
        }

        credential.set(name, value);
    }

    return credential;
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

#ifdef _WIN32
    std::cout << "\nPress Ctrl+Z then Enter to cancel.\n";
#else
    std::cout << "\nPress Ctrl+D to cancel.\n";
#endif

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
        {
            DAQ_THROW_EXCEPTION(ProviderCancelledException);
        }

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

            case 3:    // Ctrl+C
            case 0x1A: // Ctrl+Z (the conventional Windows console EOF/cancel keystroke)
                DAQ_THROW_EXCEPTION(ProviderCancelledException);

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
    {
        DAQ_THROW_EXCEPTION(ProviderCancelledException);
    }

    std::cout << std::endl;

    return value;

#endif
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, CmdLineCredentialProvider, ICredentialProvider)

END_NAMESPACE_OPENDAQ
