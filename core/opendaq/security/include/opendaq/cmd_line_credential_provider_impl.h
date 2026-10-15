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

#include <coretypes/impl.h>
#include <opendaq/credential_provider.h>
#include <opendaq/credential_request_ptr.h>
#include <opendaq/authentication_method_ptr.h>
#include <coretypes/dictobject_factory.h>
#include <map>
#include <tuple>

BEGIN_NAMESPACE_OPENDAQ

class CmdLineCredentialProviderImpl : public ImplementationOf<ICredentialProvider>
{
public:
    explicit CmdLineCredentialProviderImpl();

    ErrCode INTERFACE_FUNC getDescription(IString** description) override;
    ErrCode INTERFACE_FUNC requestCredentials(ICredentialRequest* request, IDict** credentials) override;
    ErrCode INTERFACE_FUNC cacheCredentials(ICredentialRequest* request, IDict* credentials) override;

private:
    // manufacturer, serialNumber, model, authentication method id
    using CacheKey = std::tuple<std::string, std::string, std::string, std::string>;

    static void printRequestDetails(const CredentialRequestPtr& request);
    static std::string readLine(const std::string& prompt, bool hide);

    static CacheKey MakeCacheKey(const CredentialRequestPtr& request);
    static bool isFileAccessible(const std::string& path);

    // Builds the whole credential in one pass, one dictionary entry per field, prompting according to each
    // field's own kind (masked for `Secret`, plain otherwise). `FilePath` is still validated locally right after
    // being freshly read (not on a cache hit): the entered path must exist and be readable. A method's fields -
    // every kind - are cached together, in-memory for the lifetime of this provider (i.e. for the active session),
    // as one entry keyed by `(manufacturer, serialNumber, model, authentication method id)` - so re-authenticating
    // a second connection to the same device via the same method reuses every value already entered instead of
    // prompting again.
    DictPtr<IString, IString> readCredential(const CredentialRequestPtr& request, const DictPtr<IString, ICredentialField>& fields);

    std::map<CacheKey, DictPtr<IString, IString>> credentialCache;
};

END_NAMESPACE_OPENDAQ
