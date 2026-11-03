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

#include <opendaq/credential_request_builder_ptr.h>
#include <opendaq/credential_request_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @brief Creates a `CredentialRequestBuilder` with no values set.
 */
inline CredentialRequestBuilderPtr CredentialRequestBuilder()
{
    CredentialRequestBuilderPtr obj(CredentialRequestBuilder_Create());
    return obj;
}

/*!
 * @brief Creates a `CredentialRequest` from a `ICredentialRequestBuilder`'s currently configured values.
 */
inline CredentialRequestPtr CredentialRequestFromBuilder(const CredentialRequestBuilderPtr& builder)
{
    CredentialRequestPtr obj(CredentialRequestFromBuilder_Create(builder));
    return obj;
}

END_NAMESPACE_OPENDAQ
