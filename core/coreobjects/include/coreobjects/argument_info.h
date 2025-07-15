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
#include <coretypes/baseobject.h>
#include <coretypes/stringobject.h>
#include <coretypes/coretype.h>
#include <coretypes/listobject.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup objects_utility
 * @addtogroup objects_argument_info ArgumentInfo
 * @{
 */

/*!
 * @brief Provides the name and type of a single function/procedure argument
 *
 * Usually part of a list of arguments in a Callable info object.
 *
 * Argument info objects implement the Struct methods internally and are Core type `ctStruct`.
 */
DECLARE_OPENDAQ_INTERFACE(IArgumentInfo, IBaseObject)
{
    /*!
     * @brief Gets the name of the argument.
     * @param[out] name The name of the argument.
     */
    virtual ErrCode INTERFACE_FUNC getName(IString** name) = 0;

    /*!
     * @brief Gets the core type of the argument.
     * @param[out] type The type of the argument.
     *
     * Dictionary, List and Object types should be avoided in public function/procedure callable objects
     * as their key, item, or base interface type cannot be determined without internal knowledge
     * of the function/procedure.
     */
    virtual ErrCode INTERFACE_FUNC getType(CoreType* type) = 0;

    /*!
     * @brief Gets the item type of list/dict-type Argument Info objects. The item type specifies the
     * type of values in the list or dictionary arguments.
     * @param[out] itemType The item type of the list/dictionary argument.
     *
     * In list-type Argument Info, the type of each item corresponds to the item type.
     *
     * In dict-type Argument Info, the type of each value in the <key, value> pairing corresponds to the
     * item type.
     */
    virtual ErrCode INTERFACE_FUNC getItemType(CoreType* itemType) = 0;

    /*!
     * @brief Gets the key type of dict-type Argument Info objects. The item type specifies the type of
     * keys in dictionary arguments.
     *
     * @param[out] keyType The key type of the dictionary argument.
     */
    virtual ErrCode INTERFACE_FUNC getKeyType(CoreType* keyType) = 0;
};

/*!@}*/

/*!
 * @addtogroup objects_argument_info_factories Factories
 * @{
 */

/*!
 * @brief Creates an Argument info object with the specified name and type.
 * @param name The name of the argument.
 * @param type The type expected of the argument.
 */
OPENDAQ_DECLARE_CLASS_FACTORY(
    LIBRARY_FACTORY, ArgumentInfo,
    IString*, name,
    CoreType, type
);

/*!
 * @brief Creates a list-type Argument info object with the specified name and item type.
 *
 * @param name The name of the argument.
 * @param itemType Corresponds to the expected type of items in the list argument.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY,
    ListArgumentInfo, IArgumentInfo,
    IString*, name,
    CoreType, itemType
);

/*!
 * @brief Creates a dict-type Argument info object with the specified name, key type and item type.
 *
 * @param name The name of the argument.
 * @param keyType Corresponds to the expected type of key in the dictionary argument.
 * @param itemType Corresponds to the expected type of items in the dictionary argument.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY,
    DictArgumentInfo, IArgumentInfo,
    IString*, name,
    CoreType, keyType,
    CoreType, itemType
);

/*!@}*/

END_NAMESPACE_OPENDAQ
