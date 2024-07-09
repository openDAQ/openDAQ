/*
 * Copyright 2022-2024 openDAQ d. o. o.
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
#include <coretypes/common.h>
#include <coreobjects/property_object_ptr.h>
#include <coretypes/type_manager_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup objects_property_object_obj
 * @addtogroup objects_property_object_obj_factories Factories
 * @{
 */

/*!
 * @brief Creates a empty Property object with no class.
 *
 * The Type Manager is usually obtained from the openDAQ Context object.
 */
inline PropertyObjectPtr PropertyObject()
{
    return {PropertyObject_Create()};
}

/*!
 * @brief Creates a Property object that inherits the properties of a class added to the Type manager with the specified name.
 * @param manager The Type manager.
 * @param className The name of the class from which the Property object inherits its properties.
 */
inline PropertyObjectPtr PropertyObject(const TypeManagerPtr& manager,
                                        const StringPtr& className)
{
    return {PropertyObjectWithClassAndManager_Create(manager, className)};
}

/*!@}*/

END_NAMESPACE_OPENDAQ
