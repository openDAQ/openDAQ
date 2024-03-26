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
#include <coretypes/complex_number_type.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup types_numerics
 * @defgroup types_complex_number ComplexNumber
 * @{
 */

/*!
 * Represents a complex number as `IComplexNumber` interface. Use this interface to wrap
 * complex number when you need to add the number to lists, dictionaries and
 * other containers which accept `IBaseObject` and derived interfaces.
 *
 * Complex numbers have two components: real and imaginary. Both of them are of Float type.
 *
 * Available factories:
 * @code
 * // Creates a new ComplexNumber object. Throws exception if not successful.
 * IComplexNumber* ComplexNumber_Create(ComplexFloat64* value)
 *
 * // Creates a new ComplexNumber object. Returns error code if not successful.
 * ErrCode createComplexNumber(IComplexNumber** obj, ComplexFloat64 value)
 * @endcode
 */
DECLARE_OPENDAQ_INTERFACE(IComplexNumber, IBaseObject)
{
    /*!
     * @brief Gets the number as ComplexFloat64 type.
     * @param[out] value Complex value.
     */
    virtual ErrCode INTERFACE_FUNC getValue(ComplexFloat64* value) = 0;

    /*!
     * @brief Compares stored complex value to the complex number parameter.
     * @param value Value for comparison.
     * @param[out] equal The result of the comparison.
     *
     * Call this method to directly compare the object to the value parameter.
     */
    virtual ErrCode INTERFACE_FUNC equalsValue(const ComplexFloat64 value, Bool* equal) = 0;

    /*!
     * @brief Gets the real part of the complex number value.
     * @param[out] real The real part of the complex value.
     */
    virtual ErrCode INTERFACE_FUNC getReal(Float* real) = 0;

    /*!
     * @brief Gets the imaginary part of the complex number value.
     * @param[out] imaginary The imaginary part of the complex value.
     */
    virtual ErrCode INTERFACE_FUNC getImaginary(Float* imaginary) = 0;
};

/*!@}*/

OPENDAQ_DECLARE_CLASS_FACTORY(LIBRARY_FACTORY, ComplexNumber, const Float, real, const Float, imaginary)

END_NAMESPACE_OPENDAQ
