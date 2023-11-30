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
 * @ingroup types_numerics
 * @defgroup types_ratio Ratio
 * @{
 */

/*!
 * Represents rational number as `IRatio` interface. Use this interface to wrap
 * rational number when you need to add the number to lists, dictionaries and
 * other containers which accept `IBaseObject` and derived interfaces.
 *
 * Rational numbers are defined as numerator / denominator.
 *
 * Available factories:
 * @code
 * // Creates a new Ratio object. Throws exception if not successful.
 * IRatio* Ratio_Create(Int numerator, Int denominator)
 *
 * // Creates a new Ratio object. Returns error code if not successful.
 * ErrCode createRatio(IRatio** obj, Int numerator, Int denominator)
 * @endcode
 */
DECLARE_OPENDAQ_INTERFACE(IRatio, IBaseObject)
{
    /*!
     * @brief Gets numerator part.
     * @param[out] numerator Numerator value.
     */
    virtual ErrCode INTERFACE_FUNC getNumerator(Int* numerator) = 0;

    /*!
     * @brief Gets denominator part.
     * @param[out] denominator Denominator value.
     */
    virtual ErrCode INTERFACE_FUNC getDenominator(Int* denominator) = 0;

    /*!
     * @brief Simplifies rational number if possible and returns the simplified ratio as a new object.
     * @param[out] simplifiedRatio the simplified ratio.
     *
     * Call this method to reduce stored rational number to the lowest terms possible.
     * Example: 10/100 is reduced to 1/10.
     */
    virtual ErrCode INTERFACE_FUNC simplify(IRatio** simplifiedRatio) = 0;
};

/*!@}*/

OPENDAQ_DECLARE_CLASS_FACTORY(LIBRARY_FACTORY, Ratio, Int, numerator, Int, denominator)

END_NAMESPACE_OPENDAQ
