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

#include <boost/signals2/connection.hpp>
#include <boost/system/error_code.hpp>

#include <opendaq/function_block_impl.h>
#include <opendaq/opendaq.h>

#include <ws-streaming/connection.hpp>

#include <websocket_streaming_server_module/common.h>
#include <websocket_streaming_server_module/ws_streaming_server.h>

BEGIN_NAMESPACE_OPENDAQ_NEWER_WEBSOCKET_STREAMING_SERVER_MODULE

/**
 * A function block which publishes data received from clients connected to a WebSocket Streaming
 * server. The function block is constructed with a server object, and attaches event handlers to
 * the server's underlying ws-server object events so that nested WsClientFb instances can be
 * created to manage each connected client.
 */
class WsClientSignalProvider : public FunctionBlock
{
    public:

        static constexpr const char *ID = "WsClientSignalProvider";

        static FunctionBlockTypePtr createType();

        explicit WsClientSignalProvider(
            const ContextPtr& ctx,
            const ComponentPtr& parent,
            const StringPtr& localId);

    protected:

        void removed() override;

    private:

        void onClientConnected(wss::connection_ptr connection);
        void onClientDisconnected(wss::connection_ptr connection);
        void onServerClosed(const boost::system::error_code& ec);

    private:

        std::map<wss::connection *, FunctionBlockPtr> _clientFbs;
        boost::signals2::scoped_connection _onClientConnected;
        boost::signals2::scoped_connection _onClientDisconnected;
        boost::signals2::scoped_connection _onServerClosed;
};

END_NAMESPACE_OPENDAQ_NEWER_WEBSOCKET_STREAMING_SERVER_MODULE
