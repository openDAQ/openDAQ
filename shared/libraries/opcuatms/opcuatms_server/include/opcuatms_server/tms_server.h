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
#include <opcuatms/opcuatms.h>
#include <opendaq/device_ptr.h>
#include <opendaq/instance_ptr.h>
#include <opcuaserver/opcuaserver.h>
#include <opcuatms_server/objects/tms_server_device.h>
#include <opcuatms_server/tms_server_context.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

class TmsServer
{
public:
    TmsServer(const InstancePtr& instance);
    TmsServer(const DevicePtr& device, const ContextPtr& context);
    ~TmsServer();

    void setOpcUaPort(uint16_t port);
    void setOpcUaPath(const std::string& path);
    void start();
    void stop();

protected:
    DevicePtr device;
    ContextPtr context;
    std::unique_ptr<daq::opcua::tms::TmsServerDevice> tmsDevice;
    std::shared_ptr<daq::opcua::tms::TmsServerContext> tmsContext;
    daq::opcua::OpcUaServerPtr server;
    uint16_t opcUaPort = 4840;
    std::string opcUaPath = "/";
    std::unordered_map<std::string, SizeT> registeredClientIds;
};

END_NAMESPACE_OPENDAQ_OPCUA
