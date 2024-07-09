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

/**
 * @file    arguments.h
 * @author  Martin Kraner
 * @date    09/05/2019
 * @version 1.0
 *
 * @brief Argument type list
 *
 */
#pragma once
#include <cstddef>
#include <utility>
#include <type_traits>

namespace daq
{
    template <typename... TArgs>
    struct Args;

    namespace Details
    {
        struct EndTag
        {
        };

        /////////////////////////
        //      TypeAt
        /////////////////////////

        template <typename TArgs, std::size_t Index>
        struct TypeAt;

        template <typename TArgs>
        struct TypeAt<TArgs, 0>
        {
            using Result = typename TArgs::Head;
        };

        template <typename TArgs, std::size_t Index>
        struct TypeAt
        {
            using Result = typename TypeAt<typename TArgs::Tail, Index - 1>::Result;
        };

        //////////////////////////////////
        //      AreArgumentsEqual
        //////////////////////////////////

        template <typename TArg1, typename TArg2>
        struct AreArgumentsEqual;

        template <typename TArg>
        struct AreArgumentsEqual<TArg, EndTag>
        {
            static constexpr bool Value = std::is_same_v<TArg, EndTag>;
        };

        template <typename TArg>
        struct AreArgumentsEqual<EndTag, TArg>
        {
            static constexpr bool Value = std::is_same_v<TArg, EndTag>;
        };

        template <>
        struct AreArgumentsEqual<EndTag, EndTag>
        {
            static constexpr bool Value = true;
        };

        template <typename TArg1, typename TArg2>
        struct AreArgumentsEqual
        {
            static constexpr bool Value = std::is_same_v<typename TArg1::Head, typename TArg2::Head>
                                          && AreArgumentsEqual<typename TArg1::Tail, typename TArg2::Tail>::Value;
        };

        //////////////////////////////////
        //      HasArgumentWithType
        //////////////////////////////////

        template <typename TArgs, typename T>
        struct HasArgumentWithType;

        template <typename T>
        struct HasArgumentWithType<EndTag, T>
        {
            static constexpr bool Value = false;
        };

        template <typename TArgs, typename T>
        struct HasArgumentWithType
        {
            static constexpr bool Value = std::is_same_v<typename TArgs::Head, T> || HasArgumentWithType<typename TArgs::Tail, T>::Value;
        };

        //////////////////////////////////
        //      IndexOfFirst
        //////////////////////////////////

        template <typename TArgs, typename T, int IndexOf = 0>
        struct IndexOfFirst;

        template <typename T, int IndexOf>
        struct IndexOfFirst<EndTag, T, IndexOf>
        {
            static constexpr int Index = -1;
        };

        template <typename TArgs, typename T, int IndexOf>
        struct IndexOfFirst
        {
            static constexpr int Index = !std::is_same_v<typename TArgs::Head, T>
                ? IndexOfFirst<typename TArgs::Tail, T, IndexOf + 1>::Index
                : IndexOf;
        };

        /////////////////////////
        //      RemoveAllOf
        /////////////////////////

        template <typename... TArgs>
        struct ArgumentExpander
        {
            using Result = daq::Args<TArgs...>;
        };

        template <typename T, typename TArg, typename... TArgs>
        struct RemoveAllOfImpl;

        template <typename T, typename... TArgs>
        struct RemoveAllOfImpl<T, EndTag, TArgs...>
        {
            using Folded = RemoveAllOfImpl<T, EndTag, TArgs...>;

            // GCC doesn't like Args<TArgs...> or ArgumentExpander<TArgs...>::Result
            using Result = ArgumentExpander<TArgs...>;
        };

        template <typename T, typename TArg, typename... TArgs>
        struct RemoveAllOfImpl
        {
            using Head = typename TArg::Head;
            using Tail = typename TArg::Tail;

            using Folded = std::conditional_t<std::is_same_v<T, Head>,
                typename RemoveAllOfImpl<T, Tail, TArgs...>::Folded,
                typename RemoveAllOfImpl<T, Tail, TArgs..., Head>::Folded
            >;
        };

        template <typename T, typename TArgs>
        struct RemoveAllOf;

        template <typename T>
        struct RemoveAllOf<T, Args<>>
        {
            using Folded = daq::Args<>;
        };

        template <typename T, typename TArgs>
        struct RemoveAllOf
        {
            using Head = typename TArgs::Head;
            using Tail = typename TArgs::Tail;

            using Folded = typename std::conditional_t<std::is_same_v<T, Head>,
                typename RemoveAllOfImpl<T, Tail>::Folded,
                typename RemoveAllOfImpl<T, Tail, Head>::Folded
            >::Result::Result;
        };

        /////////////////////////
        //      AddType
        /////////////////////////

        template <typename T, typename TArgs, typename... TRest>
        struct AddTypeImpl
        {
            using Head = typename TArgs::Head;
            using Tail = typename TArgs::Tail;

            using Args = typename AddTypeImpl<T, Tail, TRest..., Head>::Args;
        };

        template <typename T, typename... TRest>
        struct AddTypeImpl<T, EndTag, TRest...>
        {
            using Args = daq::Args<TRest..., T>;
        };

        template <typename T, typename TArgs>
        struct AddType;

        template <typename T>
        struct AddType<T, Args<>>
        {
            using Args = daq::Args<T>;
        };

        template <typename T, typename TArgs>
        struct AddType
        {
            using Tail = typename TArgs::Tail;
            using Head = typename TArgs::Head;

            using Args = typename AddTypeImpl<T, Tail, Head>::Args;
        };

        /////////////////////////
        //      AddTypes
        /////////////////////////

        template <typename TArgs, typename THead, typename... TTail>
        struct AddTypesImpl;

        template <typename TArgs, typename THead>
        struct AddTypesImpl<TArgs, THead>
        {
            using Args = typename AddType<THead, TArgs>::Args;
        };

        template <typename TArgs, typename THead, typename... TTail>
        struct AddTypesImpl
        {
            using TypeAdded = typename AddType<THead, TArgs>::Args;
            using Args = typename AddTypesImpl<TypeAdded, TTail...>::Args;
        };

        template <typename TArgs, typename... TTypes>
        struct AddTypes
        {
            using Args = typename AddTypesImpl<TArgs, TTypes...>::Args;
        };

        template <typename TArgs>
        struct AddTypes<TArgs>
        {
            using Args = TArgs;
        };

        template <typename... TTypes>
        struct AddTypes<EndTag, TTypes...>
        {
            using Args = daq::Args<TTypes...>;
        };

        /////////////////////////
        //      PrependTypes
        /////////////////////////

        template <typename T, typename TArgs, typename... TRest>
        struct PrependTypeImpl
        {
            using Head = typename TArgs::Head;
            using Tail = typename TArgs::Tail;

            using Args = typename PrependTypeImpl<T, Tail, TRest..., Head>::Args;
        };

        template <typename T, typename... TRest>
        struct PrependTypeImpl<T, EndTag, TRest...>
        {
            using Args = daq::Args<T, TRest...>;
        };

        template <typename T, typename TArgs>
        struct PrependType;

        template <typename T>
        struct PrependType<T, Args<>>
        {
            using Args = daq::Args<T>;
        };

        template <typename T, typename TArgs>
        struct PrependType
        {
            using Tail = typename TArgs::Tail;
            using Head = typename TArgs::Head;

            using Args = typename PrependTypeImpl<T, Tail, Head>::Args;
        };

        /////////////////////////
        //      ReverseTypes
        /////////////////////////

        template <typename TArgs, typename... TRest>
        struct ReverseTypesImpl;

        template <typename TArgs, typename... TRest>
        struct ReverseTypesImpl
        {
            using Head = typename TArgs::Head;
            using Tail = typename TArgs::Tail;

            using Args = typename ReverseTypesImpl<Tail, Head, TRest...>::Args;
        };

        template <typename... TRest>
        struct ReverseTypesImpl<EndTag, TRest...>
        {
            using Args = daq::Args<TRest...>;
        };

        template <typename TArgs>
        struct ReverseTypes;

        template <>
        struct ReverseTypes<Args<>>
        {
            using Args = daq::Args<>;
        };

        template <typename TArgs>
        struct ReverseTypes
        {
            using Head = typename TArgs::Head;
            using Tail = typename TArgs::Tail;

            using Args = typename ReverseTypesImpl<Tail, Head>::Args;
        };

        template <>
        struct ReverseTypes<EndTag>
        {
            using Args = daq::Args<>;
        };

        /////////////////////////
        //      PrependTypes
        /////////////////////////

        template <typename TArgs, typename TPrependArgs>
        struct PrependTypesImpl
        {
            using Head = typename TPrependArgs::Head;
            using Tail = typename TPrependArgs::Tail;

            using TypePrepended = typename PrependType<Head, TArgs>::Args;
            using Args = typename PrependTypesImpl<TypePrepended, Tail>::Args;
        };

        template <typename TArgs>
        struct PrependTypesImpl<TArgs, EndTag>
        {
            using Args = TArgs;
        };

        template <typename TArgs, typename... TTypes>
        struct PrependTypes
        {
            using Reversed = typename ReverseTypes<daq::Args<TTypes...>>::Args;

            using Args = typename PrependTypesImpl<TArgs, Reversed>::Args;
        };

        template <typename... TTypes>
        struct PrependTypes<EndTag, TTypes...>
        {
            using Args = daq::Args<TTypes...>;
        };

        /////////////////////////
        //      RemoveOneOf
        /////////////////////////

        template <typename T, typename TArg, typename... TArgs>
        struct RemoveOneOfImpl;

        template <typename T, typename... TArgs>
        struct RemoveOneOfImpl<T, EndTag, TArgs...>
        {
            using Folded = daq::Args<TArgs...>;
        };

        template <typename T, typename TArg, typename... TArgs>
        struct RemoveOneOfImpl
        {
            using Head = typename TArg::Head;
            using Tail = typename TArg::Tail;

            using Folded = std::conditional_t<std::is_same_v<T, Head>,
                typename PrependTypes<Tail, TArgs...>::Args,
                typename RemoveOneOfImpl<T, Tail, TArgs..., Head>::Folded
            >;
        };

        template <typename T, typename TArgs>
        struct RemoveOneOf
        {
            using Head = typename TArgs::Head;
            using Tail = typename TArgs::Tail;

            using Folded = typename std::conditional_t<std::is_same_v<T, Head>,
                Tail,
                typename RemoveOneOfImpl<T, Tail, Head>::Folded
            >;
        };

        /////////////////////////
        //      UniqueTypes
        /////////////////////////

        template <typename TResult>
        struct UniqueTypes;

        template <typename TArgs, typename TResult>
        struct UniqueTypesImpl;

        template <typename TArgs, typename TResult>
        struct UniqueTypesImpl
        {
            using Head = typename TArgs::Head;
            using Tail = typename TArgs::Tail;

            using HasType = HasArgumentWithType<Tail, Head>;
            using OmmitHeadType = typename RemoveOneOf<Head, TResult>::Folded;

            using OmmitTypeImpl = UniqueTypesImpl<Tail, OmmitHeadType>;
            using IncludeType = UniqueTypesImpl<Tail, TResult>;

            using Unique = std::conditional_t<HasType::Value, OmmitTypeImpl, IncludeType>;

            using Args = typename Unique::Args;
        };

        template <typename TResult>
        struct UniqueTypesImpl<EndTag, TResult>
        {
            using Args = TResult;
        };

        template <typename TResult>
        struct UniqueTypes;

        template <>
        struct UniqueTypes<Args<>>
        {
            using Args = daq::Args<>;
        };

        template <typename TResult>
        struct UniqueTypes
        {
            using Unordered = typename UniqueTypesImpl<TResult, typename ReverseTypes<TResult>::Args>::Args;

            using Args = typename ReverseTypes<Unordered>::Args;
        };

        /////////////////////////
        //      ConcatArgs
        /////////////////////////

        template <typename TArgs1, typename TType, typename TArgs2>
        struct ConcatTwoArgsImpl
        {
            using Head = typename TArgs2::Head;
            using Tail = typename TArgs2::Tail;

            using Args = typename AddType<TType, typename ConcatTwoArgsImpl<TArgs1, Head, Tail>::Args>::Args;
        };

        template <typename TArgs1, typename TType>
        struct ConcatTwoArgsImpl<TArgs1, TType, Details::EndTag>
        {
            using Args = typename AddType<TType, TArgs1>::Args;
        };

        template <typename TArgs1, typename TArgs2>
        struct ConcatTwoArgs;

        template <typename TArgs1>
        struct ConcatTwoArgs<TArgs1, Args<>>
        {
            using Args = TArgs1;
        };

        template <typename TArgs2>
        struct ConcatTwoArgs<Args<>, TArgs2>
        {
            using Args = TArgs2;
        };

        template <>
        struct ConcatTwoArgs<Args<>, Args<>>
        {
            using Args = daq::Args<>;
        };

        template <typename TArgs1, typename TArgs2>
        struct ConcatTwoArgs
        {
            using Reversed = typename ReverseTypes<TArgs2>::Args;
            using Head = typename Reversed::Head;
            using Tail = typename Reversed::Tail;

            using Args = typename ConcatTwoArgsImpl<TArgs1, Head, Tail>::Args;
        };

        template <typename TArg, typename... TArgs>
        struct ConcatArgs;

        template <typename TArgs1, typename TArgs2, typename... TArgs>
        struct ConcatArgsImpl
        {
            using Args = typename ConcatArgs<typename ConcatTwoArgs<TArgs1, TArgs2>::Args, TArgs...>::Args;
        };

        template <typename TArgs>
        struct ConcatArgs<TArgs>
        {
            using Args = TArgs;
        };

        template <typename TArg, typename... TArgs>
        struct ConcatArgs
        {
            using Args = typename ConcatArgsImpl<TArg, TArgs...>::Args;
        };

        /////////////////////////
        //      Flatten
        /////////////////////////

        template <typename TArgs>
        struct Flatten;

        template <>
        struct Flatten<Details::EndTag>
        {
            using Args = daq::Args<>;
        };

        template <typename TArgs>
        struct Flatten
        {
            using Args = typename ConcatArgs<typename TArgs::Head, typename Flatten<typename TArgs::Tail>::Args>::Args;
        };

        /////////////////////////
        //      WrapTypes
        /////////////////////////

        template <template <typename...> class T, typename TType, typename TArgs>
        struct WrapTypesWithImpl;

        template <template <typename...> class T, typename TType, typename TArgs>
        struct WrapTypesWithImpl
        {
            using Head = typename TArgs::Head;
            using Tail = typename TArgs::Tail;
            using WrappedType = T<TType>;

            using Wrapped = typename AddType<WrappedType, typename WrapTypesWithImpl<T, Head, Tail>::Wrapped>::Args;
        };

        template <template <typename...> class T, typename TType>
        struct WrapTypesWithImpl<T, TType, Details::EndTag>
        {
            using WrappedType = T<TType>;

            using Wrapped = daq::Args<WrappedType>;
        };

        template <template <typename...> class T, typename TArgs>
        struct WrapTypesWith;

        template <template <typename...> class T>
        struct WrapTypesWith<T, Args<>>
        {
            using Wrapped = daq::Args<>;
        };

        template <template <typename...> class T, typename TArgs>
        struct WrapTypesWith
        {
            using Head = typename TArgs::Head;
            using Tail = typename TArgs::Tail;

            using Wrapped = typename WrapTypesWithImpl<T, Head, Tail>::Wrapped;
        };

        /////////////////////////
        //      Fold
        /////////////////////////

        template <typename TArg, typename... TArgs>
        struct FoldAndCallImpl;

        template <typename... TArgs>
        struct FoldAndCallImpl<EndTag, TArgs...>
        {
            using Folded = FoldAndCallImpl<EndTag, TArgs...>;

            template <template <typename...> typename T>
            using Forward = T<TArgs...>;

            template <typename TReturn, typename... TFunctorArgs>
            static constexpr decltype(auto) Call(TReturn (*fun)(TFunctorArgs&&...), TFunctorArgs&&... args)
            {
                return fun(std::forward<TFunctorArgs>(args)...);
            }

            template <typename TReturn, typename TType, typename... TFunctorArgs>
            static constexpr decltype(auto) CallMember(TReturn (std::remove_reference_t<TType>::*fun)(TFunctorArgs&&...),
                                                       TType&& obj,
                                                       TFunctorArgs&&... args)
            {
                return (obj.*fun)(std::forward<TFunctorArgs>(args)...);
            }

            template <typename TFunctor, typename... TFunctorArgs>
            static constexpr decltype(auto) Call(TFunctor&& fun, TFunctorArgs&&... args)
            {
                return fun.template operator()<TArgs...>(std::forward<TFunctorArgs>(args)...);
            }
        };

        template <typename TArg, typename... TArgs>
        struct FoldAndCallImpl
        {
            using Head = typename TArg::Head;
            using Tail = typename TArg::Tail;

            using Folded = typename FoldAndCallImpl<Tail, Head, TArgs...>::Folded;
        };

        template <typename TArgs, template <typename...> typename T>
        struct Fold
        {
            using Folded = typename FoldAndCallImpl<typename ReverseTypes<TArgs>::Args>::Folded::template Forward<T>;
        };

        template <typename TArgs>
        struct FoldAndCall
        {
            using Folded = typename FoldAndCallImpl<typename TArgs::Tail, typename TArgs::Head>::Folded;
        };
    }

    namespace Meta
    {
        template <typename TArgs, std::size_t Index>
        using TypeAt = Details::TypeAt<TArgs, Index>;

        template <typename TArgs1, typename TArgs2>
        using AreArgumentsEqual = Details::AreArgumentsEqual<TArgs1, TArgs2>;

        template <typename TArgs, typename T>
        using HasArgumentWithType = Details::HasArgumentWithType<TArgs, T>;

        template <typename T, typename TArgs>
        using AddType = Details::AddType<T, TArgs>;

        template <typename TArgs, typename... TTypes>
        using AddTypes = Details::AddTypes<TArgs, TTypes...>;

        template <typename T, typename TArgs>
        using PrependType = Details::PrependType<T, TArgs>;

        template <typename TArgs, typename... TTypes>
        using PrependTypes = Details::PrependTypes<TArgs, TTypes...>;

        template <typename T, typename TArgs>
        using RemoveOneOf = Details::RemoveOneOf<T, TArgs>;

        template <typename T, typename TArgs>
        using RemoveAllOf = Details::RemoveAllOf<T, TArgs>;

        template <typename TArgs>
        using UniqueTypes = Details::UniqueTypes<TArgs>;

        template <typename TArgs>
        using ReverseTypes = Details::ReverseTypes<TArgs>;

        template <typename TArg, typename... TArgs>
        using ConcatArgs = Details::ConcatArgs<TArg, TArgs...>;

        template <template <typename...> class T, typename TArgs>
        using WrapTypesWith = Details::WrapTypesWith<T, TArgs>;

        template <typename TArgs, template <typename...> typename T>
        using FoldType = Details::Fold<TArgs, T>;

        template <typename TArgs, typename T>
        using IndexOf = Details::IndexOfFirst<TArgs, T>;

        template <typename TArgs>
        using Flatten = typename Details::Flatten<TArgs>;

        template <typename T>
        struct TemplateArguments
        {
            using Arguments = daq::Args<>;
        };

        template <template <typename...> typename T, typename... P>
        struct TemplateArguments<T<P...>>
        {
            using Arguments = daq::Args<P...>;
        };
    }

    template <typename THead, typename... TTail>
    struct ArgsImpl
    {
        using Head = std::remove_reference_t<std::remove_cv_t<THead>>;
        using Tail = Args<TTail...>;

        static constexpr std::size_t Arity()
        {
            return 1 + sizeof...(TTail);
        }
    };

    template <typename THead>
    struct ArgsImpl<THead>
    {
        using Head = std::remove_reference_t<std::remove_cv_t<THead>>;
        using Tail = Details::EndTag;

        static constexpr std::size_t Arity()
        {
            return 1;
        }
    };

    template <>
    struct Args<>
    {
        using Head = Details::EndTag;
        using Tail = Details::EndTag;

        static constexpr std::size_t Arity()
        {
            return 0;
        }
    };

    template <typename... TArgs>
    struct Args : ArgsImpl<TArgs...>
    {
        static constexpr std::size_t Arity()
        {
            return sizeof...(TArgs);
        }
    };
}
