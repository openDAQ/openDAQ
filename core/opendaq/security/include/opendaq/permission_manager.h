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
#include <opendaq/permission_config.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_security
 * @addtogroup opendaq_security_permission_manager PermissionManager
 * @{
 */


/*!
 * @brief Enumeration of available access permissions
 */
enum Permission
{
    Read =    0x1, // The user can see and read an object.
    Write =   0x2, // The user can change or write to the object.
    Execute = 0x4  // The user can execute an action attached to the object.
};


/*!
 * @brief A class which is responsible for managing permissions on a object level. Given a user's group,
 * it is possible to restrict or allow read, write and execute permissions for each object. It is also
 * possible to specify if permissions are inherited from parent object
 */
DECLARE_OPENDAQ_INTERFACE(IPermissionManager, IBaseObject)
{
    /*!
     * @brief Set object permisison configuration.
     * @param permissionConfig Permission configuration object.
     */
    virtual ErrCode INTERFACE_FUNC setPermissionConfig(IPermissionConfig* permissionConfig) = 0;

    /*!
     * @brief Check if user has a given permission on an object of the permission manager.
     * @param user A reference to the user.
     * @param permission A permisson to test.
     * @param authorizedOut[out] Returns true if user is authorized and false otherwise.
     */
    virtual ErrCode INTERFACE_FUNC isAuthorized(IUser * user, Permission permission, Bool * authorizedOut) = 0;
};

/*!@}*/

END_NAMESPACE_OPENDAQ
