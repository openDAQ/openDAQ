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
#include <coreobjects/permissions_builder.h>
#include <coreobjects/permissions_ptr.h>
#include <coreobjects/permission_manager.h>
#include <coreobjects/object_keys.h>

BEGIN_NAMESPACE_OPENDAQ
class PermissionsBuilderImpl : public ImplementationOf<IPermissionsBuilder>
{
public:
    using PermissionTable = std::unordered_map<StringPtr, Int, StringHash, StringEqualTo>;
    explicit PermissionsBuilderImpl();

    ErrCode INTERFACE_FUNC inherit(Bool inherit) override;
    ErrCode INTERFACE_FUNC assign(IString* groupId, IPermissionMaskBuilder* permissions) override;
    ErrCode INTERFACE_FUNC allow(IString* groupId, IPermissionMaskBuilder* permissions) override;
    ErrCode INTERFACE_FUNC deny(IString* groupId, IPermissionMaskBuilder* permissions) override;
    ErrCode INTERFACE_FUNC extend(IPermissions* config) override;
    ErrCode INTERFACE_FUNC build(IPermissions** configOut) override;

private:
    void assign(IString* groupId, Int permissionFlags);
    void allow(IString* groupId, Int permissionFlags);
    void deny(IString* groupId, Int permissionFlags);

    Bool inherited;
    
    PermissionTable allowed;
    PermissionTable denied;
    PermissionTable assigned;
};

END_NAMESPACE_OPENDAQ
