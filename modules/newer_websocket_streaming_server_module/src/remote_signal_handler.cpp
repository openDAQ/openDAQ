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

#include <functional>
#include <iostream>

#include <ws-streaming/ws-streaming.hpp>

#include <newer_websocket_streaming_server_module/common.h>
#include <newer_websocket_streaming_server_module/remote_signal_handler.h>

BEGIN_NAMESPACE_OPENDAQ_NEWER_WEBSOCKET_STREAMING_SERVER_MODULE

RemoteSignalHandler::RemoteSignalHandler(wss::remote_signal_ptr& remoteSignal)
    : _remoteSignal(remoteSignal)
{
    std::cout << "RemoteSignalHandler created at " << this << " with remoteSignal " << _remoteSignal.get() << std::endl;
}

RemoteSignalHandler::~RemoteSignalHandler()
{
    std::cout << "RemoteSignalHandler destroyed at " << this << " with remoteSignal " << _remoteSignal.get() << std::endl;
}

void RemoteSignalHandler::attach()
{
    std::cout << "RemoteSignalHandler at " << this << " with remoteSignal " << _remoteSignal.get() << " attached" << std::endl;

    _onSubscribed = _remoteSignal->on_subscribed.connect(
        std::bind(
            &RemoteSignalHandler::onSubscribed,
            shared_from_this()));

    _onMetadataChanged = _remoteSignal->on_metadata_changed.connect(
        std::bind(
            &RemoteSignalHandler::onMetadataChanged,
            shared_from_this()));

    _subscribed = true;
    _remoteSignal->subscribe();
}

void RemoteSignalHandler::detach()
{
    std::cout << "RemoteSignalHandler at " << this << " with remoteSignal " << _remoteSignal.get() << " detached" << std::endl;

    _onSubscribed.disconnect();
    _onMetadataChanged.disconnect();

    if (_subscribed)
    {
        _subscribed = false;
        _remoteSignal->unsubscribe();
    }
}

void RemoteSignalHandler::onSubscribed()
{
    std::cout << "RemoteSignalHandler at " << this << " with remoteSignal " << _remoteSignal.get() << " onSubscribed()" << std::endl;
    _remoteSignal->unsubscribe();
}

void RemoteSignalHandler::onMetadataChanged()
{
}

END_NAMESPACE_OPENDAQ_NEWER_WEBSOCKET_STREAMING_SERVER_MODULE
