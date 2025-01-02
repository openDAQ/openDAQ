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
#include <coretypes/stringobject.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup types_types
 * @defgroup types_types_type Type
 * @{
 */

/*#
 * [templated]
 */

/*!
 * @brief The base object type that is inherited by all Types (eg. Struct type, Simple type, Property object class)
 * in openDAQ.
 * 
 * Types are used for the construction of objects that are require validation/have pre-defined fields such as
 * Structs and Property objects. Types should be inserted into the Type manager to be used by different parts
 * of the SDK.
 */
DECLARE_OPENDAQ_INTERFACE(IType, IBaseObject)
{
    /*!
     * @brief Gets the name of the Type
     * @param[out] typeName The name of the Type.
     */
    virtual ErrCode INTERFACE_FUNC getName(IString** typeName) = 0;
};

/*!@}*/

END_NAMESPACE_OPENDAQ
