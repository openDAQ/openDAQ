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
#include <coretypes/enumeration_type_ptr.h>
#include <coretypes/listobject_factory.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup types_enumerations_enumeration_type
 * @defgroup types_enumerations_enumeration_type_factories Factories
 * @{
 */

/*!
 * @brief Creates an Enumeration type with a given type name, enumerator names and first enumerator value.
 * Enumerator values are automatically assigned in ascending order, starting from the specified first value.
 * @param typeName The name of the Enumeration type
 * @param enumeratorNames The list of enumerator names (String objects)
 * @param firstEnumeratorIntValue The Int value of first enumerator (Integer)
 */
inline EnumerationTypePtr EnumerationType(
    const StringPtr& typeName,
    const ListPtr<IString>& enumeratorNames,
    const Int firstEnumeratorIntValue = 0)
{
    EnumerationTypePtr obj(EnumerationType_Create(typeName, enumeratorNames, firstEnumeratorIntValue));
    return obj;
}

/*!
 * @brief Creates an Enumeration type for enum with a given type name, and dictionary of enumerator names and
 * values.
 * @param typeName The name of the Enumeration type
 * @param enumerators The dictionary of enumerators (String objects as keys, Integer objects as values)
 */
inline EnumerationTypePtr EnumerationTypeWithValues(
    const StringPtr& typeName,
    const DictPtr<IString, IInteger>& enumerators)
{
    EnumerationTypePtr obj(EnumerationTypeWithValues_Create(typeName, enumerators));
    return obj;
}

/*!@}*/
END_NAMESPACE_OPENDAQ
