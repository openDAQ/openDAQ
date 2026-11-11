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
#include <opendaq/logger.h>
#include <opendaq/logger_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    INTERNAL_FACTORY, SftpFileTransferLibssh, IFileTransfer,
    IString*, address,
    IAuthenticationConfig*, authenticationConfig,
    ILogger*, logger
)

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
 * @throws InvalidParameterException if the logger is not assigned, or the address or authentication config is
 * malformed.
 * @throws CredentialsRequiredException if no credential is supplied in the authentication config.
 * @throws NotFoundException if the server cannot be reached or the SSH/SFTP session cannot be established.
 * @throws AuthenticationFailedException if none of the server's host keys has the fingerprint (the message lists
 * the type and fingerprint of every key the server presented), or if the server rejects the credential.
 */
inline FileTransferPtr SftpFileTransferLibssh(const StringPtr& address,
                                              const AuthenticationConfigPtr& authenticationConfig,
                                              const LoggerPtr& logger)
{
    FileTransferPtr obj(SftpFileTransferLibssh_Create(address, authenticationConfig, logger));
    return obj;
}

END_NAMESPACE_OPENDAQ
