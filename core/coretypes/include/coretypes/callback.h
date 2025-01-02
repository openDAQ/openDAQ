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
#include <coretypes/common.h>
#include <coretypes/function_factory.h>
#include <coretypes/procedure_factory.h>

BEGIN_NAMESPACE_OPENDAQ

namespace Detail
{
    template <typename TFunctor>
    inline auto Callback(TFunctor functor)
    {
        using Traits = FunctionTraits<TFunctor>;
        using ResultType = typename Traits::ResultType;

        if constexpr (std::is_same_v<ResultType, void>)
        {
            return Procedure(std::forward<decltype(functor)>(functor));
        }
        else if constexpr (std::is_same_v<ResultType, ErrCode>)
        {
            if constexpr (Traits::Arity == 1)
            {
                return Procedure(std::forward<decltype(functor)>(functor));
            }
            else
            {
                return Function(std::forward<decltype(functor)>(functor));
            }
        }
        else
        {
            return Function(std::move(functor));
        }
    }
}

template <typename TFunctor>
inline auto Callback(TFunctor functor)
{
    return Detail::Callback(std::forward<decltype(functor)>(functor));
}

template <typename TFunctor>
inline auto Callback(TFunctor* functor)
{
    return Detail::Callback<TFunctor>(std::forward<decltype(functor)>(functor));
}

END_NAMESPACE_OPENDAQ
