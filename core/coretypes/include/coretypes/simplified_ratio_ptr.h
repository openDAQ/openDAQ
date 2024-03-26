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
#include <coretypes/objectptr.h>
#include <coretypes/ratio_ptr.h>
#include <coretypes/ctutils.h>

BEGIN_NAMESPACE_OPENDAQ

class SimplifiedRatioPtr : public RatioPtr
{
public:
    using RatioPtr::RatioPtr;

    SimplifiedRatioPtr() = default;

    SimplifiedRatioPtr(const int value)
        : RatioPtr(value)
    {
    }

    [[nodiscard]] friend SimplifiedRatioPtr operator*(const SimplifiedRatioPtr& x, const SimplifiedRatioPtr& y)
    {
        return MultiplySimplifiedRatios(x, y);
    }

    [[nodiscard]] friend SimplifiedRatioPtr operator*(const SimplifiedRatioPtr& x, const RatioPtr& y)
    {
        return MultiplySimplifiedRatios(x, y);
    }

    [[nodiscard]] friend SimplifiedRatioPtr operator*(const RatioPtr& x, const SimplifiedRatioPtr& y)
    {
        return MultiplySimplifiedRatios(x, y);
    }

    [[nodiscard]] friend SimplifiedRatioPtr operator*(const Int x, const SimplifiedRatioPtr& y)
    {
        return MultiplySimplifiedRatios(x, 1, y.getNumerator(), y.getDenominator());
    }

    [[nodiscard]] friend SimplifiedRatioPtr operator*(const SimplifiedRatioPtr& y, const Int x)
    {
        return x * y;
    }

    [[nodiscard]] friend SimplifiedRatioPtr operator/(const Int x, const SimplifiedRatioPtr& y)
    {
        return MultiplySimplifiedRatios(x, 1, y.getDenominator(), y.getNumerator());
    }

    [[nodiscard]] friend SimplifiedRatioPtr operator/(const SimplifiedRatioPtr& y, const Int x)
    {
        return MultiplySimplifiedRatios(y.getNumerator(), y.getDenominator(), 1, x);
    }

    [[nodiscard]] friend SimplifiedRatioPtr operator/(const SimplifiedRatioPtr& x, const SimplifiedRatioPtr& y)
    {
        return DivideSimplifiedRatios(x, y);
    }

    [[nodiscard]] friend SimplifiedRatioPtr operator/(const SimplifiedRatioPtr& x, const RatioPtr& y)
    {
        return DivideSimplifiedRatios(x, y);
    }
    [[nodiscard]] friend SimplifiedRatioPtr operator/(const RatioPtr& x, const SimplifiedRatioPtr& y)
    {
        return DivideSimplifiedRatios(x, y);
    }

protected:

    static SimplifiedRatioPtr MultiplySimplifiedRatios(Int num1, Int den1, Int num2, Int den2)
    {
        CheckDenominatorAndThrow(den1);
        CheckDenominatorAndThrow(den2);

        if (num1 > 1 && den2 > 1)
            daq::simplify(num1, den2);

        if (num2 > 1 && den1 > 1)
            daq::simplify(num2, den1);

        Int newNumerator = num1 * num2;
        Int newDenominator = den1 * den2;
        daq::simplify(newNumerator, newDenominator);
        return SimplifiedRatioPtr(Ratio_Create(newNumerator, newDenominator));
    }

    static SimplifiedRatioPtr MultiplySimplifiedRatios(const RatioPtr& ratio1, const RatioPtr& ratio2)
    {
        return MultiplySimplifiedRatios(ratio1.getNumerator(), ratio1.getDenominator(), ratio2.getNumerator(), ratio2.getDenominator());
    }

    static SimplifiedRatioPtr DivideSimplifiedRatios(const RatioPtr& ratio1, const RatioPtr& ratio2)
    {
        return MultiplySimplifiedRatios(ratio1.getNumerator(), ratio1.getDenominator(), ratio2.getDenominator(), ratio2.getNumerator());
    }
};

END_NAMESPACE_OPENDAQ
