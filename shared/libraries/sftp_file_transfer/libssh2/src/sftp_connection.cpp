#include <sftp_file_transfer/libssh2/sftp_connection.h>

#include <coretypes/exceptions.h>

#include <fstream>
#include <vector>

namespace daq::sftp::libssh2
{

namespace detail
{

// Closes a remote file; closing releases it on the server.
struct SftpFileCloser
{
    void operator()(LIBSSH2_SFTP_HANDLE* handle) const noexcept
    {
        libssh2_sftp_close_handle(handle);
    }
};

// An open remote file.
using SftpFileHandle = std::unique_ptr<LIBSSH2_SFTP_HANDLE, SftpFileCloser>;

}

SftpConnection::SftpConnection(const Endpoint& endpoint,
                               const AuthenticationConfigPtr& authenticationConfig,
                               const ConnectionProperties& properties,
                               std::chrono::milliseconds timeout)
    : chunkSize(properties.chunkSize)
    , connection(endpoint, authenticationConfig, properties.ciphers, timeout)
{
    sftp.reset(libssh2_sftp_init(connection.session()));
    if (!sftp)
        DAQ_THROW_EXCEPTION(NotFoundException, "Failed to open the SFTP channel: {}", lastErrorMessage(connection.session()));
}

SftpConnection::~SftpConnection() = default;

void SftpConnection::SftpChannelDeleter::operator()(LIBSSH2_SFTP* sftp) const noexcept
{
    libssh2_sftp_shutdown(sftp);
}

bool SftpConnection::hostKeyVerified() const noexcept
{
    return connection.hostKeyVerified();
}

void SftpConnection::upload(const StringPtr& localPath, const StringPtr& remotePath)
{
    // Local paths are passed to the standard library as narrow strings (on Windows: in the ANSI code page).
    std::ifstream source(localPath.toStdString(), std::ios::binary);
    if (!source)
        DAQ_THROW_EXCEPTION(NotFoundException, "Failed to open local file \"{}\" for reading", localPath);

    constexpr long Mode = LIBSSH2_SFTP_S_IRUSR | LIBSSH2_SFTP_S_IWUSR | LIBSSH2_SFTP_S_IRGRP | LIBSSH2_SFTP_S_IROTH;  // 0644
    detail::SftpFileHandle remote(libssh2_sftp_open_ex(sftp.get(),
                                                       remotePath.getCharPtr(),
                                                       static_cast<unsigned int>(remotePath.getLength()),
                                                       LIBSSH2_FXF_WRITE | LIBSSH2_FXF_CREAT | LIBSSH2_FXF_TRUNC,
                                                       Mode,
                                                       LIBSSH2_SFTP_OPENFILE));
    if (!remote)
        throwFileError(connection.session(), sftp.get(), "create remote file", remotePath);

    std::vector<char> buffer(chunkSize);
    while (source)
    {
        source.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        const char* data = buffer.data();
        auto remaining = static_cast<std::size_t>(source.gcount());
        while (remaining > 0)
        {
            const auto written = libssh2_sftp_write(remote.get(), data, remaining);
            if (written < 0)
                throwFileError(connection.session(), sftp.get(), "write remote file", remotePath);
            data += written;
            remaining -= static_cast<std::size_t>(written);
        }
    }
    if (source.bad())
        DAQ_THROW_EXCEPTION(GeneralErrorException, "Failed to read local file \"{}\"", localPath);

    // Closing completes the write on the server, so its result is checked.
    if (libssh2_sftp_close_handle(remote.release()) != 0)
        throwFileError(connection.session(), sftp.get(), "close remote file", remotePath);
}

void SftpConnection::download(const StringPtr& remotePath, const StringPtr& localPath)
{
    detail::SftpFileHandle remote(libssh2_sftp_open_ex(sftp.get(),
                                                       remotePath.getCharPtr(),
                                                       static_cast<unsigned int>(remotePath.getLength()),
                                                       LIBSSH2_FXF_READ,
                                                       0,
                                                       LIBSSH2_SFTP_OPENFILE));
    if (!remote)
        throwFileError(connection.session(), sftp.get(), "open remote file", remotePath);

    // Opened only once the remote file is known to be readable, so a missing remote file leaves no local file behind.
    std::ofstream destination(localPath.toStdString(), std::ios::binary | std::ios::trunc);
    if (!destination)
        DAQ_THROW_EXCEPTION(GeneralErrorException, "Failed to open local file \"{}\" for writing", localPath);

    std::vector<char> buffer(chunkSize);
    for (;;)
    {
        const auto read = libssh2_sftp_read(remote.get(), buffer.data(), buffer.size());
        if (read < 0)
            throwFileError(connection.session(), sftp.get(), "read remote file", remotePath);
        if (read == 0)
            break;
        if (!destination.write(buffer.data(), static_cast<std::streamsize>(read)))
            DAQ_THROW_EXCEPTION(GeneralErrorException, "Failed to write local file \"{}\"", localPath);
    }

    destination.close();
    if (!destination)
        DAQ_THROW_EXCEPTION(GeneralErrorException, "Failed to write local file \"{}\"", localPath);
}

}
