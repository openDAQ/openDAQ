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
#include <coretypes/common.h>
#include <coretypes/stringobject.h>
#include <coretypes/coretype.h>
#include <coretypes/type_manager.h>
#include <coretypes/enumeration_type.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup types_enumeration
 * @defgroup types_enumerations_enumeration Enumeration
 * @{
 */

/*!
 * @brief Enumerations are objects that encapsulate a value within a predefined set of named integral constants.
 * These constants are predefined in an Enumeration type with the same name as the Enumeration.
 *
 * The Enumeration types are stored within a Type manager. In any given instance of openDAQ, a single Type manager should
 * exist that is part of its Context.
 *
 * When creating an Enumeration object, the Type manager is used to validate the given enumerator value name against the
 * Enumeration type stored within the manager. If no type with the given Enumeration name is currently stored,
 * construction of the Enumeration object will fail. Similarly, if the provided enumerator value name is not part of
 * the Enumeration type, the construction of the Enumeration object will also fail.
 */
DECLARE_OPENDAQ_INTERFACE(IEnumeration, IBaseObject)
{
    /*!
     * @brief Gets the Enumeration's type.
     * @param[out] type The Enumeration type
     */
    virtual ErrCode INTERFACE_FUNC getEnumerationType(IEnumerationType** type) = 0;

    /*!
     * @brief Gets the Enumeration value as String containing the name of the enumerator constant.
     * @param[out] value Emumeration value.
     */
    virtual ErrCode INTERFACE_FUNC getValue(IString** value) = 0;
};

/*!@}*/

OPENDAQ_DECLARE_CLASS_FACTORY(
    LIBRARY_FACTORY, Enumeration,
    IString*, name,
    IString*, value,
    ITypeManager*, typeManager
)

END_NAMESPACE_OPENDAQ
