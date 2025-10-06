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
#include <memory>
#include <string>
#include <thread>

#include <boost/asio/io_context.hpp>
#include <boost/system/error_code.hpp>

#include <opendaq/opendaq.h>
#include <opendaq/server_impl.h>

#include <ws-streaming/ws-streaming.hpp>

#include <websocket_streaming/common.h>

BEGIN_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING

class WsStreamingServer : public Server
{
    public:

        static constexpr const char *ID = "OpenDAQLTStreaming";

        static PropertyObjectPtr createDefaultConfig(
            const ContextPtr& context);

        static ServerTypePtr createType(
            const ContextPtr& context);

        static PropertyObjectPtr populateDefaultConfig(
            const PropertyObjectPtr& config,
            const ContextPtr& context);

        explicit WsStreamingServer(
            const InstancePtr& instance);

        explicit WsStreamingServer(
            const DevicePtr& rootDevice,
            const PropertyObjectPtr& config,
            const ContextPtr& context);

        ~WsStreamingServer();

        wss::server& getWsServer() noexcept;

    protected:

        PropertyObjectPtr getDiscoveryConfig() override;

        void onStopServer() override;

    private:

        static void populateDefaultConfigFromProvider(
            const ContextPtr& context,
            const PropertyObjectPtr& config);

        void addCapability();

        void createListener(const SignalPtr& signal);

        void onCoreEvent(
            ComponentPtr& component,
            CoreEventArgsPtr& args);

        void onComponentAdded(
            ComponentPtr& component,
            CoreEventArgsPtr& args);

        void onComponentRemoved(
            ComponentPtr& component,
            CoreEventArgsPtr& args);

        void onComponentUpdateEnd(
            ComponentPtr& component,
            CoreEventArgsPtr& args);

        void onAttributeChanged(
            ComponentPtr& component,
            CoreEventArgsPtr& args);

        void rescan();

    private:

        struct StreamableSignal
        {
            StreamableSignal(
                    const std::string& name,
                    const wss::metadata& metadata,
                    SignalPtr openDaqSignal)
                : localSignal(name, metadata)
                , openDaqSignal(openDaqSignal)
            {
            }

            wss::local_signal localSignal;
            SignalPtr openDaqSignal;
            InputPortNotificationsPtr listener;
            SignalPtr domainSignal;
        };

        DevicePtr _rootDevice;
        boost::asio::io_context _ioc;
        wss::server _server;
        std::thread _thread;
        std::map<std::string, StreamableSignal> _localSignals;
        Int _port;
};

END_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING
