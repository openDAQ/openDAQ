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
#include <coretypes/baseobject.h>
#include <coretypes/stringobject.h>
#include <coretypes/serialized_object.h>
#include <coretypes/serialized_list.h>
#include <coretypes/updatable.h>
#include <coretypes/function.h>

BEGIN_NAMESPACE_OPENDAQ

typedef ErrCode (*daqDeserializerFactory)(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** deserialized);

/*!
 * @ingroup types_serialization
 * @defgroup types_deserializer Deserializer
 * @{
 */

DECLARE_OPENDAQ_INTERFACE(IDeserializer, IBaseObject)
{
    virtual ErrCode INTERFACE_FUNC deserialize(IString* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** object) = 0;
    virtual ErrCode INTERFACE_FUNC update(IUpdatable * updatable, IString * serialized) = 0;
};

/*!
 * @}
 */

extern "C"
ErrCode PUBLIC_EXPORT daqRegisterSerializerFactory(ConstCharPtr id, daqDeserializerFactory factory);

extern "C"
ErrCode PUBLIC_EXPORT daqUnregisterSerializerFactory(ConstCharPtr id);

extern "C"
ErrCode PUBLIC_EXPORT daqGetSerializerFactory(ConstCharPtr id, daqDeserializerFactory* factory);

#define OPENDAQ_REGISTER_DESERIALIZE_FACTORY(type)                                        \
    class type##Factory                                                              \
    {                                                                                \
    public:                                                                          \
        type##Factory()                                                              \
        {                                                                            \
            daq::daqRegisterSerializerFactory(type::SerializeId(), type::Deserialize); \
        }                                                                            \
    };                                                                               \
    static type##Factory g##type##Factory;

//// TEMPLATED <T>

#define OPENDAQ__CALL_TEMPLATED_T_DESERIALIZE_HANDLER(Params, ImplObj, Type1) \
    daq::daqRegisterSerializerFactory(ImplObj##Impl<Type1>::SerializeId(), \
                                    ImplObj##Impl<Type1>::Deserialize);

#define OPENDAQ__CREATE_DESERIALIZE_HANDLERS_TEMPLATED_T(ImplName, Info, Types)                       \
class ImplName##Factory                                                                          \
{                                                                                                \
public:                                                                                          \
    ImplName##Factory()                                                                          \
    {                                                                                            \
        FOR_EACH_1P(                                                                             \
            OPENDAQ__CALL_TEMPLATED_T_DESERIALIZE_HANDLER,                                            \
            OPENDAQ_FACTORY_INFO(LIBRARY_FACTORY, OPENDAQ_GET_FUNC_IMPLOBJ(Info), OPENDAQ_GET_FUNC_INTF(Info)), \
            OPENDAQ_GET_FUNC_IMPLOBJ(Info),                                                           \
            Types                                                                                \
        )                                                                                        \
    }                                                                                            \
};                                                                                               \
static ImplName##Factory ImplName##FactoryRegistrator;

//// TEMPLATED <T, U>

#define OPENDAQ__CALL_TEMPLATED_T_U_DESERIALIZE_HANDLER(Params, FuncParams, ImplObj, Type1, Type2)                             \
    daq::daqRegisterSerializerFactory(ImplObj##Impl<Type1, Type2>::SerializeId(), ImplObj##Impl<Type1, Type2>::Deserialize);

#define OPENDAQ__CREATE_DESERIALIZE_HANDLERS_TEMPLATED_T_U(ImplName, Info, Types)                                              \
    class ImplName##Factory                                                                                               \
    {                                                                                                                     \
    public:                                                                                                               \
        ImplName##Factory()                                                                                               \
        {                                                                                                                 \
            FOR_EACH_2P(OPENDAQ__CALL_TEMPLATED_T_U_DESERIALIZE_HANDLER,                                                       \
                        OPENDAQ__NO_DELIM,                                                                                     \
                        OPENDAQ_FACTORY_INFO(LIBRARY_FACTORY, OPENDAQ_GET_FUNC_IMPLOBJ(Info), OPENDAQ_GET_FUNC_INTF(Info)),              \
                        (),                                                                                               \
                        OPENDAQ_GET_FUNC_IMPLOBJ(Info),                                                                        \
                        Types)                                                                                            \
        }                                                                                                                 \
    };                                                                                                                    \
    static ImplName##Factory ImplName##FactoryRegistrator;

//// TEMPLATED FUNCTION <T, U>

#define OPENDAQ__CALL_TEMPLATED_T_U_DESERIALIZE_HANDLER_FUNC(Params, FuncName, ImplObj, Type1, Type2)                          \
    daq::daqRegisterSerializerFactory(ImplObj##Impl<Type1, Type2>::SerializeId(), &FuncName<Type1, Type2>);

#define OPENDAQ__CREATE_DESERIALIZE_HANDLERS_TEMPLATED_T_U_FUNC(ImplName, Info, Types, FuncName)                               \
    class ImplName##Factory                                                                                               \
    {                                                                                                                     \
    public:                                                                                                               \
        ImplName##Factory()                                                                                               \
        {                                                                                                                 \
            FOR_EACH_2P(OPENDAQ__CALL_TEMPLATED_T_U_DESERIALIZE_HANDLER_FUNC,                                                  \
                        OPENDAQ__NO_DELIM,                                                                                     \
                        OPENDAQ_FACTORY_INFO(LIBRARY_FACTORY, OPENDAQ_GET_FUNC_IMPLOBJ(Info), OPENDAQ_GET_FUNC_INTF(Info)),              \
                        FuncName,                                                                                         \
                        OPENDAQ_GET_FUNC_IMPLOBJ(Info),                                                                        \
                        Types)                                                                                            \
        }                                                                                                                 \
    };                                                                                                                    \
    static ImplName##Factory ImplName##FactoryRegistrator;

//// FUNCTION

#define OPENDAQ_REGISTER_DESERIALIZE_FACTORY_FUNC(type, func)                                                                  \
    class type##Factory                                                                                                   \
    {                                                                                                                     \
    public:                                                                                                               \
        type##Factory()                                                                                                   \
        {                                                                                                                 \
            daq::daqRegisterSerializerFactory(##type::SerializeId(), func);                                                 \
        }                                                                                                                 \
    };                                                                                                                    \
    static type##Factory g##type##Factory;

END_NAMESPACE_OPENDAQ
