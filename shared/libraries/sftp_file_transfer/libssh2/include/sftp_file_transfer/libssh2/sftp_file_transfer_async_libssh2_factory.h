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
#pragma once
#include <sftp_file_transfer/file_transfer_async_ptr.h>
#include <sftp_file_transfer/file_transfer_operation_ptr.h>
#include <sftp_file_transfer/file_transfer_status.h>
#include <sftp_file_transfer/sftp_authentication_config_factory.h>
#include <coretypes/string_ptr.h>
#include <coreobjects/property_factory.h>
#include <coreobjects/property_object_factory.h>
#include <opendaq/logger.h>
#include <opendaq/logger_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    INTERNAL_FACTORY, SftpFileTransferAsyncLibssh2, IFileTransferAsync,
    IString*, address,
    IAuthenticationConfig*, authenticationConfig,
    ILogger*, logger,
    IPropertyObject*, config
)

/*!
 * @brief Creates the default config of the asynchronous libssh2 backend, to adjust and pass to
 * `SftpFileTransferAsyncLibssh2`.
 *
 * Properties:
 * - `ChunkSize` (bytes, default 32 KiB, 1 KiB to 8 MiB): data handed to libssh2 per read or write call, as for
 *   `SftpFileTransferLibssh2DefaultConfig`, per transfer.
 * - `Ciphers` (default empty): as for `SftpFileTransferLibssh2DefaultConfig`.
 * - `MaxActiveTransfers` (default 4, 1 to 64): transfers running at the same time, each over its own SFTP channel;
 *   further ones wait in the status `Queued`. Servers limit the channels per connection, OpenSSH to 10 by default
 *   (`MaxSessions`); one more channel is used for keepalive requests.
 * - `ConnectionTimeout` (ms, default 10000, 100 to 600000): time allowed for connecting, and for the server to answer
 *   while it owes a reply; the connection counts as lost after that.
 * - `KeepaliveInterval` (ms, default 15000, 1000 to 3600000): time between keepalive requests while no transfer is
 *   running. They detect a lost connection and keep the server and network devices from closing an idle one; keep
 *   it shorter than such idle timeouts (OpenSSH: `ClientAliveInterval` times `ClientAliveCountMax`).
 * - `ReconnectAttempts` (default 5, 0 to 1000): attempts to restore a lost connection before giving up; 0 gives up at
 *   once. A rejected host key or credential gives up at once too.
 * - `ReconnectInterval` (ms, default 2000, 100 to 600000): time between attempts to restore the connection; the first
 *   attempt is made right away.
 */
inline PropertyObjectPtr SftpFileTransferAsyncLibssh2DefaultConfig()
{
    auto config = PropertyObject();
    config.addProperty(IntPropertyBuilder("ChunkSize", 32 * 1024)
                           .setMinValue(1024)
                           .setMaxValue(8 * 1024 * 1024)
                           .setDescription("Bytes handed to libssh2 per read or write call")
                           .build());
    config.addProperty(StringPropertyBuilder("Ciphers", "")
                           .setDescription("Comma-separated SSH cipher names to offer, in order of preference, e.g. "
                                           "\"chacha20-poly1305@openssh.com,aes128-gcm@openssh.com\"; empty for libssh2's default")
                           .build());
    config.addProperty(IntPropertyBuilder("MaxActiveTransfers", 4)
                           .setMinValue(1)
                           .setMaxValue(64)
                           .setDescription("Transfers running at the same time, each over its own SFTP channel")
                           .build());
    config.addProperty(IntPropertyBuilder("ConnectionTimeout", 10000)
                           .setMinValue(100)
                           .setMaxValue(600000)
                           .setDescription("Milliseconds allowed for connecting and for the server to answer")
                           .build());
    config.addProperty(IntPropertyBuilder("KeepaliveInterval", 15000)
                           .setMinValue(1000)
                           .setMaxValue(3600000)
                           .setDescription("Milliseconds between keepalive requests while no transfer is running")
                           .build());
    config.addProperty(IntPropertyBuilder("ReconnectAttempts", 5)
                           .setMinValue(0)
                           .setMaxValue(1000)
                           .setDescription("Attempts to restore a lost connection; 0 to give up at once")
                           .build());
    config.addProperty(IntPropertyBuilder("ReconnectInterval", 2000)
                           .setMinValue(100)
                           .setMaxValue(600000)
                           .setDescription("Milliseconds between attempts to restore a lost connection")
                           .build());
    return config;
}

/*!
 * @brief Connects to an SFTP server using the libssh2 backend for asynchronous transfers, verifies its host key
 * fingerprint and authenticates, as `SftpFileTransferLibssh2` does.
 *
 * Unlike `SftpFileTransferLibssh2`, the returned object keeps the authentication config, credential included, for its
 * whole lifetime: it is needed to restore a lost connection.
 *
 * @param address The server address: `host`, `host:port`, `[IPv6]` or `[IPv6]:port`. The port defaults to 22.
 * @param authenticationConfig A config created by `SftpAuthenticationConfig`, with a method selected and its
 * credential supplied.
 * @param logger Logger used by the file transfer object, e.g. for the warning when the host key is not verified and
 * for lost and restored connections.
 * @param config Backend properties: the config created by `SftpFileTransferAsyncLibssh2DefaultConfig` (the default),
 * with values adjusted as needed. Read during this call only.
 * @throws InvalidParameterException, CredentialsRequiredException, NotFoundException, AuthenticationFailedException as
 * `SftpFileTransferLibssh2`.
 */
inline FileTransferAsyncPtr SftpFileTransferAsyncLibssh2(const StringPtr& address,
                                                         const AuthenticationConfigPtr& authenticationConfig,
                                                         const LoggerPtr& logger,
                                                         const PropertyObjectPtr& config = SftpFileTransferAsyncLibssh2DefaultConfig())
{
    FileTransferAsyncPtr obj(SftpFileTransferAsyncLibssh2_Create(address, authenticationConfig, logger, config));
    return obj;
}

END_NAMESPACE_OPENDAQ
