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
#include <opendaq/context.h>
#include <opendaq/module_manager.h>
#include <coretypes/common.h>
#include <coretypes/stringobject.h>

#if __cplusplus
    #define EXPORT_C extern "C"
#else
    #define EXPORT_C
#endif

#if !defined(_WIN32)
    #define OPENDAQ_MODULE_API EXPORT_C __attribute__ ((visibility("default")))
#elif defined(OPENDAQ_MODULE_EXPORTS)
    #define OPENDAQ_MODULE_API EXPORT_C __declspec(dllexport)
#else
    #define OPENDAQ_MODULE_API EXPORT_C __declspec(dllimport)
#endif

#define DECLARE_MODULE_EXPORTS_WITHOUT_ID() \
    OPENDAQ_MODULE_API daq::ErrCode createModule(daq::IModule** module, daq::IContext* context);     \
    OPENDAQ_MODULE_API daq::ErrCode daqGetObjectCount(daq::SizeT* objCount);                         \
    OPENDAQ_MODULE_API daq::ErrCode checkDependencies(daq::IString** errMsg);                        \
                                                                                                                                 
#define DECLARE_MODULE_EXPORTS(moduleId)                                                             \
    DECLARE_MODULE_EXPORTS_WITHOUT_ID()                                                              \
    OPENDAQ_MODULE_API daq::ErrCode create##moduleId(daq::IModule** module, daq::IContext* context); \

