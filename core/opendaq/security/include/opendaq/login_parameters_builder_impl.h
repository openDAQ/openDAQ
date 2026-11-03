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
#include <opendaq/login_parameters_builder.h>
#include <opendaq/authentication_method_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

class LoginParametersBuilderImpl : public ImplementationOf<ILoginParametersBuilder>
{
public:
    explicit LoginParametersBuilderImpl();

    ErrCode INTERFACE_FUNC build(ILoginParameters** parameters) override;

    ErrCode INTERFACE_FUNC setCredentials(IDict* credentials) override;
    ErrCode INTERFACE_FUNC getCredentials(IDict** credentials) override;

    ErrCode INTERFACE_FUNC setAuthenticationMethod(IAuthenticationMethod* method) override;
    ErrCode INTERFACE_FUNC getAuthenticationMethod(IAuthenticationMethod** method) override;

private:
    DictPtr<IString, IString> credentials;
    AuthenticationMethodPtr authenticationMethod;
};

END_NAMESPACE_OPENDAQ
