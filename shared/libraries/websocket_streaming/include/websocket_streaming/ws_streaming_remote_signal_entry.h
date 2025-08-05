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

#include <opendaq/opendaq.h>

#include <boost/signals2/connection.hpp>

#include <ws-streaming/remote_signal.hpp>

#include <websocket_streaming/common.h>

BEGIN_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING

struct WsStreamingRemoteSignalEntry
{
    /**
     * The ws-streaming library's remote signal object. This object is created by ws-streaming
     * when the remote peer advertises a signal as 'available'. It is released (but not
     * necessarily destroyed, because it's a shared pointer) when the signal becomes 'unavailable'
     * or the connection closes.
     */
    wss::remote_signal_ptr ptr;

    /**
     * The current openDAQ descriptor for the signal. We generate this each time the signal's
     * metadata changes.
     */
    DataDescriptorPtr descriptor;

    /**
     * A pointer to the corresponding domain signal. When a signal's metadata changes, we inspect
     * the new metadata's "table ID." If this ID refers to a different signal that we already know
     * about, we update this value; otherwise, we set it to nullptr to indicate there is no known
     * associated domain signal.
     */
    std::shared_ptr<WsStreamingRemoteSignalEntry> domainEntry;

    /**
     * The last packet published for this signal. We must keep track of this in case this signal
     * is a domain signal referenced by another signal.
     */
    DataPacketPtr lastPacket;

    boost::signals2::scoped_connection onSubscribed;        /**< An RAII tracker for the connection to the signal object's `on_subscribed` event. */
    boost::signals2::scoped_connection onMetadataChanged;   /**< An RAII tracker for the connection to the signal object's `on_metadata_changed` event. */
    boost::signals2::scoped_connection onDataReceived;      /**< An RAII tracker for the connection to the signal object's `on_data_received` event. */
    boost::signals2::scoped_connection onUnsubscribed;      /**< An RAII tracker for the connection to the signal object's `on_unsubscribed` event. */

    bool isPublished = false;
    bool isSubscribed = false;
};

END_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING
