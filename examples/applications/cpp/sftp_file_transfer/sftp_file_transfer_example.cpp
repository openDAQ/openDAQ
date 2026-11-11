/**
 * SFTP file transfer example
 *
 * Connects to an SFTP server using user name and password authentication. Creating the file transfer
 * object connects, verifies the server host key fingerprint (if one is given), authenticates and opens the SFTP channel;
 * releasing it closes the connection.
 *
 * Once connected, it writes a small local text file, uploads it to the server, downloads it back under another name
 * and checks that the content survived the round trip. Both transfers are blocking.
 *
 * The SSH library backend is chosen with the first argument: "libssh2" or "libssh", whichever were built
 * (OPENDAQ_ENABLE_SFTP_FILE_TRANSFER_LIBSSH2 / _LIBSSH). Without an argument, the first one built is used.
 *
 * Adjust the constants below to match your server before running.
 */

#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <coreobjects/exceptions.h>
#ifdef SFTP_EXAMPLE_WITH_LIBSSH2
#include <sftp_file_transfer/libssh2/sftp_file_transfer_libssh2_factory.h>
#endif
#ifdef SFTP_EXAMPLE_WITH_LIBSSH
#include <sftp_file_transfer/libssh/sftp_file_transfer_libssh_factory.h>
#endif
#include <coretypes/dictobject_factory.h>
#include <opendaq/logger_factory.h>

using namespace daq;

// Server address: "host", "host:port", "[IPv6]" or "[IPv6]:port". The port defaults to 22.
static const char* Address = "10.1.62.22:22";

// SSH account used to log in.
static const char* UserName = "sftpuser";
static const char* Password = "qwerty";

// SHA256 fingerprint of one of the server's host keys. Only a server with this key is accepted.
// Leave it empty to connect without host key verification; a warning then lists the server's fingerprints.
static const char* HostKeyFingerprint = "SHA256:OjE3Qif5WaW/1VmSridhMuyH4YsWc7g2rNwJ7lZ/qAc";

// Files of the round trip. The local ones are created in the working directory. The remote path is relative to the
// login directory of the user on the server and must be in a directory the user may write to.
static const char* LocalFile = "sftp_example_upload.txt";
static const char* RemoteFile = "files/sftp_example.txt";
static const char* DownloadedFile = "sftp_example_download.txt";

// The backends built into this example, in the order of preference when none is chosen.
static const char* Backends[] = {
#ifdef SFTP_EXAMPLE_WITH_LIBSSH2
    "libssh2",
#endif
#ifdef SFTP_EXAMPLE_WITH_LIBSSH
    "libssh",
#endif
};

static FileTransferPtr connect(const std::string& backend, const AuthenticationConfigPtr& authConfig)
{
#ifdef SFTP_EXAMPLE_WITH_LIBSSH2
    if (backend == "libssh2")
        return SftpFileTransferLibssh2(Address, authConfig, Logger());
#endif
#ifdef SFTP_EXAMPLE_WITH_LIBSSH
    if (backend == "libssh")
        return SftpFileTransferLibssh(Address, authConfig, Logger());
#endif
    DAQ_THROW_EXCEPTION(InvalidParameterException, "Backend \"{}\" is not built into this example", backend);
}

static std::string readFile(const char* path)
{
    std::ifstream file(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

int main(int argc, const char* argv[])
{
    const std::string backend = argc > 1 ? argv[1] : Backends[0];

    // Select user name and password authentication and supply its credential.
    const AuthenticationConfigPtr authConfig = SftpAuthenticationConfig();
    authConfig.setAuthenticationMethod(SftpUserNamePasswordId);
    authConfig.setSuppliedCredential(Dict<IString, IString>({
        {"UserName", UserName},
        {"Password", Password},
        {SftpHostKeyFingerprintField, ""},
    }));

    try
    {
        const FileTransferPtr fileTransfer = connect(backend, authConfig);
        std::cout << "Connected to " << Address << " as " << UserName << " using " << backend << "; SFTP channel is open."
                  << std::endl;

        const std::string content = "Hello from the openDAQ SFTP file transfer example.\n";
        std::ofstream(LocalFile, std::ios::binary) << content;

        fileTransfer.upload(LocalFile, RemoteFile);
        std::cout << "Uploaded " << LocalFile << " to " << RemoteFile << "." << std::endl;

        fileTransfer.download(RemoteFile, DownloadedFile);
        std::cout << "Downloaded " << RemoteFile << " to " << DownloadedFile << "." << std::endl;

        if (readFile(DownloadedFile) != content)
        {
            std::cerr << "The downloaded file differs from the uploaded one." << std::endl;
            return 1;
        }
        std::cout << "The downloaded file matches the uploaded one." << std::endl;
    }
    catch (const AuthenticationFailedException& e)
    {
        std::cerr << "Authentication failed: " << e.what() << std::endl;
        return 1;
    }
    catch (const DaqException& e)
    {
        std::cerr << "Failed: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "Connection closed." << std::endl;
    return 0;
}
