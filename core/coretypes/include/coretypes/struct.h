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
#include <coretypes/dictobject.h>
#include <coretypes/coretype.h>
#include <coretypes/type_manager.h>
#include <coretypes/struct_type.h>

BEGIN_NAMESPACE_OPENDAQ

struct IStructBuilder;

/*!
 * @ingroup types_structs
 * @defgroup types_structs_struct Struct
 * @{
 */

/*!
 * @brief Structs are immutable objects that contain a set of key-value pairs. The key, as well as the types of each
 * associated value for each struct are defined in advance within a Struct type that has the same name as the Struct.
 *
 * The Struct types are stored within a Type manager. In any given instance of openDAQ, a single Type manager should
 * exist that is part of its Context.
 *
 * When creating a Struct, the Type manager is used to validate the given dictionary of keys and values against the
 * Struct type stored within the manager. If no type with the given Struct name is currently stored, a default type
 * is created using the Struct field names and values as its parameters. When creating a Struct, fields that are part
 * of the Struct type can be omitted. If so, they will be replaced by either `null` or, if provided by the Struct type,
 * the default value of the field.
 *
 * In the case that a field name is present that is not part of the struct type, or if the value type of the field does
 * not match, construction of the Struct will fail.
 *
 * NOTE: Field values of fields with the Core type `ctUndefined` can hold any value, regardless of its type.
 *
 * Structs are an openDAQ core type (ctStruct). Several objects in openDAQ such as an Unit, or DataDescriptor are Structs,
 * allowing for access to their fields through Struct methods. Such objects are, by definiton, immutable - their fields
 * cannot be modified. In order to change the value of a Struct-type object, a new Struct must be created.
 *
 * A Struct can only have fields of Core type: `ctBool`, `ctInt`, `ctFloat`, `ctString`, `ctList`, `ctDict`, `ctRatio`, `ctComplexNumber`,
 * `ctStruct`, or `ctUndefined`. Additionally, all Container types (`ctList`, `ctDict`) should only have values of the aforementioned
 * types.
 */
DECLARE_OPENDAQ_INTERFACE(IStruct, IBaseObject)
{
    /*!
     * @brief Gets the Struct's type.
     * @param[out] type The Struct type
     */
    virtual ErrCode INTERFACE_FUNC getStructType(IStructType** type) = 0;

    // [elementType(names, IString)]
    /*!
     * @brief Gets a list of all Struct field names.
     * @param[out] names The list of field names.
     *
     * The list of names will be of equal length to the list of values. Additionally, the name of a field at any given
     * index corresponds to the value stored in the list of values.
     */
    virtual ErrCode INTERFACE_FUNC getFieldNames(IList** names) = 0;

    // [elementType(values, IBaseObject)]
    /*!
     * @brief Gets a list of all Struct field values.
     * @param[out] values The list of field values.
     *
     * The list of names will be of equal length to the list of values. Additionally, the name of a field at any given
     * index corresponds to the value stored in the list of values.
     */
    virtual ErrCode INTERFACE_FUNC getFieldValues(IList** values) = 0;

    /*!
     * @brief Gets the value of a field with the given name.
     * @param name The name of the queried field.
     * @param[out] field The value of the field.
     */
    virtual ErrCode INTERFACE_FUNC get(IString* name, IBaseObject** field) = 0;

    // [elementType(dictionary, IString, IBaseObject)]
    /*!
     * @brief Gets the field names and values of the Struct as a Dictionary.
     * @param[out] dictionary The Dictionary object with field names as keys, and field values as its values.
     */
    virtual ErrCode INTERFACE_FUNC getAsDictionary(IDict** dictionary) = 0;

    /*!
     * @brief Checks whether a field with the given name exists in the Struct
     * @param name The name of the checked field.
     * @param[out] contains True if the a field with `name` exists in the Struct; false otherwise.
     */
    virtual ErrCode INTERFACE_FUNC hasField(IString* name, Bool* contains) = 0;
};

/*!@}*/

OPENDAQ_DECLARE_CLASS_FACTORY(LIBRARY_FACTORY, Struct, IString*, name, IDict*, fields, ITypeManager*, typeManager)
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, StructFromBuilder, IStruct, IStructBuilder*, builder)

END_NAMESPACE_OPENDAQ
