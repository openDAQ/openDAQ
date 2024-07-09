/*
 * Copyright 2022-2024 openDAQ d. o. o.
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
#include <type_traits>
#include <sstream>
#include <complex>
#include <coretypes/intfid.h>
#include <coretypes/complex_number_type.h>

BEGIN_NAMESPACE_OPENDAQ

#pragma pack(push, 1)

template <typename TFloatType>
struct Complex_Number
{
    using Type = TFloatType;

    static_assert(std::is_floating_point_v<TFloatType>, "Complex_Number type must be a floating-point type");

    Complex_Number()
        : Complex_Number(0, 0)
    {
    }

    Complex_Number(TFloatType real)
        : Complex_Number(real, 0)
    {
    }

    template <typename T = TFloatType, std::enable_if_t<std::is_same_v<double, T>, int> = 0>
    Complex_Number(const Complex_Number<float>& sample)
        : Complex_Number((double) sample.real, (double) sample.imaginary)
    {
    }

    Complex_Number(Type real, Type imaginary)
        : real(real)
        , imaginary(imaginary)
    {
    }

    Complex_Number(const Complex_Number<TFloatType>&) = default;
    Complex_Number& operator=(const Complex_Number& other) = default;

    template <typename V>
    friend bool operator==(const Complex_Number<TFloatType>& lhs, const Complex_Number<V>& rhs)
    {
        return (lhs.real == rhs.real) && (lhs.imaginary == rhs.imaginary);
    }

    template <typename U, typename V>
    friend bool operator!=(const Complex_Number& lhs, const Complex_Number& rhs)
    {
        return !lhs.operator=(lhs, rhs);
    }

    template <typename TValue, typename std::enable_if_t<std::is_arithmetic_v<TValue>, int> = 0>
    friend bool operator==(const Complex_Number& lhs, TValue value)
    {
        return lhs.imaginary == 0 && lhs.real == value;
    }

    friend bool operator<(const Complex_Number& lhs, const Complex_Number& rhs)
    {
        const double amplLeft = lhs.real * lhs.real + lhs.imaginary * lhs.imaginary;
        const double amplRight = rhs.real * rhs.real + rhs.imaginary * rhs.imaginary;
        return amplLeft < amplRight;
    }

    friend bool operator<=(const Complex_Number& lhs, const Complex_Number& rhs)
    {
        return !(rhs < lhs);
    }

    friend bool operator>(const Complex_Number& lhs, const Complex_Number& rhs)
    {
        return operator<(rhs, lhs);
    }

    friend bool operator>=(const Complex_Number& lhs, const Complex_Number& rhs)
    {
        return !(lhs < rhs);
    }

    explicit operator std::complex<Type>() const
    {
        return std::complex<Type>(real, imaginary);
    }

    TFloatType real{};
    TFloatType imaginary{};
};

using ComplexFloat32 = Complex_Number<float>;
using ComplexFloat64 = Complex_Number<double>;

static_assert(std::is_standard_layout_v<ComplexFloat32>, "ComplexFloat32 is not standard layout");
static_assert(std::is_trivially_copyable_v<ComplexFloat32>, "ComplexFloat32 is not trivially copyable");

static_assert(std::is_standard_layout_v<ComplexFloat64>, "ComplexFloat64 is not standard layout");
static_assert(std::is_trivially_copyable_v<ComplexFloat64>, "ComplexFloat64 is not trivially copyable");

#pragma pack(pop)

END_NAMESPACE_OPENDAQ
