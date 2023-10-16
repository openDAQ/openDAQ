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

/**
 * @file    span.h
 * @author  Martin Kraner
 * @date    24/05/2019
 * @version 1.0
 *
 * @brief Constexpr pointer + length type
 *
 */
#pragma once
#include <cstdint>
#include <cstddef>

namespace daq
{
    inline constexpr std::size_t dynamicExtent = 0;

    // template <typename T, std::size_t Size = dynamicExtent>
    // class Span;

    template <typename T, std::size_t N>
    class Span
    {
    public:
        constexpr Span(T (&a)[N]) // NOLINT(google-explicit-constructor,hicpp-explicit-conversions)
            : ptr(a)
        {
        }

        template <std::size_t K, typename std::enable_if<std::extent<T>::value != K, int>::type = 0>
        constexpr Span(T (&a)[K])
            : ptr(a)
        {
        }

        template <typename U, typename std::enable_if<std::extent<U>::value == 0, int>::type = 0>
        constexpr Span(U ptr)
            : ptr(ptr)
        {
        }

        [[nodiscard]]
        constexpr std::size_t size() const noexcept
        {
            return N;
        }

        [[nodiscard]]
        constexpr T* data() const noexcept
        {
            return ptr;
        }

        constexpr T& operator[](std::size_t index) const
        {
            return ptr[index];
        }

    protected:
        T* const ptr;
    };

    template <typename T, std::size_t N>
    Span(T (&)[N]) -> Span<T, N>;

    template <typename T,
              typename U = typename std::remove_pointer<T>::type,
              typename std::enable_if<std::extent<T>::value == 0, int>::type = 0>
    Span(T ptr) -> Span<U, dynamicExtent>;

    // class FixedString : public Span<const char>
    // {
    // public:
    //     template <std::size_t N>
    //     constexpr FixedString(const char (&a)[N]) 
    //         : Span(a, N - 1) // NOLINT(google-explicit-constructor,hicpp-explicit-conversions)
    //     {
    //     }
    // };
}
