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
 * "libssh2-async" makes the same round trip with the asynchronous libssh2 backend: the transfers run on the file
 * transfer object's I/O thread, and their status changes are printed as they happen. The upload is waited for with
 * wait(), like a blocking call; the download's completion is taken from its status change event via a promise.
 *
 * Adjust the constants below to match your server before running.
 */

#include <fstream>
#include <future>
#include <iostream>
#include <iterator>
#include <string>
#include <coreobjects/exceptions.h>
#ifdef SFTP_EXAMPLE_WITH_LIBSSH2
#include <sftp_file_transfer/libssh2/sftp_file_transfer_libssh2_factory.h>
#include <sftp_file_transfer/libssh2/sftp_file_transfer_async_libssh2_factory.h>
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
    // Each backend has its own properties, e.g. chunk size or ciphers; adjust them on its default config before
    // connecting, e.g. config.setPropertyValue("ChunkSize", 1024 * 1024).
#ifdef SFTP_EXAMPLE_WITH_LIBSSH2
    if (backend == "libssh2")
        return SftpFileTransferLibssh2(Address, authConfig, Logger(), SftpFileTransferLibssh2DefaultConfig());
#endif
#ifdef SFTP_EXAMPLE_WITH_LIBSSH
    if (backend == "libssh")
        return SftpFileTransferLibssh(Address, authConfig, Logger(), SftpFileTransferLibsshDefaultConfig());
#endif
    DAQ_THROW_EXCEPTION(InvalidParameterException, "Backend \"{}\" is not built into this example", backend);
}

static std::string readFile(const char* path)
{
    std::ifstream file(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

#ifdef SFTP_EXAMPLE_WITH_LIBSSH2
// The round trip with the asynchronous backend. upload and download only create the transfers; start hands them to
// the file transfer object's I/O thread and returns at once, and wait blocks until a transfer has finished.
static void roundTripAsync(const AuthenticationConfigPtr& authConfig)
{
    // Its properties include, besides chunk size and ciphers, how many transfers run at once and how a lost
    // connection is restored, e.g. config.setPropertyValue("ReconnectAttempts", 10).
    const FileTransferAsyncPtr fileTransfer =
        SftpFileTransferAsyncLibssh2(Address, authConfig, Logger(), SftpFileTransferAsyncLibssh2DefaultConfig());
    std::cout << "Connected to " << Address << " as " << UserName << " using libssh2-async." << std::endl;

    // Every status change of the connection and of its transfers, reported on the I/O thread.
    fileTransfer.getOnStatusChanged() += [](const BaseObjectPtr& sender, const CoreEventArgsPtr& args)
    {
        const auto parameters = args.getParameters();
        if (const auto transfer = sender.asPtrOrNull<IFileTransferOperation>(); transfer.assigned())
            std::cout << "  " << transfer.getRemotePath() << ": " << parameters.get(FileTransferStatusName) << std::endl;
        else
            std::cout << "  Connection: " << parameters.get(ConnectionStatusName) << " " << parameters.get("Message") << std::endl;
    };

    const FileTransferOperationPtr upload = fileTransfer.upload(LocalFile, RemoteFile);
    upload.start();
    upload.wait();  // throws as the blocking upload would if the transfer failed
    std::cout << "Uploaded " << LocalFile << " to " << RemoteFile << " (" << upload.getBytesTransferred() << " bytes)." << std::endl;

    // The download's completion comes from its own status change event: the handler fulfils a promise once the status
    // is final, and this thread waits on the future. Subscribed before start, so no change is missed.
    std::promise<void> downloadDone;
    const FileTransferOperationPtr download = fileTransfer.download(RemoteFile, DownloadedFile);
    download.getOnStatusChanged() += [&downloadDone](const FileTransferOperationPtr& /*transfer*/, const CoreEventArgsPtr& args)
    {
        const auto parameters = args.getParameters();
        const StringPtr status = parameters.get(FileTransferStatusName).asPtr<IEnumeration>().getValue();
        if (status == "Completed")
            downloadDone.set_value();
        else if (status == "Failed")
            downloadDone.set_exception(std::make_exception_ptr(GeneralErrorException("{}", parameters.get("Message"))));
        else if (status == "Canceled")
            downloadDone.set_exception(std::make_exception_ptr(GeneralErrorException("The download was canceled")));
    };
    download.start();
    downloadDone.get_future().get();  // throws the exception set by the handler
    std::cout << "Downloaded " << RemoteFile << " to " << DownloadedFile << " (" << download.getBytesTransferred() << " bytes)."
              << std::endl;
}
#endif

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
        const std::string content = "Hello from the openDAQ SFTP file transfer example.\n";
        std::ofstream(LocalFile, std::ios::binary) << content;

#ifdef SFTP_EXAMPLE_WITH_LIBSSH2
        if (backend == "libssh2-async")
            roundTripAsync(authConfig);
        else
#endif
        {
            const FileTransferPtr fileTransfer = connect(backend, authConfig);
            std::cout << "Connected to " << Address << " as " << UserName << " using " << backend << "; SFTP channel is open."
                      << std::endl;

            fileTransfer.upload(LocalFile, RemoteFile);
            std::cout << "Uploaded " << LocalFile << " to " << RemoteFile << "." << std::endl;

            fileTransfer.download(RemoteFile, DownloadedFile);
            std::cout << "Downloaded " << RemoteFile << " to " << DownloadedFile << "." << std::endl;
        }

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
