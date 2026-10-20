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
#include <opendaq/authentication_method_ptr.h>
#include <opendaq/credential_field_factory.h>
#include <coretypes/dictobject_factory.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @brief Creates a `AuthenticationMethod` - `fields` names every credential field it expects (empty for a
 * method that needs no credentials, e.g. anonymous access).
 * @param id The id that uniquely identifies this authentication method at least within the module that offers it.
 * @param fields The credential fields this method expects, keyed by their own name, in declaration order.
 * @param description A human-readable description of the authentication method, for the user.
 * @throws InvalidParameterException if `id`, `fields`, or `description` is unassigned.
 */
inline AuthenticationMethodPtr AuthenticationMethod(const StringPtr& id, const DictPtr<IString, ICredentialField>& fields, const StringPtr& description)
{
    AuthenticationMethodPtr obj(AuthenticationMethod_Create(id, fields, description));
    return obj;
}

/*!
 * @brief Ids of the four standard authentication methods below - shared, well-known ids every
 * module can build the exact same authentication method for, instead of each one inventing its own shape/id for the
 * same method.
 */
inline constexpr const char* StandardUserNamePasswordId = "UserNamePassword";
inline constexpr const char* StandardPinId = "Pin";
inline constexpr const char* StandardPrivateKeyFileId = "PrivateKeyFile";
inline constexpr const char* StandardAnonymousId = "Anonymous";

/*!
 * @brief The authentication method for the standard `UserName`/`Password` authentication method.
 */
inline AuthenticationMethodPtr StandardUserNamePasswordAuthenticationMethod()
{
    return AuthenticationMethod(StandardUserNamePasswordId,
                                 Dict<IString, ICredentialField>({{"UserName", CredentialField("UserName", CredentialFieldKind::Text, "User name")},
                                                                   {"Password", CredentialField("Password", CredentialFieldKind::Secret, "Password")}}),
                                 "Username and password");
}

/*!
 * @brief The authentication method for the standard PIN authentication method.
 */
inline AuthenticationMethodPtr StandardPinAuthenticationMethod()
{
    return AuthenticationMethod(StandardPinId,
                                 Dict<IString, ICredentialField>({{"Pin", CredentialField("Pin", CredentialFieldKind::Secret, "PIN code")}}),
                                 "PIN code");
}

/*!
 * @brief The authentication method for the standard private-key-file authentication method.
 */
inline AuthenticationMethodPtr StandardPrivateKeyFileAuthenticationMethod()
{
    return AuthenticationMethod(
        StandardPrivateKeyFileId,
        Dict<IString, ICredentialField>(
            {{"PrivateKeyFilePath",
              CredentialField("PrivateKeyFilePath", CredentialFieldKind::FilePath, "Private key file", Dict<IString, IString>({{"Extensions", "pem,key"}}))}}),
        "Path to the PEM-encoded private key file");
}

/*!
 * @brief The authentication method for the standard anonymous authentication method - requires no credentials
 * at all.
 */
inline AuthenticationMethodPtr StandardAnonymousAuthenticationMethod()
{
    return AuthenticationMethod(StandardAnonymousId, Dict<IString, ICredentialField>(), "No credentials required");
}

END_NAMESPACE_OPENDAQ
