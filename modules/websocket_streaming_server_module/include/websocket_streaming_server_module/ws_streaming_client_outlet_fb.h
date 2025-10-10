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

#include <websocket_streaming/remote_signal_handler.h>

#include <websocket_streaming_server_module/common.h>

using namespace daq::websocket_streaming;

BEGIN_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING_SERVER_MODULE

/*!
 * @brief A function block which publishes data received from a single WebSocket Streaming client.
 *
 * The function block is constructed (normally by an WsStreamingServerOutletFb) with a
 * ws-streaming connection object. Each WebSocket Streaming signal published by the client causes
 * the function block instance to create a corresponding openDAQ signal. All client-published
 * signals are subscribed to, and data arriving from the client causes corresponding data packets
 * to be transmitted over the openDAQ Signals.
 *
 * WsStreamingClientOutletFb instances are intended to be nested inside a
 * WsStreamingServerOutletFb function block instance.
 */
class WsStreamingClientOutletFb : public FunctionBlock
{
    public:

        /*!
         * @brief A string identifying the type of this function block.
         */
        static constexpr const char *ID = "WsStreamingClientOutletFb";

        /*!
         * @brief A FunctionBlockType object describing this function block type.
         */
        static const FunctionBlockTypePtr TYPE;

        /*!
         * @brief Constructs a new function block.
         *
         * This function attaches event handlers to the specified @p connection so that
         * member functions will be called when signals become or are no longer available from
         * the remote peer.
         *
         * @todo When called for an already-established connection, any signals already available
         *     from the remote peer are missed. Only signals that become available after this call
         *     will be detected.
         */
        explicit WsStreamingClientOutletFb(
            const ContextPtr& context,
            const ComponentPtr& parent,
            wss::connection_ptr connection);

    protected:

        /*!
         * @brief Detaches event handlers from the ws-streaming connection object.
         */
        void removed() override;

    private:

        static StringPtr calculateLocalId(const wss::connection_ptr& connection);

        void onSignalAvailable(wss::remote_signal_ptr signal);

        std::pair<SignalConfigPtr, SignalConfigPtr>
        onSignalReady(std::shared_ptr<RemoteSignalHandler> handler);

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

END_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING_SERVER_MODULE
