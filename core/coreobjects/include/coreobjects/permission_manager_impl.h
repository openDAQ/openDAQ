/*
 * Copyright 2022-2024 openDAQ d.o.o.
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
#include <coreobjects/permission_manager.h>
#include <coreobjects/permission_manager_ptr.h>
#include <coreobjects/permission_manager_internal.h>
#include <coreobjects/permission_manager_internal_ptr.h>
#include <coretypes/weakrefptr.h>
#include <coretypes/cloneable.h>

BEGIN_NAMESPACE_OPENDAQ

class PermissionManagerImpl : public ImplementationOfWeak<IPermissionManager, IPermissionManagerInternal, ICloneable>
{
public:
    explicit PermissionManagerImpl(const PermissionManagerPtr& parent);
    ~PermissionManagerImpl();

    ErrCode INTERFACE_FUNC setPermissions(IPermissions* permissions) override;
    ErrCode INTERFACE_FUNC isAuthorized(IUser* user, Permission permission, Bool* authorizedOut) override;
    ErrCode INTERFACE_FUNC clone(IBaseObject** cloneOut) override;

protected:
    ErrCode INTERFACE_FUNC setParent(IPermissionManager* parentManager) override;
    ErrCode INTERFACE_FUNC addChildManager(IPermissionManager* childManager) override;
    ErrCode INTERFACE_FUNC removeChildManager(IPermissionManager* childManager) override;
    ErrCode INTERFACE_FUNC getPermissions(IPermissions** permisisonConfigOut) override;
    ErrCode INTERFACE_FUNC updateInheritedPermissions() override;

private:
    void updateChildPermissions();
    PermissionManagerInternalPtr getParentManager();

    WeakRefPtr<IPermissionManager> parent;
    std::unordered_set<IPermissionManager*> children;
    PermissionsPtr permissions;
    PermissionsPtr localPermissions;
};

END_NAMESPACE_OPENDAQ
