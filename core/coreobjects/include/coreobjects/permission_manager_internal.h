/*
 * Copyright 2022-2024 Blueberry d.o.o.
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
#include <coreobjects/permission_manager.h>
#include <coretypes/dictobject.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @brief Internal PermissionManager interface. It should be used only in openDAQ core implementation files.
 */
DECLARE_OPENDAQ_INTERFACE(IPermissionManagerInternal, IBaseObject)
{
    /*!
     * @brief Change the parant of a permission manager.
     * @param parentManager A reference to the permission manager of a parent object.
     */
    virtual ErrCode INTERFACE_FUNC setParent(IPermissionManager * parentManager) = 0;

    /*!
     * @brief Adds a reference to the permission manager of a child object.
     * @param childManager A reference to the permission manager of a child object.
     */
    virtual ErrCode INTERFACE_FUNC addChildManager(IPermissionManager* childManager) = 0;

    /*!
     * @brief Removes a reference to the permission manager of a child object.
     * @param childManager A reference to the permission manager of a child object.
     */
    virtual ErrCode INTERFACE_FUNC removeChildManager(IPermissionManager* childManager) = 0;

    /*!
     * @brief Returns permisisons configuration object.
     * @param permissionsOut[out] A Permissions configuration object.
     */
    virtual ErrCode INTERFACE_FUNC getPermissions(IPermissions** permissionsOut) = 0;

    /*!
     * @brief Recursively update permissions of objects with new permissions from their parents.
     */
    virtual ErrCode INTERFACE_FUNC updateInheritedPermissions() = 0;
};

END_NAMESPACE_OPENDAQ
