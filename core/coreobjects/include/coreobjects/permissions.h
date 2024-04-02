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
#include <coretypes/dictobject.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_security
 * @addtogroup opendaq_security_permission_config PermissionConfig
 * @{
 */

/*!
 * @brief A class which describes a permission configuration for openDAQ object.A configuration object
 * can be constructed using the permission builder class.
 */
DECLARE_OPENDAQ_INTERFACE(IPermissions, IBaseObject)
{
    /*!
     * @brief Returns true if an object should inherit permissions from it's parent object.
     * @param inherited[out] True if permissions should be inherited from parent object.
     */
    virtual ErrCode INTERFACE_FUNC getInherited(Bool* inherited) = 0;

    // [templateType(permissions, IString, Int)]
    /*!
     * @brief Returns a dictionary of allowed permissions for each group.
     * @param permissions[out] A dictionary of allowed permissions for each group.
     */
    virtual ErrCode INTERFACE_FUNC getAllowed(IDict** permissions) = 0;

    // [templateType(permissions, IString, Int)]
    /*!
     * @brief Returns a dictionary of denied permissions for each group.
     * @param permissions[out] A dictionary of denied permissions for each group.
     */
    virtual ErrCode INTERFACE_FUNC getDenied(IDict** permissions) = 0;
};

/*!@}*/

END_NAMESPACE_OPENDAQ
