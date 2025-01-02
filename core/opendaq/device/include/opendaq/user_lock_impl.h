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
#include <opendaq/user_lock.h>
#include <coretypes/serializable.h>
#include <coretypes/intfs.h>
#include <coretypes/serialized_list.h>
#include <coreobjects/user_ptr.h>
#include <optional>

BEGIN_NAMESPACE_OPENDAQ

class UserLockImpl final : public ImplementationOf<IUserLock, ISerializable>
{
public:
    explicit UserLockImpl();

    ErrCode INTERFACE_FUNC lock(IUser* user = nullptr) override;
    ErrCode INTERFACE_FUNC unlock(IUser* user = nullptr) override;
    ErrCode INTERFACE_FUNC forceUnlock() override;
    ErrCode INTERFACE_FUNC isLocked(Bool* isLockedOut) override;

    // ISerializable
    ErrCode INTERFACE_FUNC serialize(ISerializer* serializer) override;
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;
    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* /*factoryCallback*/, IBaseObject** obj);

private:
    std::optional<UserPtr> userLock;
};

END_NAMESPACE_OPENDAQ
