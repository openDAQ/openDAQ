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

#include <map>
#include <string>
#include <thread>

#include <boost/asio/io_context.hpp>

#include <coretypes/intfs.h>
#include <opendaq/device_ptr.h>
#include <opendaq/server.h>
#include <opendaq/server_impl.h>

#include <ws-streaming/ws-streaming.hpp>

#include <newer_websocket_streaming_server_module/common.h>

BEGIN_NAMESPACE_OPENDAQ_NEWER_WEBSOCKET_STREAMING_SERVER_MODULE

class NewerWebsocketStreamingServerImpl : public Server
{
    public:

        explicit NewerWebsocketStreamingServerImpl(
            const DevicePtr& rootDevice,
            const PropertyObjectPtr& config,
            const ContextPtr& context);

        static PropertyObjectPtr createDefaultConfig(const ContextPtr& context);

        static ServerTypePtr createType(const ContextPtr& context);

        static PropertyObjectPtr populateDefaultConfig(const PropertyObjectPtr& config, const ContextPtr& context);

    protected:

        PropertyObjectPtr getDiscoveryConfig() override;

        void onStopServer() override;

    private:

        static void populateDefaultConfigFromProvider(const ContextPtr& context, const PropertyObjectPtr& config);

        void createListener(const SignalPtr& signal);

    private:

        struct StreamableSignal
        {
            StreamableSignal(const std::string& name, const wss::metadata& metadata)
                : localSignal(name, metadata)
            {
            }

            wss::local_signal localSignal;
            InputPortNotificationsPtr listener;
        };

        boost::asio::io_context _ioc;
        wss::server _server;
        std::thread _thread;
        std::map<std::string, StreamableSignal> _signals;
};

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    INTERNAL_FACTORY, NewerWebsocketStreamingServer, IServer,
    DevicePtr, rootDevice,
    PropertyObjectPtr, config,
    const ContextPtr&, context)

END_NAMESPACE_OPENDAQ_NEWER_WEBSOCKET_STREAMING_SERVER_MODULE
