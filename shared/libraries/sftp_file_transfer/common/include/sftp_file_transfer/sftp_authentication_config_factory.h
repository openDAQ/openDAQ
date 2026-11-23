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
#include <coretypes/dictobject_factory.h>
#include <opendaq/authentication_config.h>
#include <opendaq/authentication_config_ptr.h>
#include <opendaq/authentication_method_factory.h>
#include <opendaq/credential_field_factory.h>

// Authentication config shared by every SFTP file transfer backend

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @brief Id of the SFTP private key authentication method. Fields: `UserName`, `PrivateKeyFilePath` (OpenSSH or
 * PEM private key file), optional `Passphrase` for an encrypted key, and optional `HostKeyFingerprint`.
 */
inline constexpr const char* SftpPrivateKeyFileId = "SftpPrivateKeyFile";

/*!
 * @brief Id of the SFTP user name and password authentication method. Fields: `UserName`, `Password` and
 * optional `HostKeyFingerprint`.
 */
inline constexpr const char* SftpUserNamePasswordId = "SftpUserNamePassword";

/*!
 * @brief Name of the optional credential field, present in every SFTP method, holding the SHA256 fingerprint of
 * one of the server's host keys, `SHA256:<base64>`: the value the `ssh` client shows on the first connection,
 * `ssh-keygen -lF <host>` prints for a `known_hosts` entry, or `ssh-keygen -lf /etc/ssh/ssh_host_ed25519_key.pub`
 * prints on the server.
 *
 * When set, only a server presenting a key with exactly that fingerprint (compared as text) is accepted; a
 * malformed value simply never matches. When left empty, the host key is NOT verified: the connection is made to
 * whatever server answers, and a warning is logged.
 */
inline constexpr const char* SftpHostKeyFingerprintField = "HostKeyFingerprint";

/*!
 * @brief Creates the authentication config accepted by every SFTP file transfer backend factory.
 *
 * Offers two methods: `SftpPrivateKeyFileId` (selected by default) and `SftpUserNamePasswordId`. Select one with
 * `setAuthenticationMethod` and supply its fields with `setSuppliedCredential`.
 */
inline AuthenticationConfigPtr SftpAuthenticationConfig()
{
    const auto hostKeyFingerprintField = []
    {
        return CredentialField(SftpHostKeyFingerprintField,
                               CredentialFieldKind::Text,
                               "Server host key fingerprint (SHA256:...); leave empty to skip host key verification",
                               False);
    };

    const auto privateKeyFile = AuthenticationMethod(
        SftpPrivateKeyFileId,
        Dict<IString, ICredentialField>(
            {{"UserName", CredentialField("UserName", CredentialFieldKind::Text, "User name")},
             {"PrivateKeyFilePath", CredentialField("PrivateKeyFilePath", CredentialFieldKind::FilePath, "Private key file")},
             {"Passphrase", CredentialField("Passphrase", CredentialFieldKind::Secret, "Private key passphrase", False)},
             {SftpHostKeyFingerprintField, hostKeyFingerprintField()}}),
        "User name and an OpenSSH or PEM private key file, optionally protected by a passphrase");

    const auto userNamePassword = AuthenticationMethod(
        SftpUserNamePasswordId,
        Dict<IString, ICredentialField>(
            {{"UserName", CredentialField("UserName", CredentialFieldKind::Text, "User name")},
             {"Password", CredentialField("Password", CredentialFieldKind::Secret, "Password")},
             {SftpHostKeyFingerprintField, hostKeyFingerprintField()}}),
        "User name and password");

    const auto methods = Dict<IString, IAuthenticationMethod>(
        {{SftpPrivateKeyFileId, privateKeyFile}, {SftpUserNamePasswordId, userNamePassword}});

    AuthenticationConfigPtr config(AuthenticationConfig_Create(methods));
    return config;
}

END_NAMESPACE_OPENDAQ
