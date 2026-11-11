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

// Internal RAII layer over libssh. Kept private so libssh headers never reach users of the library.
//
// Ownership and lifetime:
//
//   SftpFileTransferImpl
//    +- SftpConnection one per file transfer object; closed when the last FileTransferPtr is released.
//        +- session    the SSH session, which owns its TCP socket (a handle, see SessionDeleter).
//        +- SFTP channel, opened from the session; invalid once the session is gone.
//            +- file handle, opened from the channel for the duration of one upload or download.
//
// Unlike libssh2, libssh needs no global init call when linked as a shared library (it initializes itself when
// loaded) and creates the socket itself, including the Winsock setup on Windows.
//
// While connecting, one session is opened, and only if its host key does not match, up to one more per remaining
// host key type (see SftpConnection). Only the matching session is kept; the others are closed right away.
//
// Threading: a libssh session must not be used from several threads at once, so a connection is single-threaded.
// Separate connections may be used in parallel.
//
// Uploads and downloads are pipelined with libssh's asynchronous SFTP requests: several chunks are in flight at a
// time instead of one request per network round trip.
//
// Every failure is reported as an openDAQ exception. Messages name what failed, never credential values.

#pragma once
#include <libssh/libssh.h>
#include <libssh/sftp.h>

#include <sftp_file_transfer/sftp_arguments.h>

#include <chrono>
#include <cstddef>
#include <memory>

namespace daq::sftp::libssh
{

// An authenticated SSH connection in blocking mode with an open SFTP channel. Any session-level error leaves it
// unusable; recovery is to destroy it and create a new one.
//
// Nothing is read from the environment: no SSH config files, no known_hosts files (libssh would otherwise use them
// to reorder the offered host key types), no ssh-agent and no default identity files.
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
    // Closes an SSH session: disconnect (a no-op if it never connected), then free.
    struct SessionDeleter
    {
        void operator()(ssh_session session) const noexcept;
    };

    // Closes the SFTP channel.
    struct SftpChannelDeleter
    {
        void operator()(sftp_session sftp) const noexcept;
    };

    using SessionHandle = std::unique_ptr<ssh_session_struct, SessionDeleter>;
    using SftpChannelHandle = std::unique_ptr<sftp_session_struct, SftpChannelDeleter>;

    // Creates a blocking session for the user and connects it (TCP connection and handshake), allowing only the
    // given host key algorithms (comma separated). Returns null if the connection or handshake failed; the error is
    // then in `error`.
    static SessionHandle openSession(const Endpoint& endpoint,
                                     const StringPtr& user,
                                     const char* hostKeyAlgorithms,
                                     std::chrono::milliseconds timeout,
                                     StringPtr& error);

    // Authenticates with the method selected in the config and its supplied credential, already validated by
    // validateArguments. Throws AuthenticationFailedException if the server rejects the credential.
    void authenticate(const AuthenticationConfigPtr& authenticationConfig);

    // Bytes per read or write request: the server's per-request limit, capped to bound memory use.
    std::size_t chunkSize(bool forWriting) const;

    bool verified = false;

    // Declaration order is destruction order in reverse: channel, then session.
    SessionHandle session;
    SftpChannelHandle sftp;
};

}
