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

// Blocking SFTP over an SshConnection (see ssh_connection.h for ownership and threading).
//
//   SftpFileTransferImpl
//    +- SftpConnection one per file transfer object; closed when the last FileTransferPtr is released.
//        +- SshConnection  socket and session.
//        +- SFTP channel, opened from the session; invalid once the session is gone.
//            +- file handle, opened from the channel for the duration of one upload or download.

#pragma once
#include <sftp_file_transfer/libssh2/ssh_connection.h>

#include <chrono>
#include <cstddef>
#include <memory>

namespace daq::sftp::libssh2
{

// Properties from the backend config (see SftpFileTransferLibssh2DefaultConfig), already validated.
struct ConnectionProperties
{
    std::size_t chunkSize = 0;  // bytes per libssh2 read or write call
    StringPtr ciphers;          // comma separated; empty for libssh2's default
};

// An authenticated SSH connection in blocking mode with an open SFTP channel. Any session-level error leaves it
// unusable; recovery is to destroy it and create a new one.
class SftpConnection
{
public:
    // The authentication config is read during construction only and not kept.
    SftpConnection(const Endpoint& endpoint,
                   const AuthenticationConfigPtr& authenticationConfig,
                   const ConnectionProperties& properties,
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
    // Shuts the SFTP channel down.
    struct SftpChannelDeleter
    {
        void operator()(LIBSSH2_SFTP* sftp) const noexcept;
    };

    using SftpChannelHandle = std::unique_ptr<LIBSSH2_SFTP, SftpChannelDeleter>;

    std::size_t chunkSize;

    // Declaration order is destruction order in reverse: channel, then connection.
    SshConnection connection;
    SftpChannelHandle sftp;
};

}
