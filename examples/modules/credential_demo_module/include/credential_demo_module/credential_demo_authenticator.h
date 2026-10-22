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
#include <credential_demo_module/common.h>
#include <opendaq/context_ptr.h>
#include <coretypes/dict_ptr.h>

BEGIN_NAMESPACE_CREDENTIAL_DEMO_MODULE

namespace authentication
{
    /*
     * Verifies credentials for one of the four showcased auth methods (UserName/Password, PIN,
     * PrivateKeyFile, Anonymous). Shared by both the device (authenticating a connection to it) and the
     * streaming implementation (authenticating a streaming connection).
     */
    void Authenticate(const ContextPtr& ctx, const DictPtr<IString, IString>& credentials, const StringPtr& authenticationMethodId);
}

END_NAMESPACE_CREDENTIAL_DEMO_MODULE
