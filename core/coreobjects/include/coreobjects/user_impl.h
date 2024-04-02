/*
 * Copyright 2022-2023 Blueberry d.o.o.
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
#include <coreobjects/user.h>
#include <coreobjects/user_internal.h>
#include <coretypes/intfs.h>
#include <coretypes/string_ptr.h>
#include <coretypes/listobject_factory.h>
#include <coretypes/deserializer.h>

BEGIN_NAMESPACE_OPENDAQ

class UserImpl final : public ImplementationOf<IUser, IUserInternal, ISerializable>
{
public:
    explicit UserImpl(const StringPtr& username, const StringPtr& passwordHash, const ListPtr<IString> groups);

    ErrCode INTERFACE_FUNC getUsername(IString** username) override;
    ErrCode INTERFACE_FUNC getPasswordHash(IString** passwordHash) override;
    ErrCode INTERFACE_FUNC getGroups(IList** groups) override;

    // IBaseObject
    ErrCode INTERFACE_FUNC equals(IBaseObject* other, Bool* equal) const override;

    // ISerializable
    ErrCode INTERFACE_FUNC serialize(ISerializer* serializer) override;
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;
    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* /*context*/, IFunction* /*factoryCallback*/, IBaseObject** obj);

private:
    ListPtr<IString> sanitizeGroupList(const ListPtr<IString> groups);

    StringPtr username;
    StringPtr passwordHash;
    ListPtr<IString> groups;
};

END_NAMESPACE_OPENDAQ
