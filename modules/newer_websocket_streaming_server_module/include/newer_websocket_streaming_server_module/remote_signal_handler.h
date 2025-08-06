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

#include <memory>

#include <boost/signals2/connection.hpp>
#include <boost/signals2/signal.hpp>

#include <ws-streaming/ws-streaming.hpp>

#include <newer_websocket_streaming_server_module/common.h>

BEGIN_NAMESPACE_OPENDAQ_NEWER_WEBSOCKET_STREAMING_SERVER_MODULE

class RemoteSignalHandler : public std::enable_shared_from_this<RemoteSignalHandler>
{
    public:

        RemoteSignalHandler(wss::remote_signal_ptr& remoteSignal);
        ~RemoteSignalHandler();

        void attach();
        void detach();

        boost::signals2::signal<void()> on_todo;

    private:

        void onSubscribed();
        void onMetadataChanged();

        wss::remote_signal_ptr _remoteSignal;
        boost::signals2::scoped_connection _onSubscribed;
        boost::signals2::scoped_connection _onMetadataChanged;
        bool _subscribed = false;
};

END_NAMESPACE_OPENDAQ_NEWER_WEBSOCKET_STREAMING_SERVER_MODULE
