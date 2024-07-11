/*
 * Copyright 2022-2024 openDAQ d.o.o.
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
#include <cstdint>
#include <cstddef>
#include <string>
#include <string_view>
#include <array>
#include <coretypes/constexpr_utils.h>

namespace daq
{
    template <std::size_t N>
    class ConstexprString
    {
    public:
        constexpr ConstexprString()   // NOLINT(hicpp-use-equals-default, modernize-use-equals-default)
        {
        }

        constexpr ConstexprString(std::array<char, N>&& arr) // NOLINT(google-explicit-constructor)
            : string(std::move(arr))
        {
        }

        constexpr ConstexprString(std::array<char, N> arr) // NOLINT(google-explicit-constructor)
            : string(std::move(arr))
        {
        }

        constexpr ConstexprString(const char(&str)[N + 1]) // NOLINT(google-explicit-constructor)
            : string(Detail::toArraySlice<const char, N + 1, N>(str))
        {
        }

        [[nodiscard]]
        constexpr ConstexprString<N> clone() const
        {
            return ConstexprString<N>(string);
        }

        [[nodiscard]]
        constexpr std::string_view toStringView() const
        {
            return std::string_view(string.data(), N);
        }

        [[nodiscard]] std::string toString() const
        {
            return std::string(string.data(), N);
        }

        [[nodiscard]]
        constexpr std::array<char, N> toArray() const
        {
            return string;
        }

        static constexpr std::size_t Size()
        {
            return N;
        }

        template <std::size_t NumChars, std::size_t NOther>
        static constexpr ConstexprString<N> InitializeWith(const ConstexprString<NOther>& other)
        {
            static_assert(NumChars <= N, "Requested amount too big to copy to new string");

            ConstexprString<N> str{};
            for(std::size_t i = 0; i < NumChars; ++i)
            {
                str.string[i] = other[i];
            }
            return str;
        }

        template <std::size_t Start, std::size_t Count = N - Start>
        [[nodiscard]]
        constexpr auto substring() const
        {
            ConstexprString<Count> cs{};
            for (std::size_t i = Start, j = 0; j < Count; ++i, j++)
            {
                cs[j] = string[i];
            }

            return cs;
        }

        [[nodiscard]]
        constexpr const char* data() const
        {
            return string.data();
        }

        constexpr char& operator[](std::size_t index)
        {
            return string[index];
        }

        constexpr const char& operator[](std::size_t index) const
        {
            return string[index];
        }

        template <typename... T>
        constexpr auto concat(T&&... other) const -> ConstexprString<N + Detail::concatSize<T...>()>
        {
            ConstexprString<N + Detail::concatSize<T...>()> str{};
            for (std::size_t i = 0; i < N; ++i)
            {
                str[i] = string[i];
            }

            concatArray(str, N, std::forward<decltype(other)>(other)...);
            return str;
        }

        template <std::size_t N2>
        constexpr auto concatWithLiteral(char c, const char (&array)[N2]) const -> ConstexprString<N + N2>
        {
            ConstexprString<N + N2> str{};
            for (std::size_t i = 0; i < N; ++i)
            {
                str[i] = string[i];
            }

            str[N] = c;

            for (std::size_t i = 0; i < N2 - 1; ++i)
            {
                str[i + N + 1] = array[i];
            }

            return str;
        }

        template <typename... T>
        constexpr auto concatWith(char c, T&&... other) const -> ConstexprString<N + Detail::concatWithSize<T...>()>
        {
            ConstexprString<N + Detail::concatWithSize<T...>()> str{};
            for (std::size_t i = 0; i < N; ++i)
            {
                str[i] = string[i];
            }

            if (sizeof...(other) > 0) 
            {
                str[N] = c;
            }

            Detail::concatArrayWith(str, c, N + 1, std::forward<decltype(other)>(other)...);
            return str;
        }

        friend bool operator==(const ConstexprString<N>& lhs, const char (&rhs)[N])
        {
            return strncmp(lhs.data(), rhs, N) == 0;
        }

        friend bool operator==(const ConstexprString<N>& lhs, const char (&rhs)[N + 1])
        {
            return strncmp(lhs.data(), rhs, N) == 0;
        }

        template <std::size_t N2>
        friend bool operator==(const ConstexprString<N>& lhs, const char (&rhs)[N2])
        {
            return false;
        }

    private:
        std::array<char, N> string{};
    };

    template <std::size_t N>
    constexpr ConstexprString<N - 1> constexprString(const char(&str)[N])
    {
        return ConstexprString<N - 1>(str);
    }

    template <std::size_t N, std::size_t PrefixLength, std::size_t SuffixLength, std::size_t Count = N - PrefixLength - SuffixLength - 1>
    constexpr ConstexprString<Count> constexprStringFromSubstring(const char(&str)[N])
    {
        ConstexprString<Count> cstr{};
        for (std::size_t i = PrefixLength, j = 0; j < Count; ++i, ++j)
        {
            cstr[j] = str[i];
        }

        return cstr;
    }
}
