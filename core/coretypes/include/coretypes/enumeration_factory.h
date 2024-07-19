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
#include <coretypes/enumeration_ptr.h>
#include <coretypes/string_ptr.h>
#include <coretypes/integer_factory.h>
#include <coretypes/type_manager_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup types_enumerations_enumeration
 * @defgroup types_enumerations_enumeration_factories Factories
 * @{
 */

/*!
 * @brief Creates a new Enumeration object with a specified Enumeration Type name and value. The value
 * must correspond to a valid value defined in the Enumeration type with the provided name, which is expected
 * to be present in the Type Manager.
 * @param typeName The name of the Enumeration type.
 * @param value The string representation of the enumerator value.
 * @param typeManager The type manager that contains various Enumeration types (along with other openDAQ types).
 *
 * Construction of the Enumeration will fail if the value does not match the corresponding type available
 * in the Type Manager.
 */
inline EnumerationPtr Enumeration(const StringPtr& typeName, const StringPtr& value, const TypeManagerPtr& typeManager)
{
    EnumerationPtr obj(Enumeration_Create(typeName, value, typeManager));
    return obj;
}

/*!
 * @brief Creates a new Enumeration object with a specified Enumeration Type name and value. The value
 * must correspond to a valid integer value defined in the Enumeration type, associated with a string value key.
 * @param typeName The name of the Enumeration type.
 * @param value The integer representation of the enumerator value.
 * @param typeManager The type manager that contains various Enumeration types (along with other openDAQ types).
 *
 * Construction of the Enumeration will fail if the value does not match the corresponding type available
 * in the Type Manager.
 */
inline EnumerationPtr EnumerationWithIntValue(const StringPtr& typeName, const IntegerPtr& value, const TypeManagerPtr& typeManager)
{
    EnumerationPtr obj(EnumerationWithIntValue_Create(typeName, value, typeManager));
    return obj;
}

/*!
 * @brief Creates a new Enumeration object with a specified Enumeration Type name and value. The value
 * must correspond to a valid integer value defined in the Enumeration type, associated with a string value key.
 * @param type The Enumeration type.
 * @param value The integer representation of the enumerator value.
 *
 * Construction of the Enumeration will fail if the value does not match the corresponding type available
 * in the Type Manager.
 */
inline EnumerationPtr EnumerationWithIntValueAndType(const EnumerationTypePtr& type, const IntegerPtr& value)
{
    EnumerationPtr obj(EnumerationWithIntValueAndType_Create(type, value));
    return obj;
}


/*!@}*/

END_NAMESPACE_OPENDAQ
