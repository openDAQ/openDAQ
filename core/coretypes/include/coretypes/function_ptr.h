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
#pragma once
#include <coretypes/function_impl.h>
#include <coretypes/function.h>
#include <coretypes/objectptr.h>

BEGIN_NAMESPACE_OPENDAQ

class FunctionPtr;

template <>
struct InterfaceToSmartPtr<daq::IFunction>
{
    using SmartPtr = daq::FunctionPtr;
};

FunctionPtr CustomFunction(FuncCall func);
FunctionPtr Function(std::nullptr_t value);

template <typename TFunctor, typename std::enable_if<std::is_bind_expression_v<TFunctor>>::type* = nullptr>
FunctionPtr Function(TFunctor value);

template <typename TFunctor, typename std::enable_if<!std::is_bind_expression_v<TFunctor>>::type* = nullptr>
FunctionPtr Function(TFunctor value);

template <typename TFunctor>
FunctionPtr Function(TFunctor* value);

/*!
 * @addtogroup types_function
 * @{
 */

/*!
 * @brief Holds a callback function.
 *
 * Represents a callable object. The openDAQ SDK uses this object when it needs to make
 * a call back to the client.
 *
 * Available factories:
 * @code
 * // Creates a new Function object.
 * template <typename TFunctor>
 * FunctionPtr Function(TFunctor value)
 * @endcode
 *
 * Example:
 * @code
 * auto funcObj = Function([](Int a) { return a + 1; } );
 * Int result = funcObj(2);
 * @endcode
 */
class FunctionPtr : public ObjectPtr<IFunction>
{
public:
    using ObjectPtr<IFunction>::ObjectPtr;
    using Delegate = ErrCode (*)(IBaseObject*, IBaseObject**);

    FunctionPtr() = default;

    FunctionPtr(std::nullptr_t null)
        : ObjectPtr<daq::IFunction>(null)
    {
    }

    FunctionPtr(IFunction*& raw)
        : ObjectPtr<IFunction>(raw)
    {
    }

    FunctionPtr(IFunction*&& obj)
        : ObjectPtr<IFunction>(std::forward<decltype(obj)>(obj))
    {
    }

    template <typename TFunctor>
    FunctionPtr(TFunctor call)
        : ObjectPtr<IFunction>(Function(std::forward<decltype(call)>(call)))
    {
    }

    FunctionPtr(Delegate call)
        : ObjectPtr<IFunction>(CustomFunction(call))
    {
    }

    FunctionPtr(const ObjectPtr<IFunction>& ptr)
        : ObjectPtr<IFunction>(ptr)
    {
    }

    FunctionPtr(const ObjectPtr<IFunction>&& ptr)
        : ObjectPtr<IFunction>(ptr)
    {
    }

    template <typename... Params>
    ObjectPtr<IBaseObject> operator()(Params... params)
    {
        return call(params...);
    }

    ObjectPtr<IBaseObject> operator()()
    {
        return call();
    }
};

/*!@}*/

inline FunctionPtr Function(std::nullptr_t value)
{
    FunctionPtr obj(FunctionWrapper_Create<std::nullptr_t>(value));
    return obj;
}

template <typename TFunctor, typename std::enable_if<!std::is_bind_expression_v<TFunctor>>::type*>
FunctionPtr Function(TFunctor value)
{
    static_assert(!std::is_same_v<typename FunctionTraits<TFunctor>::ResultType, void>, "Function must return a value.");

    FunctionPtr obj(FunctionWrapper_Create<TFunctor>(std::move(value)));
    return obj;
}

template <typename TFunctor, typename std::enable_if<std::is_bind_expression_v<TFunctor>>::type*>
FunctionPtr Function(TFunctor value)
{
    FunctionPtr obj(FunctionWrapper_Create<TFunctor>(std::move(value)));
    return obj;
}

template <typename TFunctor>
FunctionPtr Function(TFunctor* value)
{
    static_assert(!std::is_same_v<typename FunctionTraits<TFunctor>::ResultType, void>, "Function must return a value.");

    FunctionPtr obj(FunctionWrapper_Create<TFunctor>(value));
    return obj;
}

template <typename TReturn, typename... Params>
ErrCode wrapHandlerReturn(FunctionPtr handler, TReturn& output, Params... params)
{
    try
    {
        output = (handler)(params...);
        return OPENDAQ_SUCCESS;
    }
    catch (const DaqException& e)
    {
        return errorFromException(e);
    }
    catch (const std::exception& e)
    {
        return makeErrorInfo(OPENDAQ_ERR_GENERALERROR, e.what(), nullptr);
    }
    catch (...)
    {
        return OPENDAQ_ERR_GENERALERROR;
    }
}

END_NAMESPACE_OPENDAQ
