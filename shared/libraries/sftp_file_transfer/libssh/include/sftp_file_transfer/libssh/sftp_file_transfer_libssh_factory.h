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
#include <sftp_file_transfer/file_transfer_ptr.h>
#include <sftp_file_transfer/sftp_authentication_config_factory.h>
#include <coretypes/string_ptr.h>
#include <coreobjects/property_factory.h>
#include <coreobjects/property_object_factory.h>
#include <opendaq/logger.h>
#include <opendaq/logger_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    INTERNAL_FACTORY, SftpFileTransferLibssh, IFileTransfer,
    IString*, address,
    IAuthenticationConfig*, authenticationConfig,
    ILogger*, logger,
    IPropertyObject*, config
)

/*!
 * @brief Creates the default config of the libssh backend, to adjust and pass to `SftpFileTransferLibssh`.
 *
 * Transfers are pipelined: several read or write requests of one chunk each are in flight at a time. Their product,
 * the data in flight, decides the speed over links with longer round trips; beyond about 2 MiB (the SSH channel
 * window) it no longer helps.
 *
 * Properties:
 * - `MaxChunkSize` (bytes, default 256 KiB, 1 KiB to 4 MiB): upper bound for the data of one request. The server's
 *   own per-request limit applies as well (about 255 KiB for OpenSSH, 32 KiB for servers that report none).
 * - `RequestsInFlight` (default 16, 1 to 256): requests sent before waiting for the oldest to complete.
 * - `Ciphers` (default empty): comma-separated SSH cipher names to offer, in order of preference, e.g.
 *   `chacha20-poly1305@openssh.com,aes128-gcm@openssh.com`. Names libssh does not support are ignored. Empty offers
 *   libssh's default list. Useful on servers with a slow CPU, where encryption can limit the transfer speed.
 */
inline PropertyObjectPtr SftpFileTransferLibsshDefaultConfig()
{
    auto config = PropertyObject();
    config.addProperty(IntPropertyBuilder("MaxChunkSize", 256 * 1024)
                           .setMinValue(1024)
                           .setMaxValue(4 * 1024 * 1024)
                           .setDescription("Upper bound for the bytes of one read or write request")
                           .build());
    config.addProperty(IntPropertyBuilder("RequestsInFlight", 16)
                           .setMinValue(1)
                           .setMaxValue(256)
                           .setDescription("Read or write requests sent before waiting for the oldest to complete")
                           .build());
    config.addProperty(StringPropertyBuilder("Ciphers", "")
                           .setDescription("Comma-separated SSH cipher names to offer, in order of preference, e.g. "
                                           "\"chacha20-poly1305@openssh.com,aes128-gcm@openssh.com\"; empty for libssh's default")
                           .build());
    return config;
}

/*!
 * @brief Connects to an SFTP server using the libssh backend, verifies its host key fingerprint, authenticates and
 * opens the SFTP channel.
 *
 * libssh is licensed under LGPL-2.1 and is linked as a shared library, which has to be shipped with the application.
 *
 * Nothing is read from the user or system environment: no ssh-agent, no `known_hosts`, no SSH config
 * files and no prompting. Everything needed to connect is passed in explicitly. The host key fingerprint plays
 * the role of a `known_hosts` entry; credentials are sent only after the server's key matched it. Without a
 * fingerprint the key the server presents is accepted unverified and a warning is logged.
 *
 * @param address The server address: `host`, `host:port`, `[IPv6]` or `[IPv6]:port`. The port defaults to 22.
 * @param authenticationConfig A config created by `SftpAuthenticationConfig`, with a method selected and its
 * credential supplied. The credential is used during this call only and is not kept by the returned object.
 * @param logger Logger used by the file transfer object, e.g. for the warning when the host key is not verified.
 * @param config Backend properties: the config created by `SftpFileTransferLibsshDefaultConfig` (the
 * default), with values adjusted as needed. Read during this call only.
 * @throws InvalidParameterException if the logger is not assigned, the address or authentication config is
 * malformed, the config is not assigned, misses a property or holds a value of the wrong type or a non-positive
 * size or count, or none of the configured ciphers is supported.
 * @throws CredentialsRequiredException if no credential is supplied in the authentication config.
 * @throws NotFoundException if the server cannot be reached or the SSH/SFTP session cannot be established, e.g.
 * because the server supports none of the configured ciphers.
 * @throws AuthenticationFailedException if none of the server's host keys has the fingerprint (the message lists
 * the type and fingerprint of every key the server presented), or if the server rejects the credential.
 */
inline FileTransferPtr SftpFileTransferLibssh(const StringPtr& address,
                                              const AuthenticationConfigPtr& authenticationConfig,
                                              const LoggerPtr& logger,
                                              const PropertyObjectPtr& config = SftpFileTransferLibsshDefaultConfig())
{
    FileTransferPtr obj(SftpFileTransferLibssh_Create(address, authenticationConfig, logger, config));
    return obj;
}

END_NAMESPACE_OPENDAQ
