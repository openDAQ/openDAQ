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
#include <opendaq/credential_field.h>

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [interfaceLibrary(ICredentialField, "opendaq")]
 * [interfaceSmartPtr(ICredentialField, CredentialFieldPtr, "<opendaq/credential_field_ptr.h>")]
 */

/*!
 * @brief Describes one way to authenticate - its id, a human-readable description, and the named credential
 * fields it needs (if any).
 *
 * The id uniquely identifies the authentication method at least within the module that offers it (e.g.
 * `"UserNamePassword"`, `"Pin"`) - the same id `AuthenticationConfig` keys the resulting config's
 * `"AuthenticationMethod"` candidates by. In practice the id is often unique system-wide, deliberately reused
 * across modules: the `Standard*AuthenticationMethod` factories below key off shared, well-known ids, so any
 * two modules using the same standard id produce identically-shaped authentication methods.
 *
 * `getFields()` carries everything needed to collect a credential for this method: one `ICredentialField` per
 * named value expected (e.g. `{"UserName": Text, "Password": Secret}`), in declaration order. Empty for a
 * method that needs no credentials at all - e.g. anonymous access.
 */
DECLARE_OPENDAQ_INTERFACE(IAuthenticationMethod, IBaseObject)
{
    /*!
     * @brief Gets the id that uniquely identifies this authentication method, at least within the module
     * that offers it.
     * @param[out] id The authentication method id.
     */
    virtual ErrCode INTERFACE_FUNC getId(IString** id) = 0;

    // [elementType(fields, IString, ICredentialField)]
    /*!
     * @brief Gets the named credential fields this method expects, in declaration order - empty for a method
     * that needs no credentials at all, e.g. anonymous access.
     * @param[out] fields The credential fields, keyed by their own name.
     */
    virtual ErrCode INTERFACE_FUNC getFields(IDict** fields) = 0;

    /*!
     * @brief Gets the description of the authentication method, for the user. States how the module
     * interpretes it, e.g. "PIN-code", "username and password", "Path to file containing the SSH private key".
     * @param[out] description The method description.
     */
    virtual ErrCode INTERFACE_FUNC getDescription(IString** description) = 0;
};

// [elementType(fields, IString, ICredentialField)]
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, AuthenticationMethod, IAuthenticationMethod,
    IString*, id, IDict*, fields, IString*, description
)

END_NAMESPACE_OPENDAQ
