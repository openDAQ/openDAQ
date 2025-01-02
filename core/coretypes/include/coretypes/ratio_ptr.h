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
#include <coretypes/ratio.h>
#include <coretypes/objectptr.h>

BEGIN_NAMESPACE_OPENDAQ

class RatioPtr;

template <>
struct InterfaceToSmartPtr<IRatio>
{
    typedef RatioPtr SmartPtr;
};

/*!
 * @addtogroup types_ratio
 * @{
 */

/*!
 * Represents rational number. Use this object to wrap
 * rational number when you need to add fraction to lists, dictionaries and
 * other containers which accept `IBaseObject` and derived interfaces.
 *
 * Rational numbers are defined as numerator / denominator.
 *
 * Available factories:
 * @code
 * // Creates a new Ratio object.
 * RatioPtr Ratio(Int numerator, Int denominator)
 * @endcode
 */

class RatioPtr : public ObjectPtr<IRatio>
{
public:
    using ObjectPtr<IRatio>::ObjectPtr;

    RatioPtr()
    {
    }

    RatioPtr(ObjectPtr<IRatio>&& ptr)
        : ObjectPtr<IRatio>(std::move(ptr))
    {
    }

    RatioPtr(const ObjectPtr<IRatio>& ptr)
        : ObjectPtr<IRatio>(ptr)
    {
    }

    RatioPtr(const int value)
        : ObjectPtr<IRatio>(Ratio_Create(value, 1))
    {
    }

    RatioPtr(const ObjectPtr<IBaseObject>& obj)
    {
        if (obj.assigned())
        {
            IRatio* ratio = obj.asOrNull<IRatio>();
            if (ratio != nullptr)
                object = ratio;
            else
                object = Ratio_Create(obj, 1);
        }
        else
        {
            object = nullptr;
        }
    }

    RatioPtr(ObjectPtr<IBaseObject>&& obj)
    {
        if (obj.assigned())
        {
            IRatio* ratio = obj.asOrNull<IRatio>(true);
            if (ratio != nullptr)
            {
                object = ratio;
                obj.detach();
            }
            else
                object = Ratio_Create(obj, 1);
        }
        else
        {
            object = nullptr;
        }
    }

    /*!
     * @brief Gets numerator part.
     * @return Numerator value.
     */
    Int getNumerator() const
    {
        if (this->object == nullptr)
            throw InvalidParameterException();

        Int numerator;
        auto errCode = this->object->getNumerator(&numerator);
        checkErrorInfo(errCode);

        return numerator;
    }

    /*!
     * @brief Gets denominator part.
     * @return Denominator value.
     */
    Int getDenominator() const
    {
        if (this->object == nullptr)
            throw InvalidParameterException();

        Int denominator;
        auto errCode = this->object->getDenominator(&denominator);
        checkErrorInfo(errCode);

        return denominator;
    }

    /*!
     * @brief Simplifies rational number if possible and returns the simplified ratio as a new object.
     *
     * Call this method to reduce stored rational number to the lowest terms possible.
     * Example: 10/100 is reduced to 1/10.
     */
    RatioPtr simplify()
    {
        if (this->object == nullptr)
            throw InvalidParameterException();

        RatioPtr simplifiedRatio;
        auto errCode = this->object->simplify(&simplifiedRatio);
        checkErrorInfo(errCode);
        return simplifiedRatio;
    }

    explicit operator double() const
    {
        Int denominator;
        checkErrorInfo(this->object->getDenominator(&denominator));
        Int numerator;
        checkErrorInfo(this->object->getNumerator(&numerator));

        return static_cast<double>(numerator) / static_cast<double>(denominator);
    }

    [[nodiscard]] friend RatioPtr operator*(const RatioPtr& x, const RatioPtr& y)
    {
        CheckDenominatorAndThrow(x.getDenominator());
        CheckDenominatorAndThrow(y.getDenominator());

        return RatioPtr(Ratio_Create(x.getNumerator() * y.getNumerator(), x.getDenominator() * y.getDenominator()));
    }

    [[nodiscard]] friend RatioPtr operator*(const Int x, const RatioPtr& y)
    {
        CheckDenominatorAndThrow(y.getDenominator());

        return RatioPtr(Ratio_Create(x * y.getNumerator(), y.getDenominator()));
    }

    [[nodiscard]] friend RatioPtr operator*(const RatioPtr& y, const Int x)
    {
        return x * y;
    }

    [[nodiscard]] friend RatioPtr operator/(const Int x, const RatioPtr& y)
    {
        CheckDenominatorAndThrow(y.getNumerator());

        return RatioPtr(Ratio_Create(x * y.getDenominator(), y.getNumerator()));
    }

    [[nodiscard]] friend RatioPtr operator/(const RatioPtr& y, const Int x)
    {
        CheckDenominatorAndThrow(y.getDenominator());
        CheckDenominatorAndThrow(x);

        return RatioPtr(Ratio_Create(y.getNumerator(), y.getDenominator() * x));
    }

    [[nodiscard]] friend RatioPtr operator/(const RatioPtr& x, const RatioPtr& y)
    {
        CheckDenominatorAndThrow(x.getDenominator());
        CheckDenominatorAndThrow(y.getNumerator());

        return RatioPtr(Ratio_Create(x.getNumerator() * y.getDenominator(), x.getDenominator() * y.getNumerator()));
    }

protected:

    static void CheckDenominatorAndThrow(const Int den)
    {
        if (den == 0)
            throw InvalidParameterException("Denominator can't be zero");
    }
};

/*!@}*/

END_NAMESPACE_OPENDAQ
