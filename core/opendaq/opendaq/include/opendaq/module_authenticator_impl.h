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

BEGIN_NAMESPACE_OPENDAQ

class ModuleAuthenticator : public ImplementationOf<IModuleAuthenticator>
{
public:
    ErrCode INTERFACE_FUNC authenticateModuleBinary(Bool* binaryValid, IString* binaryPath) override;
    virtual Bool onAuthenticateModuleBinary(IString* binaryPath);

    ErrCode INTERFACE_FUNC getAuthenticatedModules(IDict** certModuleDict) override;
    virtual DictPtr<IString, IString> onGetAuthenticatedModules();
};

inline ErrCode INTERFACE_FUNC ModuleAuthenticator::authenticateModuleBinary(Bool* binaryValid, IString* binaryPath)
{
    OPENDAQ_PARAM_NOT_NULL(binaryValid);
    OPENDAQ_PARAM_NOT_NULL(binaryPath);

    Bool valid;
    const ErrCode errCode = wrapHandlerReturn(this, &ModuleAuthenticator::onAuthenticateModuleBinary, valid, binaryPath);
    OPENDAQ_RETURN_IF_FAILED(errCode);

    *binaryValid = valid;
    return errCode;
}

inline Bool ModuleAuthenticator::onAuthenticateModuleBinary(IString* binaryPath)
{
    SizeT len = 0;
    binaryPath->getLength(&len);

    return len > 0;
}

inline ErrCode INTERFACE_FUNC ModuleAuthenticator::getAuthenticatedModules(IDict** certModuleDict)
{
    OPENDAQ_PARAM_NOT_NULL(certModuleDict);

    DictPtr<IString, IString> dict;
    const ErrCode errCode = wrapHandlerReturn(this, &ModuleAuthenticator::onGetAuthenticatedModules, dict);

    OPENDAQ_RETURN_IF_FAILED(errCode);

    *certModuleDict = dict.detach();
    return errCode;
}

inline DictPtr<IString, IString> ModuleAuthenticator::onGetAuthenticatedModules()
{
    return DictPtr<IString, IString>();
}

END_NAMESPACE_OPENDAQ
