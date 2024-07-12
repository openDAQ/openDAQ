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
#include <coretypes/struct_type_ptr.h>
#include <coretypes/listobject_factory.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup types_structs_struct
 * @defgroup types_structs_struct_factories Factories
 * @{
 */

/*!
 * @brief Creates a Struct type with a given name, fieldn ames, default values, and field types.
 * @param name The name of the Struct type
 * @param fieldNames The list of field names (String objects)
 * @param defaultValues The list of default values (Base objects)
 * @param fieldTypes The list of field types (Type objects)
 *
 * Struct type construction follows the standard rules of Structs. The three lists must be of equal
 * size, and the field types must be of types supported in Structs.
 */
inline StructTypePtr StructType(const StringPtr& name,
                                const ListPtr<IString>& fieldNames,
                                const ListPtr<IBaseObject>& defaultValues,
                                const ListPtr<IType>& fieldTypes)
{
    StructTypePtr obj(StructType_Create(name, fieldNames, defaultValues, fieldTypes));
    return obj;
}

/*!
 * @brief Creates a Struct type with a given name, field names and field types.
 * @param name The name of the Struct type
 * @param fieldNames The list of field names (String objects)
 * @param fieldTypes The list of field types (Type objects)
 *
 * Struct type construction follows the standard rules of Structs. The two lists must be of equal
 * size, and the field types must be of types supported in Structs.
 */
inline StructTypePtr StructType(const StringPtr& name,
                                const ListPtr<IString>& fieldNames,
                                const ListPtr<IType>& fieldTypes)
{
    StructTypePtr obj(StructTypeNoDefaults_Create(name, fieldNames, fieldTypes));
    return obj;
}

/*!@}*/
END_NAMESPACE_OPENDAQ
