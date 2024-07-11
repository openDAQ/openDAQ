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

#pragma once
#include <coretypes/version.h>
#include <coretypes/coretypes_config.h>

#if defined(OPENDAQ_LINKS_CORE_OBJECTS)
    #include <coreobjects/version.h>
    #include <coreobjects/coreobjects_config.h>
#endif

#if defined(OPENDAQ_LINKS_OPENDAQ)
    #include <opendaq/version.h>
    #include <opendaq/opendaq_config.h>
    #include <opendaq/module_manager_errors.h>
    #include <opendaq/module_check_dependencies.h>
#endif

#define DEFINE_MODULE_EXPORTS(moduleImpl)                                               \
    std::atomic<std::size_t> daq::daqSharedLibObjectCount(0);                           \
                                                                                        \
    OPENDAQ_MODULE_API daq::ErrCode createModule(daq::IModule** module,                 \
                                                daq::IContext* context)                 \
    {                                                                                   \
        return daq::createObject<daq::IModule, moduleImpl>(                             \
            module,                                                                     \
            context                                                                     \
        );                                                                              \
    }                                                                                   \
                                                                                        \
    OPENDAQ_MODULE_API daq::ErrCode create##moduleImpl(daq::IModule** module,           \
                                                      daq::IContext* context)           \
    {                                                                                   \
        return daq::createObject<daq::IModule, moduleImpl>(                             \
            module,                                                                     \
            context                                                                     \
        );                                                                              \
    }                                                                                   \
                                                                                        \
    OPENDAQ_MODULE_API daq::ErrCode daqGetObjectCount(daq::SizeT* count)                \
    {                                                                                   \
        *count = daq::daqSharedLibObjectCount;                                          \
        return OPENDAQ_SUCCESS;                                                         \
    }

// ReSharper disable once CppNonInlineFunctionDefinitionInHeaderFile
OPENDAQ_MODULE_API daq::ErrCode checkDependencies(daq::IString** errMsg)
{
#if defined(OPENDAQ_LINKS_CORE_OBJECTS)
    daq::LibraryVersion version{
        OPENDAQ_CORETYPES_MAJOR_VERSION,
        OPENDAQ_CORETYPES_MINOR_VERSION,
        OPENDAQ_CORETYPES_PATCH_VERSION,
    };

    if (!isCompatibleVersion("CoreTypes", daqCoreTypesGetVersion, version, errMsg))
    {
        return OPENDAQ_ERR_MODULE_INCOMPATIBLE_DEPENDENCIES;
    }

    version.major = OPENDAQ_COREOBJECTS_MAJOR_VERSION;
    version.minor = OPENDAQ_COREOBJECTS_MINOR_VERSION;
    version.patch = OPENDAQ_COREOBJECTS_PATCH_VERSION;
    if (!isCompatibleVersion("CoreObjects", daqCoreObjectsGetVersion, version, errMsg))
    {
        return OPENDAQ_ERR_MODULE_INCOMPATIBLE_DEPENDENCIES;
    }
#endif

#if defined(OPENDAQ_LINKS_OPENDAQ)

    version.major = OPENDAQ_OPENDAQ_MAJOR_VERSION;
    version.minor = OPENDAQ_OPENDAQ_MINOR_VERSION;
    version.patch = OPENDAQ_OPENDAQ_PATCH_VERSION;
    if (!isCompatibleVersion("OpenDaq", daqOpenDaqGetVersion, version, errMsg))
    {
        return OPENDAQ_ERR_MODULE_INCOMPATIBLE_DEPENDENCIES;
    }
#endif

    return OPENDAQ_SUCCESS;
}
