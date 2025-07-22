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
 * @brief Creates a list-type Argument info object with the specified name and item type.
 *
 * @param name The name of the argument.
 * @param itemType Corresponds to the expected type of items in the list argument.
 */
inline ArgumentInfoPtr ListArgumentInfo(const StringPtr& name, CoreType itemType)
{
    return ArgumentInfoPtr::Adopt(ListArgumentInfo_Create(name, itemType));
}

/*!
 * @brief Creates a dict-type Argument info object with the specified name, key type and item type.
 *
 * @param name The name of the argument.
 * @param keyType Corresponds to the expected type of key in the dictionary argument.
 * @param itemType Corresponds to the expected type of items in the dictionary argument.
 */
inline ArgumentInfoPtr DictArgumentInfo(const StringPtr& name, CoreType keyType, CoreType itemType)
{
    return ArgumentInfoPtr::Adopt(DictArgumentInfo_Create(name, keyType, itemType));
}

/*!
 * @brief Creates the Struct type object that defines the Argument info struct.
 */
inline StructTypePtr ArgumentInfoStructType()
{
    return StructType("ArgumentInfo",
                      List<IString>("Name", "Type", "KeyType", "ItemType"),
                      List<IBaseObject>("", static_cast<Int>(ctUndefined), static_cast<Int>(ctUndefined), static_cast<Int>(ctUndefined)),
                      List<IType>(SimpleType(ctString), SimpleType(ctInt), SimpleType(ctInt), SimpleType(ctInt)));
}

/*!@}*/

END_NAMESPACE_OPENDAQ
