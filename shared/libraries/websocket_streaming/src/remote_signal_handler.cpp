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

#include <cstddef>
#include <cstdint>
#include <functional>

#include <opendaq/opendaq.h>

#include <ws-streaming/ws-streaming.hpp>

#include <websocket_streaming/common.h>
#include <websocket_streaming/remote_signal_handler.h>

using namespace std::placeholders;

BEGIN_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING

RemoteSignalHandler::RemoteSignalHandler(
        wss::remote_signal_ptr remoteSignal)
    : _remoteSignal(remoteSignal)
{
}

RemoteSignalHandler::~RemoteSignalHandler()
{
}

void RemoteSignalHandler::attach()
{
    _onSubscribed = _remoteSignal->on_subscribed.connect(
        std::bind(
            &RemoteSignalHandler::onSubscribed,
            shared_from_this()));

    _onMetadataChanged = _remoteSignal->on_metadata_changed.connect(
        std::bind(
            &RemoteSignalHandler::onMetadataChanged,
            shared_from_this()));

    _onDataReceived = _remoteSignal->on_data_received.connect(
        std::bind(
            &RemoteSignalHandler::onDataReceived,
            shared_from_this(),
            _1,
            _2,
            _3,
            _4));

    _remoteSignal->subscribe();
}

const wss::remote_signal_ptr& RemoteSignalHandler::signal() const noexcept
{
    return _remoteSignal;
}

void RemoteSignalHandler::onSubscribed()
{
}

void RemoteSignalHandler::onMetadataChanged()
{
    if (_signal.assigned())
    {
        // XXX TODO update signal descriptor!
    }

    else
    {
        auto result = onSignalReady();
        if (!result.has_value())
            return;

        _signal = result.value().first;
        _domainSignal = result.value().second;

        if (!_signal.assigned())
            return;
    }
}

void RemoteSignalHandler::onDataReceived(
    std::int64_t domainValue,
    std::size_t sampleCount,
    const void *data,
    std::size_t size)
{
    if (!_signal.assigned())
        return;

    if (_domainSignal.assigned())
    {
        auto domainPacket = DataPacket(_domainSignal.getDescriptor(), sampleCount, domainValue);
        auto packet = DataPacketWithDomain(domainPacket, _signal.getDescriptor(), sampleCount);

        std::memcpy(
            packet.getRawData(),
            data,
            std::min(
                size,
                packet.getRawDataSize()));

        _domainSignal.sendPacket(domainPacket);
        _signal.sendPacket(packet);
    }

    else
    {
        auto packet = DataPacket(_signal.getDescriptor(), 0);
        _signal.sendPacket(packet);
    }
}

END_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING
