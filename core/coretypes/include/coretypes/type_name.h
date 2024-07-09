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
#include <cstdint>
#include <cstddef>
#include <coretypes/type_name_detail.h>
#include <coretypes/constexpr_utils.h>
#include <coretypes/constexpr_string.h>
#include <coretypes/arguments.h>

namespace daq
{
    template <typename T>
    constexpr auto typeName()
    {
        using namespace Detail;

        if constexpr (!IsTemplate<T>::value)
        {
            return nameOf<T>();
        }
        else
        {
            using TemplateTypes = typename Meta::FoldType<
                typename Meta::TemplateArguments<T>::Arguments,
                AddTemplateArguments
            >::Folded;

            constexpr auto nameSize = getTemplateStart<T>();
            constexpr auto argumentsSize = TemplateTypes::GetAdjustedSize();

            constexpr auto fullNameSize = nameSize        // Templated type name size
                                          + argumentsSize // template arguments with separators and spaces
                                          + 2;            // < > (start and end angle bracket)

            auto platformName = nameOf<T>();

            ConstexprString<fullNameSize> str;
            for(std::size_t i = 0; i < nameSize; ++i)
            {
                str[i] = platformName[i];
            }
            str[nameSize] = '<';
            str[fullNameSize - 1] = '>';

            TemplateTypes::AddTemplateTypeNames(str, nameSize + 1);
            return str;
        }
    }

    template <typename T>
    constexpr auto typeNameQualified()
    {
        using namespace Detail;

        if constexpr (!IsTemplate<T>::value)
        {
            return nameOfQualified<T>();
        }
        else
        {
            using TemplateTypes = typename Meta::FoldType<
                typename Meta::TemplateArguments<T>::Arguments,
                AddTemplateArgumentsQualified
            >::Folded;

            constexpr auto nameSize = getTemplateStartQualified<T>();
            constexpr auto argumentsSize = TemplateTypes::GetAdjustedSize();

            constexpr auto fullNameSize = nameSize        // Templated type name size
                                          + argumentsSize // template arguments with separators and spaces
                                          + 2;            // < > (start and end angle bracket)

            auto platformName = nameOfQualified<T>();

            ConstexprString<fullNameSize> str;
            for(std::size_t i = 0; i < nameSize; ++i)
            {
                str[i] = platformName[i];
            }
            str[nameSize] = '<';
            str[fullNameSize - 1] = '>';

            TemplateTypes::AddTemplateTypeNames(str, nameSize + 1);
            return str;
        }
    }

    template <typename... TArgs, std::size_t N>
    constexpr auto typeFactoryId(const char (&factoryPrefix)[N])
    {
        constexpr auto concatTypesSize = Detail::adjustedConcatWithSize<decltype(typeName<TArgs>())...>();
        constexpr auto nameSize = (N - 1)           // factoryPrefix without zero terminator
                                  + concatTypesSize // template arguments with separators and spaces
                                  + 2;              // start and end angle bracket < and >

        ConstexprString<nameSize + 1> str{};
        for (std::size_t i = 0; i < N - 1; ++i)
        {
            str[i] = factoryPrefix[i];
        }

        str[N - 1] = '<';
        str[nameSize - 1] = '>';

        Detail::addTemplateTypeNames<nameSize + 1, TArgs...>(str, N);
        return str;
    }


    template <typename TInterface>
    constexpr auto interfaceFactoryId()
    {
        // Remove the "I" in e.g. "IInterface"
        constexpr auto id = typeName<TInterface>().template substring<1>();

        // Zero terminate the string
        constexpr std::size_t size = decltype(id)::Size();
        return ConstexprString<size + 1>:: template InitializeWith<size>(id);
    }

    template <std::size_t N1, std::size_t N2, typename... T>
    constexpr auto interfaceGuidSource(const char (&interfaceName)[N1], const char (&namespaceName)[N2])
        -> ConstexprString<Detail::templatedInterfaceSize<N1, N2, T...>() - 2>
    {
        using namespace Detail;

        if constexpr (sizeof...(T) == 0)
        {
            return constexprString(interfaceName).concatWithLiteral('.', namespaceName);
        }
        else
        {
            constexpr auto namespaceSeparatorSize = 1;
            constexpr auto namespaceNameSize = N2 - 1;

            constexpr auto concatTypesSize = Detail::concatWithSize<decltype(nameOf<T>())...>();
            constexpr auto interfaceNameSize = N1 + concatTypesSize;

            constexpr auto resultSize = interfaceNameSize + namespaceSeparatorSize + namespaceNameSize;

            ConstexprString<resultSize> str{};
            for (std::size_t i = 0; i < N1; ++i)
            {
                str[i] = interfaceName[i];
            }

            str[N1 - 1] = '<';
            concatArrayWith(str, ',', N1, nameOf<T>()...);
            str[interfaceNameSize - 1] = '>';

            str[interfaceNameSize] = '.';

            for (std::size_t i = interfaceNameSize + 1, j = 0; i < resultSize; ++i, ++j)
            {
                str[i] = namespaceName[j];
            }

            return str;
        }
    }
}
