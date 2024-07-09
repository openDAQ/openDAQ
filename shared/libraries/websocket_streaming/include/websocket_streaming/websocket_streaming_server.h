/*
 * Copyright 2022-2024 openDAQ d. o. o.
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
#include <opendaq/device_ptr.h>
#include <opendaq/instance_ptr.h>
#include "websocket_streaming/streaming_server.h"
#include "websocket_streaming/async_packet_reader.h"

BEGIN_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING

class WebsocketStreamingServer
{
public:
    WebsocketStreamingServer(const InstancePtr& instance);
    WebsocketStreamingServer(const DevicePtr& device, const ContextPtr& context);
    ~WebsocketStreamingServer();

    void setStreamingPort(uint16_t port);
    void setControlPort(uint16_t port);
    void start();
    void stop();

protected:
    DevicePtr device;
    ContextPtr context;

    uint16_t streamingPort = 0;
    uint16_t controlPort = 0;
    daq::websocket_streaming::StreamingServer streamingServer;
    daq::websocket_streaming::AsyncPacketReader packetReader;

private:
    void stopInternal();
};

END_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING
