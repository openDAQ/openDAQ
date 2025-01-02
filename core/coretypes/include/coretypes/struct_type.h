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
#include <coretypes/common.h>
#include <coretypes/type.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup types_structs
 * @defgroup types_structs_struct StructType
 * @{
 */

/*#
 * [templated(IType)]
 * [interfaceSmartPtr(IType, TypePtr)]
 */
/*!
 * @brief Struct types define the fields (names and value types, as well as optional default values) of Structs with a name
 * matching that of the Struct type.
 *
 * A Struct type contains a String-type name, a list of field names (list of Strings), a list of field types (list of Type objects),
 * and an optional list of Default values (list of Base objects). The Struct types should be added to the Type manager to be used
 * for Struct validation on creation. Alternatively, if a Struct is created with no matching Struct type in the manager, a default
 * Struct type is created based on the field names and types of the Created struct. Said Struct type is then added to the manager.
 *
 * The field types are a list of Type objects. These determine the types of values that should be used to fill in the corresponding
 * field value. The Type objects at the moment available in openDAQ are Simple types and Struct types. When adding any field other
 * than a Struct type, the Simple type corresponding to the Core type of the value should be created. When adding Struct fields, a
 * Struct type should be added to the field types.
 *
 * A Struct can only have fields of Core type: `ctBool`, `ctInt`, `ctFloat`, `ctString`, `ctList`, `ctDict`, `ctRatio`, `ctComplexNumber`,
 * `ctStruct`, or `ctUndefined`. Additionally, all Container types (`ctList`, `ctDict`) should only have values of the aforementioned
 * types.
 */
DECLARE_OPENDAQ_INTERFACE(IStructType, IType)
{
    // [elementType(names, IString)]
    /*!
     * @brief Gets the list of field names.
     * @param[out] names The list of field names (String objects)
     */
    virtual ErrCode INTERFACE_FUNC getFieldNames(IList** names) = 0;

    // [elementType(defaultValues, IBaseObject)]
    /*!
     * @brief Gets the list of field default values.
     * @param[out] defaultValues The list of field default values (Base objects)
     */
    virtual ErrCode INTERFACE_FUNC getFieldDefaultValues(IList** defaultValues) = 0;

    // [elementType(types, IType)]
    /*!
     * @brief Gets the list of field types.
     * @param[out] types The list of field types (Type objects)
     */
    virtual ErrCode INTERFACE_FUNC getFieldTypes(IList** types) = 0;
};

/*!@}*/

OPENDAQ_DECLARE_CLASS_FACTORY(LIBRARY_FACTORY, StructType, IString*, name, IList*, names, IList*, defaultValues, IList*, types)
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, StructTypeNoDefaults, IStructType, IString*, name, IList*, names, IList*, types)


END_NAMESPACE_OPENDAQ
