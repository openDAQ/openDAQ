/*
 * Copyright 2022-2024 Blueberry d.o.o.
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
#include <coretypes/procedure.h>
#include <stdexcept>
#include <coretypes/intfs.h>
#include <coretypes/listobject_factory.h>
#include <coretypes/function_traits.h>

BEGIN_NAMESPACE_OPENDAQ

// Workaround for std::bind
static constexpr std::size_t ProcObjectNativeArgs = (std::numeric_limits<std::size_t>::max)();

// -------------------------

template <class F, size_t... Is>
void dispatchMultipleParams(const F& f, const ListPtr<IBaseObject>& list, std::index_sequence<Is...>)
{
    f(list.getItemAt(Is)...);
}

// Must store function pointer differently than lambda/bind/function
template <typename TFunctor, typename Enable = void>
class ProcedureFunctor;

template <typename TFunctor>
class ProcedureFunctor<TFunctor, typename std::enable_if<!std::is_function<TFunctor>::value>::type>
{
public:
    explicit ProcedureFunctor(TFunctor functor)
        : functor(std::move(functor))
    {
    }

protected:
    TFunctor functor;
};

template <typename TFunctor>
class ProcedureFunctor<TFunctor, typename std::enable_if<std::is_function<TFunctor>::value>::type>
{
public:
    explicit ProcedureFunctor(TFunctor* functor)
        : functor(functor)
    {
    }

protected:
    TFunctor* functor;
};

template <typename TFunctor>
class ProcedureBase : public ImplementationOf<IProcedure, ICoreType>
                    , public ProcedureFunctor<TFunctor>
{
public:
    explicit ProcedureBase(TFunctor functor)
        : ProcedureFunctor<TFunctor>(std::move(functor))
    {
    }

    virtual ErrCode INTERFACE_FUNC dispatch(IBaseObject* args) override = 0; // NOLINT(modernize-use-override)

    ErrCode INTERFACE_FUNC getCoreType(CoreType* coreType) override
    {
        if (coreType == nullptr)
        {
            return makeErrorInfo(OPENDAQ_ERR_ARGUMENT_NULL, "Cannot return by a null pointer.");
        }

        *coreType = ctProc;
        return OPENDAQ_SUCCESS;
    }

protected:
    template <typename F = typename std::remove_pointer<TFunctor>::type,
              typename ReturnsErrorCode<F, true>::type* = nullptr,
              typename... TArgs>
    ErrCode dispatchInternal(TArgs&&... args)
    {
        return this->functor(std::forward<decltype(args)>(args)...);
    }

    template <typename F = typename std::remove_pointer<TFunctor>::type,
              typename ReturnsErrorCode<F, false>::type* = nullptr,
              typename... TArgs>
    ErrCode dispatchInternal(TArgs&&... args)
    {
        try
        {
            this->functor(std::forward<decltype(args)>(args)...);
        }
        catch (const DaqException& e)
        {
            return e.getErrCode();
        }
        catch (...)
        {
            return OPENDAQ_ERR_CALLFAILED;
        }

        return OPENDAQ_SUCCESS;
    }
};

template <typename TFunctor, std::size_t ArgCount = FunctionTraits<TFunctor>::Arity>
class ProcedureImpl : public ProcedureBase<TFunctor>
{
public:
    explicit ProcedureImpl(TFunctor functor)
        : ProcedureBase<TFunctor>(std::move(functor))
    {
    }

    ErrCode INTERFACE_FUNC dispatch(IBaseObject* args) override
    {
        try
        {
            dispatchMultipleParams(this->functor, ListPtr<IBaseObject>(args), std::make_index_sequence<ArgCount>{});
        }
        catch (const DaqException& e)
        {
            return e.getErrCode();
        }
        catch (...)
        {
            return OPENDAQ_ERR_CALLFAILED;
        }

        return OPENDAQ_SUCCESS;
    }
};

template <typename TFunctor>
class ProcedureImpl<TFunctor, 1> : public ProcedureBase<TFunctor>
{
public:
    explicit ProcedureImpl(TFunctor functor)
        : ProcedureBase<TFunctor>(std::move(functor))
    {
    }

    ErrCode INTERFACE_FUNC dispatch(IBaseObject* args) override
    {
        ObjectPtr<IBaseObject> arg = args;
        return this->dispatchInternal(arg);
    }
};

template <typename TFunctor>
class ProcedureImpl<TFunctor, 0> : public ProcedureBase<TFunctor>
{
public:
    explicit ProcedureImpl(TFunctor functor)
        : ProcedureBase<TFunctor>(std::move(functor))
    {
    }

    ErrCode INTERFACE_FUNC dispatch(IBaseObject* /*args*/) override
    {
        return this->dispatchInternal();
    }
};

template <typename TFunctor>
class ProcedureImpl<TFunctor, ProcObjectNativeArgs> : public ProcedureBase<TFunctor>
{
public:
    explicit ProcedureImpl(TFunctor func)
        : ProcedureBase<TFunctor>(std::move(func))
    {
    }

    ErrCode INTERFACE_FUNC dispatch(IBaseObject* args) override
    {
        return this->functor(args);
    }
};

class ProcedureNull : public ImplementationOf<IProcedure, ICoreType>
{
public:
    ErrCode INTERFACE_FUNC dispatch(IBaseObject* /*args*/) override
    {
        return OPENDAQ_ERR_NOTASSIGNED;
    }

    ErrCode INTERFACE_FUNC getCoreType(CoreType* coreType) override
    {
        if (coreType == nullptr)
        {
            return makeErrorInfo(OPENDAQ_ERR_ARGUMENT_NULL, "Cannot return by a null pointer.");
        }

        *coreType = ctProc;
        return OPENDAQ_SUCCESS;
    }
};

// Lambda

template <typename TFunctor, typename std::enable_if<!std::is_bind_expression<TFunctor>::value>::type* = nullptr>
ErrCode createProcedureWrapper(IProcedure** obj, [[maybe_unused]] TFunctor proc)
{
    if (!obj)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    try
    {
        if constexpr (std::is_same_v<TFunctor, std::nullptr_t>)
        {
            *obj = new ProcedureNull();
        }
        else
        {
            *obj = new ProcedureImpl<TFunctor>(std::move(proc));
        }
    }
    catch (const DaqException& e)
    {
        setErrorInfoWithSource(nullptr, e.what());
        return e.getErrCode();
    }
    catch (const std::bad_alloc&)
    {
        return OPENDAQ_ERR_NOMEMORY;
    }
    catch (const std::exception&)
    {
        return OPENDAQ_ERR_GENERALERROR;
    }

    (*obj)->addRef();

    return OPENDAQ_SUCCESS;
}

// Handle std::bind()

template <typename TFunctor, typename std::enable_if<std::is_bind_expression<TFunctor>::value>::type* = nullptr>
ErrCode createProcedureWrapper(IProcedure** obj, TFunctor proc)
{
    if (!obj)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    try
    {
        *obj = new ProcedureImpl<TFunctor, ProcObjectNativeArgs>(std::move(proc));
    }
    catch (const DaqException& e)
    {
        setErrorInfoWithSource(nullptr, e.what());
        return e.getErrCode();
    }
    catch (const std::bad_alloc&)
    {
        return OPENDAQ_ERR_NOMEMORY;
    }
    catch (const std::exception&)
    {
        return OPENDAQ_ERR_GENERALERROR;
    }

    (*obj)->addRef();

    return OPENDAQ_SUCCESS;
}

// Function pointer

template <typename TFunctor>
ErrCode createProcedureWrapper(IProcedure** obj, TFunctor* proc)
{
    if (!obj)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    try
    {
        *obj = new ProcedureImpl<TFunctor>(proc);
    }
    catch (const DaqException& e)
    {
        setErrorInfoWithSource(nullptr, e.what());
        return e.getErrCode();
    }
    catch (const std::bad_alloc&)
    {
        return OPENDAQ_ERR_NOMEMORY;
    }
    catch (const std::exception&)
    {
        return OPENDAQ_ERR_GENERALERROR;
    }

    (*obj)->addRef();

    return OPENDAQ_SUCCESS;
}

// Lambda / std::bind

template <typename TFunctor>
IProcedure* ProcedureWrapper_Create(TFunctor proc)
{
    IProcedure* obj;
    const ErrCode res = createProcedureWrapper<TFunctor>(&obj, std::move(proc));
    checkErrorInfo(res);

    return obj;
}

// Function pointer

template <typename TFunctor>
IProcedure* ProcedureWrapper_Create(TFunctor* proc)
{
    IProcedure* obj;
    const ErrCode res = createProcedureWrapper<TFunctor>(&obj, proc);
    checkErrorInfo(res);

    return obj;
}

END_NAMESPACE_OPENDAQ
