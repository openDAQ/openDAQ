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
#include <utility>
#include <type_traits>
#include <array>

namespace daq
{
    template <std::size_t N>
    class ConstexprString;

    namespace Detail
    {
        template <typename T, std::size_t N, std::size_t... I>
        constexpr std::array<T, sizeof...(I)> toArrayImpl(T(&a)[N], std::index_sequence<I...>)
        {
            return { {a[I]...} };
        }

        template <typename T, std::size_t N>
        constexpr std::array<T, N> toArray(T(&a)[N])
        {
            return toArrayImpl(a, std::make_index_sequence<N>{});
        }

        template <typename T, std::size_t N, std::size_t... I>
        constexpr std::array<std::remove_cv_t<T>, sizeof...(I)> toArraySliceImpl(T(&a)[N], std::size_t prefix, std::index_sequence<I...>)
        {
            // ReSharper disable once CppCStyleCast
            return { {(std::remove_cv_t<T>)a[prefix + I]...} };
        }

        template <typename T, std::size_t N, std::size_t Count>
        constexpr std::array<std::remove_cv_t<T>, Count> toArraySlice(T(&a)[N])
        {
            return toArraySliceImpl(a, 0, std::make_index_sequence<Count>{});
        }

        template <std::size_t N, typename TString>
        constexpr void concatArrayWith(ConstexprString<N>& array, char, std::size_t start, const TString& str)
        {
            for (std::size_t i = 0, j = start; i < TString::Size(); i++, j++)
            {
                array[j] = str[i];
            }
        }

        template <std::size_t N, typename THead, typename... TTail>
        constexpr void concatArrayWith(ConstexprString<N>& array, char c, std::size_t start, const THead& head, const TTail&... tail)
        {
            for (std::size_t i = 0, j = start; i < THead::Size(); i++, j++)
            {
                array[j] = head[i];
            }

            array[start + THead::Size()] = c;
            concatArrayWith(array, c, start + THead::Size() + 1, std::forward<decltype(tail)>(tail)...);
        }

        template <std::size_t N, typename TString>
        constexpr void concatArray(ConstexprString<N>& string, std::size_t start, const TString& str)
        {
            for (std::size_t i = 0, j = start; i < TString::Size(); i++, j++)
            {
                string[j] = str[i];
            }
        }

        template <std::size_t N, typename THead, typename... TTail>
        constexpr void concatArray(ConstexprString<N>& string, std::size_t start, const THead& head, const TTail&... tail)
        {
            for (std::size_t i = 0, j = start; i < THead::Size(); i++, j++)
            {
                string[j] = head[i];
            }

            concatArrayWith(string, start + THead::Size(), std::forward<decltype(tail)>(tail)...);
        }

        template <typename... T>
        constexpr std::size_t concatSize()
        {
            return (... + std::remove_reference_t<std::remove_cv_t<T>>::Size());
        }

        template <typename... T>
        constexpr std::size_t concatWithSize()
        {
            return (sizeof...(T) + ... + std::remove_reference_t<std::remove_cv_t<T>>::Size());
        }

        template <typename... T>
        constexpr std::size_t adjustedConcatWithSize()
        {
            return ((2 * sizeof...(T) - 2) + ... + std::remove_reference_t<std::remove_cv_t<T>>::Size());
        }

        template <typename Type, template <typename...> typename Template>
        struct IsTemplateOfImpl : std::false_type
        {
        };

        template <template <typename...> typename Template, typename... Types>
        struct IsTemplateOfImpl<Template<Types...>, Template> : std::true_type
        {
        };
    }

    template <typename T>
    struct IsTemplate : std::false_type
    {
    };

    template <template <typename...> class T, typename... P>
    struct IsTemplate<T<P...>> : std::true_type
    {
    };

    template <typename Type, template <typename...> typename Template>
    struct IsTemplateOf : Detail::IsTemplateOfImpl<Type, Template>
    {
    };

    template <typename Type, template <typename> typename Template>
    struct IsDerivedFromTemplate
    {
    private:
        template <typename V>
        static decltype(static_cast<const Template<V>&>(std::declval<Type>()), std::true_type{}) Test(const Template<V>&);
        static std::false_type Test(...);

    public:
        static constexpr bool Value = decltype(IsDerivedFromTemplate::Test(std::declval<Type>()))::value;
    };

    template <typename T>
    struct TemplateArgumentCount
    {
        static constexpr std::size_t Arity = 0;
    };

    template <template <typename...> typename T, typename... P>
    struct TemplateArgumentCount<T<P...>>
    {
        static constexpr std::size_t Arity = sizeof...(P);
    };
}
