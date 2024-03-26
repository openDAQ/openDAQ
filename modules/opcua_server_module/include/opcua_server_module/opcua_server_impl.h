/*
 * Copyright 2022-2024 Blueberry d.o.o.
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
#include <opcua_server_module/common.h>
#include <opendaq/device_ptr.h>
#include <opendaq/server.h>
#include <opendaq/server_impl.h>
#include <coretypes/intfs.h>
#include <opcuatms_server/tms_server.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA_SERVER_MODULE

class OpcUaServerImpl : public daq::Server
{
public:
    explicit OpcUaServerImpl(daq::DevicePtr rootDevice, PropertyObjectPtr config, const ContextPtr& context);
    static PropertyObjectPtr createDefaultConfig();
    static ServerTypePtr createType();

protected:
    void onStopServer() override;

    daq::opcua::TmsServer server;
    PropertyObjectPtr config;
    ContextPtr context;
};

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    INTERNAL_FACTORY, OpcUaServer, daq::IServer,
    DevicePtr, rootDevice,
    PropertyObjectPtr, config,
    const ContextPtr&, context
)

END_NAMESPACE_OPENDAQ_OPCUA_SERVER_MODULE
