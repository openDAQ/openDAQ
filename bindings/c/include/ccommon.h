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

    typedef uint32_t daqErrCode;
    typedef uint8_t daqBool;
    typedef int64_t daqInt;
    typedef uint64_t daqUInt;
    typedef double daqFloat;
    typedef char* daqCharPtr;
    typedef const char* daqConstCharPtr;
    typedef void* daqVoidPtr;
    typedef size_t daqSizeT;
    typedef uint32_t daqEnumType;
    typedef void daqBaseObject;

    const daqBool True = 1;
    const daqBool False = 0;

    typedef enum daqCoreType
    {
        daqCtBool = 0,             ///< Boolean, True or False
        daqCtInt,                  ///< 64 bit signed integer
        daqCtFloat,                ///< IEEE 754 64 bit floating point
        daqCtString,               ///< UTF8 zero terminated string
        daqCtList,                 ///< List of IBaseObject
        daqCtDict,                 ///< Dictionary of (key: IBaseObject, value: IBaseObject)
        daqCtRatio,                ///< Rational number (numerator / denominator)
        daqCtProc,                 ///< Callback without return value
        daqCtObject,               ///< Generic object
        daqCtBinaryData,           ///< Binary buffer with predefined size
        daqCtFunc,                 ///< Callback with return value
        daqCtComplexNumber,        ///< Complex number (real, imaginary)
        daqCtStruct,               ///< Constant structure with dictionary of fields and types
        daqCtEnumeration,          ///< Enumeration representing a predefined set of named integral constants
        daqCtUndefined = 0xFFFF,   ///< Undefined
    } daqCoreType;

    typedef daqErrCode (*daqFuncCall)(daqBaseObject*, daqBaseObject**);
    typedef daqErrCode (*daqProcCall)(daqBaseObject*);
    typedef void (*daqEventCall)(daqBaseObject*, daqBaseObject*);

    typedef struct daqIntfID
    {
        uint32_t Data1;
        uint16_t Data2;
        uint16_t Data3;
        uint64_t Data4;
    } daqIntfID;

#include <ccoreobjects/common.h>

#include <copendaq/component/common.h>
#include <copendaq/context/common.h>
#include <copendaq/device/common.h>
#include <copendaq/functionblock/common.h>
#include <copendaq/logger/common.h>
#include <copendaq/modulemanager/common.h>
#include <copendaq/opendaq/common.h>
#include <copendaq/reader/common.h>
#include <copendaq/scheduler/common.h>
#include <copendaq/server/common.h>
#include <copendaq/signal/common.h>
#include <copendaq/streaming/common.h>
#include <copendaq/synchronization/common.h>

#ifdef __cplusplus
}
#endif
