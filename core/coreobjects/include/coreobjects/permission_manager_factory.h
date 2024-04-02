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
#include <coreobjects/permission_manager_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_security
 * @addtogroup opendaq_security_permission_manager Factories
 * @{
 */

/*!
 * @brief Creates a default permission manager object.
 */
inline PermissionManagerPtr PermissionManager()
{
    PermissionManagerPtr obj(PermissionManager_Create(nullptr));
    return obj;
}

/*!
 * @brief Creates a permission manager object which inherits permissions from a parent permission manager.
 * @param parent Permission manager of a parent component.
 */
inline PermissionManagerPtr PermissionManager(const PermissionManagerPtr& parent)
{
    PermissionManagerPtr obj(PermissionManager_Create(parent));
    return obj;
}

/*!@}*/

END_NAMESPACE_OPENDAQ
