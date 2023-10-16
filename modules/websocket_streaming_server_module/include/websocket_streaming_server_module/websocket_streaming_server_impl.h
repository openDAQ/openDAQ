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
#include <websocket_streaming_server_module/common.h>
#include <opendaq/device_ptr.h>
#include <opendaq/server.h>
#include <opendaq/server_impl.h>
#include <coretypes/intfs.h>
#include <websocket_streaming/websocket_streaming_server.h>

BEGIN_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING_SERVER_MODULE

class WebsocketStreamingServerImpl : public daq::Server
{
public:
    explicit WebsocketStreamingServerImpl(daq::DevicePtr rootDevice, PropertyObjectPtr config, const ContextPtr& context);
    static PropertyObjectPtr createDefaultConfig();
    static ServerTypePtr createType();

protected:
    void onStopServer() override;

    daq::websocket_streaming::WebsocketStreamingServer websocketStreamingServer;
    PropertyObjectPtr config;
};

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    INTERNAL_FACTORY, WebsocketStreamingServer, daq::IServer,
    DevicePtr, rootDevice,
    PropertyObjectPtr, config,
    const ContextPtr&, context
)

END_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING_SERVER_MODULE
