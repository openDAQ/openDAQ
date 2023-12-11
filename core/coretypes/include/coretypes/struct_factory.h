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
#include <coretypes/struct_ptr.h>
#include <coretypes/struct_builder_ptr.h>
#include <coretypes/dict_ptr.h>
#include <coretypes/string_ptr.h>
#include <coretypes/type_manager_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup types_structs_struct
 * @defgroup types_structs_struct_factories Factories
 * @{
 */

/*!
 * @brief Creates a new Struct with a given name and fields. The field names and values must match the Struct type definition
 * of with the given name that's present in the Type manager. If no such Struct type is present, a new one is created and added
 * to the Type manager.
 * @param name The name of the struct and its corresponding Struct type.
 * @param fields The dictionary of field names as keys, and corresponding field values as dictionary values.
 * @param typeManager The type manager that holds various Struct types (and other openDAQ types).
 *
 * Construction of the struct will fail if the field names do not match the corresponding type available in the Type manager.
 * Similarly, construction will fail if the field values are of different types than those defined in the Type manager. If
 * a field that is part of the Struct type is missing from the dictionary of fields, if available, the default value will be used.
 * Otherwise, the value of the field will be set to `nullptr`.
 *
 * Fields of Core type `ctUndefined` can have any type value.
 */
inline StructPtr Struct(const StringPtr& name, const DictPtr<IString, IBaseObject>& fields, const TypeManagerPtr& typeManager)
{
    StructPtr obj(Struct_Create(name, fields, typeManager));
    return obj;
}

inline StructPtr StructFromBuilder(const StructBuilderPtr& builder)
{
    StructPtr obj(StructFromBuilder_Create(builder));
    return obj;
}

inline StructBuilderPtr StructBuilder(const StringPtr& name, const TypeManagerPtr& typeManager)
{
    StructBuilderPtr obj(StructBuilder_Create(name, typeManager));
    return obj;
}

inline StructBuilderPtr StructBuilder(const StructPtr& struct_)
{
    StructBuilderPtr obj(StructBuilderFromStruct_Create(struct_));
    return obj;
}

/*!@}*/

END_NAMESPACE_OPENDAQ
