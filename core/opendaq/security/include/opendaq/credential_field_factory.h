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
#include <opendaq/credential_field_ptr.h>
#include <coretypes/dictobject_factory.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @brief Creates a `ICredentialField`.
 * @param id The key this field is stored under in the credential - e.g. "Username".
 * @param kind How this field's value should be collected/presented.
 * @param name A human-readable name for this field, for the user - e.g. "User name".
 * @param metadata Hints for whoever collects this field's value (see `ICredentialField::getMetadata`). Empty by default.
 * @param required Whether this field cannot be left unset or empty. `True` by default.
 */
inline CredentialFieldPtr CredentialField(const StringPtr& id,
                                           CredentialFieldKind kind,
                                           const StringPtr& name,
                                           const DictPtr<IString, IString>& metadata = Dict<IString, IString>(),
                                           Bool required = True)
{
    CredentialFieldPtr obj(CredentialField_Create(id, kind, name, metadata, required));
    return obj;
}

END_NAMESPACE_OPENDAQ
