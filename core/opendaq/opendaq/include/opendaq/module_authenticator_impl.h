/*
 * Copyright 2022-2025 openDAQ d.o.o.
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
#include <opendaq/module_authenticator.h>
#include <opendaq/module_authenticator_ptr.h>
#include <opendaq/module_authenticator_factory.h>

BEGIN_NAMESPACE_OPENDAQ

class ModuleAuthenticatorImpl : public ImplementationOf<IModuleAuthenticator>
{
public:
    explicit ModuleAuthenticatorImpl(IString* certPath);

    ErrCode INTERFACE_FUNC authenticateModuleBinary(Bool* binaryValid, IString* binaryPath) override;
    Bool onAuthenticateModuleBinary(IString* binaryPath);

private:
    StringPtr certificatePath;
};

END_NAMESPACE_OPENDAQ
