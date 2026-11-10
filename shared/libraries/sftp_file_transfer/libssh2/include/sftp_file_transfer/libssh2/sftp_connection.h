/*
 * Copyright 2022-2026 openDAQ d.o.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// Internal RAII layer over libssh2. Kept private so libssh2 headers never reach users of the library.
//
// Ownership and lifetime:
//
//   Library            per connection, counted process-wide: the first live connection initializes libssh2 (and
//                      Winsock on Windows), the last one cleans them up.
//   SftpFileTransferImpl
//    +- SftpConnection one per file transfer object; closed when the last FileTransferPtr is released.
//        +- Socket     the TCP connection.
//        +- session    the SSH session over that socket (a handle, see SessionDeleter).
//        +- SFTP channel, opened from the session; invalid once the session is gone.
//            +- file handle, opened from the channel for the duration of one upload or download.
//
// Socket, session and channel always live and die together inside SftpConnection and are torn down in reverse:
// channel, session (disconnect, then free), socket, then the connection's reference to the Library.
//
// While connecting, one socket + session pair is opened, and only if its host key does not match, up to one more
// per remaining host key type (see SftpConnection). Only the matching pair is kept; the others are closed right away.
//
// Threading: a libssh2 session must not be used from several threads at once, so a connection is single-threaded.
// Separate connections may be used in parallel; libssh2's global init state is the only thing they share, and it is
// lock-protected (see Library).
//
// Every failure is reported as an openDAQ exception. Messages name what failed, never credential values.

#pragma once
// libssh2.h first: on Windows it brings in winsock2.h, which must precede any windows.h.
#include <libssh2.h>
#include <libssh2_sftp.h>

#include <sftp_file_transfer/sftp_arguments.h>

#include <chrono>
#include <memory>

namespace daq::sftp::libssh2
{

// An authenticated SSH connection in blocking mode with an open SFTP channel. Any session-level error leaves it
// unusable; recovery is to destroy it and create a new one.
//
// The fingerprint is the optional SftpHostKeyFingerprintField of the supplied credential, compared as is with the
// fingerprint of the key the server presents. A server presents one host key per handshake, picked from the key
// types the client offers, and a fingerprint does not say which key type it belongs to. So the first handshake
// offers every supported type (ED25519, ECDSA, RSA, in that order of preference). Only if the presented key does not
// match, the remaining types are tried one at a time, each in a new connection. Nothing is authenticated before a
// match.
//
// With an empty fingerprint the host key is NOT verified: the key presented in the first handshake is accepted.
class SftpConnection
{
public:
    // The authentication config is read during construction only and not kept.
    SftpConnection(const Endpoint& endpoint,
                   const AuthenticationConfigPtr& authenticationConfig,
                   std::chrono::milliseconds timeout = std::chrono::seconds(5));
    ~SftpConnection();
    SftpConnection(const SftpConnection&) = delete;
    SftpConnection& operator=(const SftpConnection&) = delete;

    // False if no fingerprint was supplied and the connection was made without host key verification.
    bool hostKeyVerified() const noexcept;

    // Copies the local file to the server, creating or overwriting the remote file. Blocks until done.
    void upload(const StringPtr& localPath, const StringPtr& remotePath);

    // Copies the remote file to the local file, creating or overwriting it. Blocks until done; on failure the local
    // file may be left incomplete.
    void download(const StringPtr& remotePath, const StringPtr& localPath);

private:
    // libssh2_init in the constructor, libssh2_exit in the destructor (see the .cpp).
    class Library
    {
    public:
        Library();
        ~Library();
        Library(const Library&) = delete;
        Library& operator=(const Library&) = delete;
    };

    // Defined in the .cpp.
    class Socket;

    // Closes a handshaked SSH session: disconnect, then free.
    struct SessionDeleter
    {
        void operator()(LIBSSH2_SESSION* session) const noexcept;
    };

    // Shuts the SFTP channel down.
    struct SftpChannelDeleter
    {
        void operator()(LIBSSH2_SFTP* sftp) const noexcept;
    };

    using SessionHandle = std::unique_ptr<LIBSSH2_SESSION, SessionDeleter>;
    using SftpChannelHandle = std::unique_ptr<LIBSSH2_SFTP, SftpChannelDeleter>;

    // Creates a blocking session and performs the handshake over the connected socket, allowing only the given host
    // key algorithms (comma separated). Throws NoMatchingHostKeyType if the server has no host key of those types,
    // NotFoundException on any other failure.
    static SessionHandle openSession(libssh2_socket_t socket, const char* hostKeyAlgorithms, std::chrono::milliseconds timeout);

    // Authenticates with the method selected in the config and its supplied credential, already validated by
    // validateArguments. Throws AuthenticationFailedException if the server rejects the credential.
    void authenticate(const AuthenticationConfigPtr& authenticationConfig);

    bool verified = false;

    // Declaration order is destruction order in reverse: channel, session, socket, library.
    Library library;
    std::unique_ptr<Socket> socket;
    SessionHandle session;
    SftpChannelHandle sftp;
};

}
