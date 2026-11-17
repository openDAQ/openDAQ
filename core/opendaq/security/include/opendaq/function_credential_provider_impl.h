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
#include <coretypes/function_ptr.h>
#include <coretypes/dictobject_factory.h>
#include <opendaq/credential_provider.h>

BEGIN_NAMESPACE_OPENDAQ

class FunctionCredentialProviderImpl : public ImplementationOf<ICredentialProvider>
{
public:
    explicit FunctionCredentialProviderImpl(const FunctionPtr& f);

    ErrCode INTERFACE_FUNC getDescription(IString** description) override;
    ErrCode INTERFACE_FUNC requestCredentials(ICredentialRequest* request, IDict** credentials) override;

private:
    BaseObjectPtr callFunction(const char* methodName, const DictPtr<IString, IBaseObject>& params, const char* resultKey);

    FunctionPtr f;
};

END_NAMESPACE_OPENDAQ
