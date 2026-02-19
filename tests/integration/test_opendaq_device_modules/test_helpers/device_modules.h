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
#include <opendaq/instance_ptr.h>
#include <native_streaming_client_module/module_dll.h>
#include <native_streaming_server_module/module_dll.h>
#include <opcua_client_module/module_dll.h>
#include <opcua_server_module/module_dll.h>
#include <websocket_streaming_client_module/module_dll.h>
#include <websocket_streaming_server_module/module_dll.h>
#include <ref_device_module/module_dll.h>
#include <ref_fb_module/module_dll.h>

inline void addNativeServerModule(const daq::InstancePtr& instance)
{
    daq::ModulePtr nativeServerModule;
    createNativeStreamingServerModule(&nativeServerModule, instance.getContext());

    instance.getModuleManager().addModule(nativeServerModule);
}

inline void addNativeClientModule(const daq::InstancePtr& instance)
{
    daq::ModulePtr nativeClientModule;
    createNativeStreamingClientModule(&nativeClientModule, instance.getContext());

    instance.getModuleManager().addModule(nativeClientModule);
}

inline void addLtServerModule(const daq::InstancePtr& instance)
{
    daq::ModulePtr ltServerModule;
    createWebsocketStreamingServerModule(&ltServerModule, instance.getContext());

    instance.getModuleManager().addModule(ltServerModule);
}

inline void addLtClientModule(const daq::InstancePtr& instance)
{
    daq::ModulePtr ltClientModule;
    createWebsocketStreamingClientModule(&ltClientModule, instance.getContext());

    instance.getModuleManager().addModule(ltClientModule);
}

inline void addOpcuaServerModule(const daq::InstancePtr& instance)
{
    daq::ModulePtr opcuaServerModule;
    createOpcUaServerModule(&opcuaServerModule, instance.getContext());

    instance.getModuleManager().addModule(opcuaServerModule);
}

inline void addOpcuaClientModule(const daq::InstancePtr& instance)
{
    daq::ModulePtr opcuaClientModule;
    createOpcUaClientModule(&opcuaClientModule, instance.getContext());

    instance.getModuleManager().addModule(opcuaClientModule);
}

inline void addRefDeviceModule(const daq::InstancePtr& instance)
{
    daq::ModulePtr refDeviceModule;
    createRefDeviceModule(&refDeviceModule, instance.getContext());

    instance.getModuleManager().addModule(refDeviceModule);
}

inline void addRefFBModule(const daq::InstancePtr& instance)
{
    daq::ModulePtr refFBModule;
    createRefFBModule(&refFBModule, instance.getContext());

    instance.getModuleManager().addModule(refFBModule);
}
