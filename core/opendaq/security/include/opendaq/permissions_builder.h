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
#include <opendaq/permissions.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_security
 * @addtogroup opendaq_security_permission_config_builder PermissionConfigBuilder
 * @{
 */

/*!
 * @brief A class which is responsible for assigning permissions to a property object.
 * Permisison builder can specified allowed permissions for each group. It can also inherit
 * or overwrite premissions from parent objects.
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
     * @brief Set permissions for given group.
     * @param groupId The id of a group to set permissions for.
     * @param permissionFlags Bit mask of Permission enum flags to allow or deny.
     */
    virtual ErrCode INTERFACE_FUNC set(IString* groupId, Int permissionFlags) = 0;

    // [returnSelf]
    /*!
     * @brief Allow permissions for given group.
     * @param groupId The id of a group to allow permissions for.
     * @param permissionFlags Bit mask of Permission enum flags to allow. Allowed flags should be set to 1 and denied to 0.
     */
    virtual ErrCode INTERFACE_FUNC allow(IString * groupId, Int permissionFlags) = 0;

    // [returnSelf]
    /*!
     * @brief Deny permissions for given group.
     * @param groupId The id of a group to deny permissions for.
     * @param permissionFlags Bit mask of Permission enum flags to deny. Denied flags should be set to 1 and allowed to 0.
     */
    virtual ErrCode INTERFACE_FUNC deny(IString * groupId, Int permissionFlags) = 0;

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

END_NAMESPACE_OPENDAQ
