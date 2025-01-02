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
 * Represents float number as `IFloat` interface. Use this interface to wrap float
 * variable when you need to add the number to lists, dictionaries and other containers which
 * accept `IBaseObject` and derived interfaces.
 *
 * Float type is defined as double-precision IEEE 754 value.
 *
 * Available factories:
 * @code
 * // Creates a new Float. Throws exception if not successful.
 * IFloat* Float_Create(Float value)
 *
 * // Creates a new Float. Returns error code if not successful.
 * ErrCode createFloat(IFloat** obj, Float value)
 * @endcode
 */
DECLARE_OPENDAQ_INTERFACE(IFloat, IBaseObject)
{
    /*!
     * @brief Gets a float value stored in the object.
     * @param[out] value Stored float value.
     * @return OPENDAQ_SUCCESS if succeeded, error code otherwise.
     *
     * Call this method to extract the float value that is stored in the object.
     */
    virtual ErrCode INTERFACE_FUNC getValue(Float* value) = 0;

    /*!
     * @brief Compares stored float value to the float parameter.
     * @param value Value for comparison.
     * @param[out] equals The result of the comparison.
     *
     * Call this method to directly compare the object to the value parameter.
     */
    virtual ErrCode INTERFACE_FUNC equalsValue(const Float value, Bool* equals) = 0;
};

/*!@}*/

OPENDAQ_DECLARE_CLASS_FACTORY(LIBRARY_FACTORY, Float, const Float, value)
OPENDAQ_DECLARE_CLASS_FACTORY_FUNC_WITH_INTERFACE(
    LIBRARY_FACTORY,
    FloatObject, IFloat,
    const Float,
    value
)

END_NAMESPACE_OPENDAQ
