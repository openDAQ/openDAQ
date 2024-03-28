/*
 * Copyright 2022-2024 Blueberry d.o.o.
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
#include <coretypes/baseobject.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @addtogroup types_numerics
 * @{
 */

/*!
 * @brief Represents either a float or an int number.
 *
 * Number is used if data type of the number is not strictly defined, i.e.
 * it can accept a float or an int.
 */
DECLARE_OPENDAQ_INTERFACE(INumber, IBaseObject)
{
    /*!
     * @brief Gets a value stored in the object as a floating point value.
     * @param[out] value Stored value as a floating point.
     * @return OPENDAQ_SUCCESS if succeeded, error code otherwise.
     */    
    virtual ErrCode INTERFACE_FUNC getFloatValue(Float* value) = 0;

    /*!
     * @brief Gets a value stored in the object as an integer value.
     * @param[out] value Stored value as an integer.
     * @return OPENDAQ_SUCCESS if succeeded, error code otherwise.
     */
    virtual ErrCode INTERFACE_FUNC getIntValue(Int* value) = 0;
};

/*!@}*/

END_NAMESPACE_OPENDAQ
