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

#include <cstddef>
#include <cstdint>
#include <future>
#include <map>
#include <memory>
#include <string>
#include <thread>

#include <boost/asio/io_context.hpp>
#include <boost/signals2/connection.hpp>
#include <boost/signals2/signal.hpp>
#include <boost/system/error_code.hpp>

#include <opendaq/opendaq.h>
#include <opendaq/streaming_impl.h>

#include <ws-streaming/client.hpp>
#include <ws-streaming/connection.hpp>
#include <ws-streaming/remote_signal.hpp>

#include <websocket_streaming/common.h>
#include <websocket_streaming/ws_streaming_remote_signal_entry.h>

BEGIN_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING

/*!
 * @brief An openDAQ streaming object based on the WebSocket Streaming Protocol.
 *
 * This object uses the ws-streaming library to establish a WebSocket streaming connection to a
 * remote peer.
 *
 * openDAQ requires signal objects to have a valid descriptor, even if they are not connected. In
 * contrast, the WebSocket Streaming protocol only provides signal metadata after a signal has
 * been subscribed. To solve this, the streaming object does not immediately register signals with
 * openDAQ when they become available. Instead, it does an initial subscribe. When the signal's
 * metadata is received, the signal is then unsubscribed, and registered with openDAQ using
 * addToAvailableSignals() now that its metadata is known and a valid descriptor can be created.
 *
 * Once registered with openDAQ, the onAddSignal() and onRemoveSignal() functions are implemented
 * to manage the subscription state of each known signal. When data is received for an active
 * signal, openDAQ packets are created and passed into openDAQ via onPacket().
 *
 * This object also adapts the WebSocket Streaming Protocol linear domain concept to openDAQ's.
 * Specifically, in the WebSocket Streaming Protocol, data is not normally transmitted for a
 * linear-rule domain signal. Instead the domain value is implicitly calculated based on a sample
 * counter and the linear rule parameters. The streaming object understands this and handles the
 * creation of synthetic domain packets.
 *
 * This object creates a Boost.Asio I/O context (which is required by the ws-streaming library),
 * and manages a std::thread to pump it. The thread is automatically stopped and destroyed when
 * the streaming object is destroyed.
 */
class WsStreaming : public Streaming
{
    public:

        /*!
         * @brief Creates an openDAQ streaming type object using the `daq.lt://` prefix.
         *
         * @return An openDAQ streaming type object using the `daq.lt://` prefix.
         */
        static StreamingTypePtr createType();

    public:

        /*!
         * @brief Constructs a streaming object and initiates a connection to the remote peer.
         *
         * @param connectionString The openDAQ connection string, which must use the `daq.lt://`
         *     prefix. The remote peer address and TCP port number are parsed from the connection
         *     string.
         * @param context The openDAQ context object.
         */
        explicit WsStreaming(
            const StringPtr& connectionString,
            const ContextPtr& context);

        /*!
         * @brief Destroys a streaming object and stops the Boost.Asio I/O context's thread.
         */
        ~WsStreaming();

        /*!
         * @brief An event raised when a signal becomes available.
         *
         * This event is only raised after an initial subscribe has acquired the metadata for a
         * signal. Refer to the class documentation for details.
         *
         * @param signal The ws-streaming library's remote signal object.
         * @param domainSignal The ws-streaming library's remote signal object for the descriptor,
         *     or nullptr if there is no associated domain signal.
         * @param descriptor The openDAQ descriptor for the signal.
         */
        boost::signals2::signal<
            void(wss::remote_signal_ptr signal,
                wss::remote_signal_ptr domainSignal,
                const DataDescriptorPtr& descriptor)
        > onSignalAvailable;

        /**
         * @brief An event raised when a signal is no longer available.
         *
         * @param signal The ws-streaming library's remote signal object.
         */
        boost::signals2::signal<
            void(wss::remote_signal_ptr signal)
        > onSignalUnavailable;

    protected:

        static PropertyObjectPtr createDefaultConfig();

        void onSetActive(bool active) override;
        void onAddSignal(const MirroredSignalConfigPtr& signal) override;
        void onRemoveSignal(const MirroredSignalConfigPtr& signal) override;
        void onSubscribeSignal(const StringPtr& signalId) override;
        void onUnsubscribeSignal(const StringPtr& signalId) override;

    private:

        void onConnected(
            const boost::system::error_code& ec,
            wss::connection_ptr connection);

        void onRemoteSignalAvailable(wss::remote_signal_ptr signal);

        void onRemoteSignalSubscribed(std::weak_ptr<WsStreamingRemoteSignalEntry> weakEntry);

        void onRemoteSignalMetadataChanged(std::weak_ptr<WsStreamingRemoteSignalEntry> weakEntry);

        void onRemoteSignalDataReceived(
            std::weak_ptr<WsStreamingRemoteSignalEntry> weakEntry,
            std::int64_t domain_value,
            std::size_t sample_count,
            const void *data,
            std::size_t size);

        void onRemoteSignalUnsubscribed(std::weak_ptr<WsStreamingRemoteSignalEntry> weakEntry);

        void onRemoteSignalUnavailable(wss::remote_signal_ptr signal);

        boost::asio::io_context ioContext;
        std::thread thread;

        wss::client wsClient;
        wss::connection_ptr wsConnection;

        std::map<std::string, std::shared_ptr<WsStreamingRemoteSignalEntry>> signals;

        std::promise<boost::system::error_code> promise;
};

END_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING
