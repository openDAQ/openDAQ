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
#include <memory>
#include <utility>

#include <boost/signals2/connection.hpp>
#include <boost/signals2/signal.hpp>

#include <opendaq/component_ptr.h>
#include <opendaq/context_ptr.h>
#include <opendaq/signal_factory.h>

#include <ws-streaming/ws-streaming.hpp>

#include <websocket_streaming/common.h>

BEGIN_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING

class RemoteSignalHandler
    : public std::enable_shared_from_this<RemoteSignalHandler>
{
    public:

        RemoteSignalHandler(wss::remote_signal_ptr remoteSignal);

        ~RemoteSignalHandler();

        void attach();

        const wss::remote_signal_ptr& signal() const noexcept;

        boost::signals2::signal<
            std::pair<SignalConfigPtr, SignalConfigPtr>()
        > onSignalReady;

    private:

        void onSubscribed();

        void onMetadataChanged();

        void onDataReceived(
            std::int64_t domainValue,
            std::size_t sampleCount,
            const void *data,
            std::size_t size);

        wss::remote_signal_ptr _remoteSignal;
        boost::signals2::scoped_connection _onSubscribed;
        boost::signals2::scoped_connection _onMetadataChanged;
        boost::signals2::scoped_connection _onDataReceived;

        SignalConfigPtr _signal;
        SignalConfigPtr _domainSignal;
};

END_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING
