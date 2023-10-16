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
#include <coretypes/stringobject.h>
#include <coretypes/coretype.h>

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

/*!@}*/

END_NAMESPACE_OPENDAQ
