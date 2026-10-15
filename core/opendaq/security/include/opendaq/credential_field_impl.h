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
#include <opendaq/credential_field.h>
#include <coretypes/impl.h>
#include <coretypes/serializable.h>
#include <coretypes/serialized_object.h>
#include <coretypes/string_ptr.h>
#include <coretypes/dictobject_factory.h>

BEGIN_NAMESPACE_OPENDAQ

class CredentialFieldImpl final : public ImplementationOf<ICredentialField, ISerializable>
{
public:
    CredentialFieldImpl(IString* id, CredentialFieldKind kind, IString* name, IDict* metadata, Bool required);

    ErrCode INTERFACE_FUNC getId(IString** id) override;
    ErrCode INTERFACE_FUNC getKind(CredentialFieldKind* kind) override;
    ErrCode INTERFACE_FUNC getName(IString** name) override;
    ErrCode INTERFACE_FUNC getMetadata(IDict** metadata) override;
    ErrCode INTERFACE_FUNC isRequired(Bool* required) override;

    ErrCode INTERFACE_FUNC serialize(ISerializer* serializer) override;
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;
    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

private:
    StringPtr id;
    CredentialFieldKind kind;
    StringPtr name;
    DictPtr<IString, IString> metadata;
    Bool required;
};

END_NAMESPACE_OPENDAQ
