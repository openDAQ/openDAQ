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
#include <coretypes/baseobject.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @addtogroup types_numerics
 * @{
 */

/*!
 * Represents int number as `IInteger` interface. Use this interface to wrap integer
 * variable when you need to add the number to lists, dictionaries and other containers which
 * accept `IBaseObject` and derived interfaces.
 *
 * Available factories:
 * @code
 * // Creates a new Integer. Throws exception if not successful.
 * IInteger* Integer_Create(Int value)
 *
 * // Creates a new IntObject. Returns error code if not successful.
 * ErrCode createInteger(IInteger** obj, Int value)
 * @endcode
 */
DECLARE_OPENDAQ_INTERFACE(IInteger, IBaseObject)
{
    /*!
     * @brief Gets an int value stored in the object.
     * @param[out] value Stored int value.
     *
     * Call this method to extract the int value that is stored in the object.
     */
    virtual ErrCode INTERFACE_FUNC getValue(Int* value) = 0;

    /*!
     * @brief Compares stored int value to the int parameter.
     * @param value Value for comparison.
     * @param[out] equals The result of the comparison.
     *
     * Call this method to directly compare the object to the value parameter.
     */
    virtual ErrCode INTERFACE_FUNC equalsValue(Int value, Bool* equals) = 0;
};

/*!@}*/

OPENDAQ_DECLARE_CLASS_FACTORY(LIBRARY_FACTORY, Integer, Int, value)

END_NAMESPACE_OPENDAQ
