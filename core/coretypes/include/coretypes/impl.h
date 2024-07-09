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
#include <coretypes/common.h>
#include <coretypes/errors.h>
#include <coretypes/ctutils.h>
#include <coretypes/intfs.h>
#include <coretypes/customalloc.h>
#include <coretypes/bb_exception.h>
#include <coretypes/factoryselectors.h>

BEGIN_NAMESPACE_OPENDAQ

template <typename T>
T* addRefIfNotNull(T* obj)
{
    if (obj != nullptr)
        obj->addRef();
    return obj;
}

template <typename T>
void releaseRefIfNotNull(T* obj)
{
    if (obj != nullptr)
        obj->releaseRef();
}

template <typename T>
void setRef(T*& obj, T* newObj)
{
    releaseRefIfNotNull(obj);
    obj = addRefIfNotNull(newObj);
}

template <class Interface, class Impl, class... Params>
ErrCode createObjectForwarding(Interface** intf, Params... params)
{
    if (!intf)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    Impl* impl;
    try
    {
        impl = new Impl(std::forward<Params>(params)...);
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

    ErrCode errCode;
    if (impl->getRefAdded())
        errCode = impl->borrowInterface(Interface::Id, reinterpret_cast<void**>(intf));
    else
        errCode = impl->queryInterface(Interface::Id, reinterpret_cast<void**>(intf));

    if (OPENDAQ_FAILED(errCode))
    {
        delete impl;
    }

    return errCode;
}

template <class Interface, class Impl, class... Params>
ErrCode createObject(Interface** intf, Params... params)
{
    if (!intf)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    Impl* impl;
    try
    {
        impl = new Impl(params...);
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
    catch (const std::exception& e)
    {
        return errorFromException(e);
    }

    ErrCode errCode;
    if (impl->getRefAdded())
        errCode = impl->borrowInterface(Interface::Id, reinterpret_cast<void**>(intf));
    else
        errCode = impl->queryInterface(Interface::Id, reinterpret_cast<void**>(intf));

    if (OPENDAQ_FAILED(errCode))
    {
        delete impl;
    }
    return errCode;
}

template <typename TInterface>
struct ObjectCreator;

template <typename TInterface>
struct ObjectCreator
{
    static_assert(daq::DependentFalse<TInterface>::value, "Must specialize to enable custom factory");

    template <typename... TArgs>
    static ErrCode Create(TInterface** obj, TArgs... args) noexcept
    {
        return OPENDAQ_ERR_NOTIMPLEMENTED;
    }
};

#define OPENDAQ_DEFINE_CLASS_INTERFACE_FACTORY(EXPIMP, obj, intf)   \
    extern "C"                                                 \
    ErrCode EXPIMP create##obj(##intf** obj)                   \
    {                                                          \
        if (!obj)                                              \
            return OPENDAQ_ERR_INVALIDPARAMETER;                    \
                                                               \
        ##intf* object;                                        \
        object = new (std::nothrow) obj##Impl();               \
        if (!object)                                           \
            return OPENDAQ_ERR_NOMEMORY;                            \
                                                               \
        object->addRef();                                      \
                                                               \
        *obj = object;                                         \
                                                               \
        return OPENDAQ_SUCCESS;                                     \
    }

template <class T, class ImplT>
static ErrCode GetSingletonObject(T** singletonStorage, T** resIntf)
{
    if (singletonStorage == nullptr || resIntf == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    if (*singletonStorage == nullptr)
    {
        *singletonStorage = new(std::nothrow) ImplT();
        if (*singletonStorage == nullptr)
            return OPENDAQ_ERR_NOMEMORY;
        (*singletonStorage)->addRef();  // this prevents destruction of singleton object
    }

    *resIntf = *singletonStorage;
    (*resIntf)->addRef();

    return OPENDAQ_SUCCESS;
}

template <class T>
struct GenericSingletonStruct
{
    T* intf;

    GenericSingletonStruct()
        : intf(nullptr)
    {
        // ensure proper creation order (Scott Meyers, Effective C++, 3rd edition, item 3)
        // heap must be created before singleton object, which means that heap will be destroyed after singleton object
        daqPrepareHeapAlloc();
    }

    ~GenericSingletonStruct()
    {
        releaseRefIfNotNull(intf);
    }

    void release()
    {
        releaseRefIfNotNull(intf);
        intf = nullptr;
    }
};

#define OPENDAQ_CHECK_INTERFACE(intf, ptr)         \
    if (!ptr)                                 \
        return OPENDAQ_ERR_INVALIDPARAMETER;       \
                                              \
    if (id == intf::Id)                       \
    {                                         \
        intf* obj = static_cast<intf*>(this); \
        obj->addRef();                        \
        *ptr = obj;                           \
        return OPENDAQ_SUCCESS;                    \
    }

#if !defined(BUILDING_STATIC_LIBRARY)

#define OPENDAQ_DEFINE_CLASS_SINGLETON_FACTORY(EXPIMP, obj)                                                   \
    static GenericSingletonStruct<I##obj> I##obj##Singleton;                                             \
                                                                                                         \
    extern "C"                                                                                           \
    ErrCode EXPIMP create##obj(I##obj** intf)                                                            \
    {                                                                                                    \
        return daq::GetSingletonObject<I##obj, obj##Impl>(&I##obj##Singleton.intf, intf); \
    }

#define OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC_OBJ(EXPIMP, obj, intf, createFunc, ...)           \
    extern "C"                                                                                                  \
    daq::ErrCode EXPIMP createFunc(intf** objTmp OPENDAQ_FACTORY_TYPES_AND_NAMES(__VA_ARGS__))                        \
    {                                                                                                           \
        return daq::createObject<intf, obj OPENDAQ_FACTORY_TYPES(__VA_ARGS__)>(objTmp OPENDAQ_FACTORY_NAMES(__VA_ARGS__)); \
    }

#define OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC_OBJ_TEMPLATE(EXPIMP, obj, intf, createFunc, ...)  \
    template class obj;                                                                                         \
                                                                                                                \
    extern "C"                                                                                                  \
    daq::ErrCode EXPIMP createFunc(intf ** objTmp OPENDAQ_FACTORY_TYPES_AND_NAMES(__VA_ARGS__))                       \
    {                                                                                                           \
        return daq::createObject<intf, obj OPENDAQ_FACTORY_TYPES(__VA_ARGS__)>(objTmp OPENDAQ_FACTORY_NAMES(__VA_ARGS__)); \
    }

#define OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC_OBJ_PAR(EXPIMP, obj, intf, createFunc, ...)                         \
    template class obj;                                                                                                           \
                                                                                                                                  \
    extern "C"                                                                                                                    \
    daq::ErrCode EXPIMP createFunc(OPENDAQ__STRIP_PAR intf ** objTmp OPENDAQ_FACTORY_TYPES_AND_NAMES_PAR(__VA_ARGS__))                       \
    {                                                                                                                             \
        return daq::createObject<OPENDAQ__STRIP_PAR intf, obj OPENDAQ_FACTORY_TYPES_PAR(__VA_ARGS__)>(objTmp OPENDAQ_FACTORY_NAMES(__VA_ARGS__)); \
    }

#define OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(EXPIMP, obj, intf, createFunc, ...)               \
    OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC_OBJ(EXPIMP, obj##Impl, intf, createFunc, __VA_ARGS__)

#define OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(EXPIMP, obj, intf, ...) \
    OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(EXPIMP, obj, intf, create##obj, __VA_ARGS__)

#define OPENDAQ_DEFINE_CLASS_FACTORY(EXPIMP, obj, ...) OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(EXPIMP, obj, I##obj, __VA_ARGS__)

#define OPENDAQ__DEFINE_CLASS_FACTORY_WITH_INTERFACE_TEMPLATE_T_SINGLE(Params, FuncParams, Type) \
    OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC_OBJ_TEMPLATE(                     \
        OPENDAQ__CALL_MACRO(EXTRACT_ITEM_1, OPENDAQ__STRIP_PAR Params),                               \
        OPENDAQ__CONCATENATE(OPENDAQ_GET_FUNC_IMPLOBJ(Params), Impl) <Type>,                          \
        OPENDAQ__BUILD_TYPE(OPENDAQ_GET_FUNC_INTF(Params), Type, Unused),                             \
        OPENDAQ__TEMPLATED_CREATE_FUNC(OPENDAQ_GET_FUNC_IMPLOBJ(Params), FACTORY_TYPE_NAME(Type)),    \
        OPENDAQ__BUILD_ARGS(Type, OPENDAQ__STRIP_PAR FuncParams))

#define OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_TEMPLATE_T(Params, Types, FuncParams)                             \
    FOR_EACH_1P(OPENDAQ__DEFINE_CLASS_FACTORY_WITH_INTERFACE_TEMPLATE_T_SINGLE, Params, (FuncParams), Types)

#define OPENDAQ__DEFINE_CLASS_FACTORY_WITH_INTERFACE_TEMPLATE_T_U_SINGLE(Params, FuncParams, Unused, Type1, Type2)    \
    OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC_OBJ_PAR(                                               \
        OPENDAQ__CALL_MACRO(EXTRACT_ITEM_1, OPENDAQ__STRIP_PAR Params),                                                    \
        OPENDAQ__CONCATENATE(OPENDAQ_GET_FUNC_IMPLOBJ(Params), Impl) <Type1 OPENDAQ__COMMA Type2>,                              \
        (OPENDAQ__BUILD_TYPE(OPENDAQ_GET_FUNC_INTF(Params), Type1, Type2)),                                                \
         OPENDAQ__TEMPLATED_CREATE_FUNC(OPENDAQ_GET_FUNC_IMPLOBJ(Params),                                                  \
                                   OPENDAQ_CONCATENATE_DELIM(_, FACTORY_TYPE_NAME(Type1), FACTORY_TYPE_NAME(Type2))), \
        OPENDAQ__BUILD_ARGS_2(Type1, Type2, OPENDAQ__STRIP_PAR FuncParams))


#define OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_TEMPLATE_T_U(Params, Types, FuncParams)                          \
    FOR_EACH_2P(OPENDAQ__DEFINE_CLASS_FACTORY_WITH_INTERFACE_TEMPLATE_T_U_SINGLE, (), Params, (FuncParams), , Types)

//

#define OPENDAQ__DEFINE_CLASS_FACTORY_WITH_INTERFACE_TEMPLATE_IMPL_T_SINGLE(Params, FuncParams, Type) \
    OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC_OBJ(                                   \
        OPENDAQ__CALL_MACRO(EXTRACT_ITEM_1, OPENDAQ__STRIP_PAR Params),                                    \
        OPENDAQ__CONCATENATE(OPENDAQ_GET_FUNC_IMPLOBJ(Params), Impl) <Type>,                               \
        OPENDAQ__BUILD_TYPE(OPENDAQ_GET_FUNC_INTF(Params), Type, Unused),                                  \
        OPENDAQ__TEMPLATED_CREATE_FUNC(OPENDAQ_GET_FUNC_FACTORY(Params), FACTORY_TYPE_NAME(Type)),         \
        OPENDAQ__BUILD_ARGS(Type, OPENDAQ__STRIP_PAR FuncParams))

#define OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_TEMPLATE_IMPL_T(Params, Types, FuncParams)                          \
    FOR_EACH_1P(OPENDAQ__DEFINE_CLASS_FACTORY_WITH_INTERFACE_TEMPLATE_IMPL_T_SINGLE, Params, (FuncParams), Types)

#define OPENDAQ__DEFINE_CLASS_FACTORY_WITH_INTERFACE_TEMPLATE_IMPL_T_U_SINGLE(Params, FuncParams, Unused, Type1, Type2) \
    OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC_OBJ_PAR(                                                 \
        OPENDAQ__CALL_MACRO(EXTRACT_ITEM_1, OPENDAQ__STRIP_PAR Params),                                                      \
        OPENDAQ__CONCATENATE(OPENDAQ_GET_FUNC_IMPLOBJ(Params), Impl) <Type1 OPENDAQ__COMMA Type2>,                                \
        (OPENDAQ__BUILD_TYPE(OPENDAQ_GET_FUNC_INTF(Params), Type1, Type2)),                                                  \
         OPENDAQ__TEMPLATED_CREATE_FUNC(OPENDAQ_GET_FUNC_FACTORY(Params),                                                    \
                                   OPENDAQ_CONCATENATE_DELIM(_, FACTORY_TYPE_NAME(Type1), FACTORY_TYPE_NAME(Type2))),   \
        OPENDAQ__BUILD_ARGS_2(Type1, Type2, OPENDAQ__STRIP_PAR FuncParams))

#define OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_TEMPLATE_IMPL_T_U(Params, Types, FuncParams)                         \
    FOR_EACH_2P(OPENDAQ__DEFINE_CLASS_FACTORY_WITH_INTERFACE_TEMPLATE_IMPL_T_U_SINGLE, (), Params, (FuncParams), , Types)

//
// Custom
//

//
// T, U
//

#define OPENDAQ_DEFINE_CUSTOM_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC_OBJ_PAR(EXPIMP, intf, createFunc, ...) \
    extern "C"                                                                                              \
    daq::ErrCode EXPIMP createFunc(OPENDAQ__STRIP_PAR intf ** objTmp OPENDAQ_FACTORY_TYPES_AND_NAMES_PAR(__VA_ARGS__)) \
    {                                                                                                       \
        return daq::ObjectCreator<OPENDAQ__STRIP_PAR intf>::Create(objTmp OPENDAQ_FACTORY_NAMES(__VA_ARGS__));         \
    }

#define OPENDAQ__DEFINE_CUSTOM_CLASS_FACTORY_WITH_INTERFACE_TEMPLATE_T_U_SINGLE(Params, FuncParams, Unused, Type1, Type2) \
    OPENDAQ_DEFINE_CUSTOM_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC_OBJ_PAR(                                            \
        OPENDAQ__CALL_MACRO(EXTRACT_ITEM_1, OPENDAQ__STRIP_PAR Params),                                                        \
        (OPENDAQ__BUILD_TYPE(OPENDAQ_GET_FUNC_INTF(Params), Type1, Type2)),                                                    \
         OPENDAQ__TEMPLATED_CREATE_FUNC(OPENDAQ_GET_FUNC_IMPLOBJ(Params),                                                      \
                                     OPENDAQ_CONCATENATE_DELIM(_, FACTORY_TYPE_NAME(Type1), FACTORY_TYPE_NAME(Type2))),   \
        OPENDAQ__BUILD_ARGS_2(Type1, Type2, OPENDAQ__STRIP_PAR FuncParams))

#define OPENDAQ_DEFINE_CUSTOM_CLASS_FACTORY_WITH_INTERFACE_TEMPLATE_T_U(Params, Types, FuncParams)                         \
    FOR_EACH_2P(OPENDAQ__DEFINE_CUSTOM_CLASS_FACTORY_WITH_INTERFACE_TEMPLATE_T_U_SINGLE, (), Params, (FuncParams), , Types)

//
// T, U (No params)
//

#define OPENDAQ__DEFINE_CLASS_FACTORY_WITH_INTERFACE_TEMPLATE_T_U_ZERO(Params, FuncParams, Unused, Type1, Type2)   \
    OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC_OBJ_PAR(                                            \
        OPENDAQ__CALL_MACRO(EXTRACT_ITEM_1, OPENDAQ__STRIP_PAR Params),                                                 \
        OPENDAQ__CONCATENATE(OPENDAQ_GET_FUNC_IMPLOBJ(Params), Impl) <Type1 OPENDAQ__COMMA Type2>,                           \
        (OPENDAQ__BUILD_TYPE(OPENDAQ_GET_FUNC_INTF(Params), Type1, Type2)),                                             \
        OPENDAQ__TEMPLATED_CREATE_FUNC(OPENDAQ_GET_FUNC_IMPLOBJ(Params),                                                \
                                  OPENDAQ_CONCATENATE_DELIM(_, FACTORY_TYPE_NAME(Type1), FACTORY_TYPE_NAME(Type2)) \
        )                                                                                                     \
    )

#define OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_TEMPLATE_T_U_ZERO(Params, Types, FuncParams)                   \
    FOR_EACH_2P(OPENDAQ__DEFINE_CLASS_FACTORY_WITH_INTERFACE_TEMPLATE_T_U_ZERO, (), Params, (FuncParams), , Types)

//
// T
//

#define OPENDAQ_DEFINE_CUSTOM_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC_OBJ(EXPIMP, intf, createFunc, ...) \
    extern "C"                                                                                          \
    daq::ErrCode EXPIMP createFunc(intf** objTmp OPENDAQ_FACTORY_TYPES_AND_NAMES(__VA_ARGS__))                \
    {                                                                                                   \
        return daq::ObjectCreator<intf>::Create(objTmp OPENDAQ_FACTORY_NAMES(__VA_ARGS__));                   \
    }

#define OPENDAQ__DEFINE_CUSTOM_CLASS_FACTORY_WITH_INTERFACE_TEMPLATE_T_SINGLE(Params, FuncParams, Type) \
    OPENDAQ_DEFINE_CUSTOM_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC_OBJ(                              \
        OPENDAQ__CALL_MACRO(EXTRACT_ITEM_1, OPENDAQ__STRIP_PAR Params),                                      \
        OPENDAQ__BUILD_TYPE(OPENDAQ_GET_FUNC_INTF(Params), Type, Unused),                                    \
        OPENDAQ__TEMPLATED_CREATE_FUNC(OPENDAQ_GET_FUNC_IMPLOBJ(Params), FACTORY_TYPE_NAME(Type)),           \
        OPENDAQ__BUILD_ARGS(Type, OPENDAQ__STRIP_PAR FuncParams)                                             \
    )

#define OPENDAQ_DEFINE_CUSTOM_CLASS_FACTORY_WITH_INTERFACE_TEMPLATE_T(Params, Types, FuncParams)                    \
    FOR_EACH_1P(OPENDAQ__DEFINE_CUSTOM_CLASS_FACTORY_WITH_INTERFACE_TEMPLATE_T_SINGLE, Params, (FuncParams), Types)

#else

#define OPENDAQ_DEFINE_CLASS_SINGLETON_FACTORY(EXPIMP, obj)
#define OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC_OBJ(EXPIMP, obj, intf, createFunc, ...)
#define OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC_OBJ_TEMPLATE(EXPIMP, obj, intf, createFunc, ...)
#define OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC_OBJ_PAR(EXPIMP, obj, intf, createFunc, ...)
#define OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(EXPIMP, obj, intf, createFunc, ...)
#define OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(EXPIMP, obj, intf, ...)
#define OPENDAQ_DEFINE_CLASS_FACTORY(EXPIMP, obj, ...)
#define OPENDAQ__DEFINE_CLASS_FACTORY_WITH_INTERFACE_TEMPLATE_T_SINGLE(Params, FuncParams, Type)
#define OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_TEMPLATE_T(Params, Types, FuncParams)
#define OPENDAQ__DEFINE_CLASS_FACTORY_WITH_INTERFACE_TEMPLATE_T_U_SINGLE(Params, FuncParams, Unused, Type1, Type2)
#define OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_TEMPLATE_T_U(Params, Types, FuncParams)
#define OPENDAQ__DEFINE_CLASS_FACTORY_WITH_INTERFACE_TEMPLATE_IMPL_T_SINGLE(Params, FuncParams, Type)
#define OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_TEMPLATE_IMPL_T(Params, Types, FuncParams)
#define OPENDAQ__DEFINE_CLASS_FACTORY_WITH_INTERFACE_TEMPLATE_IMPL_T_U_SINGLE(Params, FuncParams, Unused, Type1, Type2)
#define OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_TEMPLATE_IMPL_T_U(Params, Types, FuncParams)
#define OPENDAQ_DEFINE_CUSTOM_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC_OBJ_PAR(EXPIMP, intf, createFunc, ...)
#define OPENDAQ__DEFINE_CUSTOM_CLASS_FACTORY_WITH_INTERFACE_TEMPLATE_T_U_SINGLE(Params, FuncParams, Unused, Type1, Type2)
#define OPENDAQ_DEFINE_CUSTOM_CLASS_FACTORY_WITH_INTERFACE_TEMPLATE_T_U(Params, Types, FuncParams)
#define OPENDAQ__DEFINE_CLASS_FACTORY_WITH_INTERFACE_TEMPLATE_T_U_ZERO(Params, FuncParams, Unused, Type1, Type2)
#define OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_TEMPLATE_T_U_ZERO(Params, Types, FuncParams)
#define OPENDAQ_DEFINE_CUSTOM_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC_OBJ(EXPIMP, intf, createFunc, ...)
#define OPENDAQ__DEFINE_CUSTOM_CLASS_FACTORY_WITH_INTERFACE_TEMPLATE_T_SINGLE(Params, FuncParams, Type)
#define OPENDAQ_DEFINE_CUSTOM_CLASS_FACTORY_WITH_INTERFACE_TEMPLATE_T(Params, Types, FuncParams)

#endif

END_NAMESPACE_OPENDAQ
