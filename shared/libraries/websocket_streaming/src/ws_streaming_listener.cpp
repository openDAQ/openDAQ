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

#include <cstdint>

#include <opendaq/opendaq.h>

#include <ws-streaming/local_signal.hpp>

#include <websocket_streaming/descriptor_to_metadata.h>

#include <websocket_streaming/common.h>
#include <websocket_streaming/ws_streaming_listener.h>

using namespace daq::websocket_streaming;

BEGIN_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING

WsStreamingListener::WsStreamingListener(
        IContext *context,
        ISignal *signal,
        wss::local_signal *localSignal)
    : _signal(signal)
    , _port(
        InputPort(
            context,
            nullptr,
            String("ws-streaming")))
    , _lastDescriptor(_signal.getDescriptor())
    , _localSignal(*localSignal)
{
    _localSignal.set_metadata(
        descriptorToMetadata(
            signal,
            _lastDescriptor));
}

void WsStreamingListener::start()
{
    _port.setListener(this->template thisPtr<InputPortNotificationsPtr>());
    _port.setNotificationMethod(PacketReadyNotification::SameThread);
    _port.connect(_signal);
}

ErrCode WsStreamingListener::acceptsSignal(
    IInputPort *port,
    ISignal *signal,
    Bool *accept)
{
    *accept = true;

    return OPENDAQ_SUCCESS;
};

ErrCode WsStreamingListener::connected(IInputPort *port)
{
    return OPENDAQ_SUCCESS;
}

ErrCode WsStreamingListener::disconnected(IInputPort *port)
{
    return OPENDAQ_SUCCESS;
}

ErrCode WsStreamingListener::packetReceived(IInputPort *port)
{
    while (true)
    {
        auto packet = _port.getConnection().dequeue();
        if (!packet.assigned())
            break;

        if (packet.getType() == PacketType::Data)
            onDataPacketReceived(packet);
    }

    return OPENDAQ_SUCCESS;
}

void WsStreamingListener::onDataPacketReceived(DataPacketPtr packet)
{
    std::int64_t offset = 0;

    if (auto domainPacket = packet.getDomainPacket(); domainPacket.assigned())
        if (auto offsetPtr = domainPacket.getOffset(); offsetPtr.assigned())
            offset = offsetPtr;

    auto descriptor = packet.getDataDescriptor();

    if (descriptor != _lastDescriptor)
    {
        _localSignal.set_metadata(
            descriptorToMetadata(
                _signal,
                descriptor));

        _lastDescriptor = descriptor;
    }

    if (packet.getRawDataSize())
        _localSignal.publish_data(
            offset,
            packet.getSampleCount(),
            packet.getRawData(),
            packet.getRawDataSize());
}

END_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING
