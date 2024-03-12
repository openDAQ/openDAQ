/*
 * Copyright 2022-2023 Blueberry d.o.o.
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

#include <native_streaming_protocol/base_session_handler.h>

#include <opendaq/context_ptr.h>
#include <opendaq/signal_ptr.h>

#include <packet_streaming/packet_streaming_server.h>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_PROTOCOL

class ServerSessionHandler : public BaseSessionHandler
{
public:
    ServerSessionHandler(const ContextPtr& daqContext,
                         const std::shared_ptr<boost::asio::io_context>& ioContextPtr,
                         SessionPtr session,
                         OnStreamingRequestCallback streamingInitHandler,
                         OnSignalSubscriptionCallback signalSubscriptionHandler,
                         native_streaming::OnSessionErrorCallback errorHandler);

    void sendSignalAvailable(const SignalNumericIdType& signalNumericId, const SignalPtr &signal);
    void sendSignalUnavailable(const SignalNumericIdType& signalNumericId, const SignalPtr& signal);
    void sendStreamingInitDone();
    void sendPacket(const SignalNumericIdType signalId, const PacketPtr& packet);
    void sendSubscribingDone(const SignalNumericIdType signalNumericId);
    void sendUnsubscribingDone(const SignalNumericIdType signalNumericId);

    void setTransportLayerPropsHandler(const OnTrasportLayerPropertiesCallback& transportLayerPropsHandler);

private:
    daq::native_streaming::ReadTask readHeader(const void* data, size_t size) override;

    daq::native_streaming::ReadTask readSignalSubscribe(const void* data, size_t size);
    daq::native_streaming::ReadTask readSignalUnsubscribe(const void* data, size_t size);
    daq::native_streaming::ReadTask readTransportLayerProperties(const void* data, size_t size);

    void sendPacketBuffer(const packet_streaming::PacketBufferPtr& packetBuffer);

    OnStreamingRequestCallback streamingInitHandler;
    OnSignalSubscriptionCallback signalSubscriptionHandler;
    OnTrasportLayerPropertiesCallback transportLayerPropsHandler;

    packet_streaming::PacketStreamingServer packetStreamingServer;
};
END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_PROTOCOL
