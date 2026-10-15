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
#include <coretypes/baseobject.h>
#include <coretypes/string_ptr.h>
#include <coretypes/dictobject.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @brief How a single credential field's value should be collected/presented.
 */
enum class CredentialFieldKind : EnumType
{
    Text = 0,   ///< Shown as typed - e.g. a username.
    Secret,     ///< Masked as typed - e.g. a password or PIN.
    FilePath    ///< A path to a file - e.g. a private key file.
};

/*!
 * @brief Describes a single named credential field of an `IAuthenticationMethod`.
 */
DECLARE_OPENDAQ_INTERFACE(ICredentialField, IBaseObject)
{
    /*!
     * @brief Gets the key this field is stored under in the credential - e.g. "Username", "Password",
     * "PrivateKeyFilePath".
     * @param[out] id The field id.
     */
    virtual ErrCode INTERFACE_FUNC getId(IString** id) = 0;

    /*!
     * @brief Gets how this field's value should be collected/presented.
     * @param[out] kind The field kind.
     */
    virtual ErrCode INTERFACE_FUNC getKind(CredentialFieldKind* kind) = 0;

    /*!
     * @brief Gets a human-readable name for this field, for the user - e.g. "User name", "Private key file".
     * @param[out] name The field name.
     */
    virtual ErrCode INTERFACE_FUNC getName(IString** name) = 0;

    // [elementType(metadata, IString, IString)]
    /*!
     * @brief Gets hints for whoever collects this field's value. Well-known key `"Extensions"` for `FilePath`:
     * comma-separated, without the dot (e.g. `"pem,key"`); absent or empty means any file.
     * @param[out] metadata The field's metadata.
     */
    virtual ErrCode INTERFACE_FUNC getMetadata(IDict** metadata) = 0;

    /*!
     * @brief Gets whether this field cannot be left unset or empty - e.g. a password might be optional but not
     * the username within the same authentication method.
     * @param[out] required `True` if the field is required.
     */
    virtual ErrCode INTERFACE_FUNC isRequired(Bool* required) = 0;
};

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, CredentialField, ICredentialField,
                                              IString*, id, CredentialFieldKind, kind, IString*, name, IDict*, metadata, Bool, required)

END_NAMESPACE_OPENDAQ
