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

#include <websocket_streaming/ws_streaming_server.h>

#include <websocket_streaming_server_module/common.h>

BEGIN_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING_SERVER_MODULE

/*!
 * @brief A function block which publishes data received from clients connected to a WebSocket
 *     Streaming server.
 *
 * Upon creation, this function block searches the openDAQ instance in which it was created for a
 * WsStreamingServer instance attached to the root device. If found, the function block attaches
 * to the server such that nested WsStreamingClientOutletFb function blocks are created for each
 * client connected to the server.
 */
class WsStreamingServerOutletFb : public FunctionBlock
{
    public:

        /*!
         * @brief A string identifying the type of this function block.
         */
        static constexpr const char *ID = "WsStreamingServerOutletFb";

        /*!
         * @brief Creates a FunctionBlockType object describing this function block type.
         *
         * @return A FunctionBlockType object describing this function block type.
         */
        static FunctionBlockTypePtr createType();

        /*!
         * @brief Constructs a new function block.
         *
         * This function searches the openDAQ instance in which it was created for a
         * WsStreamingServer instance attached to the root device. If found, the function block
         * attaches to the server such that nested WsStreamingClientOutletFb function blocks are
         * created for each client connected to the server.
         *
         * @param context The openDAQ context object.
         * @param parent The parent component of this function block.
         * @param localId The local identifier of this function block.
         */
        explicit WsStreamingServerOutletFb(
            const ContextPtr& context,
            const ComponentPtr& parent,
            const StringPtr& localId);

    protected:

        /*!
         * @brief Removes the nested WsStreamingClientOutletFb function blocks for all clients,
         *     and detaches event handlers from the WsStreamingServer.
         */
        void removed() override;

    private:

        // Creates nested FBs for all clients currently connected, and attaches event handlers
        // for any future connections. This function must be called from the execution context of
        // the underlying wss::server object in order to avoid race conditions.
        void attach();

        void onClientConnected(wss::connection_ptr connection);
        void onClientDisconnected(wss::connection_ptr connection);
        void onServerClosed(const boost::system::error_code& ec);

    private:

        // A pointer to the WsStreamingServer instance. We must hold this pointer to ensure
        // the server isn't destroyed while we have event handlers that reference it.
        ServerPtr _server;

        // A pointer to the underlying ws-streaming server object held by _server.
        wss::server *_wsServer = nullptr;

        // A lookup table of WsStreamingClientOutletFb instances for each known client.
        std::map<wss::connection *, FunctionBlockPtr> _clientFbs;

        // RAII objects to detach event handlers when the FB is destroyed.
        boost::signals2::scoped_connection _onClientConnected;
        boost::signals2::scoped_connection _onClientDisconnected;
        boost::signals2::scoped_connection _onServerClosed;
};

END_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING_SERVER_MODULE
