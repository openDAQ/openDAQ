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
 * @brief Enables conversion of the object to either Int, Float, or Bool type.
 *
 * An object which implements `IIntObject` will typically also implement IConvertible,
 * which allows conversion to other types.
 *
 * Example:
 * @code
 * IIntObject* intObj = ...;
 * IConvertible* conv;
 * auto errCode = intObj.queryInterface(IConvertible::Id, reinterpret_cast<void**>(&conv));
 * if (OPENDAQ_FAILED(errCode))
 *   return;
 *
 * Float val;
 * errCode = conv->toFloat(&val);
 * if (OPENDAQ_FAILED(errCode))
 *   return;
 *
 * // print val
 * @endcode 
 */

DECLARE_OPENDAQ_INTERFACE(IConvertible, IBaseObject)
{
    /*!
     * @brief Converts the object to Float type.
     * @param[out] val Float value.
     * @retval OPENDAQ_ERR_CONVERSIONFAILED Conversion has failed
     */
    virtual ErrCode INTERFACE_FUNC toFloat(Float* val) = 0;
    /*!
     * @brief Converts the object to Int type.
     * @param[out] val Int value.
     * @retval OPENDAQ_ERR_CONVERSIONFAILED Conversion has failed
     */
    virtual ErrCode INTERFACE_FUNC toInt(Int* val) = 0;
    /*!
     * @brief Converts the object to Bool type.
     * @param[out] val Bool value
     * @retval OPENDAQ_ERR_CONVERSIONFAILED Conversion has failed
     */
    virtual ErrCode INTERFACE_FUNC toBool(Bool * val) = 0;
};

/*!@}*/

END_NAMESPACE_OPENDAQ
