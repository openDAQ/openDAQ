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
#include <coretypes/dictobject.h>
#include <opendaq/credential_request.h>
#include <opendaq/authentication_method.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @brief Supplies the credentials requested via a `ICredentialRequest` - e.g. by prompting the user, reading
 * from a file, or fetching from a credential store.
 *
 * Declares no supported field kinds up front - if a request's authentication method needs a field kind this
 * implementation can't handle, `requestCredentials` is expected to fail on its own (an ordinary failed
 * `ErrCode`, propagated the same way any other failure from it would be), rather than being pre-checked by
 * the caller.
 */
DECLARE_OPENDAQ_INTERFACE(ICredentialProvider, IBaseObject)
{
    /*!
     * @brief Gets a human-readable description of the credential provider.
     * @param[out] description The provider's description.
     */
    virtual ErrCode INTERFACE_FUNC getDescription(IString** description) = 0;

    // [templateType(credentials, IString, IString)]
    /*!
     * @brief Requests credentials for the given request, in the shape described by its authentication method's
     * nested credential fields.
     * @param request The credential request to obtain credentials for.
     * @param[out] credentials The obtained credential - a dictionary of field value(s), keyed by their own
     * field id (see `ICredentialField::getId`).
     */
    virtual ErrCode INTERFACE_FUNC requestCredentials(ICredentialRequest* request, IDict** credentials) = 0;
};

/*!
 * @brief Creates a `ICredentialProvider` that prompts the user for credentials via the command line -
 * supporting every `CredentialFieldKind`.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, CmdLineCredentialProvider, ICredentialProvider)

END_NAMESPACE_OPENDAQ
