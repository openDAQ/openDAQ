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
#include <coreobjects/property_object_class_builder_ptr.h>
#include <coretypes/type_manager_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup objects_property_object_class
 * @addtogroup objects_property_object_class_factories Factories
 * @{
 */

/*!
 * @brief Creates a property object class builder object with a given name.
 * @param name The name of the class.
 */
inline PropertyObjectClassBuilderPtr PropertyObjectClassBuilder(const StringPtr& name)
{
    PropertyObjectClassBuilderPtr obj(PropertyObjectClassBuilder_Create(name));
    return obj;
}

/*!
 * @brief Creates a Property object class builder object with a given name, and a reference to the Property object class manager.
 * @param manager The Property object class manager object.
 * @param name The name of the class.
 */
inline PropertyObjectClassBuilderPtr PropertyObjectClassBuilder(const TypeManagerPtr& manager, const StringPtr& name)
{
    PropertyObjectClassBuilderPtr obj(PropertyObjectClassBuilderWithManager_Create(manager, name));
    return obj;
}

/*!
 * @brief Creates a PropertyObjectClass using Builder
 * @param builder PropertyObjectClass Builder
 */
inline PropertyObjectClassPtr PropertyObjectClassFromBuilder(const PropertyObjectClassBuilderPtr& builder)
{
    PropertyObjectClassPtr obj(PropertyObjectClassFromBuilder_Create(builder));
    return obj;
}

/*!@}*/

END_NAMESPACE_OPENDAQ
