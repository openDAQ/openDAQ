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
#include <coretypes/baseobject.h>
#include <coretypes/common.h>
#include <coreobjects/user.h>
#include <coreobjects/permissions.h>
#include <coreobjects/permission_mask_builder.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup objects_security
 * @addtogroup objects_security_permission_manager PermissionManager
 * @{
 */

/*!
 * @brief A class which is responsible for managing permissions on an object level. Given a user's group,
 * it is possible to restrict or allow read, write and execute permissions for each object. It is also
 * possible to specify if permissions are inherited from parent object
 */
DECLARE_OPENDAQ_INTERFACE(IPermissionManager, IBaseObject)
{
    /*!
     * @brief Set object permission configuration.
     * @param permissions Permissions configuration object.
     */
    virtual ErrCode INTERFACE_FUNC setPermissions(IPermissions* permissions) = 0;

    /*!
     * @brief Check if user has a given permission on an object of the permission manager.
     * @param user A reference to the user.
     * @param permission A permission to test.
     * @param[out] authorizedOut Returns true if user is authorized and false otherwise.
     */
    virtual ErrCode INTERFACE_FUNC isAuthorized(IUser * user, Permission permission, Bool* authorizedOut) = 0;
};

/*!@}*/

/*!
 * @brief Creates an permission manager with a given parent.
 * @param parent Permission manager of a parent object. It can be null for a root object.
 */
OPENDAQ_DECLARE_CLASS_FACTORY(LIBRARY_FACTORY, PermissionManager, IPermissionManager*, parent)

/*!
 * @brief Creates a permission manager which never restricts any access to any object.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(LIBRARY_FACTORY,
                                                            DisabledPermissionManager,
                                                            IPermissionManager,
                                                            createDisabledPermissionManager)

END_NAMESPACE_OPENDAQ
