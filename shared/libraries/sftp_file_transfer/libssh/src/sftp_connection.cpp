#include <sftp_file_transfer/libssh/sftp_connection.h>

#include <sftp_file_transfer/sftp_authentication_config_factory.h>

#include <coretypes/exceptions.h>
#include <coreobjects/exceptions.h>
#include <fmt/format.h>

#include <fcntl.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <fstream>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace daq::sftp::libssh
{

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

using KeyHandle = std::unique_ptr<ssh_key_struct, Deleter<&ssh_key_free>>;
using SftpFileHandle = std::unique_ptr<sftp_file_struct, Deleter<&sftp_close>>;
using SftpLimitsHandle = std::unique_ptr<sftp_limits_struct, Deleter<&sftp_limits_free>>;
using SftpAttributesHandle = std::unique_ptr<sftp_attributes_struct, Deleter<&sftp_attributes_free>>;
// An outstanding asynchronous request. Waiting for it frees it; one freed without waiting is abandoned (only done
// after a failure, when the transfer is aborted anyway).
using SftpAioHandle = std::unique_ptr<sftp_aio_struct, Deleter<&sftp_aio_free>>;

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

// Points libssh's known_hosts lookups at an empty file. libssh reads them in ssh_connect to reorder the offered
// host key types, and the options cannot be unset (null restores the default path, empty is rejected).
#ifdef _WIN32
constexpr const char* NoKnownHostsFile = "NUL";
#else
constexpr const char* NoKnownHostsFile = "/dev/null";
#endif

// Transfers are pipelined: up to this many read or write requests are outstanding at a time, so the transfer does not
// wait a full round trip per chunk.
constexpr std::size_t InFlightRequests = 16;

// Upper bound for the chunk size, which is otherwise the per-request limit reported by the server (about 255 KiB
// for OpenSSH; libssh assumes 32 KiB if the server reports none). Bounds the memory held by outstanding requests.
constexpr std::uint64_t MaxChunkSize = 256 * 1024;

const char* hostKeyTypeName(enum ssh_keytypes_e type)
{
    switch (type)
    {
        case SSH_KEYTYPE_ED25519:
            return "ED25519";
        case SSH_KEYTYPE_ECDSA_P256:
        case SSH_KEYTYPE_ECDSA_P384:
        case SSH_KEYTYPE_ECDSA_P521:
            return "ECDSA";
        case SSH_KEYTYPE_RSA:
            return "RSA";
        default:
            return "unknown";
    }
}

// Every supported host key algorithm, in the order of HostKeyTypes: offered in the first handshake.
std::string allHostKeyAlgorithms()
{
    std::string all;
    for (const auto& keyType : HostKeyTypes)
        all += (all.empty() ? "" : ",") + std::string(keyType.algorithms);
    return all;
}

// The host key a server presented during the handshake.
struct PresentedHostKey
{
    StringPtr typeName;     // "ED25519", "ECDSA" or "RSA", as OpenSSH names them
    StringPtr fingerprint;  // "SHA256:..."
};

// Type and SHA256 fingerprint of the host key the server presented in the handshake.
PresentedHostKey hostKey(ssh_session session)
{
    ssh_key rawKey = nullptr;
    if (ssh_get_server_publickey(session, &rawKey) != SSH_OK)
        DAQ_THROW_EXCEPTION(NotFoundException, "Failed to read the server host key: {}", ssh_get_error(session));
    const KeyHandle key(rawKey);

    unsigned char* hash = nullptr;
    std::size_t hashLength = 0;
    if (ssh_get_publickey_hash(key.get(), SSH_PUBLICKEY_HASH_SHA256, &hash, &hashLength) != 0)
        DAQ_THROW_EXCEPTION(NotFoundException, "Failed to compute the server host key fingerprint");

    // OpenSSH form: "SHA256:" and the base64 of the digest without '=' padding.
    char* fingerprint = ssh_get_fingerprint_hash(SSH_PUBLICKEY_HASH_SHA256, hash, hashLength);
    ssh_clean_pubkey_hash(&hash);
    if (!fingerprint)
        DAQ_THROW_EXCEPTION(NotFoundException, "Failed to format the server host key fingerprint");
    const StringPtr result = String(fingerprint);
    ssh_string_free_char(fingerprint);

    return {String(hostKeyTypeName(ssh_key_type(key.get()))), result};
}

// Throws for a failed SFTP file operation on `path`. If the server refused the operation, its SFTP status is mapped
// to NotFound or AccessDenied where one fits; any other failure (including a session-level one) is a GeneralError.
[[noreturn]] void throwFileError(ssh_session session, sftp_session sftp, const char* operation, const StringPtr& path)
{
    // libssh reports a status from the server as SSH_REQUEST_DENIED; sftp_get_error is only meaningful then.
    if (ssh_get_error_code(session) == SSH_REQUEST_DENIED)
    {
        switch (const int status = sftp_get_error(sftp))
        {
            case SSH_FX_NO_SUCH_FILE:
            case SSH_FX_NO_SUCH_PATH:
                DAQ_THROW_EXCEPTION(NotFoundException, "Failed to {} \"{}\": no such file or directory", operation, path);
            case SSH_FX_PERMISSION_DENIED:
            case SSH_FX_WRITE_PROTECT:
                DAQ_THROW_EXCEPTION(AccessDeniedException, "Failed to {} \"{}\": permission denied", operation, path);
            default:
                DAQ_THROW_EXCEPTION(GeneralErrorException, "Failed to {} \"{}\": SFTP status {}", operation, path, status);
        }
    }
    DAQ_THROW_EXCEPTION(GeneralErrorException, "Failed to {} \"{}\": {}", operation, path, ssh_get_error(session));
}

}

// ---------------------------------------------------------------------------
// SftpConnection
// ---------------------------------------------------------------------------

SftpConnection::SftpConnection(const Endpoint& endpoint,
                               const AuthenticationConfigPtr& authenticationConfig,
                               std::chrono::milliseconds timeout)
{
    const DictPtr<IString, IString> credential = authenticationConfig.getSuppliedCredential();
    const StringPtr user = credential.get("UserName");

    // Compared as is with the fingerprint of each presented key. Empty: no verification.
    const StringPtr hostKeyFingerprint =
        credential.hasKey(SftpHostKeyFingerprintField) ? credential.get(SftpHostKeyFingerprintField) : String("");
    verified = hostKeyFingerprint.getLength() > 0;

    // Opens a new connection offering the given host key algorithms. Keeps it if its host key is accepted, otherwise
    // closes it and returns the rejected key. Returns no key if the connection failed; the error is then in `error`.
    StringPtr error;
    const auto attempt = [&](const char* hostKeyAlgorithms) -> std::optional<detail::PresentedHostKey>
    {
        SessionHandle candidate = openSession(endpoint, user, hostKeyAlgorithms, timeout, error);
        if (!candidate)
            return std::nullopt;

        detail::PresentedHostKey key = detail::hostKey(candidate.get());
        if (!verified || key.fingerprint == hostKeyFingerprint)
            session = std::move(candidate);
        return key;
    };

    // First handshake: every supported key type, the server presents one of them. A failure here (unreachable
    // server, not an SSH server, no host key of a supported type) would fail for every key type.
    const std::optional<detail::PresentedHostKey> firstKey = attempt(detail::allHostKeyAlgorithms().c_str());
    if (!firstKey)
        DAQ_THROW_EXCEPTION(NotFoundException, "Failed to connect to {}:{}: {}", endpoint.host, endpoint.port, error);

    if (!session)
    {
        // The fingerprint may belong to another of the server's keys: try the remaining types one at a time. A
        // failed handshake here means the server has no key of that type.
        std::string presented = fmt::format("\n  {} {}", firstKey->typeName, firstKey->fingerprint);
        for (const auto& keyType : detail::HostKeyTypes)
        {
            if (firstKey->typeName == keyType.typeName)
                continue;

            const std::optional<detail::PresentedHostKey> key = attempt(keyType.algorithms);
            if (session)
                break;
            if (key)
                presented += fmt::format("\n  {} {}", key->typeName, key->fingerprint);
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

    sftp.reset(sftp_new(session.get()));
    if (!sftp || sftp_init(sftp.get()) != SSH_OK)
        DAQ_THROW_EXCEPTION(NotFoundException, "Failed to open the SFTP channel: {}", ssh_get_error(session.get()));
}

void SftpConnection::SessionDeleter::operator()(ssh_session session) const noexcept
{
    ssh_disconnect(session);
    ssh_free(session);
}

void SftpConnection::SftpChannelDeleter::operator()(sftp_session sftp) const noexcept
{
    sftp_free(sftp);
}

SftpConnection::SessionHandle SftpConnection::openSession(const Endpoint& endpoint,
                                                          const StringPtr& user,
                                                          const char* hostKeyAlgorithms,
                                                          std::chrono::milliseconds timeout,
                                                          StringPtr& error)
{
    SessionHandle session(ssh_new());
    if (!session)
        DAQ_THROW_EXCEPTION(GeneralErrorException, "Failed to create an SSH session");

    const bool processConfig = false;
    const unsigned int port = endpoint.port;
    const long timeoutSeconds = static_cast<long>(timeout.count() / 1000);
    const long timeoutMicroseconds = static_cast<long>(timeout.count() % 1000) * 1000;

    if (ssh_options_set(session.get(), SSH_OPTIONS_PROCESS_CONFIG, &processConfig) != SSH_OK ||
        ssh_options_set(session.get(), SSH_OPTIONS_KNOWNHOSTS, detail::NoKnownHostsFile) != SSH_OK ||
        ssh_options_set(session.get(), SSH_OPTIONS_GLOBAL_KNOWNHOSTS, detail::NoKnownHostsFile) != SSH_OK ||
        ssh_options_set(session.get(), SSH_OPTIONS_HOST, endpoint.host.getCharPtr()) != SSH_OK ||
        ssh_options_set(session.get(), SSH_OPTIONS_PORT, &port) != SSH_OK ||
        ssh_options_set(session.get(), SSH_OPTIONS_USER, user.getCharPtr()) != SSH_OK ||
        ssh_options_set(session.get(), SSH_OPTIONS_TIMEOUT, &timeoutSeconds) != SSH_OK ||
        ssh_options_set(session.get(), SSH_OPTIONS_TIMEOUT_USEC, &timeoutMicroseconds) != SSH_OK ||
        ssh_options_set(session.get(), SSH_OPTIONS_HOSTKEYS, hostKeyAlgorithms) != SSH_OK)
        DAQ_THROW_EXCEPTION(GeneralErrorException, "Failed to set SSH session options: {}", ssh_get_error(session.get()));

    if (ssh_connect(session.get()) != SSH_OK)
    {
        error = String(ssh_get_error(session.get()));
        return nullptr;
    }
    return session;
}

SftpConnection::~SftpConnection() = default;

bool SftpConnection::hostKeyVerified() const noexcept
{
    return verified;
}

void SftpConnection::authenticate(const AuthenticationConfigPtr& authenticationConfig)
{
    const StringPtr methodId = authenticationConfig.getSelectedAuthenticationMethod().getId();
    const DictPtr<IString, IString> credential = authenticationConfig.getSuppliedCredential();

    // The user name was set as a session option, so null is passed as the user below.
    if (methodId == SftpPrivateKeyFileId)
    {
        const StringPtr privateKeyFile = credential.get("PrivateKeyFilePath");

        // Optional: absent or empty if the key is not encrypted.
        const StringPtr passphrase = credential.hasKey("Passphrase") ? credential.get("Passphrase") : String("");

        // No prompt callback: an encrypted key without a passphrase fails instead of prompting.
        ssh_key rawKey = nullptr;
        const int rc = ssh_pki_import_privkey_file(privateKeyFile.getCharPtr(),
                                                   passphrase.getLength() == 0 ? nullptr : passphrase.getCharPtr(),
                                                   nullptr,
                                                   nullptr,
                                                   &rawKey);
        if (rc == SSH_EOF)
            DAQ_THROW_EXCEPTION(InvalidParameterException, "Private key file \"{}\" does not exist or cannot be read", privateKeyFile);
        if (rc != SSH_OK)
            DAQ_THROW_EXCEPTION(AuthenticationFailedException,
                                "Failed to load private key file \"{}\": wrong passphrase or unsupported format",
                                privateKeyFile);
        const detail::KeyHandle key(rawKey);

        if (ssh_userauth_publickey(session.get(), nullptr, key.get()) != SSH_AUTH_SUCCESS)
            DAQ_THROW_EXCEPTION(AuthenticationFailedException, "Private key authentication failed: {}", ssh_get_error(session.get()));
    }
    else
    {
        const StringPtr password = credential.get("Password");
        if (ssh_userauth_password(session.get(), nullptr, password.getCharPtr()) != SSH_AUTH_SUCCESS)
            DAQ_THROW_EXCEPTION(AuthenticationFailedException, "Password authentication failed: {}", ssh_get_error(session.get()));
    }
}

std::size_t SftpConnection::chunkSize(bool forWriting) const
{
    const detail::SftpLimitsHandle limits(sftp_limits(sftp.get()));
    if (!limits)
        DAQ_THROW_EXCEPTION(GeneralErrorException, "Failed to get SFTP limits: {}", ssh_get_error(session.get()));
    const std::uint64_t limit = forWriting ? limits->max_write_length : limits->max_read_length;
    if (limit == 0)
        DAQ_THROW_EXCEPTION(GeneralErrorException, "Server reported an SFTP {} limit of 0", forWriting ? "write" : "read");
    return static_cast<std::size_t>(std::min(limit, detail::MaxChunkSize));
}

void SftpConnection::upload(const StringPtr& localPath, const StringPtr& remotePath)
{
    // Local paths are passed to the standard library as narrow strings (on Windows: in the ANSI code page).
    std::ifstream source(localPath.toStdString(), std::ios::binary);
    if (!source)
        DAQ_THROW_EXCEPTION(NotFoundException, "Failed to open local file \"{}\" for reading", localPath);

    // 0644: owner read/write, group and others read; applied by the server only when it creates the file.
    detail::SftpFileHandle remote(sftp_open(sftp.get(), remotePath.getCharPtr(), O_WRONLY | O_CREAT | O_TRUNC, 0644));
    if (!remote)
        detail::throwFileError(session.get(), sftp.get(), "create remote file", remotePath);

    // Each chunk is copied into the request when it is sent, so one buffer serves all requests. Chunks never exceed
    // the server's write limit, so each is sent whole by a single request.
    std::vector<char> buffer(chunkSize(true));
    // Declared after the file handle, so abandoned requests are freed before the file is closed.
    std::deque<detail::SftpAioHandle> pending;

    const auto waitOldest = [&]
    {
        sftp_aio aio = pending.front().release();
        pending.pop_front();
        // Frees the request in blocking mode.
        if (sftp_aio_wait_write(&aio) == SSH_ERROR)
            detail::throwFileError(session.get(), sftp.get(), "write remote file", remotePath);
    };

    while (source)
    {
        source.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        const auto count = static_cast<std::size_t>(source.gcount());
        if (count == 0)
            break;

        if (pending.size() == detail::InFlightRequests)
            waitOldest();

        sftp_aio aio = nullptr;
        if (sftp_aio_begin_write(remote.get(), buffer.data(), count, &aio) == SSH_ERROR)
            detail::throwFileError(session.get(), sftp.get(), "write remote file", remotePath);
        pending.emplace_back(aio);
    }
    if (source.bad())
        DAQ_THROW_EXCEPTION(GeneralErrorException, "Failed to read local file \"{}\"", localPath);

    while (!pending.empty())
        waitOldest();

    // Closing completes the write on the server, so its result is checked.
    if (sftp_close(remote.release()) != SSH_NO_ERROR)
        detail::throwFileError(session.get(), sftp.get(), "close remote file", remotePath);
}

void SftpConnection::download(const StringPtr& remotePath, const StringPtr& localPath)
{
    detail::SftpFileHandle remote(sftp_open(sftp.get(), remotePath.getCharPtr(), O_RDONLY, 0));
    if (!remote)
        detail::throwFileError(session.get(), sftp.get(), "open remote file", remotePath);

    // The size decides how much to request, as libssh's guide to the aio API does.
    const detail::SftpAttributesHandle attributes(sftp_fstat(remote.get()));
    if (!attributes)
        detail::throwFileError(session.get(), sftp.get(), "get size of remote file", remotePath);
    if ((attributes->flags & SSH_FILEXFER_ATTR_SIZE) == 0)
        DAQ_THROW_EXCEPTION(GeneralErrorException, "Server did not report the size of remote file \"{}\"", remotePath);
    const std::uint64_t fileSize = attributes->size;

    // Opened only once the remote file is known to be readable, so a missing remote file leaves no local file behind.
    std::ofstream destination(localPath.toStdString(), std::ios::binary | std::ios::trunc);
    if (!destination)
        DAQ_THROW_EXCEPTION(GeneralErrorException, "Failed to open local file \"{}\" for writing", localPath);

    // Requests never exceed the server's read limit, so each is answered in full unless the file ends first.
    std::vector<char> buffer(chunkSize(false));
    std::uint64_t requested = 0;
    // Declared after the file handle, so abandoned requests are freed before the file is closed.
    std::deque<std::pair<detail::SftpAioHandle, std::size_t>> pending;

    const auto requestNext = [&]
    {
        const auto count = static_cast<std::size_t>(std::min<std::uint64_t>(buffer.size(), fileSize - requested));
        sftp_aio aio = nullptr;
        if (sftp_aio_begin_read(remote.get(), count, &aio) == SSH_ERROR)
            detail::throwFileError(session.get(), sftp.get(), "read remote file", remotePath);
        pending.emplace_back(detail::SftpAioHandle(aio), count);
        requested += count;
    };

    while (pending.size() < detail::InFlightRequests && requested < fileSize)
        requestNext();

    while (!pending.empty())
    {
        sftp_aio aio = pending.front().first.release();
        const std::size_t expected = pending.front().second;
        pending.pop_front();
        // Frees the request in blocking mode.
        const auto read = sftp_aio_wait_read(&aio, buffer.data(), buffer.size());
        if (read == SSH_ERROR)
            detail::throwFileError(session.get(), sftp.get(), "read remote file", remotePath);
        // A short answer means the file shrank while it was being read; the later chunks would leave a gap.
        if (static_cast<std::size_t>(read) != expected)
            DAQ_THROW_EXCEPTION(GeneralErrorException, "Remote file \"{}\" changed while it was being read", remotePath);

        if (!destination.write(buffer.data(), static_cast<std::streamsize>(read)))
            DAQ_THROW_EXCEPTION(GeneralErrorException, "Failed to write local file \"{}\"", localPath);

        if (requested < fileSize)
            requestNext();
    }

    destination.close();
    if (!destination)
        DAQ_THROW_EXCEPTION(GeneralErrorException, "Failed to write local file \"{}\"", localPath);
}

}
