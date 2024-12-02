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

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup objects_security
 * @addtogroup objects_security_permission_mask_builder PermissionMaskBuilder
 * @{
 */

/*!
 * @brief Enumeration of available access permissions
 */
enum class Permission : EnumType
{
    None = 0x0,    // The user has no permissions on the object.
    Read = 0x1,    // The user can see and read an object.
    Write = 0x2,   // The user can change or write to the object.
    Execute = 0x4  // The user can execute an action attached to the object.
};

/*!
 * @brief A class which is responsible for creating a permission mask. This is a collection of Permission
 * values which are allowed or denied for a given group id. Permission mask is defined as a 64-bit integer,
 * where each bit corespond to a specific permission defined by Permission enum.
 */
DECLARE_OPENDAQ_INTERFACE(IPermissionMaskBuilder, IBaseObject)
{
    // [returnSelf]
    /*!
     * @brief Add read permission to the bit mask.
     */
    virtual ErrCode INTERFACE_FUNC read() = 0;

    // [returnSelf]
    /*!
     * @brief Add write permission to the bit mask.
     */
    virtual ErrCode INTERFACE_FUNC write() = 0;

    // [returnSelf]
    /*!
     * @brief Add execute permission to the bit mask.
     */
    virtual ErrCode INTERFACE_FUNC execute() = 0;

    // [returnSelf]
    /*!
     * @brief Removes all permissions from bit mask.
     */
    virtual ErrCode INTERFACE_FUNC clear() = 0;

    /*!
     * @brief Build permission mask and return it as 64-bit integer.
     * @param permissionMask[out] Permission mask defined as 64-bit integer where each bit corresponds
     * to a specific permissoin defined by Permission enum.
     */
    virtual ErrCode INTERFACE_FUNC build(Int* permissionMask) = 0;
};

/*!@}*/

/*!
 * @brief Creates a permision mask builder object.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(LIBRARY_FACTORY,
                                                            PermissionMaskBuilder,
                                                            IPermissionMaskBuilder,
                                                            createPermissionMaskBuilder)

/*!
 * @brief Creates a permission mask builder object from integer permission mask.
 * @param permissionMask Permission mask defined as 64-bit integer where each bit corresponds
 * to a specific permission defined in Permission enum.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(LIBRARY_FACTORY,
                                                            PermissionMaskBuilderFromMask,
                                                            IPermissionMaskBuilder,
                                                            createPermissionMaskBuilderFromMask,
                                                            Int,
                                                            permissionMask)

END_NAMESPACE_OPENDAQ
