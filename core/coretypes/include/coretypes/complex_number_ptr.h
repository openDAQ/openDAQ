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
#include <coretypes/coretypes.h>
#include <coretypes/complex_number.h>

BEGIN_NAMESPACE_OPENDAQ

class ComplexNumberPtr;

template <>
struct InterfaceToSmartPtr<IComplexNumber>
{
    typedef ComplexNumberPtr SmartPtr;
};

/*!
 * @addtogroup types_complex_number
 * @{
 */

/*!
 * Represents a complex number as `ICommplexNumber` interface. Use this interface to wrap
 * complex number when you need to add the number to lists, dictionaries and
 * other containers which accept `IBaseObject` and derived interfaces.
 *
 * Complex numbers have two components: real and imaginary. Both of them are of Float type.
 *
 * Available factories:
 * @code
 * // Creates a new Ratio object. Throws exception if not successful.
 * ComplexNumberPtr ComplexNumber(ComplexFloat64* value)
 * @endcode
 */
class ComplexNumberPtr : public daq::ObjectPtr<IComplexNumber>
{
public:
    using daq::ObjectPtr<IComplexNumber>::ObjectPtr;

    ComplexNumberPtr()
        : daq::ObjectPtr<IComplexNumber>()
    {
    }

    ComplexNumberPtr(daq::ObjectPtr<IComplexNumber>&& ptr)
        : daq::ObjectPtr<IComplexNumber>(std::move(ptr))
    {
    }

    ComplexNumberPtr(const daq::ObjectPtr<IComplexNumber>& ptr)
        : daq::ObjectPtr<IComplexNumber>(ptr)
    {
    }

    ComplexNumberPtr(const ComplexNumberPtr& other)
        : daq::ObjectPtr<IComplexNumber>(other)
    {
    }

    ComplexNumberPtr(ComplexNumberPtr&& other) noexcept
        : daq::ObjectPtr<IComplexNumber>(std::move(other))
    {
    }

    ComplexNumberPtr& operator=(const ComplexNumberPtr& other)
    {
        if (this == &other)
            return *this;

        daq::ObjectPtr<IComplexNumber>::operator=(other);
        return *this;
    }

    ComplexNumberPtr& operator=(ComplexNumberPtr&& other) noexcept
    {
        if (this == std::addressof(other))
            return *this;

        daq::ObjectPtr<IComplexNumber>::operator=(std::move(other));
        return *this;
    }

    /*!
     * @brief Gets the real part of the complex number value.
     * @return The real part of the complex value.
     */
    Float getReal() const
    {
        validateHasObject();
        Float value;
        auto errCode = this->object->getReal(&value);
        checkErrorInfo(errCode);
        return value;
    }

    /*!
     * @brief Gets the imaginary part of the complex number value.
     * @return The imaginary part of the complex value.
     */
    Float getImaginary() const
    {
        validateHasObject();
        Float value;
        auto errCode = this->object->getImaginary(&value);
        checkErrorInfo(errCode);
        return value;
    }

    ComplexFloat64 getValue() const
    {
        validateHasObject();
        ComplexFloat64 value;
        auto errCode = this->object->getValue(&value);
        checkErrorInfo(errCode);
        return value;
    }

    bool equalsValue(const ComplexFloat64 other) const
    {
        return getValue() == other;
    }

private:

    void validateHasObject() const
    {
        if (this->object == nullptr)
            throw daq::InvalidParameterException();
    }

};

/*!@}*/

END_NAMESPACE_OPENDAQ
