#include <sftp_file_transfer/libssh2/sftp_connection.h>

#include <sftp_file_transfer/sftp_authentication_config_factory.h>

#include <coretypes/exceptions.h>
#include <coreobjects/exceptions.h>
#include <fmt/format.h>

#include <openssl/evp.h>

#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <cstddef>
#include <filesystem>
#include <mutex>
#include <system_error>

namespace daq::sftp::libssh2
{

// Connected TCP socket. The only place that deals with POSIX socket APIs.
class SftpConnection::Socket
{
public:
    explicit Socket(const Endpoint& endpoint);
    ~Socket();
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

    libssh2_socket_t native() const noexcept;

private:
    libssh2_socket_t fd = LIBSSH2_INVALID_SOCKET;
};

namespace detail
{

// Turns a C "free" function into a unique_ptr deleter, for handles used only in this file.
template <auto Fn>
struct Deleter
{
    template <class T>
    void operator()(T* p) const noexcept
    {
        Fn(p);
    }
};

// Host key algorithms per key type, tried one type per handshake. Legacy SHA-1 "ssh-rsa" signatures are
// excluded, as in current OpenSSH.
struct HostKeyTypeAlgorithms
{
    const char* typeName;
    const char* algorithms;
};

constexpr HostKeyTypeAlgorithms HostKeyTypes[] = {
    {"ED25519", "ssh-ed25519"},
    {"ECDSA", "ecdsa-sha2-nistp256,ecdsa-sha2-nistp384,ecdsa-sha2-nistp521"},
    {"RSA", "rsa-sha2-512,rsa-sha2-256"},
};

const char* hostKeyTypeName(int type)
{
    switch (type)
    {
        case LIBSSH2_HOSTKEY_TYPE_ED25519:
            return "ED25519";
        case LIBSSH2_HOSTKEY_TYPE_ECDSA_256:
        case LIBSSH2_HOSTKEY_TYPE_ECDSA_384:
        case LIBSSH2_HOSTKEY_TYPE_ECDSA_521:
            return "ECDSA";
        case LIBSSH2_HOSTKEY_TYPE_RSA:
            return "RSA";
        default:
            return "unknown";
    }
}

bool offersMethod(std::string_view methods, std::string_view method)
{
    while (!methods.empty())
    {
        const auto comma = methods.find(',');
        if (methods.substr(0, comma) == method)
            return true;
        if (comma == std::string_view::npos)
            break;
        methods.remove_prefix(comma + 1);
    }
    return false;
}

// Number of live connections, guarded by libraryMutex. Both are constant-initialized (no dynamic initialization), so
// they exist before and outlive every object built during dynamic initialization: a connection held in a static
// object can still be released during static destruction.
std::mutex libraryMutex;
std::size_t libraryUsers = 0;

// The host key a server presented during the handshake.
struct PresentedHostKey
{
    StringPtr typeName;     // "ED25519", "ECDSA" or "RSA", as OpenSSH names them
    StringPtr fingerprint;  // "SHA256:..."
};

// Every supported host key algorithm, in the order of HostKeyTypes: offered in the first handshake.
std::string allHostKeyAlgorithms()
{
    std::string all;
    for (const auto& keyType : HostKeyTypes)
        all += (all.empty() ? "" : ",") + std::string(keyType.algorithms);
    return all;
}

// Thrown by openSession when the server has no host key of the requested types.
struct NoMatchingHostKeyType
{
    StringPtr message;
};

StringPtr lastErrorMessage(LIBSSH2_SESSION* session)
{
    char* message = nullptr;
    int length = 0;
    libssh2_session_last_error(session, &message, &length, 0);
    return (message && length > 0) ? String(message, static_cast<SizeT>(length)) : String("unknown error");
}

// Type and SHA256 fingerprint of the host key the server presented in the handshake.
PresentedHostKey hostKey(LIBSSH2_SESSION* session)
{
    std::size_t length = 0;
    int type = LIBSSH2_HOSTKEY_TYPE_UNKNOWN;
    if (!libssh2_session_hostkey(session, &length, &type))
        DAQ_THROW_EXCEPTION(NotFoundException, "Failed to read the server host key: {}", lastErrorMessage(session));

    const char* hash = libssh2_hostkey_hash(session, LIBSSH2_HOSTKEY_HASH_SHA256);
    if (!hash)
        DAQ_THROW_EXCEPTION(NotFoundException, "Failed to compute the server host key fingerprint");

    // OpenSSH form: "SHA256:" and the base64 of the digest without '=' padding.
    constexpr int DigestSize = 32;
    unsigned char base64[4 * ((DigestSize + 2) / 3) + 1];
    const int encodedLength = EVP_EncodeBlock(base64, reinterpret_cast<const unsigned char*>(hash), DigestSize);
    std::string fingerprint = "SHA256:" + std::string(reinterpret_cast<const char*>(base64), encodedLength);
    fingerprint.erase(fingerprint.find_last_not_of('=') + 1);

    return {String(hostKeyTypeName(type)), String(fingerprint)};
}

}

// ---------------------------------------------------------------------------
// Library
// ---------------------------------------------------------------------------

// libssh2_init / libssh2_exit set up and tear down process-wide state and are not thread-safe (documented). The
// documentation does not say whether repeated calls are counted, so they are made exactly once: libssh2_init by the
// first live connection, libssh2_exit by the last one, both under libraryMutex.
SftpConnection::Library::Library()
{
    std::scoped_lock lock(detail::libraryMutex);
    if (detail::libraryUsers == 0 && libssh2_init(0) != 0)
        DAQ_THROW_EXCEPTION(GeneralErrorException, "libssh2 initialization failed");
    ++detail::libraryUsers;
}

SftpConnection::Library::~Library()
{
    std::scoped_lock lock(detail::libraryMutex);
    if (--detail::libraryUsers == 0)
        libssh2_exit();
}

// ---------------------------------------------------------------------------
// Socket
// ---------------------------------------------------------------------------

SftpConnection::Socket::Socket(const Endpoint& endpoint)
{
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo* raw = nullptr;
    const std::string port = std::to_string(endpoint.port);
    if (const int rc = ::getaddrinfo(endpoint.host.getCharPtr(), port.c_str(), &hints, &raw); rc != 0)
        DAQ_THROW_EXCEPTION(NotFoundException, "Failed to resolve \"{}\": {}", endpoint.host, ::gai_strerror(rc));
    const std::unique_ptr<addrinfo, detail::Deleter<&::freeaddrinfo>> addresses(raw);

    // POSIX socket calls report failures only via errno; it is captured into a std::error_code right away.
    std::error_code lastError = std::make_error_code(std::errc::host_unreachable);
    for (const addrinfo* rp = addresses.get(); rp; rp = rp->ai_next)
    {
        const int candidate = ::socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (candidate < 0)
        {
            lastError = {errno, std::system_category()};
            continue;
        }
        if (::connect(candidate, rp->ai_addr, rp->ai_addrlen) == 0)
        {
            fd = candidate;
            return;
        }
        lastError = {errno, std::system_category()};
        ::close(candidate);
    }

    DAQ_THROW_EXCEPTION(NotFoundException, "Failed to connect to {}:{}: {}", endpoint.host, endpoint.port, lastError.message());
}

SftpConnection::Socket::~Socket()
{
    if (fd != LIBSSH2_INVALID_SOCKET)
        ::close(fd);
}

libssh2_socket_t SftpConnection::Socket::native() const noexcept
{
    return fd;
}

// ---------------------------------------------------------------------------
// SftpConnection
// ---------------------------------------------------------------------------

SftpConnection::SftpConnection(const Endpoint& endpoint,
                               const AuthenticationConfigPtr& authenticationConfig,
                               std::chrono::milliseconds timeout)
{
    // Compared as is with the fingerprint of each presented key. Empty: no verification.
    const DictPtr<IString, IString> credential = authenticationConfig.getSuppliedCredential();
    const StringPtr hostKeyFingerprint =
        credential.hasKey(SftpHostKeyFingerprintField) ? credential.get(SftpHostKeyFingerprintField) : String("");
    verified = hostKeyFingerprint.getLength() > 0;

    // Opens a new connection offering the given host key algorithms. Keeps it if its host key is accepted, otherwise
    // closes it (disconnect before close) and returns the rejected key.
    const auto attempt = [&](const char* hostKeyAlgorithms) -> detail::PresentedHostKey
    {
        auto candidateSocket = std::make_unique<Socket>(endpoint);
        SessionHandle candidateSession = openSession(candidateSocket->native(), hostKeyAlgorithms, timeout);

        detail::PresentedHostKey key = detail::hostKey(candidateSession.get());
        if (!verified || key.fingerprint == hostKeyFingerprint)
        {
            socket = std::move(candidateSocket);
            session = std::move(candidateSession);
        }
        return key;
    };

    // First handshake: every supported key type, the server presents one of them.
    detail::PresentedHostKey firstKey;
    try
    {
        firstKey = attempt(detail::allHostKeyAlgorithms().c_str());
    }
    catch (const detail::NoMatchingHostKeyType& e)
    {
        DAQ_THROW_EXCEPTION(NotFoundException,
                            "SSH handshake with {}:{} failed: the server has no ED25519, ECDSA or RSA host key: {}",
                            endpoint.host,
                            endpoint.port,
                            e.message);
    }

    if (!session)
    {
        // The fingerprint may belong to another of the server's keys: try the remaining types one at a time.
        std::string presented = fmt::format("\n  {} {}", firstKey.typeName, firstKey.fingerprint);
        for (const auto& keyType : detail::HostKeyTypes)
        {
            if (firstKey.typeName == keyType.typeName)
                continue;

            try
            {
                const detail::PresentedHostKey key = attempt(keyType.algorithms);
                if (session)
                    break;
                presented += fmt::format("\n  {} {}", key.typeName, key.fingerprint);
            }
            catch (const detail::NoMatchingHostKeyType&)
            {
                // The server has no key of this type.
            }
        }

        if (!session)
            DAQ_THROW_EXCEPTION(AuthenticationFailedException,
                                "None of the host keys presented by {} matches the expected fingerprint {}; the server may have "
                                "been reinstalled, or the connection may be intercepted. Presented:{}",
                                endpoint.host,
                                hostKeyFingerprint,
                                presented);
    }

    authenticate(authenticationConfig);

    sftp.reset(libssh2_sftp_init(session.get()));
    if (!sftp)
        DAQ_THROW_EXCEPTION(NotFoundException, "Failed to open the SFTP channel: {}", detail::lastErrorMessage(session.get()));
}

void SftpConnection::SessionDeleter::operator()(LIBSSH2_SESSION* session) const noexcept
{
    libssh2_session_disconnect(session, "Normal shutdown");
    libssh2_session_free(session);
}

void SftpConnection::SftpChannelDeleter::operator()(LIBSSH2_SFTP* sftp) const noexcept
{
    libssh2_sftp_shutdown(sftp);
}

SftpConnection::SessionHandle SftpConnection::openSession(libssh2_socket_t socket, const char* hostKeyAlgorithms, std::chrono::milliseconds timeout)
{
    // Only freed, not disconnected, until the handshake has succeeded.
    std::unique_ptr<LIBSSH2_SESSION, detail::Deleter<&libssh2_session_free>> session(libssh2_session_init());
    if (!session)
        DAQ_THROW_EXCEPTION(GeneralErrorException, "Failed to create an SSH session");

    libssh2_session_set_blocking(session.get(), 1);
    libssh2_session_set_timeout(session.get(), static_cast<long>(timeout.count()));

    if (libssh2_session_method_pref(session.get(), LIBSSH2_METHOD_HOSTKEY, hostKeyAlgorithms) != 0)
        DAQ_THROW_EXCEPTION(GeneralErrorException, "Failed to set host key algorithm preferences: {}", detail::lastErrorMessage(session.get()));

    if (libssh2_session_handshake(session.get(), socket) != 0)
    {
        // Key exchange fails when no host key algorithm is common to client and server; any other error (timeout,
        // dropped connection, not an SSH server) would fail for every key type, so it is reported right away.
        if (libssh2_session_last_errno(session.get()) == LIBSSH2_ERROR_KEX_FAILURE)
            throw detail::NoMatchingHostKeyType{detail::lastErrorMessage(session.get())};
        DAQ_THROW_EXCEPTION(NotFoundException, "SSH handshake failed: {}", detail::lastErrorMessage(session.get()));
    }
    return SessionHandle(session.release());
}

// Defined here, where Socket is complete.
SftpConnection::~SftpConnection() = default;

bool SftpConnection::hostKeyVerified() const noexcept
{
    return verified;
}

void SftpConnection::authenticate(const AuthenticationConfigPtr& authenticationConfig)
{
    const StringPtr methodId = authenticationConfig.getSelectedAuthenticationMethod().getId();
    const DictPtr<IString, IString> credential = authenticationConfig.getSuppliedCredential();

    const StringPtr user = credential.get("UserName");
    const char* methodsRaw = libssh2_userauth_list(session.get(), user.getCharPtr(), static_cast<unsigned int>(user.getLength()));
    if (!methodsRaw)
    {
        if (libssh2_userauth_authenticated(session.get()))
            return;  // The server accepted "none" authentication.
        DAQ_THROW_EXCEPTION(NotFoundException, "Failed to query the server's authentication methods: {}", detail::lastErrorMessage(session.get()));
    }
    const std::string_view methods(methodsRaw);

    if (methodId == SftpPrivateKeyFileId)
    {
        if (!detail::offersMethod(methods, "publickey"))
            DAQ_THROW_EXCEPTION(AuthenticationFailedException, "The server does not accept private key authentication (offers: {})", methods);

        const StringPtr privateKeyFile = credential.get("PrivateKeyFilePath");
        std::error_code ec;
        if (!std::filesystem::is_regular_file(privateKeyFile.toStdString(), ec))
            DAQ_THROW_EXCEPTION(InvalidParameterException, "Private key file \"{}\" does not exist", privateKeyFile);

        // Optional: absent or empty if the key is not encrypted.
        const StringPtr passphrase = credential.hasKey("Passphrase") ? credential.get("Passphrase") : String("");

        // A null public key path makes libssh2 derive the public key from the private key.
        if (libssh2_userauth_publickey_fromfile_ex(session.get(),
                                                   user.getCharPtr(),
                                                   static_cast<unsigned int>(user.getLength()),
                                                   nullptr,
                                                   privateKeyFile.getCharPtr(),
                                                   passphrase.getLength() == 0 ? nullptr : passphrase.getCharPtr()) != 0)
            DAQ_THROW_EXCEPTION(AuthenticationFailedException, "Private key authentication failed: {}", detail::lastErrorMessage(session.get()));
    }
    else
    {
        if (!detail::offersMethod(methods, "password"))
            DAQ_THROW_EXCEPTION(AuthenticationFailedException, "The server does not accept password authentication (offers: {})", methods);

        const StringPtr password = credential.get("Password");
        if (libssh2_userauth_password_ex(session.get(),
                                         user.getCharPtr(),
                                         static_cast<unsigned int>(user.getLength()),
                                         password.getCharPtr(),
                                         static_cast<unsigned int>(password.getLength()),
                                         nullptr) != 0)
            DAQ_THROW_EXCEPTION(AuthenticationFailedException, "Password authentication failed: {}", detail::lastErrorMessage(session.get()));
    }
}

}
