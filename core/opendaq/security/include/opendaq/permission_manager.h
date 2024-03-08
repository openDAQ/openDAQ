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
#include <coretypes/baseobject.h>
#include <coretypes/common.h>
#include <opendaq/user.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_security
 * @addtogroup opendaq_security_permission_manager PermissionManager
 * @{
 */


/*!
 * @brief Enumeration of available access permissions
 */
enum AccessPermission
{
    Read =    0x1, // The user can see and read a component.
    Write =   0x2, // The user can change or write to the component.
    Execute = 0x4  // The user can execute an action attached to the component.
};


/*!
 * @brief A class which is responsible for managing permissions on a component level. Given a user's group,
 * it is possible to restrict or allow read, write and execute permissions for each component. It is also
 * possible to specify if permissions are inherited from parent component
 */
DECLARE_OPENDAQ_INTERFACE(IPermissionManager, IBaseObject)
{
    /*!
     * @brief Configure component to inherit or ignore permissions from parent component.
     * @param inherit Flag if components should inherit permissions from it's parent.
     * @param managerOut[out] A reference to permission manager.
     */
    virtual ErrCode INTERFACE_FUNC inherit(Bool* inherit, IPermissionManager** managerOut) = 0;

    /*!
     * @brief Set prmissions for given group.
     * @param groupId The id of a group to set permissions for.
     * @param permissions Bit mask of AccessPermission permissions.
     * @param managerOut[out] A reference to permission manager.
     */
    virtual ErrCode INTERFACE_FUNC setPermissions(IString* groupId, Int permissionFlags, IPermissionManager** managerOut) = 0;

    /*!
     * @brief Check if user has a given permission on component of the permission manager.
     * @param user A reference to the user.
     * @param permission A permisson to test.
     * @param authorizedOut[out] Returns true if user is authorized and false otherwise.
     */
    virtual ErrCode INTERFACE_FUNC isAuthorized(IUser* user, AccessPermission permission, Bool* authorizedOut) = 0;
};

/*!@}*/

OPENDAQ_DECLARE_CLASS_FACTORY(LIBRARY_FACTORY, PermissionManager)

END_NAMESPACE_OPENDAQ
