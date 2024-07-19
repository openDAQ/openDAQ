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
#include <coreobjects/permission_mask_builder_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup objects_security
 * @addtogroup objects_security_permission_mask_builder Factories
 * @{
 */

/*!
 * @brief Creates a permission mask builder object.
 */
inline PermissionMaskBuilderPtr PermissionMaskBuilder()
{
    PermissionMaskBuilderPtr obj(PermissionMaskBuilder_Create(0));
    return obj;
}

/*!
 * @brief Creates a permission mask builder object from integer permission mask.
 * @param permissionMask Permission mask defined as 64-bit integer where each bit corresponds
 * to a specific permission defined in Permission enum.
 */
inline PermissionMaskBuilderPtr PermissionMaskBuilder(Int permissionMask)
{
    PermissionMaskBuilderPtr obj(PermissionMaskBuilder_Create(permissionMask));
    return obj;
}

/*!@}*/

END_NAMESPACE_OPENDAQ
