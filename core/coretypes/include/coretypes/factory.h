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
#include <coretypes/factoryselectors.h>
#include <coretypes/macro_utils.h>
#include <coretypes/exceptions.h>
#include <new>

#define EXTRACT_ITEM_1(X1, ...) X1
#define EXTRACT_ITEM_2(X1, X2, ...) X2
#define EXTRACT_ITEM_3(X1, X2, X3, ...) X3
#define EXTRACT_ITEM_4(X1, X2, X3, X4, ...) X4
#define EXTRACT_ITEM_5(X1, X2, X3, X4, X5, ...) X5
#define EXTRACT_ITEM_6(X1, X2, X3, X4, X5, X6, ...) X6
#define EXTRACT_ITEM_7(X1, X2, X3, X4, X5, X6, X7, ...) X7

#define FACTORY_TYPE_NAME_int8_t int8
#define FACTORY_TYPE_NAME_uint8_t byte
#define FACTORY_TYPE_NAME_int16_t int16
#define FACTORY_TYPE_NAME_int32_t int32
#define FACTORY_TYPE_NAME_int64_t int64
#define FACTORY_TYPE_NAME_float float
#define FACTORY_TYPE_NAME_double double
#define FACTORY_TYPE_NAME_RangeType64 RangeType
#define FACTORY_TYPE_NAME_ComplexFloat32 ComplexFloat32
#define FACTORY_TYPE_NAME_ComplexFloat64 ComplexFloat64
#define FACTORY_TYPE_NAME_BinarySample Binary
#define FACTORY_TYPE_NAME_void
#define FACTORY_TYPE_NAME_RawPtr Raw
#define FACTORY_TYPE_NAME_Unscaled unscaled

#define FACTORY_TYPE_NAME(Type) FACTORY_TYPE_NAME_##Type

#define OPENDAQ_IF(cond) OPENDAQ__CONCATENATE(OPENDAQ__IF_, cond)
#define OPENDAQ__IF_0(t, f) f
#define OPENDAQ__IF_1(t, f) t

#define OPENDAQ_EQ_0(x) OPENDAQ__CONCATENATE(OPENDAQ__RES_EQ_0_, x)
#define OPENDAQ__RES_EQ_0_0 1
#define OPENDAQ__RES_EQ_0_1 0
#define OPENDAQ__RES_EQ_0_2 0

#define OPENDAQ_EQ_1(x) OPENDAQ__CONCATENATE(OPENDAQ__RES_EQ_1_, x)
#define OPENDAQ__RES_EQ_1_0 0
#define OPENDAQ__RES_EQ_1_1 1
#define OPENDAQ__RES_EQ_1_2 0

#define OPENDAQ_EQ_2(x) OPENDAQ__CONCATENATE(OPENDAQ__RES_EQ_2_, x)
#define OPENDAQ__RES_EQ_2_0 0
#define OPENDAQ__RES_EQ_2_1 0
#define OPENDAQ__RES_EQ_2_2 1

#define OPENDAQ_SAMPLE_TYPES(...) __VA_ARGS__
#define OPENDAQ_FACTORY_PARAMS(...) __VA_ARGS__
#define OPENDAQ_FACTORY_INFO(Accessibility, ImplementationObject, Interface) (Accessibility, ImplementationObject, Interface)
#define OPENDAQ_FACTORY_INFO_IMPL(Accessibility, ImplementationObject, FactoryName, Interface) (Accessibility, ImplementationObject, Interface, FactoryName)
#define OPENDAQ_FUNC_NO_PARAMS

#define OPENDAQ__CALL_MACRO(Macro, ...) OPENDAQ__EXPAND(Macro(__VA_ARGS__))

#define OPENDAQ__TEMPLATED_CREATE_FUNC_(Name, Type) create##Name##_##Type
#define OPENDAQ__TEMPLATED_CREATE_FUNC(Name, Type) OPENDAQ__TEMPLATED_CREATE_FUNC_(Name, Type)

#define OPENDAQ_GET_FUNC_ACCESS(Params) OPENDAQ__CALL_MACRO(EXTRACT_ITEM_1, OPENDAQ__STRIP_PAR Params)
#define OPENDAQ_GET_FUNC_INTF(Params) OPENDAQ__CALL_MACRO(EXTRACT_ITEM_3, OPENDAQ__STRIP_PAR Params)
#define OPENDAQ_GET_FUNC_IMPLOBJ(Params) OPENDAQ__CALL_MACRO(EXTRACT_ITEM_2, OPENDAQ__STRIP_PAR Params)
#define OPENDAQ_GET_FUNC_FACTORY(Params) OPENDAQ__CALL_MACRO(EXTRACT_ITEM_4, OPENDAQ__STRIP_PAR Params)

#define OPENDAQ__GET_TYPE_ITEM(Desc, Item) OPENDAQ__CALL_MACRO(OPENDAQ__CONCATENATE(EXTRACT_ITEM_, Item), OPENDAQ__STRIP_PAR Desc)

#define OPENDAQ__NO_ARG      0
#define OPENDAQ__ARG_T       1
#define OPENDAQ__ARG_U       2

#define OPENDAQ__IS_TEMPLATED_TYPE    1
#define OPENDAQ__IS_TYPE              1
#define OPENDAQ__IS_TEMPLATE          0

#define OPENDAQ__TYPE_FLAGS_NONE      0
#define OPENDAQ__TYPE_FLAGS_PTR       1
#define OPENDAQ__TYPE_FLAGS_SMARTPTR  2

#define OPENDAQ__TEMPLATED_TYPE_TWO_ARGS(Name, Arg1, Arg2, Flags)  (OPENDAQ__IS_TEMPLATED_TYPE, Name, Arg1, Arg2, Flags)
#define OPENDAQ__TEMPLATED_TYPE_ONE_ARG(Name, Arg, Flags)          (OPENDAQ__IS_TEMPLATED_TYPE, Name, Arg, OPENDAQ__NO_ARG, Flags)
#define OPENDAQ__TEMPLATE(Arg, Flags)                              (OPENDAQ__IS_TEMPLATE, , Arg, OPENDAQ__NO_ARG, Flags)
#define OPENDAQ__TYPE(Name, Flags)                                 (OPENDAQ__IS_TYPE, Name, OPENDAQ__NO_ARG, OPENDAQ__NO_ARG, Flags)

#define OPENDAQ_TYPE(Name)                                  OPENDAQ__TYPE(Name, OPENDAQ__TYPE_FLAGS_NONE)
#define OPENDAQ_SMARTPTR(Name)                              OPENDAQ__TYPE(Name, OPENDAQ__TYPE_FLAGS_SMARTPTR)
#define OPENDAQ_TEMPLATED_TYPE_T(Name)                      OPENDAQ__TEMPLATED_TYPE_ONE_ARG(Name, OPENDAQ__ARG_T, OPENDAQ__TYPE_FLAGS_NONE)
#define OPENDAQ_TEMPLATED_TYPE_U(Name)                      OPENDAQ__TEMPLATED_TYPE_ONE_ARG(Name, OPENDAQ__ARG_U, OPENDAQ__TYPE_FLAGS_NONE)
#define OPENDAQ_TEMPLATED_TYPE_T_U(Name)                    OPENDAQ__TEMPLATED_TYPE_TWO_ARGS(Name, OPENDAQ__ARG_T, OPENDAQ__ARG_U, OPENDAQ__TYPE_FLAGS_NONE)
#define OPENDAQ_TEMPLATED_TYPE_U_T(Name)                    OPENDAQ__TEMPLATED_TYPE_TWO_ARGS(Name, OPENDAQ__ARG_U, OPENDAQ__ARG_T, OPENDAQ__TYPE_FLAGS_NONE)
#define OPENDAQ_TEMPLATE_T                                  OPENDAQ__TEMPLATE(OPENDAQ__ARG_T, OPENDAQ__TYPE_FLAGS_NONE)
#define OPENDAQ_TEMPLATE_U                                  OPENDAQ__TEMPLATE(OPENDAQ__ARG_U, OPENDAQ__TYPE_FLAGS_NONE)

#define OPENDAQ_TEMPLATED_SMARTPTR_T(Name)                  OPENDAQ__TEMPLATED_TYPE_ONE_ARG(Name, OPENDAQ__ARG_T, OPENDAQ__TYPE_FLAGS_SMARTPTR)

#define OPENDAQ_PTR(Name)                                   OPENDAQ__TYPE(Name, OPENDAQ__TYPE_FLAGS_PTR)
#define OPENDAQ_TEMPLATED_PTR_T(Name)                       OPENDAQ__TEMPLATED_TYPE_ONE_ARG(Name, OPENDAQ__ARG_T, OPENDAQ__TYPE_FLAGS_PTR)
#define OPENDAQ_TEMPLATED_PTR_U(Name)                       OPENDAQ__TEMPLATED_TYPE_ONE_ARG(Name, OPENDAQ__ARG_U, OPENDAQ__TYPE_FLAGS_PTR)
#define OPENDAQ_TEMPLATED_PTR_T_U(Name)                     OPENDAQ__TEMPLATED_TYPE_TWO_ARGS(Name, OPENDAQ__ARG_T, OPENDAQ__ARG_U, OPENDAQ__TYPE_FLAGS_PTR)
#define OPENDAQ_TEMPLATED_PTR_U_T(Name)                     OPENDAQ__TEMPLATED_TYPE_TWO_ARGS(Name, OPENDAQ__ARG_U, OPENDAQ__ARG_T, OPENDAQ__TYPE_FLAGS_PTR)
#define OPENDAQ_TEMPLATE_PTR_T                              OPENDAQ__TEMPLATE(OPENDAQ__ARG_T, OPENDAQ__TYPE_FLAGS_PTR)
#define OPENDAQ_TEMPLATE_PTR_U                              OPENDAQ__TEMPLATE(OPENDAQ__ARG_U, OPENDAQ__TYPE_FLAGS_PTR)

#define OPENDAQ__GET_TYPE_TYPE(TypeDesc)  OPENDAQ__GET_TYPE_ITEM(TypeDesc, 1)
#define OPENDAQ__GET_TYPE_NAME(TypeDesc)  OPENDAQ__GET_TYPE_ITEM(TypeDesc, 2)
#define OPENDAQ__GET_TYPE_ARG_1(TypeDesc) OPENDAQ__GET_TYPE_ITEM(TypeDesc, 3)
#define OPENDAQ__GET_TYPE_ARG_2(TypeDesc) OPENDAQ__GET_TYPE_ITEM(TypeDesc, 4)
#define OPENDAQ__GET_TYPE_FLAGS(TypeDesc) OPENDAQ__GET_TYPE_ITEM(TypeDesc, 5)


#define OPENDAQ__BUILD_TEMPLATED_TYPE_HAS_ONLY_FIRST_PARAM(TypeDesc, TemplateParam1, TemplateParam2)       \
    OPENDAQ__GET_TYPE_NAME(TypeDesc) OPENDAQ_IF(OPENDAQ_EQ_1(OPENDAQ__GET_TYPE_ARG_1(TypeDesc)))(<TemplateParam1>, <TemplateParam2>)

#define OPENDAQ__BUILD_TEMPLATED_TYPE_HAS_FIRST_AND_SECOND_PARAM(TypeDesc, TemplateParam1, TemplateParam2) \
    OPENDAQ__GET_TYPE_NAME(TypeDesc) OPENDAQ_IF(OPENDAQ_EQ_1(OPENDAQ__GET_TYPE_ARG_1(TypeDesc)))(<TemplateParam1 OPENDAQ__COMMA TemplateParam2>, <TemplateParam2 OPENDAQ__COMMA TemplateParam1>)

#define OPENDAQ__BUILD_TEMPLATED_TYPE_HAS_FIRST_PARAM(TypeDesc, TemplateParam1, TemplateParam2)            \
    OPENDAQ_IF(OPENDAQ_EQ_0(OPENDAQ__GET_TYPE_ARG_2(TypeDesc)))(                                                   \
        OPENDAQ__BUILD_TEMPLATED_TYPE_HAS_ONLY_FIRST_PARAM(TypeDesc, TemplateParam1, TemplateParam2),      \
        OPENDAQ__BUILD_TEMPLATED_TYPE_HAS_FIRST_AND_SECOND_PARAM(TypeDesc, TemplateParam1, TemplateParam2) \
    )

#define OPENDAQ__OUTPUT_IF_SMARTPTR(TypeDesc, Str) OPENDAQ_IF(OPENDAQ_EQ_2(OPENDAQ__GET_TYPE_FLAGS(TypeDesc))) (Str,)
#define OPENDAQ__OUTPUT_IF_PTR(TypeDesc, Str) OPENDAQ_IF(OPENDAQ_EQ_1(OPENDAQ__GET_TYPE_FLAGS(TypeDesc))) (Str,)

#define OPENDAQ__BUILD_TEMPLATED_TYPE(TypeDesc, TemplateParam1, TemplateParam2)                            \
    OPENDAQ__OUTPUT_IF_SMARTPTR(TypeDesc, const)                                                           \
    OPENDAQ_IF(OPENDAQ_EQ_0(OPENDAQ__GET_TYPE_ARG_1(TypeDesc)))(                                                     \
        OPENDAQ__GET_TYPE_NAME(TypeDesc),                                                                  \
        OPENDAQ__BUILD_TEMPLATED_TYPE_HAS_FIRST_PARAM(TypeDesc, TemplateParam1, TemplateParam2)            \
    ) OPENDAQ__OUTPUT_IF_SMARTPTR(TypeDesc, &) OPENDAQ__OUTPUT_IF_PTR(TypeDesc, *)

#define OPENDAQ__BUILD_RAW_TYPE(TypeDesc, TemplateParam1, TemplateParam2)                                  \
    OPENDAQ_IF(OPENDAQ_EQ_1(OPENDAQ__GET_TYPE_ARG_1(TypeDesc))) (TemplateParam1, TemplateParam2)

#define OPENDAQ__BUILD_TYPE(TypeDesc, TemplateParam1, TemplateParam2)                                      \
    OPENDAQ_IF(OPENDAQ_EQ_0(OPENDAQ__GET_TYPE_TYPE(TypeDesc)))(                                                      \
        OPENDAQ__BUILD_RAW_TYPE(TypeDesc, TemplateParam1, TemplateParam2),                                 \
        OPENDAQ__BUILD_TEMPLATED_TYPE(TypeDesc, TemplateParam1, TemplateParam2)                            \
    )

#define OPENDAQ__OUTPUT_TEMPLATE_FROM_TYPE(TypeDesc, TemplateParam1, TemplateParam2)                       \
    OPENDAQ_IF(OPENDAQ_EQ_0(OPENDAQ__GET_TYPE_ARG_1(TypeDesc)))(                                                     \
        ,                                                                                             \
        OPENDAQ__OUTPUT_TEMPLATE_FROM_TYPE_INTERNAL(TypeDesc, TemplateParam1, TemplateParam2)              \
    )

#define OPENDAQ__OUTPUT_TEMPLATE_FROM_TYPE_INTERNAL(TypeDesc, TemplateParam1, TemplateParam2)              \
    OPENDAQ_IF(OPENDAQ_EQ_0(OPENDAQ__GET_TYPE_ARG_2(TypeDesc)))(                                                     \
        OPENDAQ__OUTPUT_TEMPLATE_FROM_TYPE_SINGLE_PARAM(TypeDesc, TemplateParam1, TemplateParam2),         \
        OPENDAQ__OUTPUT_TEMPLATE_FROM_TYPE_DUAL_PARAM(TypeDesc, TemplateParam1, TemplateParam2)            \
    )

#define OPENDAQ__OUTPUT_TEMPLATE_FROM_TYPE_SINGLE_PARAM(TypeDesc, TemplateParam1, TemplateParam2)          \
    OPENDAQ_IF(OPENDAQ_EQ_1(OPENDAQ__GET_TYPE_ARG_1(TypeDesc)))(                                                     \
        template <typename TemplateParam1>,                                                                \
        template <typename TemplateParam2>                                                                 \
    )

#define OPENDAQ__OUTPUT_TEMPLATE_FROM_TYPE_DUAL_PARAM(TypeDesc, TemplateParam1, TemplateParam2)            \
    OPENDAQ_IF(OPENDAQ_EQ_1(OPENDAQ__GET_TYPE_ARG_1(TypeDesc)))(                                                     \
        template <typename TemplateParam1 OPENDAQ__COMMA typename TemplateParam2>,                         \
        template <typename TemplateParam2 OPENDAQ__COMMA typename TemplateParam1>                          \
    )

#define OPENDAQ__OUTPUT_TEMPLATE_PARAMS_FROM_TYPE(TypeDesc, TemplateParam1, TemplateParam2)                \
    OPENDAQ_IF(OPENDAQ_EQ_0(OPENDAQ__GET_TYPE_ARG_1(TypeDesc)))(                                                     \
        ,                                                                                                  \
        OPENDAQ__OUTPUT_TEMPLATE_PARAMS_FROM_TYPE_INTERNAL(TypeDesc, TemplateParam1, TemplateParam2)       \
    )

#define OPENDAQ__OUTPUT_TEMPLATE_PARAMS_FROM_TYPE_INTERNAL(TypeDesc, TemplateParam1, TemplateParam2)       \
    OPENDAQ_IF(OPENDAQ_EQ_0(OPENDAQ__GET_TYPE_ARG_2(TypeDesc)))(                                                     \
        OPENDAQ__OUTPUT_TEMPLATE_PARAMS_FROM_TYPE_SINGLE_PARAM(TypeDesc, TemplateParam1, TemplateParam2),  \
        OPENDAQ__OUTPUT_TEMPLATE_PARAMS_FROM_TYPE_DUAL_PARAM(TypeDesc, TemplateParam1, TemplateParam2)     \
    )

#define OPENDAQ__OUTPUT_TEMPLATE_PARAMS_FROM_TYPE_SINGLE_PARAM(TypeDesc, TemplateParam1, TemplateParam2)   \
    OPENDAQ_IF(OPENDAQ_EQ_1(OPENDAQ__GET_TYPE_ARG_1(TypeDesc)))(                                                     \
        <TemplateParam1>,                                                                                  \
        <TemplateParam2>                                                                                   \
    )

#define OPENDAQ__OUTPUT_TEMPLATE_PARAMS_FROM_TYPE_DUAL_PARAM(TypeDesc, TemplateParam1, TemplateParam2)     \
    OPENDAQ_IF(OPENDAQ_EQ_1(OPENDAQ__GET_TYPE_ARG_1(TypeDesc)))(                                                     \
        <TemplateParam1 OPENDAQ__COMMA TemplateParam2>,                                                    \
        <TemplateParam2 OPENDAQ__COMMA TemplateParam1>                                                     \
    )

#define OPENDAQ__BUILD_ARGS_SINGLE(TemplatedType, Unused2, Unused3, Type, Name)          OPENDAQ__BUILD_TYPE(Type, TemplatedType,  Unused), Name
#define OPENDAQ__BUILD_ARGS_SINGLE_2(TemplatedType1, TemplatedType2, Param3, Type, Name) (OPENDAQ__BUILD_TYPE(Type, TemplatedType1, TemplatedType2)), Name

#define OPENDAQ__BUILD_ARGS(TemplatedType, ...)                    _FOR_EACH_2P(OPENDAQ__BUILD_ARGS_SINGLE,   OPENDAQ__COMMA_DELIM,  TemplatedType,  unused2,         unused3, __VA_ARGS__)
#define OPENDAQ__BUILD_ARGS_2(TemplatedType1, TemplatedType2, ...) _FOR_EACH_2P(OPENDAQ__BUILD_ARGS_SINGLE_2, OPENDAQ__COMMA_DELIM , TemplatedType1, TemplatedType2, _unused3, __VA_ARGS__)

#define OPENDAQ__CALL_TEMPLATED_CREATE_FUNCTION(Params, FuncParams, Type)                                                        \
    if constexpr (std::is_same_v<T, Type>)                                                                                       \
    {                                                                                                                            \
        res = OPENDAQ__TEMPLATED_CREATE_FUNC(OPENDAQ_GET_FUNC_IMPLOBJ(Params), FACTORY_TYPE_NAME(Type))(                              \
            &obj OPENDAQ_FACTORY_NAMES_EX(OPENDAQ__STRIP_PAR FuncParams));                                                            \
    } else

#define OPENDAQ__CALL_TEMPLATED_2_CREATE_FUNCTION(Params, FuncParams, Unused, Type1, Type2)                                      \
    if constexpr (std::is_same_v<T, Type1> && std::is_same_v<U, Type2>)                                                          \
    {                                                                                                                            \
        res = OPENDAQ__TEMPLATED_CREATE_FUNC(OPENDAQ_GET_FUNC_IMPLOBJ(Params),                                                        \
                                          OPENDAQ_CONCATENATE_DELIM(_, FACTORY_TYPE_NAME(Type1), FACTORY_TYPE_NAME(Type2)))      \
              (&obj OPENDAQ_FACTORY_NAMES_EX(OPENDAQ__STRIP_PAR FuncParams));                                                         \
    } else

#define OPENDAQ__DECLARE_CREATE_FUNC_TEMPLATE_ARG(EXPIMP, IntfName, CreateFunc, TemplateType, ...) \
    extern "C" daq::ErrCode EXPIMP CreateFunc(IntfName** obj OPENDAQ__CALL_MACRO(OPENDAQ_FACTORY_TYPES_AND_NAMES, OPENDAQ__BUILD_ARGS(TemplateType, __VA_ARGS__)));

#define OPENDAQ__DECLARE_CREATE_FUNC_TEMPLATE_2_ARG(EXPIMP, IntfName, CreateFunc, Type1, Type2, ...) \
    extern "C" daq::ErrCode EXPIMP CreateFunc(IntfName** obj OPENDAQ__CALL_MACRO(OPENDAQ_FACTORY_TYPES_AND_NAMES_PAR, OPENDAQ__BUILD_ARGS_2(Type1, Type2, __VA_ARGS__)));

#define OPENDAQ__DECLARE_CLASS_FACTORY_WITH_INTERFACE_TEMPLATE_T_SINGLE(Params, FuncParams, Type)                                \
    OPENDAQ__DECLARE_CREATE_FUNC_TEMPLATE_ARG(OPENDAQ__CALL_MACRO(EXTRACT_ITEM_1, OPENDAQ__STRIP_PAR Params),                         \
                             OPENDAQ__BUILD_TYPE(OPENDAQ_GET_FUNC_INTF(Params), Type, Unused),                                        \
                             OPENDAQ__TEMPLATED_CREATE_FUNC(OPENDAQ_GET_FUNC_IMPLOBJ(Params), FACTORY_TYPE_NAME(Type)),               \
                             Type,                                                                                               \
                             OPENDAQ__STRIP_PAR FuncParams)

#define OPENDAQ__DECLARE_CLASS_FACTORY_WITH_INTERFACE_TEMPLATE_T_U_SINGLE(Params, FuncParams, Unused, Type1, Type2)              \
    OPENDAQ__DECLARE_CREATE_FUNC_TEMPLATE_2_ARG(OPENDAQ__CALL_MACRO(EXTRACT_ITEM_1, OPENDAQ__STRIP_PAR Params),                       \
                             OPENDAQ__BUILD_TYPE(OPENDAQ_GET_FUNC_INTF(Params), Type1, Type2),                                        \
                                              OPENDAQ__TEMPLATED_CREATE_FUNC(OPENDAQ_GET_FUNC_IMPLOBJ(Params),                        \
                                              OPENDAQ_CONCATENATE_DELIM(_, FACTORY_TYPE_NAME(Type1), FACTORY_TYPE_NAME(Type2))), \
                             Type1,                                                                                              \
                             Type2,                                                                                              \
                             OPENDAQ__STRIP_PAR FuncParams)


//////////////////////////////

#define OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE_TEMPLATE_T(Info, Types, FuncParams)                                         \
    FOR_EACH_1P(OPENDAQ__DECLARE_CLASS_FACTORY_WITH_INTERFACE_TEMPLATE_T_SINGLE, Info, (FuncParams), Types)                      \
                                                                                                                            \
    template <typename T>                                                                                                   \
    OPENDAQ__BUILD_TYPE(OPENDAQ_GET_FUNC_INTF(Info), T, U) * OPENDAQ__CONCATENATE(OPENDAQ_GET_FUNC_IMPLOBJ(Info),                               \
      _Create(OPENDAQ__CALL_MACRO(OPENDAQ_FACTORY_CM_TYPES_AND_NAMES, OPENDAQ__BUILD_ARGS(T, FuncParams))))                                \
    {                                                                                                                       \
        OPENDAQ__BUILD_TYPE(OPENDAQ_GET_FUNC_INTF(Info), T, U) * obj;                                                                 \
        daq::ErrCode res = OPENDAQ_ERR_NOTIMPLEMENTED;                                                                            \
        FOR_EACH_1P(OPENDAQ__CALL_TEMPLATED_CREATE_FUNCTION, Info, (FuncParams), Types)                                          \
            static_assert(daq::DependentFalse<T>::value, "Type not supported");                                              \
                                                                                                                            \
        daq::checkErrorInfo(res);                                                                                            \
        return obj;                                                                                                         \
    }

#define OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE_TEMPLATE_T_U(Info, Types, FuncParams)                                       \
    FOR_EACH_2P(OPENDAQ__DECLARE_CLASS_FACTORY_WITH_INTERFACE_TEMPLATE_T_U_SINGLE, OPENDAQ__NO_DELIM, Info, (FuncParams), , Types)    \
                                                                                                                                 \
    template <typename T, typename U>                                                                                            \
    OPENDAQ__BUILD_TYPE(OPENDAQ_GET_FUNC_INTF(Info), T, U) * OPENDAQ__CONCATENATE(OPENDAQ_GET_FUNC_IMPLOBJ(Info),                          \
      _Create(OPENDAQ__CALL_MACRO(OPENDAQ_FACTORY_CM_TYPES_AND_NAMES_PAR, OPENDAQ__BUILD_ARGS_2(T, U, FuncParams))))                       \
    {                                                                                                                            \
        OPENDAQ__BUILD_TYPE(OPENDAQ_GET_FUNC_INTF(Info), T, U) * obj;                                                                 \
        daq::ErrCode res = OPENDAQ_ERR_NOTIMPLEMENTED;                                                                           \
        FOR_EACH_2P(OPENDAQ__CALL_TEMPLATED_2_CREATE_FUNCTION, OPENDAQ__NO_DELIM , Info, (FuncParams), , Types)                       \
            static_assert(daq::DependentFalse<std::pair<T, U>>::value, "Type not supported");                                     \
                                                                                                                                 \
        daq::checkErrorInfo(res);                                                                                                 \
        return obj;                                                                                                              \
    }

#define OPENDAQ_DEFINE_CLASS_FACTORY_HELPER_WITH_INTERFACE_AND_CREATEFUNC(ObjName, IntfName, CreateFunc, ...)                    \
    inline IntfName* ObjName##_Create(OPENDAQ_FACTORY_CM_TYPES_AND_NAMES(__VA_ARGS__))                                                \
    {                                                                                                                            \
        IntfName* obj;                                                                                                           \
        daq::ErrCode res = CreateFunc(&obj OPENDAQ_FACTORY_NAMES(__VA_ARGS__));                                                   \
        daq::checkErrorInfo(res);                                                                                                 \
        return obj;                                                                                                              \
    }

#define OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(EXPIMP, ObjName, IntfName, CreateFunc, ...)                  \
    extern "C"                                                                                                                   \
    daq::ErrCode EXPIMP CreateFunc(IntfName** obj OPENDAQ_FACTORY_TYPES_AND_NAMES(__VA_ARGS__));                                  \
                                                                                                                                 \
    OPENDAQ_DEFINE_CLASS_FACTORY_HELPER_WITH_INTERFACE_AND_CREATEFUNC(ObjName, IntfName, CreateFunc, __VA_ARGS__)

#define OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(EXPIMP, ObjName, IntfName, ...)                                             \
    OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(EXPIMP, ObjName, IntfName, create##ObjName, __VA_ARGS__)

#define OPENDAQ_DECLARE_CLASS_FACTORY(EXPIMP, ObjName, ...)                                                                      \
    OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(EXPIMP, ObjName, I##ObjName, create##ObjName, __VA_ARGS__)

#define OPENDAQ_DECLARE_CREATE_FUNC(EXPIMP, IntfName, CreateFunc, ...)                                                           \
    extern "C"                                                                                                                   \
    daq::ErrCode EXPIMP CreateFunc(IntfName** obj OPENDAQ_FACTORY_TYPES_AND_NAMES(__VA_ARGS__));

#define OPENDAQ_DECLARE_CLASS_FACTORY_FUNC_WITH_INTERFACE(EXPIMP, Name, Interface, ...)                                          \
    OPENDAQ_DECLARE_CREATE_FUNC(EXPIMP, Interface, create##Name, __VA_ARGS__)

//////////////
//// No params
//////////////

#define OPENDAQ__DECLARE_CREATE_FUNC_TEMPLATE_2_ARG_ZERO(EXPIMP, IntfName, CreateFunc, Type1, Type2) \
    extern "C" daq::ErrCode EXPIMP CreateFunc(IntfName** obj);

#define OPENDAQ__DECLARE_CLASS_FACTORY_WITH_INTERFACE_TEMPLATE_T_U_SINGLE_ZERO(Params, FuncParams, Unused, Type1, Type2) \
    OPENDAQ__DECLARE_CREATE_FUNC_TEMPLATE_2_ARG_ZERO(                                                                    \
        OPENDAQ__CALL_MACRO(EXTRACT_ITEM_1, OPENDAQ__STRIP_PAR Params),                                                  \
        OPENDAQ__BUILD_TYPE(OPENDAQ_GET_FUNC_INTF(Params), Type1, Type2),                                                     \
        OPENDAQ__TEMPLATED_CREATE_FUNC(                                                                                  \
            OPENDAQ_GET_FUNC_IMPLOBJ(Params),                                                                            \
            OPENDAQ_CONCATENATE_DELIM(_, FACTORY_TYPE_NAME(Type1), FACTORY_TYPE_NAME(Type2))                             \
        ),                                                                                                               \
        Type1,                                                                                                           \
        Type2                                                                                                            \
    )

#define OPENDAQ__CALL_TEMPLATED_2_CREATE_FUNCTION_ZERO(Params, FuncParams, Unused, Type1, Type2)                          \
    if constexpr (std::is_same_v<T, Type1> && std::is_same_v<U, Type2>)                                                   \
    {                                                                                                                     \
        res = OPENDAQ__TEMPLATED_CREATE_FUNC(OPENDAQ_GET_FUNC_IMPLOBJ(Params),                                                 \
                                        OPENDAQ_CONCATENATE_DELIM(_, FACTORY_TYPE_NAME(Type1), FACTORY_TYPE_NAME(Type2))) \
              (&obj OPENDAQ_FACTORY_NAMES_EX());                                                                          \
    } else

#define OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE_TEMPLATE_T_U_ZERO(Info, Types, FuncParams)                                    \
    FOR_EACH_2P(OPENDAQ__DECLARE_CLASS_FACTORY_WITH_INTERFACE_TEMPLATE_T_U_SINGLE_ZERO, OPENDAQ__NO_DELIM, Info, (FuncParams), , Types) \
                                                                                                                                   \
    template <typename T, typename U>                                                                                              \
    OPENDAQ__BUILD_TYPE(OPENDAQ_GET_FUNC_INTF(Info), T, U)* OPENDAQ__CONCATENATE(OPENDAQ_GET_FUNC_IMPLOBJ(Info), _Create())                  \
    {                                                                                                                              \
        OPENDAQ__BUILD_TYPE(OPENDAQ_GET_FUNC_INTF(Info), T, U) * obj;                                                                   \
        daq::ErrCode res = OPENDAQ_ERR_NOTIMPLEMENTED;                                                                             \
        FOR_EACH_2P(OPENDAQ__CALL_TEMPLATED_2_CREATE_FUNCTION_ZERO, OPENDAQ__NO_DELIM , Info, (), , Types)                              \
            static_assert(daq::DependentFalse<std::pair<T, U>>::value, "Type not supported");                                       \
                                                                                                                                   \
        daq::checkErrorInfo(res);                                                                                                   \
        return obj;                                                                                                                \
    }
