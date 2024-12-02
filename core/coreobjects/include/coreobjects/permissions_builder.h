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
#include <coretypes/baseobject.h>
#include <coretypes/common.h>
#include <coreobjects/permissions.h>
#include <coreobjects/permission_mask_builder.h>
#include <coretypes/listobject.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup objects_security
 * @addtogroup objects_security_permissions_builder PermissionsBuilder
 * @{
 */

/*!
 * @brief A class which is responsible for assigning permissions to a property object.
 * Permisison builder can specify allowed and denied permissions for each group. It can
 * also inherit or overwrite premissions from parent objects.
 */
DECLARE_OPENDAQ_INTERFACE(IPermissionsBuilder, IBaseObject)
{
    // [returnSelf]
    /*!
     * @brief Configure component to inherit or ignore permissions from the parent object.
     * @param inherit Flag signifying if component should inherit permissions from its parent object.
     */
    virtual ErrCode INTERFACE_FUNC inherit(Bool inherit) = 0;

    // [returnSelf]
    /*!
     * @brief Strictly assign a specified set of permissions for a given group. This method allows only the specified
     * permissions and will not inherit any permissions from the parent object for the group, even if the inherit flag is enabled.
     *
     * @param groupId The id of a group to set permissions for.
     * @param permissions A set of permissions to allow for given group.
     */
    virtual ErrCode INTERFACE_FUNC assign(IString* groupId, IPermissionMaskBuilder* permissions) = 0;

    // [returnSelf]
    /*!
     * @brief Allow a specified set of permissions for a given group. If the inherit flag is enabled, this method will
     * allow both the specified permissions and any permissions already allowed for the group on the parent component.
     * Denied permissions will always overrule allowed permissions.
     * 
     * @param groupId The id of a group to allow permissions for.
     * @param permissions A set of permissions to allow for given group.
     */
    virtual ErrCode INTERFACE_FUNC allow(IString* groupId, IPermissionMaskBuilder* permissions) = 0;

    // [returnSelf]
    /*!
     * @brief Deny a specified set of permissions for a given group. If the inherit flag is enabled, this method will
     * deny both the specified permissions and any permissions already denied for the group on the parent component.
     * Denied permissions will always overrule allowed permissions.
     *
     * @param groupId The id of a group to deny permissions for.
     * @param permissions A set of permissions to deny for given group.
     */
    virtual ErrCode INTERFACE_FUNC deny(IString* groupId, IPermissionMaskBuilder* permissions) = 0;

    // [returnSelf]
    /*!
     * @brief Add permissions of another permission config object and overwrite existing ones. Inherit flag will not be overwritten.
     * @param config Permission config object.
     */
    virtual ErrCode INTERFACE_FUNC extend(IPermissions* config) = 0;

    /*!
     * @brief Builds the permission config object.
     * @param configOut[out] Permission config object.
     */
    virtual ErrCode INTERFACE_FUNC build(IPermissions** configOut) = 0;
};

/*!@}*/

/*!
 * @brief Creates a Permissions builder object.
 */
OPENDAQ_DECLARE_CLASS_FACTORY(LIBRARY_FACTORY, PermissionsBuilder)

END_NAMESPACE_OPENDAQ
