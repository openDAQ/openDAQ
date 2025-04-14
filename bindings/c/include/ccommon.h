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

#ifdef __cplusplus
extern "C"
{
#endif

#if !defined(_WIN32)
#define EXPORTED __attribute__((visibility("default")))
#elif defined(BUILDING_SHARED_LIBRARY)
#define EXPORTED __declspec(dllexport)
#else
#define EXPORTED __declspec(dllimport)
#endif

#include <stddef.h>
#include <stdint.h>

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
    typedef void BaseObject;

    const Bool True = 1;
    const Bool False = 0;

    enum CoreType
    {
        ctBool = 0,             ///< Boolean, True or False
        ctInt,                  ///< 64 bit signed integer
        ctFloat,                ///< IEEE 754 64 bit floating point
        ctString,               ///< UTF8 zero terminated string
        ctList,                 ///< List of IBaseObject
        ctDict,                 ///< Dictionary of (key: IBaseObject, value: IBaseObject)
        ctRatio,                ///< Rational number (numerator / denominator)
        ctProc,                 ///< Callback without return value
        ctObject,               ///< Generic object
        ctBinaryData,           ///< Binary buffer with predefined size
        ctFunc,                 ///< Callback with return value
        ctComplexNumber,        ///< Complex number (real, imaginary)
        ctStruct,               ///< Constant structure with dictionary of fields and types
        ctEnumeration,          ///< Enumeration representing a predefined set of named integral constants
        ctUndefined = 0xFFFF,   ///< Undefined
    };

    typedef ErrCode (*FuncCall)(BaseObject*, BaseObject**);
    typedef ErrCode (*ProcCall)(BaseObject*);
    typedef void (*EventCall)(BaseObject*, BaseObject*);

    typedef struct IntfID
    {
        uint32_t Data1;
        uint16_t Data2;
        uint16_t Data3;
        uint64_t Data4;
    } IntfID;

#include "ccoreobjects/common.h"

#include "copendaq/component/common.h"
#include "copendaq/context/common.h"
#include "copendaq/device/common.h"
#include "copendaq/functionblock/common.h"
#include "copendaq/logger/common.h"
#include "copendaq/modulemanager/common.h"
#include "copendaq/opendaq/common.h"
#include "copendaq/reader/common.h"
#include "copendaq/scheduler/common.h"
#include "copendaq/server/common.h"
#include "copendaq/signal/common.h"
#include "copendaq/streaming/common.h"
#include "copendaq/synchronization/common.h"
#include "copendaq/utility/common.h"

#ifdef __cplusplus
}
#endif
