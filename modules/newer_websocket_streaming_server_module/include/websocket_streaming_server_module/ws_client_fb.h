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
#include <utility>

#include <boost/signals2/connection.hpp>
#include <boost/system/error_code.hpp>

#include <opendaq/function_block_impl.h>
#include <opendaq/opendaq.h>

#include <ws-streaming/connection.hpp>
#include <ws-streaming/remote_signal.hpp>

#include <websocket_streaming_server_module/common.h>
#include <websocket_streaming_server_module/remote_signal_handler.h>

BEGIN_NAMESPACE_OPENDAQ_NEWER_WEBSOCKET_STREAMING_SERVER_MODULE

/*!
 * A function block which publishes data received from a single WebSocket Streaming client. The
 * function block is constructed with a ws-streaming connection object. Each WebSocket Streaming
 * signal published by the client causes the function block instance to create a corresponding
 * openDAQ Signal. All client-published signals are subscribed to, and data arriving from the
 * client causes corresponding data packets to be transmitted over the openDAQ Signals.
 *
 * WsClientFb instances are intended to be nested inside a WsClientSignalProvider function block
 * instance.
 */
class WsClientFb : public FunctionBlock
{
    public:

        static constexpr const char *ID = "WsClientFb";

        static FunctionBlockTypePtr createType();

        explicit WsClientFb(
            const ContextPtr& ctx,
            const ComponentPtr& parent,
            wss::connection_ptr connection);

        void attach();

    protected:

        void removed() override;

    private:

        static StringPtr calculateLocalId(const wss::connection_ptr& connection);

        void onSignalAvailable(wss::remote_signal_ptr signal);

        std::pair<SignalConfigPtr, SignalConfigPtr>
        onSignalReady(std::shared_ptr<RemoteSignalHandler> handler );

        void onSignalUnavailable(wss::remote_signal_ptr signal);

        void onConnectionDisconnected(const boost::system::error_code& ec);

    private:

        wss::connection_ptr _connection;
        boost::signals2::scoped_connection _onSignalAvailable;
        boost::signals2::scoped_connection _onSignalUnavailable;
        boost::signals2::scoped_connection _onConnectionDisconnected;

        struct HandlerEntry
        {
            std::shared_ptr<RemoteSignalHandler> handler;
            boost::signals2::scoped_connection onSignalReady;
            SignalConfigPtr signal;
            SignalConfigPtr domainSignal;
        };

        std::map<wss::remote_signal *, HandlerEntry> _handlers;
};

END_NAMESPACE_OPENDAQ_NEWER_WEBSOCKET_STREAMING_SERVER_MODULE
