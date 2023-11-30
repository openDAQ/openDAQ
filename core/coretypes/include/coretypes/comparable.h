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
#include <coretypes/baseobject.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @addtogroup types_utility
 * @{
 */

/*!
 * @brief Enables comparison to another object.
 *
 * Use this interface to compare the object to another object. his interface is implemented by types
 * whose values can be ordered or sorted. It requires that implementing types define a single method,
 * `compareTo`, that indicates whether the position of the current instance in the sort order is before,
 * after, or the same as a second object of the same type.
 */

DECLARE_OPENDAQ_INTERFACE(IComparable, IBaseObject)
{
    /*!
     * @brief Compares the object to another object.
     * @param obj Object for comparison.
     * @retval OPENDAQ_LOWER The object's value is lower than the value of the compared object.
     * @retval OPENDAQ_HIGHER The object's value is higher than the value of the compared object.
     * @retval OPENDAQ_EQUAL The object's value is equal to the value of the compared object.
     *
     * Compares the current instance with another object of the same type and returns an integer that
     * indicates whether the current instance precedes, follows, or occurs in the same position in the
     * sort order as the other object. 
     */
    virtual ErrCode INTERFACE_FUNC compareTo(IBaseObject* obj) = 0;
};

/*!@}*/

END_NAMESPACE_OPENDAQ
