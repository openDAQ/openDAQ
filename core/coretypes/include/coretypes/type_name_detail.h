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
#include <coretypes/constexpr_string.h>
#include <coretypes/constexpr_utils.h>

namespace daq
{
    template <typename T>
    constexpr auto typeName();

    template <typename T>
    constexpr auto typeNameQualified();
}

namespace daq::Detail
{
    template <typename T>
    constexpr auto nameOf();

    template <>
    constexpr auto nameOf<std::int32_t>()
    {
        return constexprString("int32");
    }

    template <>
    constexpr auto nameOf<std::uint32_t>()
    {
        return constexprString("uint32");
    }

    template <>
    constexpr auto nameOf<std::int16_t>()
    {
        return constexprString("int16");
    }

    template <>
    constexpr auto nameOf<std::uint16_t>()
    {
        return constexprString("uint16");
    }

    template <>
    constexpr auto nameOf<std::int8_t>()
    {
        return constexprString("int8");
    }

    template <>
    constexpr auto nameOf<std::uint8_t>()
    {
        return constexprString("uint8");
    }

    template <>
    constexpr auto nameOf<char>()
    {
        return constexprString("char");
    }
    
    template <>
    constexpr auto nameOf<bool>()
    {
        return constexprString("bool");
    }
    
    template <>
    constexpr auto nameOf<float>()
    {
        return constexprString("float");
    }
    
    template <>
    constexpr auto nameOf<double>()
    {
        return constexprString("double");
    }

    // Predefined type names of fundamental types according to the data-model
    // see: https://en.cppreference.com/w/cpp/language/types

#if defined(__LP64__)
    template <>
    constexpr auto nameOf<long>()
    {
        return constexprString("int64");
    }

    template <>
    constexpr auto nameOf<unsigned long>()
    {
        return constexprString("uint64");
    }

    template <>
    constexpr auto nameOf<long long>()
    {
        return constexprString("int64");
    }

    template <>
    constexpr auto nameOf<unsigned long long>()
    {
        return constexprString("uint64");
    }
#else
    template <>
    constexpr auto nameOf<long>()
    {
        return constexprString("int32");
    }

    template <>
    constexpr auto nameOf<unsigned long>()
    {
        return constexprString("uint32");
    }

    template <>
    constexpr auto nameOf<long long>()
    {
        return constexprString("int64");
    }

    template <>
    constexpr auto nameOf<unsigned long long>()
    {
        return constexprString("uint64");
    }
#endif

    ///////////////
    //// Details
    //////////////

    template <typename T>
    constexpr auto nameOfSize() noexcept
    {
    #if defined(__FUNCSIG__)
        // "auto __cdecl daq::Detail::nameOfSize<int>(void) noexcept\0"
        //                                 ^~~~ = 4       ^~~~~~~~~~ = 10
        return sizeof(__FUNCSIG__) - (4 + 10);
    #else
        // "constexpr auto daq::Detail::nameOfSize() [with T = int]\0"
        //                                   ^~~~ = 4             ^ = 1
        return sizeof(__PRETTY_FUNCTION__) - (4 + 1);
    #endif
    }

#if defined(__FUNCSIG__)
    inline constexpr auto prefixLength = nameOfSize<int>() - sizeof("int>(void)") + 1;
    inline constexpr long suffixLength = sizeof(">(void)") - 1;
#else
    inline constexpr auto prefixLength = nameOfSize<int>() - sizeof("int]") + 1;
    inline constexpr auto suffixLength = sizeof("]") - 1;
#endif

    template <std::size_t N>
    constexpr std::size_t getPrefixLength(const char (&signature)[N], std::size_t offset = 0)
    {
        auto classStart = prefixLength + offset;

        if (N > classStart + 5
            && signature[classStart + 0] == 'c'
            && signature[classStart + 1] == 'l'
            && signature[classStart + 2] == 'a'
            && signature[classStart + 3] == 's'
            && signature[classStart + 4] == 's'
            && signature[classStart + 5] == ' ')
        {
            return classStart + 6;
        }

        auto structStart = prefixLength + offset;

        if (N > structStart + 6
            && signature[structStart + 0] == 's'
            && signature[structStart + 1] == 't'
            && signature[structStart + 2] == 'r'
            && signature[structStart + 3] == 'u'
            && signature[structStart + 4] == 'c'
            && signature[structStart + 5] == 't'
            && signature[structStart + 6] == ' ')
        {
            return structStart + 7;
        }

        return prefixLength + offset;
    }

    template <std::size_t N>
    constexpr std::size_t getPrefixLength(const ConstexprString<N>& signature, std::size_t offset = 0)
    {
        auto classStart = prefixLength + offset;

        if (N > classStart + 5
            && signature[classStart + 0] == 'c'
            && signature[classStart + 1] == 'l'
            && signature[classStart + 2] == 'a'
            && signature[classStart + 3] == 's'
            && signature[classStart + 4] == 's'
            && signature[classStart + 5] == ' ')
        {
            return classStart + 6;
        }

        auto structStart = prefixLength + offset;

        if (N > structStart + 6
            && signature[structStart + 0] == 's'
            && signature[structStart + 1] == 't'
            && signature[structStart + 2] == 'r'
            && signature[structStart + 3] == 'u'
            && signature[structStart + 4] == 'c'
            && signature[structStart + 5] == 't'
            && signature[structStart + 6] == ' ')
        {
            return structStart + 7;
        }

        return prefixLength + offset;
    }

    template <std::size_t N>
    constexpr std::size_t getPrefixLengthWithoutNamespace(const char (&signature)[N], bool isTemplate)
    {
        std::size_t namespacePrefix = 0;

        std::size_t templateStart = N - 1;
        if (isTemplate)
        {
            for (std::size_t i = Detail::prefixLength; i < N; ++i)
            {
                if (signature[i] == '<')
                {
                    templateStart = i;
                    break;
                }
            }
        }

        std::size_t colonCount = 0;
        for (std::size_t i = templateStart; i >= Detail::prefixLength; --i)
        {
            if (colonCount == 2)
            {
                namespacePrefix = i + 3;
                break;
            }

            if (signature[i] == ':')
            {
                colonCount++;
            }
            else
            {
                colonCount = 0;
            }
        }

        if (namespacePrefix > 0)
        {
            return namespacePrefix;
        }
        return getPrefixLength(signature);
    }

    template <std::size_t N>
    constexpr std::size_t getNamespacePrefixLength(const ConstexprString<N>& signature, bool isTemplate)
    {
        std::size_t namespacePrefix = 0;

        std::size_t templateStart = N - 1;
        if (isTemplate)
        {
            for (std::size_t i = 0; i < N; ++i)
            {
                if (signature[i] == '<')
                {
                    templateStart = i;
                    break;
                }
            }
        }

        std::size_t colonCount = 0;
        for (std::size_t i = templateStart; i > 0; --i)
        {
            if (colonCount == 2)
            {
                namespacePrefix = i + 3;
                break;
            }

            if (signature[i] == ':')
            {
                colonCount++;
            }
            else
            {
                colonCount = 0;
            }
        }

        return namespacePrefix;
    }

    template <typename T>
    constexpr std::size_t getTemplateSpacesCount()
    {
        if constexpr (TemplateArgumentCount<T>::Arity == 0)
        {
            return 0u;
        }
        return TemplateArgumentCount<T>::Arity - 1;
    }

    template <std::size_t N>
    constexpr auto adjustTemplateSpaces(ConstexprString<N>&& typeName)
    {
        std::size_t start = 0;
        std::size_t startComma = 0;
        for (std::size_t i = 0; i < N; ++i)
        {
            if (typeName[i] == '<')
            {
                start = i;
            }

            if (i > start && typeName[i] == ',')
            {
                startComma = i;
                break;
            }
        }

        if (startComma == 0)
        {
            return typeName;
        }

        // ReSharper disable once CppInitializedValueIsAlwaysRewritten
        char nextChar{};
        std::size_t nextComma = 0;
        while (startComma != nextComma)
        {
            nextComma = startComma;

            nextChar = typeName[startComma + 1];
            typeName[startComma + 1] = ' ';

            for (std::size_t i = startComma + 2; i < N; i++)
            {
                char temp = typeName[i];
                typeName[i] = nextChar;
                nextChar = temp;

                if (temp == ',' && nextComma == startComma)
                {
                    startComma = i + 1;
                }
            }
        }

        return typeName;
    }

    template <typename T>
    constexpr auto nameOfQualifiedType()
    {
        using namespace Detail;

        constexpr std::size_t offset = 13; // strlen("QualifiedType")
#if defined(__FUNCSIG__)
        constexpr auto removePrefix = getPrefixLength(__FUNCSIG__, offset);
        return constexprStringFromSubstring<sizeof(__FUNCSIG__), removePrefix, suffixLength>(__FUNCSIG__);
#else
        constexpr auto removePrefix = prefixLength + offset;
        return constexprStringFromSubstring<sizeof(__PRETTY_FUNCTION__), removePrefix, suffixLength>(__PRETTY_FUNCTION__);
#endif
    }

    template <typename T>
    constexpr auto nameOfQualifiedFundamental(int) -> decltype(nameOf<T>())
    {
        return nameOf<T>();
    }

    template <typename T>
    constexpr auto nameOfQualifiedFundamental(double)
    {
        return nameOfQualifiedType<T>();
    }

    template <typename T, typename std::enable_if<!std::is_fundamental<T>::value, int>::type = 0>
    constexpr auto nameOfQualified()
    {
        return nameOfQualifiedType<T>();
    }

    template <typename T, typename std::enable_if<std::is_fundamental<T>::value, int>::type = 0>
    constexpr auto nameOfQualified()
    {
        return nameOfQualifiedFundamental<T>(0);
    }

#if defined(__FUNCSIG__)
    template <typename T>
    constexpr auto nameOf()
    {
        using namespace Detail;

        constexpr auto removePrefix = getPrefixLengthWithoutNamespace(__FUNCSIG__, IsTemplate<T>::value);
        return constexprStringFromSubstring<sizeof(__FUNCSIG__), removePrefix, suffixLength>(__FUNCSIG__);
    }
#else

    template <typename T>
    constexpr auto nameOf()
    {
        using namespace Detail;
        constexpr auto qualifiedType = nameOfQualified<T>();
        constexpr auto prefix = getNamespacePrefixLength(qualifiedType, IsTemplate<T>::value);

        if constexpr (prefix > 0)
        {
            return qualifiedType.template substring<prefix>();
        }
        else
        {
            return qualifiedType;
        }
    }
#endif

    template <typename T>
    constexpr auto typeNameFull()
    {
        using namespace Detail;

    #if defined(__FUNCSIG__)
        return constexprString(__FUNCSIG__);
    #else
        return constexprString(__PRETTY_FUNCTION__);
    #endif
    }

    template <std::size_t InterfaceNameSize, std::size_t NamespaceNameSize, typename... T>
    constexpr std::size_t templatedInterfaceSize()
    {
        constexpr auto namespaceSeparatorSize = 1;
        constexpr auto resultSize = InterfaceNameSize
                                    + NamespaceNameSize
                                    + namespaceSeparatorSize
                                    + Detail::concatWithSize<decltype(nameOf<T>())...>();

        if constexpr (sizeof...(T) == 0)
        {
            return resultSize;
        }
        else
        {
            return resultSize + 1;
        }
    }

    template <std::size_t N, typename... T>
    constexpr auto concatTypesWith(char c, const ConstexprString<N>& string) -> ConstexprString<templatedInterfaceSize<N - 1, 0, T...>()>
    {
        using namespace Detail;

        constexpr auto TypeCount = sizeof...(T);
        if constexpr (TypeCount == 0)
        {
            return string;
        }
        else
        {
            constexpr auto resultSize = templatedInterfaceSize<N - 1, 0, T...>();

            ConstexprString<resultSize> str{};
            for (std::size_t i = 0; i < N; ++i)
            {
                str[i] = string[i];
            }

            if (sizeof...(T) > 0)
            {
                str[N] = '<';
            }

            concatArrayWith(str, c, N + 1, nameOf<T>()...);

            if (sizeof...(T) > 0)
            {
                str[decltype(str)::Size() - 1] = '>';
            }
            return str;
        }
    }

    template <typename T>
    static constexpr std::size_t getTemplateStart()
    {
        constexpr auto name = nameOf<T>();
        constexpr auto N = decltype(name)::Size();

        for (std::size_t i = 0; i < N; ++i)
        {
            if (name[i] == '<')
            {
                return i;
            }
        }

        return N;
    }

    template <typename T>
    static int constexpr getTemplateStartQualified()
    {
        constexpr auto name = nameOfQualified<T>();
        constexpr auto N = decltype(name)::Size();

        int index = -1;
        for (std::size_t i = 0; i < N; ++i)
        {
            if (name[i] == '<')
            {
                index = i;
                break;
            }
        }
        return index;
    }

    template <typename T, std::size_t N>
    constexpr std::size_t addTemplateTypeName(ConstexprString<N>& accumulator, std::size_t start, bool last)
    {
        constexpr auto argName = typeName<T>();
        constexpr auto size = decltype(argName)::Size();

        std::size_t end = start + size;
        for (std::size_t i = start, j = 0; i < end; ++i, ++j)
        {
            accumulator[i] = argName[j];
        }

        if (!last)
        {
            accumulator[end] = ',';
            accumulator[end + 1] = ' ';

            return end + 2;
        }
        else
        {
            return end;
        }
    }

    template <std::size_t N, typename THead, typename... TTail>
    constexpr void addTemplateTypeNames(ConstexprString<N>& accumulator, std::size_t start)
    {
        start = addTemplateTypeName<THead>(accumulator, start, sizeof...(TTail) == 0);

        if constexpr (sizeof...(TTail) != 0)
        {
            addTemplateTypeNames<N, TTail...>(accumulator, start);
        }
    }

    template <typename T, std::size_t N>
    constexpr std::size_t addTemplateTypeNameQualified(ConstexprString<N>& accumulator, std::size_t start, bool last)
    {
        constexpr auto argName = typeNameQualified<T>();
        constexpr auto size = decltype(argName)::Size();

        std::size_t end = start + size;
        for (std::size_t i = start, j = 0; i < end; ++i, ++j)
        {
            accumulator[i] = argName[j];
        }

        if (!last)
        {
            accumulator[end] = ',';
            accumulator[end + 1] = ' ';

            return end + 2;
        }
        else
        {
            return end;
        }
    }

    template <std::size_t N, typename THead, typename... TTail>
    constexpr void addTemplateTypeNamesQualified(ConstexprString<N>& accumulator, std::size_t start)
    {
        start = addTemplateTypeNameQualified<THead>(accumulator, start, sizeof...(TTail) == 0);

        if constexpr (sizeof...(TTail) != 0)
        {
            addTemplateTypeNamesQualified<N, TTail...>(accumulator, start);
        }
    }

    template <typename... TArgs>
    struct AddTemplateArgumentsQualified
    {
        static constexpr auto GetAdjustedSize()
        {
            return adjustedConcatWithSize<decltype(typeNameQualified<TArgs>())...>();
        }

        template <std::size_t N>
        static constexpr void AddTemplateTypeNames(ConstexprString<N>& accumulator, std::size_t start)
        {
            addTemplateTypeNamesQualified<N, TArgs...>(accumulator, start);
        }
    };

    template <typename... TArgs>
    struct AddTemplateArguments
    {
        static constexpr auto GetAdjustedSize()
        {
            return adjustedConcatWithSize<decltype(typeName<TArgs>())...>();
        }

        template <std::size_t N>
        static constexpr void AddTemplateTypeNames(ConstexprString<N>& accumulator, std::size_t start)
        {
            addTemplateTypeNames<N, TArgs...>(accumulator, start);
        }
    };
}
