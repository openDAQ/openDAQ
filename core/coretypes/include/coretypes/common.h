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

/*!
 * @file common.h
 */

#pragma once
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <coretypes/constexpr_string.h>
#include <coretypes/type_name.h>
#include <coretypes/intfid.h>
#include <type_traits>
#include <limits>

/*!
 * @namespace daq
 * The openDAQ namespace. All interfaces and type definitions are within the
 * `daq` namespace.
 */
#define BEGIN_NAMESPACE_OPENDAQ \
    namespace daq          \
    {

#define END_NAMESPACE_OPENDAQ \
    }

#define BEGIN_NAMESPACE_OPENDAQ_MODULE(name) \
    namespace daq::modules::name         \
    {

#define END_NAMESPACE_OPENDAQ_MODULE \
    }

#define BEGIN_NAMESPACE_OPENDAQ_SEARCH \
    namespace daq::search \
    {

#define END_NAMESPACE_OPENDAQ_SEARCH \
    }

BEGIN_NAMESPACE_OPENDAQ

/*! Creates a type name "ErrCode" for uint32_t */
typedef uint32_t ErrCode;
typedef uint8_t Bool;
typedef int64_t Int;
typedef uint64_t UInt;
typedef double Float;
typedef char* CharPtr;
typedef const char* ConstCharPtr;
typedef void* VoidPtr;
typedef size_t SizeT;
typedef uint32_t EnumType;

const Bool True = 1;
const Bool False = 0;

const Float FloatEpsilon = Float(std::numeric_limits<float>::epsilon());

inline bool IsTrue(const Bool val)
{
    return val != False;
}

inline bool IsFalse(const Bool val)
{
    return val == False;
}

END_NAMESPACE_OPENDAQ

#if defined(OPENDAQ_SKIP_DLL_IMPORT)
    #define PUBLIC_EXPORT
#elif !defined(_WIN32)
    #define PUBLIC_EXPORT __attribute__ ((visibility ("default")))
#elif defined(BUILDING_SHARED_LIBRARY)
    #define PUBLIC_EXPORT __declspec(dllexport)
#else
    #define PUBLIC_EXPORT __declspec(dllimport)
#endif

#if !defined(_WIN32)
    #define INTERFACE_FUNC
#else
    #define INTERFACE_FUNC __stdcall
#endif

#if defined(_MSC_VER)
    #define OPENDAQ_NO_VTABLE __declspec(novtable)
    #define DAQ_EMPTY_BASES __declspec(empty_bases)
#else
    #define OPENDAQ_NO_VTABLE
    #define DAQ_EMPTY_BASES
#endif

#define EXPORT comment(linker, "/EXPORT:"__FUNCTION__"="__FUNCDNAME__)

#define INTERNAL_FACTORY
#define LIBRARY_FACTORY PUBLIC_EXPORT
#define INLINE_FACTORY inline

#define OPENDAQ_INTERFACE(...) __VA_ARGS__

// Have to implement "baseInterface" through indirection otherwise "using Base" becomes ambiguous
// as it might exist in both "baseInterface" and "interfaceName##Impl"

#define DECLARE_OPENDAQ_INTERFACE_EX(interfaceName, baseInterface) \
    struct interfaceName;                                     \
                                                              \
    namespace InterfaceBase                                   \
    {                                                         \
        struct OPENDAQ_NO_VTABLE DAQ_EMPTY_BASES interfaceName##Impl : public baseInterface     \
        {                                                     \
            using Base = baseInterface;                       \
            using Actual = interfaceName;                     \
        };                                                    \
    }                                                         \
                                                              \
    struct OPENDAQ_NO_VTABLE DAQ_EMPTY_BASES interfaceName : public InterfaceBase::interfaceName##Impl

#define DECLARE_OPENDAQ_INTERFACE(interfaceName, baseInterface) \
    struct interfaceName;                                  \
                                                           \
    namespace InterfaceBase                                \
    {                                                      \
        struct OPENDAQ_NO_VTABLE DAQ_EMPTY_BASES interfaceName##Impl : public baseInterface  \
        {                                                  \
            using Base = baseInterface;                    \
            using Actual = interfaceName;                  \
                                                           \
            DEFINE_INTFID(#interfaceName)                  \
        };                                                 \
    }                                                      \
                                                           \
    struct OPENDAQ_NO_VTABLE DAQ_EMPTY_BASES interfaceName : public InterfaceBase::interfaceName##Impl

#define DECLARE_TEMPLATED_OPENDAQ_INTERFACE_T(interfaceName, baseInterface) \
    template <typename T>                                              \
    struct interfaceName;                                              \
                                                                       \
    namespace InterfaceBase                                            \
    {                                                                  \
        template <typename T>                                          \
        struct interfaceName##Impl : public baseInterface              \
        {                                                              \
            using Base = baseInterface;                                \
            using Actual = interfaceName<T>;                           \
                                                                       \
            DEFINE_INTFID(#interfaceName, T)                           \
        };                                                             \
    }                                                                  \
                                                                       \
    template <typename T>                                              \
    struct interfaceName : public InterfaceBase::interfaceName##Impl<T>

#define DECLARE_TEMPLATED_OPENDAQ_INTERFACE_T_U(interfaceName, baseInterface) \
    template <typename T, typename U>                                    \
    struct interfaceName;                                                \
                                                                         \
    namespace InterfaceBase                                              \
    {                                                                    \
        template <typename T, typename U>                                \
        struct interfaceName##Impl : public baseInterface                \
        {                                                                \
            using Base = baseInterface;                                  \
            using Actual = interfaceName<T, U>;                          \
                                                                         \
            DEFINE_INTFID(#interfaceName, T, U)                          \
        };                                                               \
    }                                                                    \
                                                                         \
    template <typename T, typename U>                                    \
    struct interfaceName : public InterfaceBase::interfaceName##Impl<T, U>

#define DECLARE_OPENDAQ_CUSTOM_INTERFACE(interfaceName, baseInterface, namespaceName) \
    struct interfaceName;                                                        \
                                                                                 \
    namespace InterfaceBase                                                      \
    {                                                                            \
        struct OPENDAQ_NO_VTABLE DAQ_EMPTY_BASES interfaceName##Impl : public baseInterface      \
        {                                                                        \
            using Base = baseInterface;                                          \
            using Actual = interfaceName;                                        \
                                                                                 \
            DEFINE_CUSTOM_INTFID(#interfaceName, namespaceName)                  \
        };                                                                       \
    }                                                                            \
                                                                                 \
    struct OPENDAQ_NO_VTABLE DAQ_EMPTY_BASES interfaceName : public InterfaceBase::interfaceName##Impl

/**************************/

#if defined(OPENDAQ_DISABLE_CURRENT_FUNCTION)
    #define OPENDAQ_CURRENT_FUNCTION "Object"
#elif defined(__GNUC__) || (defined(__MWERKS__) && (__MWERKS__ >= 0x3000)) || (defined(__ICC) && (__ICC >= 600)) || defined(__ghs__)
    #define OPENDAQ_CURRENT_FUNCTION __PRETTY_FUNCTION__
#elif defined(__DMC__) && (__DMC__ >= 0x810)
    #define OPENDAQ_CURRENT_FUNCTION __PRETTY_FUNCTION__
#elif defined(__FUNCTION__)
    #define OPENDAQ_CURRENT_FUNCTION __FUNCTION__
#elif (defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 600)) || (defined(__IBMCPP__) && (__IBMCPP__ >= 500))
    #define OPENDAQ_CURRENT_FUNCTION __FUNCTION__
#elif defined(__BORLANDC__) && (__BORLANDC__ >= 0x550)
    #define OPENDAQ_CURRENT_FUNCTION __FUNC__
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901)
    #define OPENDAQ_CURRENT_FUNCTION __func__
#elif defined(__cplusplus) && (__cplusplus >= 201103)
    #define OPENDAQ_CURRENT_FUNCTION __func__
#else
    #define OPENDAQ_CURRENT_FUNCTION "Object"
#endif

/**************************/

#if defined(__x86_64__) || defined(__aarch64__) || defined(_M_AMD64)
    #define OPENDAQ_64 1
#endif

extern "C"
daq::ErrCode PUBLIC_EXPORT daqDuplicateCharPtrN(daq::ConstCharPtr source,
                                                daq::SizeT length,
                                                daq::CharPtr* dest);

extern "C"
daq::ErrCode PUBLIC_EXPORT daqDuplicateCharPtr(daq::ConstCharPtr source,
                                               daq::CharPtr* dest);

extern "C"
daq::ErrCode PUBLIC_EXPORT daqInterfaceIdToString(const daq::IntfID& iid, daq::CharPtr dest);

#define OPENDAQ_TYPE_TOSTRING                                             \
    static daq::ErrCode OpenDaqType(daq::CharPtr* str)                    \
    {                                                                     \
        constexpr const auto intfName = daq::typeNameQualified<Actual>(); \
        return daqDuplicateCharPtrN(                                      \
            intfName.data(),                                              \
            decltype(intfName)::Size(),                                   \
            str                                                           \
        );                                                                \
    }

#define CUSTOM_INTFID(interfaceName, namespaceName, ...) \
    daq::FromTemplatedTypeName<sizeof(interfaceName), sizeof(namespaceName), ##__VA_ARGS__>(interfaceName, namespaceName);

#define OPENDAQ_INTFID(interfaceName, ...) CUSTOM_INTFID(interfaceName, "daq", ##__VA_ARGS__)

#if defined(NDEBUG) || defined(DS_CI_RUNNER)
    #define DEFINE_INTFID(interfaceName, ...)                                                           \
        static constexpr daq::IntfID Id = OPENDAQ_INTFID(interfaceName, ##__VA_ARGS__)                  \
        OPENDAQ_TYPE_TOSTRING

    #define DEFINE_CUSTOM_INTFID(interfaceName, namespaceName, ...)                                     \
        static constexpr daq::IntfID Id = CUSTOM_INTFID(interfaceName, namespaceName, ##__VA_ARGS__);   \
        OPENDAQ_TYPE_TOSTRING
#else
    #define DEFINE_INTFID(interfaceName, ...)                                                           \
        static constexpr daq::IntfID Id = OPENDAQ_INTFID(interfaceName, ##__VA_ARGS__)                  \
        static constexpr auto GuidSource =                                                              \
            daq::interfaceGuidSource<sizeof(interfaceName), sizeof("daq"), ##__VA_ARGS__>(              \
                interfaceName,                                                                          \
                "daq"                                                                                   \
            );                                                                                          \
        OPENDAQ_TYPE_TOSTRING

    #define DEFINE_CUSTOM_INTFID(interfaceName, namespaceName, ...)                                     \
        static constexpr daq::IntfID Id = CUSTOM_INTFID(interfaceName, namespaceName, ##__VA_ARGS__);   \
        static constexpr auto GuidSource =                                                              \
            daq::interfaceGuidSource<sizeof(interfaceName), sizeof(namespaceName), ##__VA_ARGS__>(      \
                interfaceName,                                                                          \
                namespaceName                                                                           \
            );                                                                                          \
        OPENDAQ_TYPE_TOSTRING
#endif

#define DEFINE_EXTERNAL_INTFID(id)       \
    static constexpr daq::IntfID Id = id; \
    OPENDAQ_TYPE_TOSTRING

BEGIN_NAMESPACE_OPENDAQ

template <std::size_t N1, std::size_t N2, typename... TArgs>
constexpr daq::IntfID FromTemplatedTypeName(const char (&interfaceName)[N1], const char (&namespaceName)[N2])
{
    using namespace daq;

    if constexpr (sizeof...(TArgs) == 0)
    {
        return IntfID::FromTypeName(interfaceName, namespaceName);
    }
    else
    {
        return IntfID::FromTypeName(interfaceGuidSource<N1, N2, TArgs...>(interfaceName, namespaceName));
    }
}

template<typename T>
struct DependentFalse : std::false_type
{
};

END_NAMESPACE_OPENDAQ
