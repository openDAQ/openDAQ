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
#include <coretypes/intfs.h>
#include <coreobjects/permissions.h>
#include <coretypes/dictobject_factory.h>
#include <coreobjects/permissions_ptr.h>
#include <coreobjects/permissions_internal.h>
#include <coreobjects/object_keys.h>

BEGIN_NAMESPACE_OPENDAQ
class PermissionsImpl : public ImplementationOf<IPermissions, IPermissionsInternal>
{
public:
    explicit PermissionsImpl();
    explicit PermissionsImpl(Bool inherited,
                             const std::unordered_map<StringPtr, Int, StringHash, StringEqualTo>& allowed,
                             const std::unordered_map<StringPtr, Int, StringHash, StringEqualTo>& denied,
                             const std::unordered_map<StringPtr, Int, StringHash, StringEqualTo>& assigned);

    ErrCode INTERFACE_FUNC getInherited(Bool* inherited) override;
    ErrCode INTERFACE_FUNC getAllowed(IDict** permissions) override;
    ErrCode INTERFACE_FUNC getDenied(IDict** permissions) override;

    // IPermissionsInternal
    ErrCode INTERFACE_FUNC getAssigned(IDict** permissions) override;

private:
    static void cloneDict(const std::unordered_map<StringPtr, Int, StringHash, StringEqualTo>& dict, DictPtr<IString, Int>& target);

    Bool inherited;
    DictPtr<IString, Int> allowed;
    DictPtr<IString, Int> denied;
    DictPtr<IString, Int> assigned;
};

END_NAMESPACE_OPENDAQ
