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

#include <opendaq/credential_provider_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @brief Creates a CredentialProvider that prompts the user for credentials via the command line - supporting every `CredentialFieldKind`.
 */
inline CredentialProviderPtr CmdLineCredentialProvider()
{
    CredentialProviderPtr obj(CmdLineCredentialProvider_Create());
    return obj;
}

END_NAMESPACE_OPENDAQ
