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
#include <coretypes/function.h>
#include <coretypes/intfs.h>
#include <coretypes/baseobject_factory.h>
#include <coretypes/listobject_factory.h>
#include <coretypes/function_traits.h>
#include <stdexcept>

BEGIN_NAMESPACE_OPENDAQ

// Workaround for std::bind
static constexpr std::size_t FuncObjectNativeArgs = (std::numeric_limits<std::size_t>::max)();

// -------------------------

template <class F, size_t... Is>
auto callMultipleParams(const F& f, const ListPtr<IBaseObject>& list, std::index_sequence<Is...>)
{
    return f(list.getItemAt(Is)...);
}

// Must store function pointer differently than lambda/bind/function
template <typename TFunctor, typename Enable = void>
class Functor;

template <typename TFunctor>
class Functor<TFunctor, typename std::enable_if<!std::is_function<TFunctor>::value>::type>
{
public:
   explicit Functor(TFunctor functor)
       : functor(std::move(functor))
   {
   }

protected:
   TFunctor functor;
};

template <typename TFunctor>
class Functor<TFunctor, typename std::enable_if<std::is_function<TFunctor>::value>::type>
{
public:
   explicit Functor(TFunctor* functor)
       : functor(functor)
   {
   }

protected:
   TFunctor* functor;
};

template <typename TFunctor>
class FunctionBase : public ImplementationOf<IFunction, ICoreType>
                   , public Functor<TFunctor>
{
public:
    explicit FunctionBase(TFunctor functor)
       : Functor<TFunctor>(std::move(functor))
   {
   }

   virtual ErrCode INTERFACE_FUNC call(IBaseObject* params, IBaseObject** result) override = 0; // NOLINT(modernize-use-override)

   ErrCode INTERFACE_FUNC getCoreType(CoreType* coreType) override
   {
       if (coreType == nullptr)
       {
           return makeErrorInfo(OPENDAQ_ERR_ARGUMENT_NULL, "Cannot return by a null pointer.");
       }

       *coreType = ctFunc;
       return OPENDAQ_SUCCESS;
   }

protected:
   template <typename F = typename std::remove_pointer<TFunctor>::type,
             typename ReturnsErrorCode<F, true>::type* = nullptr,
             typename... TArgs>
   ErrCode dispatchInternal(IBaseObject** result, TArgs&&... args)
   {
       return this->functor(std::forward<decltype(args)>(args)..., result);
   }

   template <typename F = typename std::remove_pointer<TFunctor>::type,
             typename ReturnsErrorCode<F, false>::type* = nullptr,
             typename... TArgs>
   ErrCode dispatchInternal(IBaseObject** result, TArgs&&... args)
   {
       try
       {
           BaseObjectPtr ret = this->functor(std::forward<decltype(args)>(args)...);
           *result = ret.detach();
       }
       catch (const DaqException& e)
       {
           return errorFromException(e);
       }
       catch (...)
       {
           return OPENDAQ_ERR_CALLFAILED;
       }

       return OPENDAQ_SUCCESS;
   }
};

template <typename TFunctor, std::size_t ArgCount = FunctionTraits<TFunctor>::Arity>
class FunctionImpl : public FunctionBase<TFunctor>
{
public:
    explicit FunctionImpl(TFunctor functor)
       : FunctionBase<TFunctor>(std::move(functor))
   {
   }

   ErrCode INTERFACE_FUNC call(IBaseObject* args, IBaseObject** result) override
   {
       if (result == nullptr)
           return OPENDAQ_ERR_ARGUMENT_NULL;

       if constexpr (std::is_same<typename FunctionTraits<TFunctor>::ResultType, ErrCode>::value)
       {
           return this->dispatchInternal(result, args);
       }
       else
       {
           try
           {
               BaseObjectPtr funcReturn = callMultipleParams(this->functor,
                                                             ListPtr<IBaseObject>(args),
                                                             std::make_index_sequence<ArgCount>{});
               *result = funcReturn.detach();
           }
           catch (const DaqException& e)
           {
               return errorFromException(e);
           }
           catch (...)
           {
               return OPENDAQ_ERR_CALLFAILED;
           }

           return OPENDAQ_SUCCESS;
       }
   }
};

template <typename TFunctor>
class FunctionImpl<TFunctor, 1> : public FunctionBase<TFunctor>
{
public:
    explicit FunctionImpl(TFunctor functor)
       : FunctionBase<TFunctor>(std::move(functor))
   {
   }

   ErrCode INTERFACE_FUNC call(IBaseObject* args, IBaseObject** result) override
   {
       if (result == nullptr)
           return OPENDAQ_ERR_ARGUMENT_NULL;

       auto arg = BaseObjectPtr::Borrow(args);
       return this->dispatchInternal(result, arg);
   }
};

template <typename TFunctor>
class FunctionImpl<TFunctor, 0> : public FunctionBase<TFunctor>
{
public:
    explicit FunctionImpl(TFunctor functor)
       : FunctionBase<TFunctor>(std::move(functor))
   {
   }

   ErrCode INTERFACE_FUNC call(IBaseObject* /*args*/, IBaseObject** result) override
   {
       if (result == nullptr)
           return OPENDAQ_ERR_ARGUMENT_NULL;

       return this->dispatchInternal(result);
   }
};

template <typename TFunctor>
class FunctionImpl<TFunctor, FuncObjectNativeArgs> : public FunctionBase<TFunctor>
{
public:
    explicit FunctionImpl(TFunctor func)
       : FunctionBase<TFunctor>(std::move(func))
   {
   }

   ErrCode INTERFACE_FUNC call([[maybe_unused]] IBaseObject* args,
                               [[maybe_unused]] IBaseObject** result) override
   {
       if (result == nullptr)
           return OPENDAQ_ERR_ARGUMENT_NULL;

       if constexpr (std::is_same_v<typename TFunctor::result_type, ErrCode>)
       {
           return this->functor(args, result);
       }
       else
       {
           return OPENDAQ_ERR_NOTIMPLEMENTED;
       }
   }
};

class FunctionNull : public ImplementationOf<ICoreType, IFunction>
{
public:
    ErrCode INTERFACE_FUNC call(IBaseObject* /*args*/, IBaseObject** /*result*/) override
    {
        return OPENDAQ_ERR_NOTASSIGNED;
    }

    ErrCode INTERFACE_FUNC getCoreType(CoreType* coreType) override
    {
        if (coreType == nullptr)
        {
            return makeErrorInfo(OPENDAQ_ERR_ARGUMENT_NULL, "Cannot return by a null pointer.");
        }

        *coreType = ctFunc;
        return OPENDAQ_SUCCESS;
    }
};

// Lambda

template <typename TFunctor, typename std::enable_if<!std::is_bind_expression<TFunctor>::value>::type* = nullptr>
ErrCode createFunctionWrapper(IFunction** obj, [[maybe_unused]] TFunctor func)
{
   if (!obj)
       return OPENDAQ_ERR_ARGUMENT_NULL;

   try
   {
       if constexpr (std::is_same_v<TFunctor, std::nullptr_t>)
       {
           *obj = new FunctionNull();
       }
       else
       {
           *obj = new FunctionImpl<TFunctor>(std::move(func));
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
ErrCode createFunctionWrapper(IFunction** obj, TFunctor func)
{
   if (!obj)
       return OPENDAQ_ERR_ARGUMENT_NULL;

   try
   {
       *obj = new FunctionImpl<TFunctor, FuncObjectNativeArgs>(std::move(func));
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
ErrCode createFunctionWrapper(IFunction** obj, TFunctor* func)
{
   if (!obj)
       return OPENDAQ_ERR_ARGUMENT_NULL;

   try
   {
       *obj = new FunctionImpl<TFunctor>(func);
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
IFunction* FunctionWrapper_Create(TFunctor func)
{
   IFunction* obj;
   const ErrCode res = createFunctionWrapper<TFunctor>(&obj, std::move(func));
   checkErrorInfo(res);

   return obj;
}

// Function pointer

template <typename TFunctor>
IFunction* FuncObject_Create2(TFunctor* func)
{
   IFunction* obj;
   const ErrCode res = createFunctionWrapper<TFunctor>(&obj, func);
   checkErrorInfo(res);

   return obj;
}

END_NAMESPACE_OPENDAQ
