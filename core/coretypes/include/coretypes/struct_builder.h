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
#include <coretypes/struct.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup types_structs
 * @defgroup types_structs_struct Struct Builder
 * @{
 */

/*!
 * @brief Builder component of Struct objects. Contains setter methods to configure the Struct parameters, and a
 * `build` method that builds the Struct object.
 */
DECLARE_OPENDAQ_INTERFACE(IStructBuilder, IBaseObject)
{
    /*!
     * @brief Builds and returns a Struct object using the currently set values of the Builder.
     * @param[out] struct_ The built Struct.
     */
    virtual ErrCode INTERFACE_FUNC build(IStruct** struct_) = 0;
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
    virtual ErrCode INTERFACE_FUNC setFieldValues(IList* values) = 0;

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
     * @brief Sets the value of a field with the given name.
     * @param name The name of the queried field.
     * @param field The value of the field.
     */
    virtual ErrCode INTERFACE_FUNC set(IString* name, IBaseObject* field) = 0;

    /*!
     * @brief Gets the value of a field with the given name.
     * @param name The name of the queried field.
     * @param[out] field The value of the field.
     */
    virtual ErrCode INTERFACE_FUNC get(IString* name, IBaseObject** field) = 0;

    /*!
     * @brief Checks whether a field with the given name exists in the Struct
     * @param name The name of the checked field.
     * @param[out] contains True if the a field with `name` exists in the Struct; false otherwise.
     */
    virtual ErrCode INTERFACE_FUNC hasField(IString* name, Bool* contains) = 0;

    // [elementType(dictionary, IString, IBaseObject)]
    /*!
     * @brief Gets the field names and values of the Struct as a Dictionary.
     * @param[out] dictionary The Dictionary object with field names as keys, and field values as its values.
     */
    virtual ErrCode INTERFACE_FUNC getAsDictionary(IDict** dictionary) = 0;
};

/*!@}*/

OPENDAQ_DECLARE_CLASS_FACTORY(LIBRARY_FACTORY, StructBuilder, IString*, name, ITypeManager*, typeManager)
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, StructBuilderFromStruct, IStructBuilder, IStruct*, struct_)

END_NAMESPACE_OPENDAQ
