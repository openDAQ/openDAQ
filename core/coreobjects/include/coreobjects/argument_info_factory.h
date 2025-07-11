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

#include <coreobjects/argument_info_ptr.h>
#include <coretypes/struct_type_factory.h>
#include <coretypes/simple_type_factory.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup objects_argument_info
 * @addtogroup objects_argument_info_factories Factories
 * @{
 */

/*!
 * @brief Creates an Argument info object with the specified name and type.
 * @param name The name of the argument.
 * @param type The type expected of the argument.
 */
inline ArgumentInfoPtr ArgumentInfo(const StringPtr& name, CoreType type)
{
    return ArgumentInfoPtr::Adopt(ArgumentInfo_Create(name, type));
}

/*!
 * @brief Creates an Argument info object with the specified name, type, and container argument info.
 * The type must be either List or Dictionary.
 * @param name The name of the argument.
 * @param type The type expected of the argument. Must be ctList or ctDict.
 * @param containerArgumentInfo List of Argument Info objects that convey the expected member types of the list/dictionary argument.
 *
 * In list-type Argument Info, the type of each item of the Container Argument Info list corresponds
 * to the expected argument type of the function.
 *
 * In dictionary-type Argument Info, the name of the argument corresponds to the string key of the
 * dictionary, while the type corresponds to the expected argument type. Do note that dictionary
 * entries with non-string keys can not be represented by Argument Info objects.
 */
inline ArgumentInfoPtr ContainerArgumentInfo(const StringPtr& name, CoreType type, const ListPtr<IArgumentInfo>& containerArgumentInfo)
{
    return ArgumentInfoPtr::Adopt(ContainerArgumentInfo_Create(name, type, containerArgumentInfo));
}

/*!
 * @brief Creates the Struct type object that defines the Argument info struct.
 */
inline StructTypePtr ArgumentInfoStructType()
{
    return StructType("ArgumentInfo",
                      List<IString>("Name", "Type", "ContainerArgumentInfo"),
                      List<IBaseObject>("", static_cast<Int>(ctUndefined), List<IArgumentInfo>()),
                      List<IType>(SimpleType(ctString), SimpleType(ctInt), SimpleType(ctList)));
}

/*!@}*/

END_NAMESPACE_OPENDAQ
