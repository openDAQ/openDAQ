/**
 * SFTP file transfer example
 *
 * Connects to an SFTP server using user name and password authentication. Creating the file transfer
 * object connects, verifies the server host key fingerprint (if one is given), authenticates and opens the SFTP channel;
 * releasing it closes the connection.
 *
 * Adjust the constants below to match your server before running.
 */

#include <iostream>
#include <coreobjects/exceptions.h>
#include <sftp_file_transfer/libssh2/sftp_file_transfer_libssh2_factory.h>
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

int main(int /*argc*/, const char* /*argv*/[])
{
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
        const FileTransferPtr fileTransfer = SftpFileTransferLibssh2(Address, authConfig, Logger());
        std::cout << "Connected to " << Address << " as " << UserName << "; SFTP channel is open." << std::endl;
    }
    catch (const AuthenticationFailedException& e)
    {
        std::cerr << "Authentication failed: " << e.what() << std::endl;
        return 1;
    }
    catch (const NotFoundException& e)
    {
        std::cerr << "Failed to connect: " << e.what() << std::endl;
        return 1;
    }
    catch (const DaqException& e)
    {
        std::cerr << "Failed to connect: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "Connection closed." << std::endl;
    return 0;
}
