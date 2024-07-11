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
#include <coretypes/common.h>
#include <coretypes/arguments.h>
#include <type_traits>

BEGIN_NAMESPACE_OPENDAQ

template <typename T>
struct FunctionTraits : public FunctionTraits<decltype(&T::operator())>
{
};

// For free functions
template <typename TReturnType, typename... TArgs>
struct FunctionTraits<TReturnType(TArgs...)>
{
    using ResultType = TReturnType;
    using ArgTuple = Args<TArgs...>;
    static constexpr auto Arity = sizeof...(TArgs);
};

// For lambdas
template <typename TClassType, typename TReturnType, typename... TArgs>
struct FunctionTraits<TReturnType (TClassType::*)(TArgs...) const>
{
    using ResultType = TReturnType;
    using ArgTuple = Args<TArgs...>;
    static constexpr auto Arity = sizeof...(TArgs);
};

template <typename TClassType, typename TReturnType, typename... TArgs>
struct FunctionTraits<TReturnType (TClassType::*)(TArgs...)>
{
    using ResultType = TReturnType;
    using ArgTuple = Args<TArgs...>;
    static constexpr auto Arity = sizeof...(TArgs);
};

// Enable / disable method depending on if it returns ErrCode or not
template <typename TFunctor, bool IsTrue>
using ReturnsErrorCode = typename std::enable_if<std::is_same<typename FunctionTraits<TFunctor>::ResultType, ErrCode>::value == IsTrue>;

END_NAMESPACE_OPENDAQ
