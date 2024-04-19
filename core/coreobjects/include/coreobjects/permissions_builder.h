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
     * @brief Configure component to inherit or ignore permissions from parent object.
     * @param inherit Flag if components should inherit permissions from it's parent object.
     */
    virtual ErrCode INTERFACE_FUNC inherit(Bool inherit) = 0;

    // [returnSelf]
    /*!
     * @brief Allow a set of permissions for a given group and deny all others.
     * @param groupId The id of a group to set permissions for.
     * @param permissions A set of permissions to allow for given group. Permissions not present in the set are denied.
     */
    virtual ErrCode INTERFACE_FUNC set(IString* groupId, IPermissionMaskBuilder* permissions) = 0;

    // [returnSelf]
    /*!
     * @brief Allow a set of permissions for a given group and inherit all others.
     * @param groupId The id of a group to allow permissions for.
     * @param permissions A set of permissions to allow for given group. Permissions not persent in the set are inherited.
     */
    virtual ErrCode INTERFACE_FUNC allow(IString * groupId, IPermissionMaskBuilder* permissions) = 0;

    // [returnSelf]
    /*!
     * @brief Deny a set of permissions for a given group and inherit all others.
     * @param groupId The id of a group to deny permissions for.
     * @param permissions A set of permissions to deny for given group. Permissions not persent in the set are inherited.
     */
    virtual ErrCode INTERFACE_FUNC deny(IString * groupId, IPermissionMaskBuilder* permissions) = 0;

    // [returnSelf]
    /*!
     * @brief Add permisisons of another permission config object, overwrite existing ones.
     * @param config Permisison config object.
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
