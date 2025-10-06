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

#include <list>
#include <map>
#include <string>

#include <boost/signals2/connection.hpp>

#include <opendaq/device_impl.h>
#include <opendaq/opendaq.h>

#include <ws-streaming/remote_signal.hpp>

#include <websocket_streaming/common.h>

BEGIN_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING

/*!
 * @brief A mirrored device object for remote devices connected via the WebSocket Streaming
 *     Protocol.
 *
 * This object wraps an underlying WsStreaming object to manage the connection to the remote
 * device. The WsStreaming object can either be created by the constructor, or created externally
 * and passed to the constructor.
 *
 * XXX TODO: Then what happens?
 */
class WsStreamingDevice : public Device
{
    public:

        /*!
         * @brief Creates an openDAQ device type for the old-style `daq.ws://` connection string.
         *
         * @return An openDAQ device type for the old-style `daq.ws://` connection string.
         */
        static DeviceTypePtr createOldType();

        /*!
         * @brief Creates an openDAQ device type for the new-style `daq.lt://` connection string.
         *
         * @return An openDAQ device type for the new-style `daq.lt://` connection string.
         */
        static DeviceTypePtr createNewType();

        /*!
         * @brief Opens a new WebSocket streaming connection.
         *
         * This constructor creates a new WsStreaming object using the specified connection
         * string. This begins the TCP connection attempt. Event handlers are attached such that
         * when the connection is established and remote signals become available, mirrored signal
         * objects are created for them.
         *
         * @param context The openDAQ context object.
         * @param parent The openDAQ parent component.
         * @param localId The local ID of this component.
         * @param connectionString The connection string, which must have a `daq.ws://` or
         *     `daq.lt://` prefix.
         */
        explicit WsStreamingDevice(
            const ContextPtr& context,
            const ComponentPtr& parent,
            const StringPtr& localId,
            const StringPtr& connectionString);

    protected:

        void removed() override;

        DeviceInfoPtr onGetInfo() override;

        void onSignalAvailable(
            wss::remote_signal_ptr signal,
            const DataDescriptorPtr& valueDescriptor,
            const DataDescriptorPtr& domainDescriptor);

        void onSignalUnavailable(
            wss::remote_signal_ptr signal);

        StringPtr connectionString;
        StreamingPtr streaming;

        std::list<boost::signals2::scoped_connection> streamingEvents;
        std::map<std::string, MirroredSignalPrivatePtr> streamingSignals;
};

END_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING
